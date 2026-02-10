#include "scheduler/task_scheduler.h"
#include "scheduler/event_trigger.h"
#include "query/query_engine.h"
#include "query/aql_runner.h"
#include "security/aql_injection_detector.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include "utils/audit_logger.h"
#include "utils/cron_parser.h"
#include "cdc/changefeed.h"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <deque>
#include <cctype>

#ifndef _WIN32
#include <sys/stat.h>
#endif

// ⚠️ SECURITY WARNING: This implementation executes arbitrary AQL queries and functions.
// Production deployments MUST implement proper security controls:
// - Authentication and authorization for all task operations
// - Input validation and query sanitization
// - Resource limits and quotas per task
// - Comprehensive audit logging
// - Secure task definition storage (encryption at rest)
// - Sandboxed execution environments

namespace themis {

// ===== TaskScheduler Implementation =====

TaskScheduler::TaskScheduler(QueryEngine* query_engine, const Config& config, Changefeed* changefeed)
    : query_engine_(query_engine), changefeed_(changefeed), config_(config) {
    if (!query_engine_) {
        throw std::invalid_argument("TaskScheduler: query_engine cannot be null");
    }
    
    // Initialize event trigger manager if changefeed is provided
    if (changefeed_) {
        event_trigger_manager_ = std::make_unique<EventTriggerManager>(changefeed_);
        THEMIS_INFO("TaskScheduler initialized with CDC event support");
    } else {
        THEMIS_INFO("TaskScheduler initialized without CDC event support");
    }
    
    if (config_.persist_tasks) {
        loadTasks();
    }
}

TaskScheduler::~TaskScheduler() {
    stop();
}

// ===== Lifecycle =====

void TaskScheduler::start() {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    if (running_.load()) {
        THEMIS_WARN("TaskScheduler already running");
        return;
    }
    
    running_.store(true);
    scheduler_thread_ = std::thread(&TaskScheduler::schedulerLoop, this);
    
    THEMIS_INFO("TaskScheduler started with {} tasks, check interval: {}s",
                tasks_.size(), 
                config_.check_interval.count() / 1000);
}

void TaskScheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        if (!running_.load()) {
            return;
        }
        running_.store(false);
    }
    
    cv_.notify_all();
    
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    
    // Wait for running tasks to complete (with timeout)
    auto timeout = std::chrono::seconds(30);
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        {
            std::lock_guard<std::mutex> lock(running_mutex_);
            if (running_task_threads_.empty()) {
                break;
            }
        }
        
        if (std::chrono::steady_clock::now() - start > timeout) {
            THEMIS_WARN("TaskScheduler: Timeout waiting for tasks to complete");
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Join remaining threads
    {
        std::lock_guard<std::mutex> lock(running_mutex_);
        for (auto& [id, thread] : running_task_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        running_task_threads_.clear();
    }
    
    if (config_.persist_tasks) {
        saveTasks();
    }
    
    THEMIS_INFO("TaskScheduler stopped. Total executions: {}, Failed: {}",
                total_executions_.load(), failed_executions_.load());
}

// ===== Task Management =====

std::string TaskScheduler::registerTask(const ScheduledTask& task) {
    // ⚠️ SECURITY: In production, add authentication/authorization checks here
    
    // Validate AQL query for SQL injection patterns
    if (task.type == ScheduledTask::TaskType::AQL_QUERY) {
        validateAqlQuery(task.aql_query);
    }
    
    // Validate resource limits (timeout, max_retries)
    validateResourceLimits(task);
    
    // Validate trigger-specific configuration
    if (task.trigger_type == ScheduledTask::TriggerType::CRON) {
        validateCronExpression(task.cron_expression);
    } else if (task.trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
        validateCDCTrigger(task.cdc_trigger);
        if (!changefeed_) {
            throw std::invalid_argument("CDC event triggers require a Changefeed instance");
        }
    }
    
    // Sanitize task parameters
    auto sanitized_task = sanitizeTask(task);
    
    // Note: User permission checks should be added here when authentication
    // system is integrated. Example:
    // if (!auth_context || !auth_context->hasPermission("task:register")) {
    //     throw std::runtime_error("Unauthorized: User lacks permission to register tasks");
    // }
    
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    std::string id = sanitized_task.id;
    if (id.empty()) {
        id = generateTaskId(sanitized_task);
    }
    
    auto task_ptr = std::make_shared<ScheduledTask>(sanitized_task);
    task_ptr->id = id;
    
    // Initialize next_run based on trigger type
    if (task_ptr->trigger_type == ScheduledTask::TriggerType::CRON) {
        // Parse cron expression and calculate next run
        updateCronExpression(id, task_ptr->cron_expression);
        auto cron = getCronExpression(id);
        if (cron) {
            auto next = cron->getNextExecution(std::chrono::system_clock::now());
            if (next) {
                task_ptr->next_run = *next;
            }
        }
    } else if (task_ptr->trigger_type == ScheduledTask::TriggerType::INTERVAL) {
        // Traditional interval-based scheduling
        if (task_ptr->next_run == std::chrono::system_clock::time_point{}) {
            task_ptr->next_run = std::chrono::system_clock::now() + task_ptr->interval;
        }
    } else if (task_ptr->trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
        // Setup CDC event trigger
        setupEventTrigger(task_ptr);
    }
    
    tasks_[id] = task_ptr;
    
    THEMIS_INFO("Registered task: {} (name={}, trigger_type={})",
                id, sanitized_task.name,
                static_cast<int>(sanitized_task.trigger_type));
    
    if (config_.persist_tasks) {
        saveTasks();
    }
    
    return id;
}

void TaskScheduler::unregisterTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        // Remove CDC event trigger if present
        if (it->second->trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
            removeEventTrigger(task_id);
        }
        
        // Remove cron expression from cache
        cron_expressions_.erase(task_id);
        
        THEMIS_INFO("Unregistered task: {}", task_id);
        tasks_.erase(it);
        
        if (config_.persist_tasks) {
            saveTasks();
        }
    }
}

void TaskScheduler::enableTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second->enabled = true;
        THEMIS_INFO("Enabled task: {}", task_id);
        
        if (config_.persist_tasks) {
            saveTasks();
        }
    }
}

void TaskScheduler::disableTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        it->second->enabled = false;
        THEMIS_INFO("Disabled task: {}", task_id);
        
        if (config_.persist_tasks) {
            saveTasks();
        }
    }
}

void TaskScheduler::updateTask(const ScheduledTask& task) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task.id);
    if (it != tasks_.end()) {
        // Keep statistics and state
        auto old_stats = *it->second;
        *it->second = task;
        it->second->total_executions = old_stats.total_executions;
        it->second->successful_executions = old_stats.successful_executions;
        it->second->failed_executions = old_stats.failed_executions;
        it->second->avg_execution_time_ms = old_stats.avg_execution_time_ms;
        it->second->last_run_ms = old_stats.last_run_ms;
        it->second->running = old_stats.running;
        
        THEMIS_INFO("Updated task: {}", task.id);
        
        if (config_.persist_tasks) {
            saveTasks();
        }
    }
}

// ===== Manual Execution =====

nlohmann::json TaskScheduler::executeTaskNow(const std::string& task_id) {
    // ⚠️ SECURITY: In production, add authentication/authorization checks here
    
    // Log execution attempt for audit trail
    THEMIS_INFO("Manual task execution requested: task_id={}", task_id);
    
    // Check rate limiting to prevent abuse
    if (!checkRateLimit(task_id)) {
        THEMIS_WARN("Rate limit exceeded for task execution: task_id={}", task_id);
        return nlohmann::json{{"error", "Rate limit exceeded. Please try again later."}};
    }
    
    // Note: User permission verification should be added here when authentication
    // system is integrated. Example:
    // if (!auth_context || !auth_context->hasPermission("task:execute")) {
    //     THEMIS_WARN("Unauthorized task execution attempt: task_id={}, user={}", 
    //                 task_id, auth_context ? auth_context->user_id : "unknown");
    //     return nlohmann::json{{"error", "Unauthorized: User lacks permission to execute tasks"}};
    // }
    
    auto span = Tracer::startSpan("TaskScheduler.executeTaskNow");
    span.setAttribute("task_id", task_id);
    
    std::shared_ptr<ScheduledTask> task;
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        auto it = tasks_.find(task_id);
        if (it == tasks_.end()) {
            THEMIS_WARN("Cannot execute unknown task: {}", task_id);
            return nlohmann::json{{"error", "Task not found"}};
        }
        task = it->second;
    }
    
    // Execute synchronously
    nlohmann::json result;
    try {
        if (task->type == ScheduledTask::TaskType::AQL_QUERY) {
            result = executeAqlQuery(task->aql_query);
        } else {
            result = executeFunction(task->function_name, task->parameters);
        }
        
        task->successful_executions++;
        task->last_success_time = std::chrono::system_clock::now();
        
        if (task->on_success) {
            task->on_success(task_id, result);
        }
    } catch (const std::exception& e) {
        result = nlohmann::json{{"error", e.what()}};
        task->failed_executions++;
        task->last_error = e.what();
        task->last_failure_time = std::chrono::system_clock::now();
        
        if (task->on_failure) {
            task->on_failure(task_id, e.what());
        }
        
        span.recordError(e.what());
    }
    
    task->total_executions++;
    total_executions_++;
    
    return result;
}

// ===== Function Registration =====

void TaskScheduler::registerFunction(const std::string& name, TaskFunction func) {
    // ⚠️ SECURITY CRITICAL: This allows arbitrary code execution
    
    // Audit log all function registrations
    THEMIS_INFO("Function registration attempt: name={}", name);
    
    // Note: Strict access controls should be enforced here when authentication
    // system is integrated. Example:
    // if (!auth_context || !auth_context->hasPermission("task:register_function") || 
    //     !auth_context->hasRole("system_admin")) {
    //     THEMIS_ERROR("Unauthorized function registration attempt: name={}, user={}", 
    //                  name, auth_context ? auth_context->user_id : "unknown");
    //     throw std::runtime_error("Unauthorized: Only system administrators can register functions");
    // }
    
    // TODO: Consider sandboxing function execution in future versions
    // Functions should run with limited privileges and resource constraints
    
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    functions_[name] = func;
    THEMIS_INFO("Registered task function: {}", name);
}

void TaskScheduler::unregisterFunction(const std::string& name) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    functions_.erase(name);
    THEMIS_INFO("Unregistered task function: {}", name);
}

// ===== Statistics =====

TaskScheduler::Stats TaskScheduler::getStats() const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    Stats stats;
    stats.registered_tasks = tasks_.size();
    stats.active_tasks = std::count_if(tasks_.begin(), tasks_.end(),
        [](const auto& pair) { return pair.second->enabled; });
    
    {
        std::lock_guard<std::mutex> running_lock(running_mutex_);
        stats.running_tasks = running_task_threads_.size();
    }
    
    stats.total_executions = total_executions_.load();
    stats.failed_executions = failed_executions_.load();
    stats.last_run = last_run_;
    
    // Find next scheduled run
    auto next = std::chrono::system_clock::time_point::max();
    for (const auto& [id, task] : tasks_) {
        if (task->enabled && task->next_run < next) {
            next = task->next_run;
        }
    }
    stats.next_run = next;
    
    return stats;
}

std::vector<ScheduledTask> TaskScheduler::listTasks() const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    std::vector<ScheduledTask> result;
    result.reserve(tasks_.size());
    
    for (const auto& [id, task] : tasks_) {
        result.push_back(*task);
    }
    
    return result;
}

std::shared_ptr<ScheduledTask> TaskScheduler::getTask(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        return it->second;
    }
    
    return nullptr;
}

// ===== Scheduler Loop =====

void TaskScheduler::schedulerLoop() {
    THEMIS_INFO("TaskScheduler loop started");
    
    while (running_.load()) {
        auto span = Tracer::startSpan("TaskScheduler.tick");
        auto now = std::chrono::system_clock::now();
        
        std::vector<std::shared_ptr<ScheduledTask>> tasks_to_execute;
        
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            last_run_ = now;
            
            for (auto& [id, task] : tasks_) {
                if (!task->enabled || task->running) {
                    continue;
                }
                
                if (shouldExecute(*task, now)) {
                    // Check concurrent task limit
                    size_t running_count = 0;
                    {
                        std::lock_guard<std::mutex> running_lock(running_mutex_);
                        running_count = running_task_threads_.size();
                    }
                    
                    if (running_count >= config_.max_concurrent_tasks) {
                        THEMIS_DEBUG("Max concurrent tasks reached ({}), delaying task {}",
                                    config_.max_concurrent_tasks, id);
                        continue;
                    }
                    
                    tasks_to_execute.push_back(task);
                }
            }
        }
        
        // Execute tasks outside the lock
        for (auto& task : tasks_to_execute) {
            task->running = true;
            
            // Launch task in separate thread
            std::thread task_thread([this, task]() {
                executeTask(task);
                
                // Remove from running threads
                {
                    std::lock_guard<std::mutex> lock(running_mutex_);
                    running_task_threads_.erase(task->id);
                }
            });
            
            // Store thread for cleanup
            {
                std::lock_guard<std::mutex> lock(running_mutex_);
                running_task_threads_[task->id] = std::move(task_thread);
            }
        }
        
        span.setAttribute("tasks_executed", static_cast<int64_t>(tasks_to_execute.size()));
        
        // Wait for next check interval or shutdown signal
        std::unique_lock<std::mutex> lock(tasks_mutex_);
        cv_.wait_for(lock, config_.check_interval, [this] { return !running_.load(); });
    }
    
    THEMIS_INFO("TaskScheduler loop stopped");
}

void TaskScheduler::executeTask(std::shared_ptr<ScheduledTask> task) {
    auto span = Tracer::startSpan("TaskScheduler.executeTask");
    span.setAttribute("task_id", task->id);
    span.setAttribute("task_name", task->name);
    
    auto start = std::chrono::steady_clock::now();
    
    try {
        nlohmann::json result;
        
        if (task->type == ScheduledTask::TaskType::AQL_QUERY) {
            span.setAttribute("task_type", "aql");
            result = executeAqlQuery(task->aql_query);
        } else {
            span.setAttribute("task_type", "function");
            result = executeFunction(task->function_name, task->parameters);
        }
        
        auto end = std::chrono::steady_clock::now();
        double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Update statistics
        task->last_run_ms = getCurrentTimeMs();
        task->total_executions++;
        task->successful_executions++;
        task->last_success_time = std::chrono::system_clock::now();
        
        // Update moving average of execution time
        task->avg_execution_time_ms = 
            (task->avg_execution_time_ms * (task->total_executions - 1) + elapsed_ms) 
            / task->total_executions;
        
        updateNextRun(*task);
        
        total_executions_++;
        
        span.setAttribute("execution_time_ms", elapsed_ms);
        
        THEMIS_DEBUG("Executed task {} ({}): {:.2f}ms",
                     task->id, task->name, elapsed_ms);
        
        if (task->on_success) {
            task->on_success(task->id, result);
        }
        
    } catch (const std::exception& e) {
        task->failed_executions++;
        task->last_error = e.what();
        task->last_failure_time = std::chrono::system_clock::now();
        failed_executions_++;
        
        updateNextRun(*task);
        
        span.recordError(e.what());
        THEMIS_ERROR("Failed to execute task {}: {}", task->id, e.what());
        
        if (task->on_failure) {
            task->on_failure(task->id, e.what());
        }
    }
    
    task->running = false;
}

nlohmann::json TaskScheduler::executeAqlQuery(const std::string& aql) {
    // ⚠️ SECURITY: AQL queries can read/write any data
    
    // Implement query validation and sanitization
    validateAqlQuery(aql);
    
    // Apply query complexity limits to prevent resource exhaustion
    enforceQueryComplexityLimits(aql);
    
    // Note: Row/resource limits should be enforced at query engine level
    // The query engine should have configurable limits for:
    // - Maximum execution time
    // - Maximum memory usage
    // - Maximum result set size
    // - Maximum number of operations
    
    auto span = Tracer::startSpan("TaskScheduler.executeAqlQuery");
    span.setAttribute("aql", aql);
    
    auto result = executeAql(aql, *query_engine_);
    
    if (!result) {
        throw std::runtime_error("AQL query failed: " + result.error().message());
    }
    
    return *result;
}

nlohmann::json TaskScheduler::executeFunction(const std::string& name, const nlohmann::json& params) {
    auto span = Tracer::startSpan("TaskScheduler.executeFunction");
    span.setAttribute("function_name", name);
    
    auto it = functions_.find(name);
    if (it == functions_.end()) {
        throw std::runtime_error("Function not found: " + name);
    }
    
    return it->second(params);
}

// ===== Scheduling Logic =====

bool TaskScheduler::shouldExecute(const ScheduledTask& task, 
                                   const std::chrono::system_clock::time_point& now) const {
    if (task.running && !config_.allow_task_overlap) {
        return false;  // Task already running
    }
    
    // Check based on trigger type
    if (task.trigger_type == ScheduledTask::TriggerType::CRON) {
        return shouldExecuteCron(task, now);
    } else if (task.trigger_type == ScheduledTask::TriggerType::INTERVAL) {
        return now >= task.next_run;
    } else if (task.trigger_type == ScheduledTask::TriggerType::CDC_EVENT) {
        // CDC events trigger tasks directly via callbacks
        return false;
    } else if (task.trigger_type == ScheduledTask::TriggerType::MANUAL) {
        // Manual tasks are only executed via executeTaskNow()
        return false;
    }
    
    return false;
}

void TaskScheduler::updateNextRun(ScheduledTask& task) {
    if (task.trigger_type == ScheduledTask::TriggerType::CRON) {
        // Calculate next run from cron expression
        auto it = cron_expressions_.find(task.id);
        if (it != cron_expressions_.end()) {
            auto next = it->second->getNextExecution(std::chrono::system_clock::now());
            if (next) {
                task.next_run = *next;
            }
        }
    } else if (task.trigger_type == ScheduledTask::TriggerType::INTERVAL) {
        // Traditional interval-based scheduling
        task.next_run = std::chrono::system_clock::now() + task.interval;
    }
    // CDC_EVENT and MANUAL types don't have a next_run
}

// ===== Persistence =====

void TaskScheduler::saveTasks() {
    // ⚠️ SECURITY: Task definitions may contain sensitive data (queries, parameters)
    
    // Note: For production deployments, implement:
    // 1. Encryption of task definitions at rest using a secure key management system
    // 2. Proper file permissions (600 or 400) to restrict access to task files
    // 3. Consider using a secure key-value store (e.g., encrypted RocksDB) instead of plain JSON files
    
    // Simple JSON-based persistence
    nlohmann::json tasks_json = nlohmann::json::array();
    
    for (const auto& [id, task] : tasks_) {
        nlohmann::json task_json;
        task_json["id"] = task->id;
        task_json["name"] = task->name;
        task_json["description"] = task->description;
        task_json["type"] = task->type == ScheduledTask::TaskType::AQL_QUERY ? "aql" : "function";
        task_json["aql_query"] = task->aql_query;
        task_json["function_name"] = task->function_name;
        task_json["parameters"] = task->parameters;
        task_json["interval_ms"] = task->interval.count();
        task_json["enabled"] = task->enabled;
        
        // Save trigger configuration
        task_json["trigger_type"] = static_cast<int>(task->trigger_type);
        task_json["cron_expression"] = task->cron_expression;
        task_json["priority"] = static_cast<int>(task->priority);
        task_json["trigger_logic"] = static_cast<int>(task->trigger_logic);
        
        // Save CDC trigger config
        nlohmann::json cdc_json;
        cdc_json["key_prefix"] = task->cdc_trigger.key_prefix;
        cdc_json["event_types"] = nlohmann::json::array();
        for (int type : task->cdc_trigger.event_types) {
            cdc_json["event_types"].push_back(type);
        }
        if (task->cdc_trigger.condition) {
            cdc_json["condition"] = *task->cdc_trigger.condition;
        }
        cdc_json["debounce_ms"] = task->cdc_trigger.debounce_ms;
        task_json["cdc_trigger"] = cdc_json;
        
        tasks_json.push_back(task_json);
    }
    
    try {
        std::string filepath = config_.persistence_path + "/tasks.json";
        std::ofstream file(filepath);
        if (!file.good()) {
            throw std::runtime_error("Failed to open file for writing: " + filepath);
        }
        
        file << tasks_json.dump(2);
        file.close();
        
        // Set restrictive file permissions (owner read/write only)
        #ifndef _WIN32
        chmod(filepath.c_str(), S_IRUSR | S_IWUSR);
        #endif
        
        THEMIS_DEBUG("Saved {} tasks to disk with secure permissions", tasks_.size());
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to save tasks: {}", e.what());
    }
}

void TaskScheduler::loadTasks() {
    try {
        std::ifstream file(config_.persistence_path + "/tasks.json");
        if (!file.good()) {
            THEMIS_DEBUG("No persisted tasks found");
            return;
        }
        
        nlohmann::json tasks_json;
        file >> tasks_json;
        
        for (const auto& task_json : tasks_json) {
            ScheduledTask task;
            task.id = task_json.value("id", "");
            task.name = task_json.value("name", "");
            task.description = task_json.value("description", "");
            
            std::string type_str = task_json.value("type", "aql");
            task.type = (type_str == "aql") ? ScheduledTask::TaskType::AQL_QUERY 
                                            : ScheduledTask::TaskType::FUNCTION;
            
            task.aql_query = task_json.value("aql_query", "");
            task.function_name = task_json.value("function_name", "");
            task.parameters = task_json.value("parameters", nlohmann::json::object());
            task.interval = std::chrono::milliseconds(task_json.value("interval_ms", 300000));
            task.enabled = task_json.value("enabled", true);
            
            // Load trigger configuration (with defaults for backward compatibility)
            task.trigger_type = static_cast<ScheduledTask::TriggerType>(
                task_json.value("trigger_type", static_cast<int>(ScheduledTask::TriggerType::INTERVAL)));
            task.cron_expression = task_json.value("cron_expression", "");
            task.priority = static_cast<ScheduledTask::Priority>(
                task_json.value("priority", static_cast<int>(ScheduledTask::Priority::NORMAL)));
            task.trigger_logic = static_cast<ScheduledTask::TriggerLogic>(
                task_json.value("trigger_logic", static_cast<int>(ScheduledTask::TriggerLogic::OR)));
            
            // Load CDC trigger config
            if (task_json.contains("cdc_trigger")) {
                auto cdc_json = task_json["cdc_trigger"];
                task.cdc_trigger.key_prefix = cdc_json.value("key_prefix", "");
                task.cdc_trigger.debounce_ms = cdc_json.value("debounce_ms", 0);
                
                if (cdc_json.contains("event_types")) {
                    for (const auto& type : cdc_json["event_types"]) {
                        task.cdc_trigger.event_types.insert(type.get<int>());
                    }
                }
                
                if (cdc_json.contains("condition")) {
                    task.cdc_trigger.condition = cdc_json["condition"].get<std::string>();
                }
            }
            
            registerTask(task);
        }
        
        THEMIS_INFO("Loaded {} tasks from disk", tasks_.size());
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load tasks: {}", e.what());
    }
}

// ===== Helpers =====

int64_t TaskScheduler::getCurrentTimeMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::string TaskScheduler::generateTaskId(const ScheduledTask& task) const {
    std::ostringstream oss;
    
    if (!task.name.empty()) {
        // Use name as base
        oss << task.name;
        // Replace spaces with underscores
        std::string id = oss.str();
        std::replace(id.begin(), id.end(), ' ', '_');
        return id;
    } else if (!task.aql_query.empty()) {
        // Use hash of AQL query
        oss << "aql_task_" << std::hash<std::string>{}(task.aql_query);
        return oss.str();
    } else {
        // Use function name
        oss << "func_" << task.function_name;
        return oss.str();
    }
}

// ===== Security & Validation Helpers =====

void TaskScheduler::validateAqlQuery(const std::string& aql) const {
    if (aql.empty()) {
        throw std::invalid_argument("AQL query cannot be empty");
    }
    
    // Use AST-based injection detection for robust security
    security::AQLInjectionDetector detector;
    auto validation_result = detector.validateAQLAST(aql);
    
    if (!validation_result.is_safe) {
        THEMIS_ERROR("AQL injection detected: {}", validation_result.error_message);
        
        // Log detected patterns for security diagnostics
        for (const auto& pattern : validation_result.detected_patterns) {
            THEMIS_WARN("Injection pattern detected: {}", pattern);
        }
        
        // Note: Formal audit logging could be added here via a class-level
        // AuditLogger instance if AccessControl integration is required.
        // For now, structured logging via THEMIS_WARN is sufficient.
        
        throw std::invalid_argument("AQL query validation failed: " + validation_result.error_message);
    }
    
    // Check for reasonable length limit (prevent DoS)
    const size_t MAX_QUERY_LENGTH = 100000; // 100KB
    if (aql.length() > MAX_QUERY_LENGTH) {
        throw std::invalid_argument("AQL query exceeds maximum length of " + 
                                   std::to_string(MAX_QUERY_LENGTH) + " characters");
    }
}

void TaskScheduler::validateResourceLimits(const ScheduledTask& task) const {
    // Validate timeout
    const auto MAX_TIMEOUT = std::chrono::hours(24);
    const auto MIN_TIMEOUT = std::chrono::seconds(1);
    
    if (task.timeout > MAX_TIMEOUT) {
        auto max_timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(MAX_TIMEOUT).count();
        throw std::invalid_argument("Task timeout exceeds maximum allowed: " + 
                                   std::to_string(max_timeout_ms) + " milliseconds");
    }
    
    if (task.timeout < MIN_TIMEOUT) {
        auto min_timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(MIN_TIMEOUT).count();
        throw std::invalid_argument("Task timeout is too short. Minimum: " + 
                                   std::to_string(min_timeout_ms) + " milliseconds");
    }
    
    // Validate max_retries
    const size_t MAX_RETRIES = 10;
    if (task.max_retries > MAX_RETRIES) {
        throw std::invalid_argument("Task max_retries exceeds maximum allowed: " + 
                                   std::to_string(MAX_RETRIES));
    }
    
    // Validate interval
    const auto MIN_INTERVAL = std::chrono::seconds(1);
    if (task.interval < MIN_INTERVAL) {
        auto min_interval_ms = std::chrono::duration_cast<std::chrono::milliseconds>(MIN_INTERVAL).count();
        throw std::invalid_argument("Task interval is too short. Minimum: " + 
                                   std::to_string(min_interval_ms) + " milliseconds");
    }
}

ScheduledTask TaskScheduler::sanitizeTask(const ScheduledTask& task) const {
    ScheduledTask sanitized = task;
    
    // Sanitize string fields to prevent injection
    auto sanitizeString = [](const std::string& input) -> std::string {
        std::string output;
        output.reserve(input.size());
        
        for (char c : input) {
            // Remove null bytes and other control characters except newline and tab
            if (c == '\0' || (c < 32 && c != '\n' && c != '\t')) {
                continue;
            }
            output += c;
        }
        
        return output;
    };
    
    sanitized.id = sanitizeString(task.id);
    sanitized.name = sanitizeString(task.name);
    sanitized.description = sanitizeString(task.description);
    sanitized.function_name = sanitizeString(task.function_name);
    
    // Limit field lengths
    const size_t MAX_NAME_LENGTH = 255;
    const size_t MAX_DESC_LENGTH = 1000;
    
    if (sanitized.name.length() > MAX_NAME_LENGTH) {
        sanitized.name = sanitized.name.substr(0, MAX_NAME_LENGTH);
    }
    
    if (sanitized.description.length() > MAX_DESC_LENGTH) {
        sanitized.description = sanitized.description.substr(0, MAX_DESC_LENGTH);
    }
    
    return sanitized;
}

void TaskScheduler::enforceQueryComplexityLimits(const std::string& aql) const {
    // Basic complexity checks to prevent resource exhaustion
    // Note: These are heuristic checks and may have false positives
    
    // Count nested levels (approximate complexity)
    int nesting_level = 0;
    int max_nesting = 0;
    
    for (char c : aql) {
        if (c == '(' || c == '{' || c == '[') {
            nesting_level++;
            max_nesting = std::max(max_nesting, nesting_level);
        } else if (c == ')' || c == '}' || c == ']') {
            nesting_level--;
        }
    }
    
    const int MAX_NESTING_LEVEL = 20;
    if (max_nesting > MAX_NESTING_LEVEL) {
        throw std::invalid_argument("Query nesting level exceeds maximum allowed: " + 
                                   std::to_string(MAX_NESTING_LEVEL));
    }
    
    // Count number of FOR loops (heuristic - may have false positives in comments/strings)
    // This is acceptable as overly complex queries in comments would still indicate
    // potential issues. The limit is set high enough to allow legitimate queries.
    size_t for_count = 0;
    size_t pos = 0;
    std::string aql_upper = aql;
    std::transform(aql_upper.begin(), aql_upper.end(), aql_upper.begin(), ::toupper);
    
    while ((pos = aql_upper.find("FOR ", pos)) != std::string::npos) {
        // Check word boundary: must be at start or preceded by non-alphanumeric
        bool valid_start = (pos == 0 || !std::isalnum(static_cast<unsigned char>(aql_upper[pos - 1])));
        if (valid_start) {
            for_count++;
        }
        pos += 4;
    }
    
    const size_t MAX_FOR_LOOPS = 5;
    if (for_count > MAX_FOR_LOOPS) {
        throw std::invalid_argument("Query contains too many FOR loops. Maximum: " + 
                                   std::to_string(MAX_FOR_LOOPS));
    }
}

bool TaskScheduler::checkRateLimit(const std::string& task_id) const {
    // Simple rate limiting implementation
    // In production, use a more sophisticated rate limiter (e.g., token bucket, sliding window)
    
    static std::mutex rate_limit_mutex;
    static std::map<std::string, std::deque<std::chrono::steady_clock::time_point>> execution_times;
    
    std::lock_guard<std::mutex> lock(rate_limit_mutex);
    
    const auto now = std::chrono::steady_clock::now();
    const auto window = std::chrono::minutes(1);
    const size_t max_executions = 10; // Max 10 manual executions per minute per task
    
    auto& times = execution_times[task_id];
    
    // Remove old entries outside the time window
    while (!times.empty() && (now - times.front()) > window) {
        times.pop_front();
    }
    
    // Check if limit is exceeded
    if (times.size() >= max_executions) {
        return false;
    }
    
    // Add current execution time
    times.push_back(now);
    
    return true;
}

// ===== Cron Expression Management =====

void TaskScheduler::validateCronExpression(const std::string& expression) const {
    if (expression.empty()) {
        throw std::invalid_argument("Cron expression cannot be empty");
    }
    
    auto validation = CronExpression::validate(expression);
    if (!validation.is_valid) {
        throw std::invalid_argument("Invalid cron expression: " + validation.error_message);
    }
}

std::shared_ptr<CronExpression> TaskScheduler::getCronExpression(const std::string& task_id) {
    auto it = cron_expressions_.find(task_id);
    if (it != cron_expressions_.end()) {
        return it->second;
    }
    return nullptr;
}

void TaskScheduler::updateCronExpression(const std::string& task_id, const std::string& expression) {
    auto parsed = CronExpression::parse(expression);
    if (!parsed) {
        throw std::invalid_argument("Failed to parse cron expression: " + expression);
    }
    cron_expressions_[task_id] = std::make_shared<CronExpression>(*parsed);
}

bool TaskScheduler::shouldExecuteCron(const ScheduledTask& task, 
                                       const std::chrono::system_clock::time_point& now) const {
    auto it = cron_expressions_.find(task.id);
    if (it == cron_expressions_.end()) {
        return false;
    }
    return it->second->matches(now);
}

// ===== CDC Trigger Management =====

void TaskScheduler::validateCDCTrigger(const ScheduledTask::CDCTrigger& trigger) const {
    if (trigger.key_prefix.empty()) {
        throw std::invalid_argument("CDC trigger key_prefix cannot be empty");
    }
    
    if (trigger.event_types.empty()) {
        throw std::invalid_argument("CDC trigger must specify at least one event type");
    }
    
    // Validate event types are in valid range (0-3)
    for (int type : trigger.event_types) {
        if (type < 0 || type > 3) {
            throw std::invalid_argument("Invalid CDC event type: " + std::to_string(type));
        }
    }
}

void TaskScheduler::setupEventTrigger(std::shared_ptr<ScheduledTask> task) {
    if (!event_trigger_manager_) {
        THEMIS_ERROR("Cannot setup CDC trigger without EventTriggerManager");
        return;
    }
    
    // Convert task CDC config to EventTrigger config
    CDCTriggerConfig config;
    config.key_prefix = task->cdc_trigger.key_prefix;
    
    // Convert event types from int to ChangeEventType
    for (int type_int : task->cdc_trigger.event_types) {
        config.event_types.insert(static_cast<Changefeed::ChangeEventType>(type_int));
    }
    
    config.condition = task->cdc_trigger.condition;
    config.debounce_ms = task->cdc_trigger.debounce_ms;
    
    // Create callback that triggers task execution
    auto callback = [this, task_id = task->id](const Changefeed::ChangeEvent& event) {
        // Retrieve task inside the callback to avoid capturing by value
        std::shared_ptr<ScheduledTask> task_ptr;
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            auto it = tasks_.find(task_id);
            if (it == tasks_.end()) {
                THEMIS_DEBUG("Task {} no longer exists, skipping execution", task_id);
                return;
            }
            task_ptr = it->second;
        }
        
        THEMIS_DEBUG("CDC event triggered task: {} (key={}, type={})",
                    task_id, event.key, static_cast<int>(event.type));
        
        // Check if task is enabled
        if (!task_ptr->enabled) {
            THEMIS_DEBUG("Task {} is disabled, skipping execution", task_id);
            return;
        }
        
        // Atomic check-and-set for task running state
        bool expected = false;
        if (!config_.allow_task_overlap) {
            // Try to set running to true atomically
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            if (task_ptr->running) {
                THEMIS_DEBUG("Task {} is already running, skipping execution", task_id);
                return;
            }
            task_ptr->running = true;
        } else {
            task_ptr->running = true;
        }
        
        // Execute task asynchronously
        std::thread task_thread([this, task_ptr]() {
            executeTask(task_ptr);
            
            // Remove from running threads
            {
                std::lock_guard<std::mutex> lock(running_mutex_);
                running_task_threads_.erase(task_ptr->id);
            }
        });
        
        // Store thread for cleanup
        {
            std::lock_guard<std::mutex> lock(running_mutex_);
            running_task_threads_[task_id] = std::move(task_thread);
        }
    };
    
    // Register trigger with manager
    event_trigger_manager_->registerTrigger(task->id, config, std::move(callback));
}

void TaskScheduler::removeEventTrigger(const std::string& task_id) {
    if (event_trigger_manager_) {
        event_trigger_manager_->unregisterTrigger(task_id);
    }
}

void TaskScheduler::onCDCEvent(std::shared_ptr<ScheduledTask> task, const void* event) {
    // This method is called by EventTrigger callback
    // The actual implementation is in setupEventTrigger's callback
}

// ===== Update shouldExecute and updateNextRun =====

} // namespace themis
