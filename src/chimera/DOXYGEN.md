# CHIMERA DOXYGEN

Author: ThemisDB Contributors
Created: 2026-09-20
Last Updated: 2026-09-20
Status: active

## Source
- This file is generated from module-scoped Doxygen XML.
- XML index: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\chimera\xml\index.xml`
- Warnings log: `C:\Projects\ThemisDB\.tmp\module_doxygen_global_e2e\chimera\doxygen-warnings.log`

## Structure Metrics
- C/C++ Files (scanned): 22
- Compounds: 85
- Classes/Structs: 40
- Namespaces: 15
- File Compounds: 22

## Namespaces
- @006331133103334303025223376351216371230151360146
- @111365301325371230015323020343100013344013346004
- @121101270303077172157154020227003362354132203153
- @137240047307255044001060055344125143037173354063
- benchmark
- chimera
- chimera::@051252242240106140134317350055200161040275130360
- chimera::@176273071342074350357016112351274371221013000143
- chimera::@312071053317361021361105377250016054367313307041
- chimera::@341303121322265055317323115314104060151241166312
- mongocxx
- mongocxx::collection
- testing
- themis
- themis::chimera

## Types
### Classes
- ChimeraAdapterFixture
- ChimeraPreparedStatementTest
- ChimeraStreamingTest
- ThemisDBCapabilityTest
- ThemisDBConnectionTest
- ThemisDBDocumentTest
- ThemisDBEngineInjectionTest
- ThemisDBGraphTest
- ThemisDBIntegrationTest
- ThemisDBPerformanceTest
- ThemisDBRelationalTest
- ThemisDBTransactionTest
- ThemisDBVectorTest
- chimera::IBatchAdapter
- chimera::IPreparedStatement
- chimera::IPreparedStatementAdapter
- chimera::IResultStream
- chimera::IStreamingAdapter
- chimera::MongoDBAdapter
- chimera::Neo4jAdapter
- chimera::QdrantAdapter
- chimera::RetryExecutor
- chimera::ThemisDBAdapter
- chimera::ThemisDBPreparedStatement
- chimera::ThemisDBResultStream
- chimera::TransactionContext
- chimera::TransactionHandle

### Structs
- chimera::BatchConfig
- chimera::BatchStatistics
- chimera::MongoDBAdapter::QueuedOperation
- chimera::Neo4jAdapter::SessionHandle
- chimera::Operation
- chimera::QdrantAdapter::QueuedVector
- chimera::QueryStatistics
- chimera::RelationalRow
- chimera::RelationalTable
- chimera::Result
- chimera::RetryPolicy
- chimera::StreamConfig
- chimera::ThemisDBAdapter::TxnEntry

## Notes
- This is a derived artifact. Do not manually edit semantic content.
- Regenerate via scripts/generate_module_doxygen_xml.py --write-module-markdown.

## Detailed Function Documentation (Soll-Ist)
- Functions extracted: 449

### ChimeraAdapterFixture

#### `void SetUp(const ::benchmark::State &) override`
- Source: `benchmarks/chimera/bench_chimera_adapter.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

#### `void TearDown(const ::benchmark::State &) override`
- Source: `benchmarks/chimera/bench_chimera_adapter.cpp`:84
- Brief: n/a
- Parameters:
  - `<unnamed>` (const ::benchmark::State &): n/a

### ChimeraPreparedStatementTest

#### `void SetUp() override`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:65
- Brief: n/a
- Parameters: none

### ChimeraStreamingTest

#### `void SetUp() override`
- Source: `tests/chimera/test_chimera_streaming.cpp`:66
- Brief: n/a
- Parameters: none

### ThemisDBCapabilityTest

#### `void SetUp() override`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:694
- Brief: n/a
- Parameters: none

### ThemisDBDocumentTest

#### `void SetUp() override`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:286
- Brief: n/a
- Parameters: none

### ThemisDBEngineInjectionTest

#### `themis::GraphIndexManager * fake_gi()`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1415
- Brief: n/a
- Parameters: none

#### `themis::QueryEngine * fake_qe()`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1409
- Brief: n/a
- Parameters: none

#### `themis::VectorIndexManager * fake_vi()`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1412
- Brief: n/a
- Parameters: none

### ThemisDBGraphTest

#### `void SetUp() override`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:224
- Brief: n/a
- Parameters: none

### ThemisDBIntegrationTest

#### `void SetUp() override`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:923
- Brief: n/a
- Parameters: none

### ThemisDBPerformanceTest

#### `void SetUp() override`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:778
- Brief: n/a
- Parameters: none

### ThemisDBRelationalTest

#### `void SetUp() override`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:112
- Brief: n/a
- Parameters: none

### ThemisDBTransactionTest

#### `void SetUp() override`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:350
- Brief: n/a
- Parameters: none

### ThemisDBVectorTest

#### `void SetUp() override`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:171
- Brief: n/a
- Parameters: none

### bench_chimera_adapter.cpp

#### `state SetItemsProcessed(state.iterations() *100)`
- Source: `benchmarks/chimera/bench_chimera_adapter.cpp`:168
- Brief: n/a
- Parameters:
  - `100` (state.iterations() *): n/a

#### `state SetItemsProcessed(state.iterations())`
- Source: `benchmarks/chimera/bench_chimera_adapter.cpp`:123
- Brief: n/a
- Parameters:
  - `iterations` (state.): n/a

#### `Unit(benchmark::kMicrosecond) -> Repetitions(kRepetitions) ->Iterations(100) ->UseRealTime()`
- Source: `benchmarks/chimera/bench_chimera_adapter.cpp`:104
- Brief: n/a
- Parameters:
  - `<unnamed>` (benchmark::kMicrosecond): n/a

#### `for(auto _ :state)`
- Source: `benchmarks/chimera/bench_chimera_adapter.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (auto _ :state): n/a

#### `for(int i=0;i< 100;++i)`
- Source: `benchmarks/chimera/bench_chimera_adapter.cpp`:160
- Brief: n/a
- Parameters: none

### bench_chimera_dedicated_gates.cpp

#### `BENCHMARK_MAIN()`
- Source: `benchmarks/chimera/bench_chimera_dedicated_gates.cpp`:143
- Brief: n/a
- Parameters: none

#### `void BM_CH_BM_01_HybridQueryThroughput(benchmark::State &state)`
- Source: `benchmarks/chimera/bench_chimera_dedicated_gates.cpp`:78
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CH_BM_02_FallbackLatency(benchmark::State &state)`
- Source: `benchmarks/chimera/bench_chimera_dedicated_gates.cpp`:96
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CH_BM_03_DispatchDecision(benchmark::State &state)`
- Source: `benchmarks/chimera/bench_chimera_dedicated_gates.cpp`:113
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `void BM_CH_BM_04_ResultMerge(benchmark::State &state)`
- Source: `benchmarks/chimera/bench_chimera_dedicated_gates.cpp`:130
- Brief: n/a
- Parameters:
  - `state` (benchmark::State &): n/a

#### `MinTime(1.0) -> UseRealTime()`
- Source: `benchmarks/chimera/bench_chimera_dedicated_gates.cpp`:106
- Brief: n/a
- Parameters:
  - `0` (1.): n/a

#### `Threads(8) -> MinTime(1.0) ->UseRealTime()`
- Source: `benchmarks/chimera/bench_chimera_dedicated_gates.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (8): n/a

### chimera::IBatchAdapter

#### `Result< BatchStatistics > flush()=0`
- Source: `include/chimera/batch_executor.hpp`:105
- Brief: Flush.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `const BatchConfig & get_batch_config() const =0`
- Source: `include/chimera/batch_executor.hpp`:124
- Brief: Get batch config.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `size_t get_pending_count() const =0`
- Source: `include/chimera/batch_executor.hpp`:111
- Brief: Get pending count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< bool > queue_delete(const std::string &table_name, const std::string &where_clause)=0`
- Source: `include/chimera/batch_executor.hpp`:96
- Brief: Queue delete.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `where_clause` (const std::string &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. where_clause Input parameter. Return value.

#### `Result< bool > queue_insert(const std::string &table_name, const RelationalRow &row)=0`
- Source: `include/chimera/batch_executor.hpp`:61
- Brief: Queue insert.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `row` (const RelationalRow &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. row Input parameter. Return value.

#### `Result< bool > queue_insert_batch(const std::string &table_name, const std::vector< RelationalRow > &rows)=0`
- Source: `include/chimera/batch_executor.hpp`:72
- Brief: Queue insert batch.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `rows` (const std::vector< RelationalRow > &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. rows Input parameter. Return value.

#### `Result< bool > queue_update(const std::string &table_name, const RelationalRow &row, const std::string &where_clause)=0`
- Source: `include/chimera/batch_executor.hpp`:84
- Brief: Queue update.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `row` (const RelationalRow &): Input parameter.
  - `where_clause` (const std::string &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. row Input parameter. where_clause Input parameter. Return value.

#### `Result< bool > set_batch_config(const BatchConfig &config)=0`
- Source: `include/chimera/batch_executor.hpp`:118
- Brief: Set batch config.
- Parameters:
  - `config` (const BatchConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `~IBatchAdapter()=default`
- Source: `include/chimera/batch_executor.hpp`:53
- Brief: IBatch Adapter.
- Parameters: none
- Return: Return value.
- Details: Return value.

### chimera::IPreparedStatement

#### `Result< bool > bind(const std::string &name, const Scalar &value)=0`
- Source: `include/chimera/database_adapter.hpp`:231
- Brief: Bind.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `value` (const Scalar &): Input parameter.
- Return: Return value.
- Details: name Input parameter. value Input parameter. Return value.

#### `Result< bool > bind(size_t position, const Scalar &value)=0`
- Source: `include/chimera/database_adapter.hpp`:239
- Brief: Bind.
- Parameters:
  - `position` (size_t): Input parameter.
  - `value` (const Scalar &): Input parameter.
- Return: Return value.
- Details: position Input parameter. value Input parameter. Return value.

#### `Result< bool > bind_all(const std::map< std::string, Scalar > &params)=0`
- Source: `include/chimera/database_adapter.hpp`:241
- Brief: n/a
- Parameters:
  - `params` (const std::map< std::string, Scalar > &): n/a

#### `Result< RelationalTable > execute()=0`
- Source: `include/chimera/database_adapter.hpp`:249
- Brief: Execute.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::future< Result< RelationalTable > > execute_async()=0`
- Source: `include/chimera/database_adapter.hpp`:255
- Brief: Execute async.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string get_id() const =0`
- Source: `include/chimera/database_adapter.hpp`:217
- Brief: Get id.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string get_query() const =0`
- Source: `include/chimera/database_adapter.hpp`:223
- Brief: Get query.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< QueryStatistics > get_statistics() const =0`
- Source: `include/chimera/database_adapter.hpp`:267
- Brief: Get statistics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< bool > reset()=0`
- Source: `include/chimera/database_adapter.hpp`:261
- Brief: Reset the modification detection flag.
- Parameters: none
- Return: None.
- Details: None.

#### `~IPreparedStatement()=default`
- Source: `include/chimera/database_adapter.hpp`:211
- Brief: IPrepared Statement.
- Parameters: none
- Return: Return value.
- Details: Return value.

### chimera::IPreparedStatementAdapter

#### `Result< std::vector< std::string > > list_prepared()=0`
- Source: `include/chimera/database_adapter.hpp`:298
- Brief: List prepared.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< std::unique_ptr< IPreparedStatement > > prepare(const std::string &query)=0`
- Source: `include/chimera/database_adapter.hpp`:283
- Brief: Prepare.
- Parameters:
  - `query` (const std::string &): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value.

#### `Result< bool > unprepare(const std::string &statement_id)=0`
- Source: `include/chimera/database_adapter.hpp`:292
- Brief: Unprepare.
- Parameters:
  - `statement_id` (const std::string &): Identifier of the statement.
- Return: Return value.
- Details: statement_id Identifier of the statement. Return value.

#### `~IPreparedStatementAdapter()=default`
- Source: `include/chimera/database_adapter.hpp`:276
- Brief: IPrepared Statement Adapter.
- Parameters: none
- Return: Return value.
- Details: Return value.

### chimera::IResultStream

#### `Result< bool > close()=0`
- Source: `include/chimera/database_adapter.hpp`:181
- Brief: Close.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `bool has_more() const =0`
- Source: `include/chimera/database_adapter.hpp`:159
- Brief: Has more.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `Result< std::vector< RelationalRow > > next_batch(size_t batch_size=0)=0`
- Source: `include/chimera/database_adapter.hpp`:161
- Brief: n/a
- Parameters:
  - `batch_size` (size_t): n/a

#### `size_t position() const =0`
- Source: `include/chimera/database_adapter.hpp`:169
- Brief: Position.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< size_t > total_size() const =0`
- Source: `include/chimera/database_adapter.hpp`:175
- Brief: Total size.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `~IResultStream()=default`
- Source: `include/chimera/database_adapter.hpp`:153
- Brief: IResult Stream.
- Parameters: none
- Return: Return value.
- Details: Return value.

### chimera::IStreamingAdapter

#### `Result< std::unique_ptr< IResultStream > > execute_query_stream(const std::string &query, const std::vector< Scalar > &params={})=0`
- Source: `include/chimera/database_adapter.hpp`:192
- Brief: n/a
- Parameters:
  - `query` (const std::string &): n/a
  - `params` (const std::vector< Scalar > &): n/a

#### `Result< bool > set_stream_config(const StreamConfig &config)=0`
- Source: `include/chimera/database_adapter.hpp`:202
- Brief: Set stream config.
- Parameters:
  - `config` (const StreamConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value.

#### `~IStreamingAdapter()=default`
- Source: `include/chimera/database_adapter.hpp`:190
- Brief: IStreaming Adapter.
- Parameters: none
- Return: Return value.
- Details: Return value.

### chimera::MongoDBAdapter

#### `MongoDBAdapter()`
- Source: `include/chimera/mongodb_adapter.hpp`:35
- Brief: n/a
- Parameters: none

#### `Result< size_t > batch_insert(const std::string &table_name, const std::vector< RelationalRow > &rows) override`
- Source: `include/chimera/mongodb_adapter.hpp`:62
- Brief: Batch insert.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `rows` (const std::vector< RelationalRow > &): Input parameter.
- Return: Return value.
- Details: param Input parameter. rows Input parameter. Return value. Calls: err(), ok(), size().

#### `Result< size_t > batch_insert_documents(const std::string &collection, const std::vector< Document > &docs) override`
- Source: `include/chimera/mongodb_adapter.hpp`:120
- Brief: Batch insert documents.
- Parameters:
  - `collection` (const std::string &): n/a
  - `docs` (const std::vector< Document > &): Input parameter.
- Return: Return value.
- Details: param Input parameter. docs Input parameter. Return value. Calls: err(), ok(), size().

#### `Result< size_t > batch_insert_vectors(const std::string &collection, const std::vector< Vector > &vectors) override`
- Source: `include/chimera/mongodb_adapter.hpp`:75
- Brief: Batch insert vectors.
- Parameters:
  - `collection` (const std::string &): n/a
  - `vectors` (const std::vector< Vector > &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< TransactionHandle > begin_transaction(IsolationLevel isolation_level=IsolationLevel::READ_COMMITTED) override`
- Source: `include/chimera/mongodb_adapter.hpp`:178
- Brief: Begin transaction.
- Parameters:
  - `isolation_level` (IsolationLevel): n/a
- Return: Return value.
- Details: IsolationLevel Input parameter. Return value. Calls: err(), generate_id(), mark_active(), lock(), ok(), TransactionHandle().

#### `Result< std::string > begin_transaction(const TransactionOptions &options={}) override`
- Source: `include/chimera/mongodb_adapter.hpp`:138
- Brief: Begin transaction.
- Parameters:
  - `options` (const TransactionOptions &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< bool > commit_transaction(const TransactionHandle &handle) override`
- Source: `include/chimera/mongodb_adapter.hpp`:182
- Brief: Commit transaction.
- Parameters:
  - `handle` (const TransactionHandle &): Input parameter.
- Return: Return value.
- Details: handle Input parameter. Return value. Calls: err(), mark_committed(), ok().

#### `Result< bool > commit_transaction(const std::string &transaction_id) override`
- Source: `include/chimera/mongodb_adapter.hpp`:142
- Brief: Commit transaction.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< bool > connect(const std::string &connection_string, const std::map< std::string, std::string > &options={}) override`
- Source: `include/chimera/mongodb_adapter.hpp`:43
- Brief: n/a
- Parameters:
  - `connection_string` (const std::string &): n/a
  - `options` (const std::map< std::string, std::string > &): n/a

#### `Result< bool > create_index(const std::string &collection, size_t dimensions, const std::map< std::string, Scalar > &index_params={}) override`
- Source: `include/chimera/mongodb_adapter.hpp`:87
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `dimensions` (size_t): n/a
  - `index_params` (const std::map< std::string, Scalar > &): n/a

#### `Result< std::string > create_savepoint(const TransactionHandle &handle, const std::string &savepoint_name) override`
- Source: `include/chimera/mongodb_adapter.hpp`:190
- Brief: Create savepoint.
- Parameters:
  - `handle` (const TransactionHandle &): Input parameter.
  - `savepoint_name` (const std::string &): Name of the savepoint.
- Return: Return value.
- Details: handle Input parameter. savepoint_name Name of the savepoint. Return value. Calls: err(), ok().

#### `Result< std::string > create_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/mongodb_adapter.hpp`:145
- Brief: Create savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > disconnect() override`
- Source: `include/chimera/mongodb_adapter.hpp`:48
- Brief: Disconnect.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: clear(), reset(), ok().

#### `Result< std::vector< GraphPath > > execute_graph_query(const std::string &query, const std::map< std::string, Scalar > &params={}) override`
- Source: `include/chimera/mongodb_adapter.hpp`:109
- Brief: n/a
- Parameters:
  - `query` (const std::string &): n/a
  - `params` (const std::map< std::string, Scalar > &): n/a

#### `Result< RelationalTable > execute_query(const std::string &query, const std::vector< Scalar > &params={}) override`
- Source: `include/chimera/mongodb_adapter.hpp`:52
- Brief: Execute query.
- Parameters:
  - `query` (const std::string &): n/a
  - `params` (const std::vector< Scalar > &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err(), ok(), std::move().

#### `Result< std::vector< Document > > find_documents(const std::string &collection, const std::map< std::string, Scalar > &filter, size_t limit=100) override`
- Source: `include/chimera/mongodb_adapter.hpp`:125
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `filter` (const std::map< std::string, Scalar > &): n/a
  - `limit` (size_t): n/a

#### `Result< BatchStatistics > flush() override`
- Source: `include/chimera/mongodb_adapter.hpp`:229
- Brief: Flush.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock(), size(), clear(), ok(), std::move().

#### `std::string generate_id()`
- Source: `include/chimera/mongodb_adapter.hpp`:276
- Brief: Generate id.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: utils::generate_uuid_v4().

#### `const BatchConfig & get_batch_config() const override`
- Source: `include/chimera/mongodb_adapter.hpp`:235
- Brief: Get batch config.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< Capability > get_capabilities() const override`
- Source: `include/chimera/mongodb_adapter.hpp`:172
- Brief: n/a
- Parameters: none

#### `Result< SystemMetrics > get_metrics() const override`
- Source: `include/chimera/mongodb_adapter.hpp`:170
- Brief: n/a
- Parameters: none

#### `size_t get_pending_count() const override`
- Source: `include/chimera/mongodb_adapter.hpp`:231
- Brief: Get pending count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< QueryStatistics > get_query_statistics() const override`
- Source: `include/chimera/mongodb_adapter.hpp`:67
- Brief: n/a
- Parameters: none

#### `Result< SystemInfo > get_system_info() const override`
- Source: `include/chimera/mongodb_adapter.hpp`:169
- Brief: n/a
- Parameters: none

#### `TransactionState get_transaction_state(const TransactionHandle &handle) const override`
- Source: `include/chimera/mongodb_adapter.hpp`:200
- Brief: n/a
- Parameters:
  - `handle` (const TransactionHandle &): n/a

#### `Result< TransactionState > get_transaction_state(const std::string &transaction_id) override`
- Source: `include/chimera/mongodb_adapter.hpp`:164
- Brief: Get transaction state.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< TransactionStats > get_transaction_stats(const std::string &transaction_id) override`
- Source: `include/chimera/mongodb_adapter.hpp`:160
- Brief: Get transaction stats.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `bool has_capability(Capability cap) const override`
- Source: `include/chimera/mongodb_adapter.hpp`:171
- Brief: n/a
- Parameters:
  - `cap` (Capability): n/a

#### `Result< std::string > insert_document(const std::string &collection, const Document &doc) override`
- Source: `include/chimera/mongodb_adapter.hpp`:115
- Brief: Insert document.
- Parameters:
  - `collection` (const std::string &): n/a
  - `doc` (const Document &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err(), generate_id(), ok().

#### `Result< std::string > insert_edge(const GraphEdge &edge) override`
- Source: `include/chimera/mongodb_adapter.hpp`:95
- Brief: Insert edge.
- Parameters:
  - `edge` (const GraphEdge &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: ok(), generate_id(), err().

#### `Result< std::string > insert_node(const GraphNode &node) override`
- Source: `include/chimera/mongodb_adapter.hpp`:94
- Brief: Insert node.
- Parameters:
  - `node` (const GraphNode &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: ok(), generate_id(), err().

#### `Result< size_t > insert_row(const std::string &table_name, const RelationalRow &row) override`
- Source: `include/chimera/mongodb_adapter.hpp`:57
- Brief: Insert row.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `row` (const RelationalRow &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err(), ok().

#### `Result< std::string > insert_vector(const std::string &collection, const Vector &vector) override`
- Source: `include/chimera/mongodb_adapter.hpp`:70
- Brief: Insert vector.
- Parameters:
  - `collection` (const std::string &): n/a
  - `vector` (const Vector &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `bool is_connected() const override`
- Source: `include/chimera/mongodb_adapter.hpp`:49
- Brief: n/a
- Parameters: none

#### `bool is_valid_connection_string(const std::string &cs)`
- Source: `include/chimera/mongodb_adapter.hpp`:282
- Brief: Is valid connection string.
- Parameters:
  - `cs` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: cs Input parameter. True when the operation succeeds. cs Input parameter. True when the operation succeeds. Calls: find().

#### `std::string mask_credentials(const std::string &cs)`
- Source: `include/chimera/mongodb_adapter.hpp`:288
- Brief: Mask credentials.
- Parameters:
  - `cs` (const std::string &): Input parameter.
- Return: Return value.
- Details: cs Input parameter. Return value. cs Input parameter. Return value. Implements mask_credentials without additional internal calls.

#### `Result< std::string > parse_query_to_mongo(const std::string &aql_query) const`
- Source: `include/chimera/mongodb_adapter.hpp`:309
- Brief: Parse query to mongo.
- Parameters:
  - `aql_query` (const std::string &): Input parameter.
- Return: Return value.
- Details: aql_query Input parameter. Return value.

#### `Result< bool > queue_delete(const std::string &table_name, const std::string &where_clause) override`
- Source: `include/chimera/mongodb_adapter.hpp`:224
- Brief: Queue delete.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `where_clause` (const std::string &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. where_clause Input parameter. Return value. Calls: err(), lock(), push_back(), ok().

#### `Result< bool > queue_insert(const std::string &table_name, const RelationalRow &row) override`
- Source: `include/chimera/mongodb_adapter.hpp`:208
- Brief: Queue insert.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `row` (const RelationalRow &): n/a
- Return: Return value.
- Details: table_name Name of the table. param Input parameter. Return value. Calls: err(), lock(), push_back(), ok().

#### `Result< bool > queue_insert_batch(const std::string &table_name, const std::vector< RelationalRow > &rows) override`
- Source: `include/chimera/mongodb_adapter.hpp`:213
- Brief: Queue insert batch.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `rows` (const std::vector< RelationalRow > &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. rows Input parameter. Return value. Calls: err(), lock(), size(), push_back(), ok().

#### `Result< bool > queue_update(const std::string &table_name, const RelationalRow &row, const std::string &where_clause) override`
- Source: `include/chimera/mongodb_adapter.hpp`:218
- Brief: Queue update.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `row` (const RelationalRow &): n/a
  - `where_clause` (const std::string &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. param Input parameter. where_clause Input parameter. Return value. Calls: err(), lock(), push_back(), ok().

#### `Result< bool > release_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/mongodb_adapter.hpp`:155
- Brief: Release savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > rollback_to_savepoint(const TransactionHandle &handle, const std::string &savepoint_name) override`
- Source: `include/chimera/mongodb_adapter.hpp`:195
- Brief: Rollback to savepoint.
- Parameters:
  - `handle` (const TransactionHandle &): Input parameter.
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: handle Input parameter. param Input parameter. Return value. Calls: err(), ok().

#### `Result< bool > rollback_to_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/mongodb_adapter.hpp`:150
- Brief: Rollback to savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > rollback_transaction(const TransactionHandle &handle) override`
- Source: `include/chimera/mongodb_adapter.hpp`:186
- Brief: Rollback transaction.
- Parameters:
  - `handle` (const TransactionHandle &): Input parameter.
- Return: Return value.
- Details: handle Input parameter. Return value. Calls: err(), mark_aborted(), ok().

#### `Result< bool > rollback_transaction(const std::string &transaction_id) override`
- Source: `include/chimera/mongodb_adapter.hpp`:143
- Brief: Rollback transaction.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `std::string row_to_bson_document(const RelationalRow &row)`
- Source: `include/chimera/mongodb_adapter.hpp`:302
- Brief: Row to bson document.
- Parameters:
  - `row` (const RelationalRow &): Input parameter.
- Return: Return value.
- Details: row Input parameter. Return value. param Input parameter. Return value. Implements row_to_bson_document without additional internal calls.

#### `std::string scalar_to_bson_string(const Scalar &scalar)`
- Source: `include/chimera/mongodb_adapter.hpp`:295
- Brief: Scalar to bson string.
- Parameters:
  - `scalar` (const Scalar &): Input parameter.
- Return: Return value.
- Details: scalar Input parameter. Return value. param Input parameter. Return value. Implements scalar_to_bson_string without additional internal calls.

#### `Result< std::vector< std::pair< Vector, double > > > search_vectors(const std::string &collection, const Vector &query_vector, size_t k, const std::map< std::string, Scalar > &filters={}) override`
- Source: `include/chimera/mongodb_adapter.hpp`:80
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `query_vector` (const Vector &): n/a
  - `k` (size_t): n/a
  - `filters` (const std::map< std::string, Scalar > &): n/a

#### `Result< bool > set_batch_config(const BatchConfig &config) override`
- Source: `include/chimera/mongodb_adapter.hpp`:233
- Brief: Set batch config.
- Parameters:
  - `config` (const BatchConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. Calls: lock(), ok().

#### `Result< GraphPath > shortest_path(const std::string &source_id, const std::string &target_id, size_t max_depth=10) override`
- Source: `include/chimera/mongodb_adapter.hpp`:97
- Brief: Shortest path.
- Parameters:
  - `source_id` (const std::string &): n/a
  - `target_id` (const std::string &): n/a
  - `max_depth` (size_t): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. size_t Input parameter. Return value. Calls: err().

#### `Result< std::vector< GraphNode > > traverse(const std::string &start_id, size_t max_depth, const std::vector< std::string > &edge_labels={}) override`
- Source: `include/chimera/mongodb_adapter.hpp`:103
- Brief: Traverse.
- Parameters:
  - `start_id` (const std::string &): n/a
  - `max_depth` (size_t): n/a
  - `edge_labels` (const std::vector< std::string > &): n/a
- Return: Return value.
- Details: param Input parameter. size_t Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< size_t > update_documents(const std::string &collection, const std::map< std::string, Scalar > &filter, const std::map< std::string, Scalar > &updates) override`
- Source: `include/chimera/mongodb_adapter.hpp`:131
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `filter` (const std::map< std::string, Scalar > &): n/a
  - `updates` (const std::map< std::string, Scalar > &): n/a

#### `~MongoDBAdapter() override`
- Source: `include/chimera/mongodb_adapter.hpp`:37
- Brief: n/a
- Parameters: none

### chimera::Neo4jAdapter

#### `Neo4jAdapter()`
- Source: `include/chimera/neo4j_adapter.hpp`:22
- Brief: n/a
- Parameters: none

#### `Result< size_t > batch_insert(const std::string &table_name, const std::vector< RelationalRow > &rows) override`
- Source: `include/chimera/neo4j_adapter.hpp`:49
- Brief: Batch insert.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `rows` (const std::vector< RelationalRow > &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< size_t > batch_insert_documents(const std::string &collection, const std::vector< Document > &docs) override`
- Source: `include/chimera/neo4j_adapter.hpp`:110
- Brief: Batch insert documents.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `docs` (const std::vector< Document > &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. docs Input parameter. Return value. Calls: err(), ok(), size().

#### `Result< size_t > batch_insert_vectors(const std::string &collection, const std::vector< Vector > &vectors) override`
- Source: `include/chimera/neo4j_adapter.hpp`:62
- Brief: Batch insert vectors.
- Parameters:
  - `collection` (const std::string &): n/a
  - `vectors` (const std::vector< Vector > &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< std::string > begin_transaction(const TransactionOptions &options={}) override`
- Source: `include/chimera/neo4j_adapter.hpp`:128
- Brief: ------------------------------------------------------------------------ Transaction Adapter (Supported via Sessions) ------------------------------------------------------------------------
- Parameters:
  - `options` (const TransactionOptions &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err(), generate_id(), lock(), ok().

#### `Result< bool > commit_transaction(const std::string &transaction_id) override`
- Source: `include/chimera/neo4j_adapter.hpp`:132
- Brief: Commit transaction.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. Return value. Calls: lock(), find(), end(), err(), ok().

#### `Result< bool > connect(const std::string &connection_string, const std::map< std::string, std::string > &options={}) override`
- Source: `include/chimera/neo4j_adapter.hpp`:30
- Brief: n/a
- Parameters:
  - `connection_string` (const std::string &): n/a
  - `options` (const std::map< std::string, std::string > &): n/a

#### `Result< bool > create_index(const std::string &collection, size_t dimensions, const std::map< std::string, Scalar > &index_params={}) override`
- Source: `include/chimera/neo4j_adapter.hpp`:74
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `dimensions` (size_t): n/a
  - `index_params` (const std::map< std::string, Scalar > &): n/a

#### `Result< std::string > create_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/neo4j_adapter.hpp`:135
- Brief: Create savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > disconnect() override`
- Source: `include/chimera/neo4j_adapter.hpp`:35
- Brief: Disconnect.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: clear(), lock(), ok().

#### `Result< std::vector< GraphPath > > execute_graph_query(const std::string &query, const std::map< std::string, Scalar > &params={}) override`
- Source: `include/chimera/neo4j_adapter.hpp`:99
- Brief: n/a
- Parameters:
  - `query` (const std::string &): n/a
  - `params` (const std::map< std::string, Scalar > &): n/a

#### `Result< RelationalTable > execute_query(const std::string &query, const std::vector< Scalar > &params={}) override`
- Source: `include/chimera/neo4j_adapter.hpp`:39
- Brief: Execute query.
- Parameters:
  - `query` (const std::string &): n/a
  - `params` (const std::vector< Scalar > &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< std::vector< Document > > find_documents(const std::string &collection, const std::map< std::string, Scalar > &filter, size_t limit=100) override`
- Source: `include/chimera/neo4j_adapter.hpp`:115
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `filter` (const std::map< std::string, Scalar > &): n/a
  - `limit` (size_t): n/a

#### `std::string generate_id()`
- Source: `include/chimera/neo4j_adapter.hpp`:194
- Brief: Generate id.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: utils::generate_uuid_v4().

#### `std::vector< Capability > get_capabilities() const override`
- Source: `include/chimera/neo4j_adapter.hpp`:162
- Brief: n/a
- Parameters: none

#### `Result< SystemMetrics > get_metrics() const override`
- Source: `include/chimera/neo4j_adapter.hpp`:160
- Brief: n/a
- Parameters: none

#### `Result< QueryStatistics > get_query_statistics() const override`
- Source: `include/chimera/neo4j_adapter.hpp`:54
- Brief: n/a
- Parameters: none

#### `Result< SystemInfo > get_system_info() const override`
- Source: `include/chimera/neo4j_adapter.hpp`:159
- Brief: n/a
- Parameters: none

#### `Result< TransactionState > get_transaction_state(const std::string &transaction_id) override`
- Source: `include/chimera/neo4j_adapter.hpp`:154
- Brief: Get transaction state.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: ok(), std::move().

#### `Result< TransactionStats > get_transaction_stats(const std::string &transaction_id) override`
- Source: `include/chimera/neo4j_adapter.hpp`:150
- Brief: Get transaction stats.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: ok(), std::move().

#### `bool has_capability(Capability cap) const override`
- Source: `include/chimera/neo4j_adapter.hpp`:161
- Brief: n/a
- Parameters:
  - `cap` (Capability): n/a

#### `Result< std::string > insert_document(const std::string &collection, const Document &doc) override`
- Source: `include/chimera/neo4j_adapter.hpp`:105
- Brief: ------------------------------------------------------------------------ Document Adapter (Via Node Properties) ------------------------------------------------------------------------
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `doc` (const Document &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. doc Input parameter. Return value. Calls: err(), generate_id(), ok().

#### `Result< std::string > insert_edge(const GraphEdge &edge) override`
- Source: `include/chimera/neo4j_adapter.hpp`:85
- Brief: Insert edge.
- Parameters:
  - `edge` (const GraphEdge &): Input parameter.
- Return: Return value.
- Details: edge Input parameter. Return value. Calls: err(), generate_id(), ok().

#### `Result< std::string > insert_node(const GraphNode &node) override`
- Source: `include/chimera/neo4j_adapter.hpp`:84
- Brief: Insert node.
- Parameters:
  - `node` (const GraphNode &): Input parameter.
- Return: Return value.
- Details: node Input parameter. Return value. Calls: err(), generate_id(), ok().

#### `Result< size_t > insert_row(const std::string &table_name, const RelationalRow &row) override`
- Source: `include/chimera/neo4j_adapter.hpp`:44
- Brief: Insert row.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `row` (const RelationalRow &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< std::string > insert_vector(const std::string &collection, const Vector &vector) override`
- Source: `include/chimera/neo4j_adapter.hpp`:57
- Brief: Insert vector.
- Parameters:
  - `collection` (const std::string &): n/a
  - `vector` (const Vector &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `bool is_connected() const override`
- Source: `include/chimera/neo4j_adapter.hpp`:36
- Brief: n/a
- Parameters: none

#### `bool is_valid_connection_string(const std::string &cs)`
- Source: `include/chimera/neo4j_adapter.hpp`:200
- Brief: Is valid connection string.
- Parameters:
  - `cs` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: cs Input parameter. True when the operation succeeds. cs Input parameter. True when the operation succeeds. Calls: find().

#### `std::string mask_credentials(const std::string &cs)`
- Source: `include/chimera/neo4j_adapter.hpp`:206
- Brief: Mask credentials.
- Parameters:
  - `cs` (const std::string &): Input parameter.
- Return: Return value.
- Details: cs Input parameter. Return value. cs Input parameter. Return value. Implements mask_credentials without additional internal calls.

#### `Result< bool > release_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/neo4j_adapter.hpp`:145
- Brief: Release savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > rollback_to_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/neo4j_adapter.hpp`:140
- Brief: Rollback to savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > rollback_transaction(const std::string &transaction_id) override`
- Source: `include/chimera/neo4j_adapter.hpp`:133
- Brief: Rollback transaction.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. Return value. Calls: lock(), find(), end(), err(), ok().

#### `std::string scalar_to_cypher_literal(const Scalar &scalar)`
- Source: `include/chimera/neo4j_adapter.hpp`:213
- Brief: Scalar to cypher literal.
- Parameters:
  - `scalar` (const Scalar &): Input parameter.
- Return: Return value.
- Details: scalar Input parameter. Return value. param Input parameter. Return value. Implements scalar_to_cypher_literal without additional internal calls.

#### `Result< std::vector< std::pair< Vector, double > > > search_vectors(const std::string &collection, const Vector &query_vector, size_t k, const std::map< std::string, Scalar > &filters={}) override`
- Source: `include/chimera/neo4j_adapter.hpp`:67
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `query_vector` (const Vector &): n/a
  - `k` (size_t): n/a
  - `filters` (const std::map< std::string, Scalar > &): n/a

#### `Result< GraphPath > shortest_path(const std::string &source_id, const std::string &target_id, size_t max_depth=10) override`
- Source: `include/chimera/neo4j_adapter.hpp`:87
- Brief: Shortest path.
- Parameters:
  - `source_id` (const std::string &): Identifier of the source.
  - `target_id` (const std::string &): Identifier of the target.
  - `max_depth` (size_t): Input parameter.
- Return: Return value.
- Details: source_id Identifier of the source. target_id Identifier of the target. max_depth Input parameter. Return value. Calls: err(), ok(), std::move().

#### `Result< std::vector< GraphNode > > traverse(const std::string &start_id, size_t max_depth, const std::vector< std::string > &edge_labels={}) override`
- Source: `include/chimera/neo4j_adapter.hpp`:93
- Brief: Traverse.
- Parameters:
  - `start_id` (const std::string &): Identifier of the start.
  - `max_depth` (size_t): Input parameter.
  - `edge_labels` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: start_id Identifier of the start. max_depth Input parameter. edge_labels Input parameter. Return value. Calls: err(), ok(), std::move().

#### `Result< size_t > update_documents(const std::string &collection, const std::map< std::string, Scalar > &filter, const std::map< std::string, Scalar > &updates) override`
- Source: `include/chimera/neo4j_adapter.hpp`:121
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `filter` (const std::map< std::string, Scalar > &): n/a
  - `updates` (const std::map< std::string, Scalar > &): n/a

#### `~Neo4jAdapter() override`
- Source: `include/chimera/neo4j_adapter.hpp`:24
- Brief: n/a
- Parameters: none

### chimera::QdrantAdapter

#### `QdrantAdapter()`
- Source: `include/chimera/qdrant_adapter.hpp`:24
- Brief: n/a
- Parameters: none

#### `Result< size_t > batch_insert(const std::string &table_name, const std::vector< RelationalRow > &rows) override`
- Source: `include/chimera/qdrant_adapter.hpp`:51
- Brief: Batch insert.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `rows` (const std::vector< RelationalRow > &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< size_t > batch_insert_documents(const std::string &collection, const std::vector< Document > &docs) override`
- Source: `include/chimera/qdrant_adapter.hpp`:112
- Brief: Batch insert documents.
- Parameters:
  - `collection` (const std::string &): n/a
  - `docs` (const std::vector< Document > &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< size_t > batch_insert_vectors(const std::string &collection, const std::vector< Vector > &vectors) override`
- Source: `include/chimera/qdrant_adapter.hpp`:67
- Brief: Batch insert vectors.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `vectors` (const std::vector< Vector > &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. vectors Input parameter. Return value. Calls: err(), lock(), push_back(), generate_id(), ok(), size().

#### `Result< std::string > begin_transaction(const TransactionOptions &options={}) override`
- Source: `include/chimera/qdrant_adapter.hpp`:130
- Brief: Begin transaction.
- Parameters:
  - `options` (const TransactionOptions &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< bool > commit_transaction(const std::string &transaction_id) override`
- Source: `include/chimera/qdrant_adapter.hpp`:134
- Brief: Commit transaction.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< bool > connect(const std::string &connection_string, const std::map< std::string, std::string > &options={}) override`
- Source: `include/chimera/qdrant_adapter.hpp`:32
- Brief: n/a
- Parameters:
  - `connection_string` (const std::string &): n/a
  - `options` (const std::map< std::string, std::string > &): n/a

#### `Result< bool > create_index(const std::string &collection, size_t dimensions, const std::map< std::string, Scalar > &index_params={}) override`
- Source: `include/chimera/qdrant_adapter.hpp`:79
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `dimensions` (size_t): n/a
  - `index_params` (const std::map< std::string, Scalar > &): n/a

#### `Result< std::string > create_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/qdrant_adapter.hpp`:137
- Brief: Create savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > disconnect() override`
- Source: `include/chimera/qdrant_adapter.hpp`:37
- Brief: Disconnect.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: clear(), ok().

#### `Result< std::vector< GraphPath > > execute_graph_query(const std::string &query, const std::map< std::string, Scalar > &params={}) override`
- Source: `include/chimera/qdrant_adapter.hpp`:101
- Brief: n/a
- Parameters:
  - `query` (const std::string &): n/a
  - `params` (const std::map< std::string, Scalar > &): n/a

#### `Result< RelationalTable > execute_query(const std::string &query, const std::vector< Scalar > &params={}) override`
- Source: `include/chimera/qdrant_adapter.hpp`:41
- Brief: Execute query.
- Parameters:
  - `query` (const std::string &): n/a
  - `params` (const std::vector< Scalar > &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< std::vector< Document > > find_documents(const std::string &collection, const std::map< std::string, Scalar > &filter, size_t limit=100) override`
- Source: `include/chimera/qdrant_adapter.hpp`:117
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `filter` (const std::map< std::string, Scalar > &): n/a
  - `limit` (size_t): n/a

#### `Result< BatchStatistics > flush() override`
- Source: `include/chimera/qdrant_adapter.hpp`:191
- Brief: Flush.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lock(), size(), clear(), ok(), std::move().

#### `std::string generate_id()`
- Source: `include/chimera/qdrant_adapter.hpp`:230
- Brief: Generate id.
- Parameters: none
- Return: Return value.
- Details: Return value. Return value. Calls: utils::generate_uuid_v4().

#### `const BatchConfig & get_batch_config() const override`
- Source: `include/chimera/qdrant_adapter.hpp`:197
- Brief: Get batch config.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::vector< Capability > get_capabilities() const override`
- Source: `include/chimera/qdrant_adapter.hpp`:164
- Brief: n/a
- Parameters: none

#### `Result< SystemMetrics > get_metrics() const override`
- Source: `include/chimera/qdrant_adapter.hpp`:162
- Brief: n/a
- Parameters: none

#### `size_t get_pending_count() const override`
- Source: `include/chimera/qdrant_adapter.hpp`:193
- Brief: Get pending count.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< QueryStatistics > get_query_statistics() const override`
- Source: `include/chimera/qdrant_adapter.hpp`:56
- Brief: n/a
- Parameters: none

#### `Result< SystemInfo > get_system_info() const override`
- Source: `include/chimera/qdrant_adapter.hpp`:161
- Brief: n/a
- Parameters: none

#### `Result< TransactionState > get_transaction_state(const std::string &transaction_id) override`
- Source: `include/chimera/qdrant_adapter.hpp`:156
- Brief: Get transaction state.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< TransactionStats > get_transaction_stats(const std::string &transaction_id) override`
- Source: `include/chimera/qdrant_adapter.hpp`:152
- Brief: Get transaction stats.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `bool has_capability(Capability cap) const override`
- Source: `include/chimera/qdrant_adapter.hpp`:163
- Brief: n/a
- Parameters:
  - `cap` (Capability): n/a

#### `Result< std::string > insert_document(const std::string &collection, const Document &doc) override`
- Source: `include/chimera/qdrant_adapter.hpp`:107
- Brief: Insert document.
- Parameters:
  - `collection` (const std::string &): n/a
  - `doc` (const Document &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< std::string > insert_edge(const GraphEdge &edge) override`
- Source: `include/chimera/qdrant_adapter.hpp`:87
- Brief: Insert edge.
- Parameters:
  - `edge` (const GraphEdge &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< std::string > insert_node(const GraphNode &node) override`
- Source: `include/chimera/qdrant_adapter.hpp`:86
- Brief: Insert node.
- Parameters:
  - `node` (const GraphNode &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< size_t > insert_row(const std::string &table_name, const RelationalRow &row) override`
- Source: `include/chimera/qdrant_adapter.hpp`:46
- Brief: Insert row.
- Parameters:
  - `table_name` (const std::string &): n/a
  - `row` (const RelationalRow &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< std::string > insert_vector(const std::string &collection, const Vector &vector) override`
- Source: `include/chimera/qdrant_adapter.hpp`:62
- Brief: Insert vector.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `vector` (const Vector &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. vector Input parameter. Return value. Calls: err(), generate_id(), ok().

#### `bool is_connected() const override`
- Source: `include/chimera/qdrant_adapter.hpp`:38
- Brief: n/a
- Parameters: none

#### `bool is_valid_connection_string(const std::string &cs)`
- Source: `include/chimera/qdrant_adapter.hpp`:236
- Brief: Is valid connection string.
- Parameters:
  - `cs` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: cs Input parameter. True when the operation succeeds. cs Input parameter. True when the operation succeeds. Calls: find().

#### `std::string mask_credentials(const std::string &cs)`
- Source: `include/chimera/qdrant_adapter.hpp`:242
- Brief: Mask credentials.
- Parameters:
  - `cs` (const std::string &): Input parameter.
- Return: Return value.
- Details: cs Input parameter. Return value. cs Input parameter. Return value. Implements mask_credentials without additional internal calls.

#### `Result< bool > queue_delete(const std::string &table_name, const std::string &where_clause) override`
- Source: `include/chimera/qdrant_adapter.hpp`:186
- Brief: Queue delete.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `where_clause` (const std::string &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. where_clause Input parameter. Return value. Calls: err().

#### `Result< bool > queue_insert(const std::string &table_name, const RelationalRow &row) override`
- Source: `include/chimera/qdrant_adapter.hpp`:170
- Brief: Queue insert.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `row` (const RelationalRow &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. row Input parameter. Return value. Calls: err().

#### `Result< bool > queue_insert_batch(const std::string &table_name, const std::vector< RelationalRow > &rows) override`
- Source: `include/chimera/qdrant_adapter.hpp`:175
- Brief: Queue insert batch.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `rows` (const std::vector< RelationalRow > &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. rows Input parameter. Return value. Calls: err().

#### `Result< bool > queue_update(const std::string &table_name, const RelationalRow &row, const std::string &where_clause) override`
- Source: `include/chimera/qdrant_adapter.hpp`:180
- Brief: Queue update.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `row` (const RelationalRow &): Input parameter.
  - `where_clause` (const std::string &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. row Input parameter. where_clause Input parameter. Return value. Calls: err().

#### `Result< bool > release_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/qdrant_adapter.hpp`:147
- Brief: Release savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > rollback_to_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/qdrant_adapter.hpp`:142
- Brief: Rollback to savepoint.
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `savepoint_name` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< bool > rollback_transaction(const std::string &transaction_id) override`
- Source: `include/chimera/qdrant_adapter.hpp`:135
- Brief: Rollback transaction.
- Parameters:
  - `transaction_id` (const std::string &): n/a
- Return: Return value.
- Details: param Input parameter. Return value. Calls: err().

#### `Result< std::vector< std::pair< Vector, double > > > search_vectors(const std::string &collection, const Vector &query_vector, size_t k, const std::map< std::string, Scalar > &filters={}) override`
- Source: `include/chimera/qdrant_adapter.hpp`:72
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `query_vector` (const Vector &): n/a
  - `k` (size_t): n/a
  - `filters` (const std::map< std::string, Scalar > &): n/a

#### `Result< bool > set_batch_config(const BatchConfig &config) override`
- Source: `include/chimera/qdrant_adapter.hpp`:195
- Brief: Set batch config.
- Parameters:
  - `config` (const BatchConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. Calls: lock(), ok().

#### `Result< GraphPath > shortest_path(const std::string &source_id, const std::string &target_id, size_t max_depth=10) override`
- Source: `include/chimera/qdrant_adapter.hpp`:89
- Brief: Shortest path.
- Parameters:
  - `source_id` (const std::string &): n/a
  - `target_id` (const std::string &): n/a
  - `max_depth` (size_t): n/a
- Return: Return value.
- Details: param Input parameter. param Input parameter. size_t Input parameter. Return value. Calls: err().

#### `Result< std::vector< GraphNode > > traverse(const std::string &start_id, size_t max_depth, const std::vector< std::string > &edge_labels={}) override`
- Source: `include/chimera/qdrant_adapter.hpp`:95
- Brief: Traverse.
- Parameters:
  - `start_id` (const std::string &): n/a
  - `max_depth` (size_t): n/a
  - `edge_labels` (const std::vector< std::string > &): n/a
- Return: Return value.
- Details: param Input parameter. size_t Input parameter. param Input parameter. Return value. Calls: err().

#### `Result< size_t > update_documents(const std::string &collection, const std::map< std::string, Scalar > &filter, const std::map< std::string, Scalar > &updates) override`
- Source: `include/chimera/qdrant_adapter.hpp`:123
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `filter` (const std::map< std::string, Scalar > &): n/a
  - `updates` (const std::map< std::string, Scalar > &): n/a

#### `~QdrantAdapter() override`
- Source: `include/chimera/qdrant_adapter.hpp`:26
- Brief: n/a
- Parameters: none

### chimera::Result

#### `Result< T > err(ErrorCode c, std::string m)`
- Source: `include/chimera/database_adapter.hpp`:107
- Brief: Err.
- Parameters:
  - `c` (ErrorCode): Input parameter.
  - `m` (std::string): Input parameter.
- Return: Return value.
- Details: c Input parameter. m Input parameter. Return value. Calls: std::move().

#### `bool is_err() const`
- Source: `include/chimera/database_adapter.hpp`:91
- Brief: n/a
- Parameters: none

#### `bool is_ok() const`
- Source: `include/chimera/database_adapter.hpp`:90
- Brief: n/a
- Parameters: none

#### `Result< T > ok(T v)`
- Source: `include/chimera/database_adapter.hpp`:99
- Brief: Ok.
- Parameters:
  - `v` (T): Input parameter.
- Return: Return value.
- Details: v Input parameter. Return value. Calls: std::move().

### chimera::RetryExecutor

#### `RetryExecutor()=default`
- Source: `include/chimera/retry_executor.hpp`:34
- Brief: n/a
- Parameters: none

#### `std::chrono::milliseconds calculate_backoff(uint32_t attempt, const RetryPolicy &policy) noexcept`
- Source: `include/chimera/retry_executor.hpp`:73
- Brief: Calculate backoff.
- Parameters:
  - `attempt` (uint32_t): Input parameter.
  - `policy` (const RetryPolicy &): Input parameter.
- Return: Return value.
- Details: attempt Input parameter. policy Input parameter. Return value. Exception safety: noexcept.

#### `typename std::invoke_result< Func >::type execute_with_retry(Func &&fn, const RetryPolicy &policy)`
- Source: `include/chimera/retry_executor.hpp`:44
- Brief: Execute with retry.
- Parameters:
  - `fn` (Func &&): Input parameter.
  - `policy` (const RetryPolicy &): Input parameter.
- Return: Return value.
- Details: fn Input parameter. policy Input parameter. Return value.

#### `double get_jitter(double factor) noexcept`
- Source: `include/chimera/retry_executor.hpp`:97
- Brief: Get jitter.
- Parameters:
  - `factor` (double): Input parameter.
- Return: Return value.
- Details: factor Input parameter. Return value. Exception safety: noexcept.

#### `bool should_retry(const Result< bool > &result, const RetryPolicy &policy) noexcept`
- Source: `include/chimera/retry_executor.hpp`:85
- Brief: Should retry.
- Parameters:
  - `result` (const Result< bool > &): Input parameter.
  - `policy` (const RetryPolicy &): Input parameter.
- Return: True when the operation succeeds.
- Details: result Input parameter. policy Input parameter. True when the operation succeeds. Exception safety: noexcept.

#### `~RetryExecutor()=default`
- Source: `include/chimera/retry_executor.hpp`:35
- Brief: n/a
- Parameters: none

### chimera::ThemisDBAdapter

#### `ThemisDBAdapter()=default`
- Source: `include/chimera/themisdb_adapter.hpp`:110
- Brief: n/a
- Parameters: none

#### `ThemisDBAdapter(themis::QueryEngine *query_engine, themis::VectorIndexManager *vector_index=nullptr, themis::GraphIndexManager *graph_index=nullptr)`
- Source: `include/chimera/themisdb_adapter.hpp`:113
- Brief: n/a
- Parameters:
  - `query_engine` (themis::QueryEngine *): n/a
  - `vector_index` (themis::VectorIndexManager *): n/a
  - `graph_index` (themis::GraphIndexManager *): n/a

#### `Result< size_t > batch_insert(const std::string &table_name, const std::vector< RelationalRow > &rows) override`
- Source: `include/chimera/themisdb_adapter.hpp`:139
- Brief: Batch insert.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `rows` (const std::vector< RelationalRow > &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. rows Input parameter. Return value. Calls: err(), lock(), insert(), end(), begin(), ok(), size().

#### `std::future< Result< size_t > > batch_insert_async(const std::string &table_name, const std::vector< RelationalRow > &rows, std::function< void(size_t processed)> progress_callback=nullptr, const AsyncQueryOptions &opts={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:253
- Brief: n/a
- Parameters:
  - `table_name` (const std::string &): n/a
  - `rows` (const std::vector< RelationalRow > &): n/a
  - `progress_callback` (std::function< void(size_t processed)>): n/a
  - `opts` (const AsyncQueryOptions &): n/a

#### `Result< size_t > batch_insert_documents(const std::string &collection, const std::vector< Document > &docs) override`
- Source: `include/chimera/themisdb_adapter.hpp`:197
- Brief: Batch insert documents.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `docs` (const std::vector< Document > &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. docs Input parameter. Return value. Calls: err(), lock(), empty(), generate_id(), std::move(), ok(), size().

#### `Result< size_t > batch_insert_vectors(const std::string &collection, const std::vector< Vector > &vectors) override`
- Source: `include/chimera/themisdb_adapter.hpp`:152
- Brief: Batch insert vectors.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `vectors` (const std::vector< Vector > &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. vectors Input parameter. Return value. Calls: err(), lock(), reserve(), size(), emplace_back(), generate_id(), ok().

#### `Result< std::string > begin_transaction(const TransactionOptions &options={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:215
- Brief: Begin transaction.
- Parameters:
  - `options` (const TransactionOptions &): Input parameter.
- Return: Return value.
- Details: options Input parameter. Return value. Calls: err(), lock(), str(), std::chrono::system_clock::now(), std::chrono::steady_clock::now(), emplace(), std::move(), ok().

#### `Result< bool > cancel_async(const std::string &operation_id) override`
- Source: `include/chimera/themisdb_adapter.hpp`:268
- Brief: Cancel async.
- Parameters:
  - `operation_id` (const std::string &): Identifier of the operation.
- Return: Return value.
- Details: operation_id Identifier of the operation. Return value. Calls: empty(), err(), lk(), find(), end(), store(), ok().

#### `Result< bool > commit_transaction(const std::string &transaction_id) override`
- Source: `include/chimera/themisdb_adapter.hpp`:219
- Brief: Commit transaction.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. Return value. Calls: err(), empty(), lock(), find(), end(), erase(), ok().

#### `Result< bool > connect(const std::string &connection_string, const std::map< std::string, std::string > &options={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:120
- Brief: n/a
- Parameters:
  - `connection_string` (const std::string &): n/a
  - `options` (const std::map< std::string, std::string > &): n/a

#### `Result< bool > create_index(const std::string &collection, size_t dimensions, const std::map< std::string, Scalar > &index_params={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:164
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `dimensions` (size_t): n/a
  - `index_params` (const std::map< std::string, Scalar > &): n/a

#### `Result< std::string > create_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/themisdb_adapter.hpp`:221
- Brief: Create savepoint.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
  - `savepoint_name` (const std::string &): Name of the savepoint.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. savepoint_name Name of the savepoint. Return value. Calls: err(), empty(), lock(), find(), end(), count(), push_back(), insert().

#### `Result< bool > disconnect() override`
- Source: `include/chimera/themisdb_adapter.hpp`:125
- Brief: Disconnect.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: clear(), ok().

#### `Result< std::vector< GraphPath > > execute_graph_query(const std::string &query, const std::map< std::string, Scalar > &params={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:186
- Brief: n/a
- Parameters:
  - `query` (const std::string &): n/a
  - `params` (const std::map< std::string, Scalar > &): n/a

#### `Result< RelationalTable > execute_query(const std::string &query, const std::vector< Scalar > &params={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:129
- Brief: Execute query.
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `params` (const std::vector< Scalar > &): Input parameter.
- Return: Return value.
- Details: query Input parameter. params Input parameter. Return value. Calls: err(), defined(), themis::executeAql(), error(), message(), value(), contains(), is_array().

#### `std::future< Result< RelationalTable > > execute_query_async(const std::string &query, const std::vector< Scalar > &params={}, const AsyncQueryOptions &opts={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:247
- Brief: Execute query async.
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `params` (const std::vector< Scalar > &): Input parameter.
  - `opts` (const AsyncQueryOptions &): Input parameter.
- Return: Return value.
- Details: query Input parameter. params Input parameter. opts Input parameter. Return value. Calls: register_cancel_token(), set_value(), err(), get_future(), std::async(), load(), execute_query().

#### `Result< std::unique_ptr< IResultStream > > execute_query_stream(const std::string &query, const std::vector< Scalar > &params={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:271
- Brief: ------------------------------------------------------------------------ IStreamingAdapter — pull-based cursor over in-memory result sets ------------------------------------------------------------------------
- Parameters:
  - `query` (const std::string &): Input parameter.
  - `params` (const std::vector< Scalar > &): Input parameter.
- Return: Return value.
- Details: query Input parameter. params Input parameter. Return value. Calls: execute_query(), is_ok(), err(), lk(), std::move(), ok().

#### `Result< std::vector< Document > > find_documents(const std::string &collection, const std::map< std::string, Scalar > &filter, size_t limit=100) override`
- Source: `include/chimera/themisdb_adapter.hpp`:202
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `filter` (const std::map< std::string, Scalar > &): n/a
  - `limit` (size_t): n/a

#### `std::string generate_id()`
- Source: `include/chimera/themisdb_adapter.hpp`:330
- Brief: ── Private helpers ──────────────────────────────────────────────────────
- Parameters: none
- Return: Return value.
- Details: Generate id. Return value. Return value. Calls: utils::generate_uuid_v4().

#### `std::vector< Capability > get_capabilities() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:244
- Brief: n/a
- Parameters: none

#### `Result< SystemMetrics > get_metrics() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:242
- Brief: n/a
- Parameters: none

#### `Result< QueryStatistics > get_query_statistics() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:144
- Brief: n/a
- Parameters: none

#### `Result< SystemInfo > get_system_info() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:241
- Brief: n/a
- Parameters: none

#### `Result< TransactionState > get_transaction_state(const std::string &transaction_id) override`
- Source: `include/chimera/themisdb_adapter.hpp`:236
- Brief: Get transaction state.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. Return value. Calls: err(), empty(), lock(), find(), end(), std::chrono::steady_clock::now(), ok(), std::move().

#### `Result< TransactionStats > get_transaction_stats(const std::string &transaction_id) override`
- Source: `include/chimera/themisdb_adapter.hpp`:233
- Brief: Get transaction stats.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. Return value. Calls: err(), empty(), lock(), find(), end(), std::chrono::steady_clock::now(), size(), ok().

#### `bool has_capability(Capability cap) const override`
- Source: `include/chimera/themisdb_adapter.hpp`:243
- Brief: n/a
- Parameters:
  - `cap` (Capability): n/a

#### `Result< std::string > insert_document(const std::string &collection, const Document &doc) override`
- Source: `include/chimera/themisdb_adapter.hpp`:192
- Brief: Insert document.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `doc` (const Document &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. doc Input parameter. Return value. Calls: err(), empty(), generate_id(), lock(), std::move(), ok().

#### `Result< std::string > insert_edge(const GraphEdge &edge) override`
- Source: `include/chimera/themisdb_adapter.hpp`:172
- Brief: Insert edge.
- Parameters:
  - `edge` (const GraphEdge &): Input parameter.
- Return: Return value.
- Details: edge Input parameter. Return value. Calls: err(), empty(), generate_id(), lock(), emplace_back(), ok().

#### `Result< std::string > insert_node(const GraphNode &node) override`
- Source: `include/chimera/themisdb_adapter.hpp`:171
- Brief: Insert node.
- Parameters:
  - `node` (const GraphNode &): Input parameter.
- Return: Return value.
- Details: node Input parameter. Return value. Calls: err(), empty(), generate_id(), lock(), std::move(), ok().

#### `Result< size_t > insert_row(const std::string &table_name, const RelationalRow &row) override`
- Source: `include/chimera/themisdb_adapter.hpp`:134
- Brief: Insert row.
- Parameters:
  - `table_name` (const std::string &): Name of the table.
  - `row` (const RelationalRow &): Input parameter.
- Return: Return value.
- Details: table_name Name of the table. row Input parameter. Return value. Calls: err(), lock(), push_back(), ok().

#### `Result< std::string > insert_vector(const std::string &collection, const Vector &vector) override`
- Source: `include/chimera/themisdb_adapter.hpp`:147
- Brief: Insert vector.
- Parameters:
  - `collection` (const std::string &): Input parameter.
  - `vector` (const Vector &): Input parameter.
- Return: Return value.
- Details: collection Input parameter. vector Input parameter. Return value. Calls: err(), generate_id(), lock(), emplace_back(), ok().

#### `bool is_connected() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:126
- Brief: n/a
- Parameters: none

#### `bool is_valid_connection_string(const std::string &connection_string)`
- Source: `include/chimera/themisdb_adapter.hpp`:351
- Brief: Is valid connection string.
- Parameters:
  - `connection_string` (const std::string &): Input parameter.
- Return: True when the operation succeeds.
- Details: connection_string Input parameter. True when the operation succeeds. connection_string Input parameter. True when the operation succeeds. Calls: rfind().

#### `Result< std::vector< std::string > > list_prepared() override`
- Source: `include/chimera/themisdb_adapter.hpp`:285
- Brief: List prepared.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: lk(), reserve(), size(), push_back(), ok(), std::move().

#### `std::string mask_credentials(const std::string &connection_string)`
- Source: `include/chimera/themisdb_adapter.hpp`:357
- Brief: Mask credentials.
- Parameters:
  - `connection_string` (const std::string &): Input parameter.
- Return: Return value.
- Details: connection_string Input parameter. Return value. connection_string Input parameter. Return value. Calls: rfind(), substr(), size(), find().

#### `Result< std::unique_ptr< IPreparedStatement > > prepare(const std::string &query) override`
- Source: `include/chimera/themisdb_adapter.hpp`:279
- Brief: ------------------------------------------------------------------------ IPreparedStatementAdapter — plan-cached statement management ------------------------------------------------------------------------
- Parameters:
  - `query` (const std::string &): Input parameter.
- Return: Return value.
- Details: query Input parameter. Return value. Calls: empty(), err(), generate_id(), lk(), emplace(), ok(), std::move().

#### `Result< bool > release_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/themisdb_adapter.hpp`:229
- Brief: Release savepoint.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
  - `savepoint_name` (const std::string &): Name of the savepoint.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. savepoint_name Name of the savepoint. Return value. Calls: err(), empty(), lock(), find(), end(), std::find(), begin(), erase().

#### `Result< bool > rollback_to_savepoint(const std::string &transaction_id, const std::string &savepoint_name) override`
- Source: `include/chimera/themisdb_adapter.hpp`:225
- Brief: Rollback to savepoint.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
  - `savepoint_name` (const std::string &): Name of the savepoint.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. savepoint_name Name of the savepoint. Return value. Calls: err(), empty(), lock(), find(), end(), std::find(), begin(), erase().

#### `Result< bool > rollback_transaction(const std::string &transaction_id) override`
- Source: `include/chimera/themisdb_adapter.hpp`:220
- Brief: Rollback transaction.
- Parameters:
  - `transaction_id` (const std::string &): Identifier of the transaction.
- Return: Return value.
- Details: transaction_id Identifier of the transaction. Return value. Calls: err(), empty(), lock(), find(), end(), erase(), ok().

#### `Result< std::vector< std::pair< Vector, double > > > search_vectors(const std::string &collection, const Vector &query_vector, size_t k, const std::map< std::string, Scalar > &filters={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:157
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `query_vector` (const Vector &): n/a
  - `k` (size_t): n/a
  - `filters` (const std::map< std::string, Scalar > &): n/a

#### `std::future< Result< std::vector< std::pair< Vector, double > > > > search_vectors_async(const std::string &collection, const Vector &query_vector, size_t k, const std::map< std::string, Scalar > &filters={}, const AsyncQueryOptions &opts={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:260
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `query_vector` (const Vector &): n/a
  - `k` (size_t): n/a
  - `filters` (const std::map< std::string, Scalar > &): n/a
  - `opts` (const AsyncQueryOptions &): n/a

#### `void setConnectionPool(std::function< void *()> acquire_fn)`
- Source: `include/chimera/themisdb_adapter.hpp`:287
- Brief: n/a
- Parameters:
  - `acquire_fn` (std::function< void *()>): n/a

#### `Result< bool > set_stream_config(const StreamConfig &config) override`
- Source: `include/chimera/themisdb_adapter.hpp`:276
- Brief: Set stream config.
- Parameters:
  - `config` (const StreamConfig &): Input parameter.
- Return: Return value.
- Details: config Input parameter. Return value. Calls: lk(), ok().

#### `Result< GraphPath > shortest_path(const std::string &source_id, const std::string &target_id, size_t max_depth=10) override`
- Source: `include/chimera/themisdb_adapter.hpp`:174
- Brief: Shortest path.
- Parameters:
  - `source_id` (const std::string &): Identifier of the source.
  - `target_id` (const std::string &): Identifier of the target.
  - `max_depth` (size_t): Input parameter.
- Return: Return value.
- Details: source_id Identifier of the source. target_id Identifier of the target. max_depth Input parameter. Return value. Calls: err(), defined(), std::tie(), dijkstraWithConstraints(), dijkstra(), find(), end(), push_back().

#### `Result< std::vector< GraphNode > > traverse(const std::string &start_id, size_t max_depth, const std::vector< std::string > &edge_labels={}) override`
- Source: `include/chimera/themisdb_adapter.hpp`:180
- Brief: Traverse.
- Parameters:
  - `start_id` (const std::string &): Identifier of the start.
  - `max_depth` (size_t): Input parameter.
  - `edge_labels` (const std::vector< std::string > &): Input parameter.
- Return: Return value.
- Details: start_id Identifier of the start. max_depth Input parameter. edge_labels Input parameter. Return value. Calls: err(), defined(), insert(), find(), end(), push_back(), std::move(), empty().

#### `Result< bool > unprepare(const std::string &statement_id) override`
- Source: `include/chimera/themisdb_adapter.hpp`:283
- Brief: Unprepare.
- Parameters:
  - `statement_id` (const std::string &): Identifier of the statement.
- Return: Return value.
- Details: statement_id Identifier of the statement. Return value. Calls: lk(), find(), end(), err(), erase(), ok().

#### `Result< size_t > update_documents(const std::string &collection, const std::map< std::string, Scalar > &filter, const std::map< std::string, Scalar > &updates) override`
- Source: `include/chimera/themisdb_adapter.hpp`:208
- Brief: n/a
- Parameters:
  - `collection` (const std::string &): n/a
  - `filter` (const std::map< std::string, Scalar > &): n/a
  - `updates` (const std::map< std::string, Scalar > &): n/a

#### `~ThemisDBAdapter() override=default`
- Source: `include/chimera/themisdb_adapter.hpp`:111
- Brief: n/a
- Parameters: none

### chimera::ThemisDBPreparedStatement

#### `ThemisDBPreparedStatement(std::string id, std::string query, IDatabaseAdapter *adapter)`
- Source: `include/chimera/themisdb_adapter.hpp`:59
- Brief: n/a
- Parameters:
  - `id` (std::string): n/a
  - `query` (std::string): n/a
  - `adapter` (IDatabaseAdapter *): n/a

#### `std::string apply_named_params() const`
- Source: `include/chimera/themisdb_adapter.hpp`:96
- Brief: Apply named params.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< bool > bind(const std::string &name, const Scalar &value) override`
- Source: `include/chimera/themisdb_adapter.hpp`:68
- Brief: Bind.
- Parameters:
  - `name` (const std::string &): Input parameter.
  - `value` (const Scalar &): Input parameter.
- Return: Return value.
- Details: name Input parameter. value Input parameter. Return value. Calls: empty(), err(), ok().

#### `Result< bool > bind(size_t position, const Scalar &value) override`
- Source: `include/chimera/themisdb_adapter.hpp`:69
- Brief: Bind.
- Parameters:
  - `position` (size_t): Input parameter.
  - `value` (const Scalar &): Input parameter.
- Return: Return value.
- Details: position Input parameter. value Input parameter. Return value. Calls: ok().

#### `Result< bool > bind_all(const std::map< std::string, Scalar > &params) override`
- Source: `include/chimera/themisdb_adapter.hpp`:70
- Brief: n/a
- Parameters:
  - `params` (const std::map< std::string, Scalar > &): n/a

#### `std::vector< Scalar > build_positional_params() const`
- Source: `include/chimera/themisdb_adapter.hpp`:102
- Brief: Build positional params.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< RelationalTable > execute() override`
- Source: `include/chimera/themisdb_adapter.hpp`:74
- Brief: Execute.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::chrono::steady_clock::now(), apply_named_params(), build_positional_params(), execute_query(), lk().

#### `std::future< Result< RelationalTable > > execute_async() override`
- Source: `include/chimera/themisdb_adapter.hpp`:75
- Brief: Execute async.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: std::async(), execute().

#### `std::string get_id() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:65
- Brief: Get id.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::string get_query() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:66
- Brief: Get query.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< QueryStatistics > get_statistics() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:77
- Brief: Get statistics.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `Result< bool > reset() override`
- Source: `include/chimera/themisdb_adapter.hpp`:76
- Brief: Reset the modification detection flag.
- Parameters: none
- Return: None.
- Details: None. Calls: clear(), ok().

### chimera::ThemisDBResultStream

#### `ThemisDBResultStream(RelationalTable table, StreamConfig config={})`
- Source: `include/chimera/themisdb_adapter.hpp`:37
- Brief: n/a
- Parameters:
  - `table` (RelationalTable): n/a
  - `config` (StreamConfig): n/a

#### `Result< bool > close() override`
- Source: `include/chimera/themisdb_adapter.hpp`:48
- Brief: Close.
- Parameters: none
- Return: Return value.
- Details: Return value. Calls: ok().

#### `bool has_more() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:42
- Brief: Has more.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds.

#### `Result< std::vector< RelationalRow > > next_batch(size_t batch_size=0) override`
- Source: `include/chimera/themisdb_adapter.hpp`:43
- Brief: Next batch.
- Parameters:
  - `batch_size` (size_t): Input parameter.
- Return: Return value.
- Details: batch_size Input parameter. Return value. Calls: err(), size(), ok(), std::min(), batch(), begin(), std::move().

#### `size_t position() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:46
- Brief: Position.
- Parameters: none
- Return: Return value.
- Details: Return value.

#### `std::optional< size_t > total_size() const override`
- Source: `include/chimera/themisdb_adapter.hpp`:47
- Brief: Total size.
- Parameters: none
- Return: Return value.
- Details: Return value.

### chimera::TransactionContext

#### `TransactionContext(const std::string &transaction_id, IsolationLevel isolation_level=IsolationLevel::READ_COMMITTED)`
- Source: `include/chimera/transaction.hpp`:47
- Brief: n/a
- Parameters:
  - `transaction_id` (const std::string &): n/a
  - `isolation_level` (IsolationLevel): n/a

#### `void clear_operations() noexcept`
- Source: `include/chimera/transaction.hpp`:129
- Brief: Clear operations.
- Parameters: none
- Details: Exception safety: noexcept.

#### `bool create_savepoint(const std::string &savepoint_name) noexcept`
- Source: `include/chimera/transaction.hpp`:137
- Brief: Create savepoint.
- Parameters:
  - `savepoint_name` (const std::string &): Name of the savepoint.
- Return: True when the operation succeeds.
- Details: savepoint_name Name of the savepoint. True when the operation succeeds. Exception safety: noexcept.

#### `std::string get_id() const noexcept`
- Source: `include/chimera/transaction.hpp`:59
- Brief: Get id.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `IsolationLevel get_isolation_level() const noexcept`
- Source: `include/chimera/transaction.hpp`:71
- Brief: Get isolation level.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `const std::vector< Operation > & get_operations() const noexcept`
- Source: `include/chimera/transaction.hpp`:124
- Brief: Get operations.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `size_t get_savepoint_operation_count(const std::string &savepoint_name) const noexcept`
- Source: `include/chimera/transaction.hpp`:150
- Brief: Get savepoint operation count.
- Parameters:
  - `savepoint_name` (const std::string &): Name of the savepoint.
- Return: Return value.
- Details: savepoint_name Name of the savepoint. Return value. Exception safety: noexcept.

#### `std::vector< std::string > get_savepoints() const noexcept`
- Source: `include/chimera/transaction.hpp`:143
- Brief: Get savepoints.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `std::chrono::system_clock::time_point get_start_time() const noexcept`
- Source: `include/chimera/transaction.hpp`:77
- Brief: Get start time.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `TransactionState get_state() const noexcept`
- Source: `include/chimera/transaction.hpp`:65
- Brief: Get state.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `bool is_active() const noexcept`
- Source: `include/chimera/transaction.hpp`:105
- Brief: Is active.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Exception safety: noexcept.

#### `bool is_terminal() const noexcept`
- Source: `include/chimera/transaction.hpp`:111
- Brief: Is terminal.
- Parameters: none
- Return: True when the operation succeeds.
- Details: True when the operation succeeds. Exception safety: noexcept.

#### `void mark_aborted() noexcept`
- Source: `include/chimera/transaction.hpp`:93
- Brief: Mark aborted.
- Parameters: none
- Details: Exception safety: noexcept.

#### `void mark_active() noexcept`
- Source: `include/chimera/transaction.hpp`:83
- Brief: Mark active.
- Parameters: none
- Details: Exception safety: noexcept.

#### `void mark_committed() noexcept`
- Source: `include/chimera/transaction.hpp`:88
- Brief: Mark committed.
- Parameters: none
- Details: Exception safety: noexcept.

#### `void mark_failed() noexcept`
- Source: `include/chimera/transaction.hpp`:98
- Brief: Mark failed.
- Parameters: none
- Details: Exception safety: noexcept.

#### `void record_operation(const Operation &op) noexcept`
- Source: `include/chimera/transaction.hpp`:118
- Brief: Record operation.
- Parameters:
  - `op` (const Operation &): Input parameter.
- Details: op Input parameter. Exception safety: noexcept.

#### `~TransactionContext()=default`
- Source: `include/chimera/transaction.hpp`:52
- Brief: n/a
- Parameters: none

### chimera::TransactionHandle

#### `TransactionHandle(std::shared_ptr< TransactionContext > context) noexcept`
- Source: `include/chimera/transaction.hpp`:169
- Brief: Transaction Handle.
- Parameters:
  - `context` (std::shared_ptr< TransactionContext >): Input parameter.
- Return: Return value.
- Details: context Input parameter. Return value. Exception safety: noexcept.

#### `const TransactionContext * get() const noexcept`
- Source: `include/chimera/transaction.hpp`:182
- Brief: Get.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Exception safety: noexcept.

#### `TransactionContext * get() noexcept`
- Source: `include/chimera/transaction.hpp`:176
- Brief: Get.
- Parameters: none
- Return: Pointer to the result.
- Details: Pointer to the result. Exception safety: noexcept.

#### `operator bool() const noexcept`
- Source: `include/chimera/transaction.hpp`:192
- Brief: Bool.
- Parameters: none
- Return: Return value.
- Details: Return value. Exception safety: noexcept.

#### `const TransactionContext & operator*() const noexcept`
- Source: `include/chimera/transaction.hpp`:184
- Brief: n/a
- Parameters: none

#### `TransactionContext & operator*() noexcept`
- Source: `include/chimera/transaction.hpp`:183
- Brief: n/a
- Parameters: none

#### `const TransactionContext * operator->() const noexcept`
- Source: `include/chimera/transaction.hpp`:186
- Brief: n/a
- Parameters: none

#### `TransactionContext * operator->() noexcept`
- Source: `include/chimera/transaction.hpp`:185
- Brief: n/a
- Parameters: none

### test_chimera_highcardinality_stress.cpp

#### `TEST(ChimeraHighCardinalityStress, ConcurrentRouterStress)`
- Source: `tests/chimera/test_chimera_highcardinality_stress.cpp`:105
- Brief: ConcurrentRouterStress.
- Parameters:
  - `<unnamed>` (ChimeraHighCardinalityStress): n/a
  - `<unnamed>` (ConcurrentRouterStress): n/a
- Details: Exercises concurrent router dispatch from 8 threads each issuing 100 000 queries. Asserts consistent routing counts with no faults.

#### `TEST(ChimeraHighCardinalityStress, FallbackPathStress)`
- Source: `tests/chimera/test_chimera_highcardinality_stress.cpp`:135
- Brief: FallbackPathStress.
- Parameters:
  - `<unnamed>` (ChimeraHighCardinalityStress): n/a
  - `<unnamed>` (FallbackPathStress): n/a
- Details: Directly invokes the fallback path 500 000 times from 4 threads. Asserts that all invocations complete without errors.

#### `TEST(ChimeraHighCardinalityStress, HighCardinalityHybridQuery)`
- Source: `tests/chimera/test_chimera_highcardinality_stress.cpp`:72
- Brief: HighCardinalityHybridQuery.
- Parameters:
  - `<unnamed>` (ChimeraHighCardinalityStress): n/a
  - `<unnamed>` (HighCardinalityHybridQuery): n/a
- Details: Issues 1 000 000 hybrid queries from 8 concurrent threads. Asserts that all queries are routed with no faults.

### test_chimera_prepared_statements.cpp

#### `TEST_F(ChimeraPreparedStatementTest, BindAllEmptyNameReturnsInvalidArgument)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:240
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (BindAllEmptyNameReturnsInvalidArgument): n/a

#### `TEST_F(ChimeraPreparedStatementTest, BindAllSucceeds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:228
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (BindAllSucceeds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, BindEmptyNameReturnsInvalidArgument)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (BindEmptyNameReturnsInvalidArgument): n/a

#### `TEST_F(ChimeraPreparedStatementTest, BindNamedBoolSucceeds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:197
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (BindNamedBoolSucceeds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, BindNamedIntegerSucceeds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:183
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (BindNamedIntegerSucceeds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, BindNamedStringSucceeds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:190
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (BindNamedStringSucceeds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, BindPositionalSucceeds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:217
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (BindPositionalSucceeds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, DynamicCastToIPreparedStatementAdapterSucceeds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:88
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (DynamicCastToIPreparedStatementAdapterSucceeds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, ExecuteAsyncResolvesSuccessfully)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:261
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (ExecuteAsyncResolvesSuccessfully): n/a

#### `TEST_F(ChimeraPreparedStatementTest, ExecuteReturnsResult)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:254
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (ExecuteReturnsResult): n/a

#### `TEST_F(ChimeraPreparedStatementTest, GetCapabilitiesIncludesPreparedStatements)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:81
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (GetCapabilitiesIncludesPreparedStatements): n/a

#### `TEST_F(ChimeraPreparedStatementTest, GetIdReturnsNonEmptyId)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (GetIdReturnsNonEmptyId): n/a

#### `TEST_F(ChimeraPreparedStatementTest, GetQueryReturnsOriginalQuery)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:115
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (GetQueryReturnsOriginalQuery): n/a

#### `TEST_F(ChimeraPreparedStatementTest, HasPreparedStatementsCapability)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:77
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (HasPreparedStatementsCapability): n/a

#### `TEST_F(ChimeraPreparedStatementTest, ListPreparedContainsAllRegisteredIds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:145
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (ListPreparedContainsAllRegisteredIds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, ListPreparedContainsRegisteredId)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:134
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (ListPreparedContainsRegisteredId): n/a

#### `TEST_F(ChimeraPreparedStatementTest, PrepareEmptyQueryReturnsInvalidArgument)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:103
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (PrepareEmptyQueryReturnsInvalidArgument): n/a

#### `TEST_F(ChimeraPreparedStatementTest, PrepareReturnsValidStatement)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:97
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (PrepareReturnsValidStatement): n/a

#### `TEST_F(ChimeraPreparedStatementTest, PreparedStatementViaBasePointer)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:347
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (PreparedStatementViaBasePointer): n/a

#### `TEST_F(ChimeraPreparedStatementTest, ResetClearsBindingsThenExecuteSucceeds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:274
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (ResetClearsBindingsThenExecuteSucceeds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, StatisticsAfterTwoExecutesHasNonNegativeTime)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:299
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (StatisticsAfterTwoExecutesHasNonNegativeTime): n/a

#### `TEST_F(ChimeraPreparedStatementTest, StatisticsBeforeExecuteHasZeroTime)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:290
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (StatisticsBeforeExecuteHasZeroTime): n/a

#### `TEST_F(ChimeraPreparedStatementTest, StringBindingIsEscapedNotConcatenated)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:315
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (StringBindingIsEscapedNotConcatenated): n/a

#### `TEST_F(ChimeraPreparedStatementTest, TwoStatementsHaveDistinctIds)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:122
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (TwoStatementsHaveDistinctIds): n/a

#### `TEST_F(ChimeraPreparedStatementTest, UnprepareRemovesStatementFromList)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (UnprepareRemovesStatementFromList): n/a

#### `TEST_F(ChimeraPreparedStatementTest, UnprepareUnknownIdReturnsNotFound)`
- Source: `tests/chimera/test_chimera_prepared_statements.cpp`:173
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraPreparedStatementTest): n/a
  - `<unnamed>` (UnprepareUnknownIdReturnsNotFound): n/a

### test_chimera_streaming.cpp

#### `TEST_F(ChimeraStreamingTest, CloseIsIdempotent)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (CloseIsIdempotent): n/a

#### `TEST_F(ChimeraStreamingTest, DynamicCastToIStreamingAdapterSucceeds)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:89
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (DynamicCastToIStreamingAdapterSucceeds): n/a

#### `TEST_F(ChimeraStreamingTest, ExplicitBatchSizeOverridesDefault)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:259
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (ExplicitBatchSizeOverridesDefault): n/a

#### `TEST_F(ChimeraStreamingTest, GetCapabilitiesIncludesStreamingResults)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:82
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (GetCapabilitiesIncludesStreamingResults): n/a

#### `TEST_F(ChimeraStreamingTest, HasMoreReturnsFalseAfterClose)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:223
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (HasMoreReturnsFalseAfterClose): n/a

#### `TEST_F(ChimeraStreamingTest, HasStreamingResultsCapability)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:78
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (HasStreamingResultsCapability): n/a

#### `TEST_F(ChimeraStreamingTest, NextBatchAfterCloseReturnsError)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:212
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (NextBatchAfterCloseReturnsError): n/a

#### `TEST_F(ChimeraStreamingTest, NextBatchAfterExhaustionReturnsEmpty)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:181
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (NextBatchAfterExhaustionReturnsEmpty): n/a

#### `TEST_F(ChimeraStreamingTest, PositionAdvancesWithNextBatch)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (PositionAdvancesWithNextBatch): n/a

#### `TEST_F(ChimeraStreamingTest, PositionStartsAtZero)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:152
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (PositionStartsAtZero): n/a

#### `TEST_F(ChimeraStreamingTest, SetStreamConfigChangesDefaultBatchSize)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:239
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (SetStreamConfigChangesDefaultBatchSize): n/a

#### `TEST_F(ChimeraStreamingTest, StreamEmptyTable)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:98
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (StreamEmptyTable): n/a

#### `TEST_F(ChimeraStreamingTest, StreamMultipleBatches)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:129
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (StreamMultipleBatches): n/a

#### `TEST_F(ChimeraStreamingTest, StreamResultMatchesSyncResult)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:283
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (StreamResultMatchesSyncResult): n/a

#### `TEST_F(ChimeraStreamingTest, StreamSingleBatch)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:109
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (StreamSingleBatch): n/a

#### `TEST_F(ChimeraStreamingTest, StreamingViaBasePointer)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:312
- Brief: n/a
- Parameters:
  - `<unnamed>` (ChimeraStreamingTest): n/a
  - `<unnamed>` (StreamingViaBasePointer): n/a

#### `RelationalRow make_row(const std::string &name, int64_t value)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:39
- Brief: n/a
- Parameters:
  - `name` (const std::string &): n/a
  - `value` (int64_t): n/a

#### `void populate_table(IDatabaseAdapter &adapter, const std::string &table, size_t n)`
- Source: `tests/chimera/test_chimera_streaming.cpp`:47
- Brief: Insert n rows into table through the adapter and assert success.
- Parameters:
  - `adapter` (IDatabaseAdapter &): n/a
  - `table` (const std::string &): n/a
  - `n` (size_t): n/a

### test_themisdb_adapter.cpp

#### `TEST_F(ThemisDBCapabilityTest, ConnectionPoolingAvailableAfterInjection)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:734
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (ConnectionPoolingAvailableAfterInjection): n/a

#### `TEST_F(ThemisDBCapabilityTest, ConnectionPoolingDisabledAfterNullInjection)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:748
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (ConnectionPoolingDisabledAfterNullInjection): n/a

#### `TEST_F(ThemisDBCapabilityTest, ConnectionPoolingNotAvailableWithoutInjection)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:721
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (ConnectionPoolingNotAvailableWithoutInjection): n/a

#### `TEST_F(ThemisDBCapabilityTest, GetCapabilitiesExcludesConnectionPoolingWithoutInjection)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:726
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (GetCapabilitiesExcludesConnectionPoolingWithoutInjection): n/a

#### `TEST_F(ThemisDBCapabilityTest, GetCapabilitiesIncludesConnectionPoolingAfterInjection)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:740
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (GetCapabilitiesIncludesConnectionPoolingAfterInjection): n/a

#### `TEST_F(ThemisDBCapabilityTest, GetMetricsReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:763
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (GetMetricsReturnsOk): n/a

#### `TEST_F(ThemisDBCapabilityTest, GetSystemInfoReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:756
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (GetSystemInfoReturnsOk): n/a

#### `TEST_F(ThemisDBCapabilityTest, HasDocumentStoreCapability)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:713
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (HasDocumentStoreCapability): n/a

#### `TEST_F(ThemisDBCapabilityTest, HasGraphTraversalCapability)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:709
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (HasGraphTraversalCapability): n/a

#### `TEST_F(ThemisDBCapabilityTest, HasRelationalCapability)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:701
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (HasRelationalCapability): n/a

#### `TEST_F(ThemisDBCapabilityTest, HasTransactionCapability)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:717
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (HasTransactionCapability): n/a

#### `TEST_F(ThemisDBCapabilityTest, HasVectorSearchCapability)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:705
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBCapabilityTest): n/a
  - `<unnamed>` (HasVectorSearchCapability): n/a

#### `TEST_F(ThemisDBConnectionTest, ConnectWithOptionsSucceeds)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:86
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBConnectionTest): n/a
  - `<unnamed>` (ConnectWithOptionsSucceeds): n/a

#### `TEST_F(ThemisDBConnectionTest, ConnectWithValidUri)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:80
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBConnectionTest): n/a
  - `<unnamed>` (ConnectWithValidUri): n/a

#### `TEST_F(ThemisDBConnectionTest, DisconnectAfterConnect)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:93
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBConnectionTest): n/a
  - `<unnamed>` (DisconnectAfterConnect): n/a

#### `TEST_F(ThemisDBConnectionTest, DisconnectWhenAlreadyDisconnected)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:100
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBConnectionTest): n/a
  - `<unnamed>` (DisconnectWhenAlreadyDisconnected): n/a

#### `TEST_F(ThemisDBConnectionTest, InitiallyDisconnected)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:76
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBConnectionTest): n/a
  - `<unnamed>` (InitiallyDisconnected): n/a

#### `TEST_F(ThemisDBDocumentTest, BatchInsertDocumentsReturnsCorrectCount)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:310
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBDocumentTest): n/a
  - `<unnamed>` (BatchInsertDocumentsReturnsCorrectCount): n/a

#### `TEST_F(ThemisDBDocumentTest, BatchInsertWhenDisconnectedReturnsError)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:336
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBDocumentTest): n/a
  - `<unnamed>` (BatchInsertWhenDisconnectedReturnsError): n/a

#### `TEST_F(ThemisDBDocumentTest, FindDocumentsReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:322
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBDocumentTest): n/a
  - `<unnamed>` (FindDocumentsReturnsOk): n/a

#### `TEST_F(ThemisDBDocumentTest, InsertDocumentReturnsId)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:294
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBDocumentTest): n/a
  - `<unnamed>` (InsertDocumentReturnsId): n/a

#### `TEST_F(ThemisDBDocumentTest, InsertDocumentWithEmptyIdGeneratesId)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:301
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBDocumentTest): n/a
  - `<unnamed>` (InsertDocumentWithEmptyIdGeneratesId): n/a

#### `TEST_F(ThemisDBDocumentTest, UpdateDocumentsReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:327
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBDocumentTest): n/a
  - `<unnamed>` (UpdateDocumentsReturnsOk): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, AllEnginesInjectedEachMethodReturnsNotImplemented)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1521
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (AllEnginesInjectedEachMethodReturnsNotImplemented): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, ExecuteQueryEmptyQueryNotImplemented)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1432
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (ExecuteQueryEmptyQueryNotImplemented): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, ExecuteQueryNotImplementedWithoutEngineHeaders)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1422
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (ExecuteQueryNotImplementedWithoutEngineHeaders): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, ExecuteQueryWithParamsNotImplemented)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1442
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (ExecuteQueryWithParamsNotImplemented): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, SearchVectorsKZeroNotImplemented)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1465
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (SearchVectorsKZeroNotImplemented): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, SearchVectorsNotImplementedWithoutEngineHeaders)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1453
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (SearchVectorsNotImplementedWithoutEngineHeaders): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, ShortestPathCustomDepthNotImplemented)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1488
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (ShortestPathCustomDepthNotImplemented): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, ShortestPathNotImplementedWithoutEngineHeaders)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1478
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (ShortestPathNotImplementedWithoutEngineHeaders): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, TraverseNotImplementedWithoutEngineHeaders)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1499
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (TraverseNotImplementedWithoutEngineHeaders): n/a

#### `TEST_F(ThemisDBEngineInjectionTest, TraverseWithLabelsNotImplemented)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1510
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBEngineInjectionTest): n/a
  - `<unnamed>` (TraverseWithLabelsNotImplemented): n/a

#### `TEST_F(ThemisDBGraphTest, ExecuteGraphQueryReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:273
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBGraphTest): n/a
  - `<unnamed>` (ExecuteGraphQueryReturnsOk): n/a

#### `TEST_F(ThemisDBGraphTest, InsertEdgeReturnsId)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:251
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBGraphTest): n/a
  - `<unnamed>` (InsertEdgeReturnsId): n/a

#### `TEST_F(ThemisDBGraphTest, InsertNodeReturnsId)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:231
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBGraphTest): n/a
  - `<unnamed>` (InsertNodeReturnsId): n/a

#### `TEST_F(ThemisDBGraphTest, InsertNodeWithEmptyIdGeneratesId)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:242
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBGraphTest): n/a
  - `<unnamed>` (InsertNodeWithEmptyIdGeneratesId): n/a

#### `TEST_F(ThemisDBGraphTest, ShortestPathReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:263
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBGraphTest): n/a
  - `<unnamed>` (ShortestPathReturnsOk): n/a

#### `TEST_F(ThemisDBGraphTest, TraverseReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:268
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBGraphTest): n/a
  - `<unnamed>` (TraverseReturnsOk): n/a

#### `TEST_F(ThemisDBIntegrationTest, BatchInsertRowsPersistedCorrectly)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:987
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (BatchInsertRowsPersistedCorrectly): n/a

#### `TEST_F(ThemisDBIntegrationTest, FindDocumentsWithNoMatchReturnsEmpty)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1163
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (FindDocumentsWithNoMatchReturnsEmpty): n/a

#### `TEST_F(ThemisDBIntegrationTest, InsertAndFindDocumentRoundTrip)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1150
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (InsertAndFindDocumentRoundTrip): n/a

#### `TEST_F(ThemisDBIntegrationTest, InsertDocumentWithEmptyIdGeneratesUuid)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1192
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (InsertDocumentWithEmptyIdGeneratesUuid): n/a

#### `TEST_F(ThemisDBIntegrationTest, InsertRowAndRetrieveViaExecuteQuery)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:971
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (InsertRowAndRetrieveViaExecuteQuery): n/a

#### `TEST_F(ThemisDBIntegrationTest, InsertVectorGeneratesUniqueIds)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:933
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (InsertVectorGeneratesUniqueIds): n/a

#### `TEST_F(ThemisDBIntegrationTest, ShortestPathFindsDirectEdge)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1054
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (ShortestPathFindsDirectEdge): n/a

#### `TEST_F(ThemisDBIntegrationTest, ShortestPathMaxDepthOne)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1255
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (ShortestPathMaxDepthOne): n/a

#### `TEST_F(ThemisDBIntegrationTest, ShortestPathNoRouteReturnsEmptyPath)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1089
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (ShortestPathNoRouteReturnsEmptyPath): n/a

#### `TEST_F(ThemisDBIntegrationTest, ShortestPathRespectMaxDepth)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1218
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (ShortestPathRespectMaxDepth): n/a

#### `TEST_F(ThemisDBIntegrationTest, ShortestPathSameSourceAndTarget)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1077
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (ShortestPathSameSourceAndTarget): n/a

#### `TEST_F(ThemisDBIntegrationTest, TransactionIdsAreUniqueUuids)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:951
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (TransactionIdsAreUniqueUuids): n/a

#### `TEST_F(ThemisDBIntegrationTest, TraverseMultiLabelDeduplicatesSharedNodes)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1317
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (TraverseMultiLabelDeduplicatesSharedNodes): n/a

#### `TEST_F(ThemisDBIntegrationTest, TraverseMultiLabelReachesNodesOfEitherType)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1281
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (TraverseMultiLabelReachesNodesOfEitherType): n/a

#### `TEST_F(ThemisDBIntegrationTest, TraverseReturnsReachableNodes)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1101
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (TraverseReturnsReachableNodes): n/a

#### `TEST_F(ThemisDBIntegrationTest, TraverseThreeLabelsAllReachable)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1342
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (TraverseThreeLabelsAllReachable): n/a

#### `TEST_F(ThemisDBIntegrationTest, TraverseWithEdgeLabelFilterApplied)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1119
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (TraverseWithEdgeLabelFilterApplied): n/a

#### `TEST_F(ThemisDBIntegrationTest, UpdateDocumentsPersistsChanges)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1173
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (UpdateDocumentsPersistsChanges): n/a

#### `TEST_F(ThemisDBIntegrationTest, VectorSearchOnEmptyCollectionReturnsEmpty)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1026
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (VectorSearchOnEmptyCollectionReturnsEmpty): n/a

#### `TEST_F(ThemisDBIntegrationTest, VectorSearchResultsAreSortedByDistance)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1033
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (VectorSearchResultsAreSortedByDistance): n/a

#### `TEST_F(ThemisDBIntegrationTest, VectorSearchReturnsInsertedVectors)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:1008
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBIntegrationTest): n/a
  - `<unnamed>` (VectorSearchReturnsInsertedVectors): n/a

#### `TEST_F(ThemisDBPerformanceTest, BulkDocumentInsertOverhead)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:804
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBPerformanceTest): n/a
  - `<unnamed>` (BulkDocumentInsertOverhead): n/a

#### `TEST_F(ThemisDBPerformanceTest, BulkRelationalInsertOverhead)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:872
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBPerformanceTest): n/a
  - `<unnamed>` (BulkRelationalInsertOverhead): n/a

#### `TEST_F(ThemisDBPerformanceTest, BulkVectorInsertOverhead)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:829
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBPerformanceTest): n/a
  - `<unnamed>` (BulkVectorInsertOverhead): n/a

#### `TEST_F(ThemisDBPerformanceTest, TransactionBeginCommitOverhead)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:896
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBPerformanceTest): n/a
  - `<unnamed>` (TransactionBeginCommitOverhead): n/a

#### `TEST_F(ThemisDBPerformanceTest, VectorSearchOverhead)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:850
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBPerformanceTest): n/a
  - `<unnamed>` (VectorSearchOverhead): n/a

#### `TEST_F(ThemisDBRelationalTest, BatchInsertReturnsCorrectCount)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:149
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBRelationalTest): n/a
  - `<unnamed>` (BatchInsertReturnsCorrectCount): n/a

#### `TEST_F(ThemisDBRelationalTest, ExecuteQueryReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:119
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBRelationalTest): n/a
  - `<unnamed>` (ExecuteQueryReturnsOk): n/a

#### `TEST_F(ThemisDBRelationalTest, ExecuteQueryWhenDisconnectedReturnsError)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:132
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBRelationalTest): n/a
  - `<unnamed>` (ExecuteQueryWhenDisconnectedReturnsError): n/a

#### `TEST_F(ThemisDBRelationalTest, ExecuteQueryWithParams)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:124
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBRelationalTest): n/a
  - `<unnamed>` (ExecuteQueryWithParams): n/a

#### `TEST_F(ThemisDBRelationalTest, GetQueryStatisticsReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:160
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBRelationalTest): n/a
  - `<unnamed>` (GetQueryStatisticsReturnsOk): n/a

#### `TEST_F(ThemisDBRelationalTest, InsertRowReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:139
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBRelationalTest): n/a
  - `<unnamed>` (InsertRowReturnsOk): n/a

#### `TEST_F(ThemisDBTransactionTest, BeginTransactionReturnsTxnId)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:357
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (BeginTransactionReturnsTxnId): n/a

#### `TEST_F(ThemisDBTransactionTest, BeginTransactionWhenDisconnectedReturnsError)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:379
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (BeginTransactionWhenDisconnectedReturnsError): n/a

#### `TEST_F(ThemisDBTransactionTest, BeginTransactionWithAllIsolationLevels)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:660
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (BeginTransactionWithAllIsolationLevels): n/a

#### `TEST_F(ThemisDBTransactionTest, BeginTransactionWithAllowNestedOption)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:679
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (BeginTransactionWithAllowNestedOption): n/a

#### `TEST_F(ThemisDBTransactionTest, CommitEmptyIdReturnsInvalidArgument)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:408
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (CommitEmptyIdReturnsInvalidArgument): n/a

#### `TEST_F(ThemisDBTransactionTest, CommitTransactionReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:363
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (CommitTransactionReturnsOk): n/a

#### `TEST_F(ThemisDBTransactionTest, CommitUnknownTransactionReturnsNotFound)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:396
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (CommitUnknownTransactionReturnsNotFound): n/a

#### `TEST_F(ThemisDBTransactionTest, CreateDuplicateSavepointReturnsAlreadyExists)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:430
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (CreateDuplicateSavepointReturnsAlreadyExists): n/a

#### `TEST_F(ThemisDBTransactionTest, CreateSavepointSucceeds)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:418
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (CreateSavepointSucceeds): n/a

#### `TEST_F(ThemisDBTransactionTest, ExecuteWithRetryDoesNotRetryNonDeadlockError)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:641
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (ExecuteWithRetryDoesNotRetryNonDeadlockError): n/a

#### `TEST_F(ThemisDBTransactionTest, ExecuteWithRetryExhaustsMaxRetries)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:625
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (ExecuteWithRetryExhaustsMaxRetries): n/a

#### `TEST_F(ThemisDBTransactionTest, ExecuteWithRetryRetriesOnDeadlock)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:607
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (ExecuteWithRetryRetriesOnDeadlock): n/a

#### `TEST_F(ThemisDBTransactionTest, ExecuteWithRetrySucceedsOnFirstAttempt)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:594
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (ExecuteWithRetrySucceedsOnFirstAttempt): n/a

#### `TEST_F(ThemisDBTransactionTest, GetTransactionStateForUnknownIdReturnsNotFound)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:567
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (GetTransactionStateForUnknownIdReturnsNotFound): n/a

#### `TEST_F(ThemisDBTransactionTest, GetTransactionStateListsSavepoints)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:573
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (GetTransactionStateListsSavepoints): n/a

#### `TEST_F(ThemisDBTransactionTest, GetTransactionStateReturnsCorrectIsolationLevel)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:549
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (GetTransactionStateReturnsCorrectIsolationLevel): n/a

#### `TEST_F(ThemisDBTransactionTest, GetTransactionStatsForUnknownIdReturnsNotFound)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:543
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (GetTransactionStatsForUnknownIdReturnsNotFound): n/a

#### `TEST_F(ThemisDBTransactionTest, GetTransactionStatsReturnsValidData)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:519
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (GetTransactionStatsReturnsValidData): n/a

#### `TEST_F(ThemisDBTransactionTest, MultipleTransactionsGetUniqueIds)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:386
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (MultipleTransactionsGetUniqueIds): n/a

#### `TEST_F(ThemisDBTransactionTest, ReleaseNonExistentSavepointReturnsNotFound)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:492
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (ReleaseNonExistentSavepointReturnsNotFound): n/a

#### `TEST_F(ThemisDBTransactionTest, ReleaseSavepointSucceeds)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:475
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (ReleaseSavepointSucceeds): n/a

#### `TEST_F(ThemisDBTransactionTest, RollbackToNonExistentSavepointReturnsNotFound)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:463
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (RollbackToNonExistentSavepointReturnsNotFound): n/a

#### `TEST_F(ThemisDBTransactionTest, RollbackToSavepointSucceeds)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:443
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (RollbackToSavepointSucceeds): n/a

#### `TEST_F(ThemisDBTransactionTest, RollbackTransactionReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:371
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (RollbackTransactionReturnsOk): n/a

#### `TEST_F(ThemisDBTransactionTest, RollbackUnknownTransactionReturnsNotFound)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:402
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (RollbackUnknownTransactionReturnsNotFound): n/a

#### `TEST_F(ThemisDBTransactionTest, SavepointOperationsOnClosedTransactionReturnNotFound)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:504
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBTransactionTest): n/a
  - `<unnamed>` (SavepointOperationsOnClosedTransactionReturnNotFound): n/a

#### `TEST_F(ThemisDBVectorTest, BatchInsertVectorsReturnsCorrectCount)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:192
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBVectorTest): n/a
  - `<unnamed>` (BatchInsertVectorsReturnsCorrectCount): n/a

#### `TEST_F(ThemisDBVectorTest, CreateIndexReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:180
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBVectorTest): n/a
  - `<unnamed>` (CreateIndexReturnsOk): n/a

#### `TEST_F(ThemisDBVectorTest, InsertVectorReturnsId)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:185
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBVectorTest): n/a
  - `<unnamed>` (InsertVectorReturnsId): n/a

#### `TEST_F(ThemisDBVectorTest, SearchVectorsReturnsOk)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:204
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBVectorTest): n/a
  - `<unnamed>` (SearchVectorsReturnsOk): n/a

#### `TEST_F(ThemisDBVectorTest, SearchVectorsWhenDisconnectedReturnsError)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:210
- Brief: n/a
- Parameters:
  - `<unnamed>` (ThemisDBVectorTest): n/a
  - `<unnamed>` (SearchVectorsWhenDisconnectedReturnsError): n/a

#### `Document make_doc(const std::string &id, const std::string &name, int64_t value)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:51
- Brief: n/a
- Parameters:
  - `id` (const std::string &): n/a
  - `name` (const std::string &): n/a
  - `value` (int64_t): n/a

#### `Vector make_vector(size_t dim, float fill)`
- Source: `tests/chimera/test_themisdb_adapter.cpp`:61
- Brief: n/a
- Parameters:
  - `dim` (size_t): n/a
  - `fill` (float): n/a

