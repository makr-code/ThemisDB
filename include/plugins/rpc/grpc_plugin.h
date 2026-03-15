#pragma once

#include "plugins/rpc_plugin_interface.h"

#include <atomic>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace rpc {
namespace grpc_plugin {

class GRPCServer : public IRPCServer {
public:
	GRPCServer();
	~GRPCServer() override;

	RPCProtocol getProtocol() const override;
	bool initialize(const RPCServerConfig& config) override;
	bool start() override;
	void stop() override;
	bool isRunning() const override;
	RPCServerStats getStats() const override;
	void registerService(void* service_impl) override;
	std::string getAddress() const override;
	void resetStats() override;

private:
	RPCServerConfig config_;
	std::unique_ptr<grpc::Server> server_;
	std::atomic<bool> running_{false};
	mutable std::mutex stats_mutex_;
	RPCServerStats stats_;
	std::string server_address_;
	std::vector<grpc::Service*> services_;

	std::string loadFile(const std::string& path);
	std::shared_ptr<grpc::ServerCredentials> configureCredentials();
};

class GRPCPlugin : public IRPCPlugin {
public:
	GRPCPlugin() = default;
	~GRPCPlugin() override = default;

	const char* getName() const override;
	const char* getVersion() const override;
	PluginType getType() const override;
	PluginCapabilities getCapabilities() const override;
	bool initialize(const char* config_json) override;
	void shutdown() override;
	void* getInstance() override;

	std::unique_ptr<IRPCServer> createServer() override;
	RPCProtocol getProtocol() const override;
	uint16_t getDefaultPort() const override;
	const char* getProtocolDescription() const override;

private:
	bool initialized_{false};
};

} // namespace grpc_plugin
} // namespace rpc
} // namespace plugins
} // namespace themis
