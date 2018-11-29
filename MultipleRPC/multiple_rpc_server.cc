#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <multiple_rpc.grpc.pb.h>

#define SERVER_IP "0.0.0.0"
#define SERVER_PORT 2333

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using multiple_rpc::InfoService;
using multiple_rpc::Address;

// Logic and data behind the server's behavior.
class InfoServiceImp final : public InfoService::Service {
    Status GetServerInfo(ServerContext* context, const Address* request,
                         ServerWriter<Address>* reply_writer) override {
        Address in_addr, out_addr;
        in_addr.set_ip(request->ip());
        in_addr.set_port(request->port());
        std::cout << "Request from " << in_addr.ip() << " " << in_addr.port()
            << " start!" << std::endl;
        for(int i = 0; i < 3; i++){
            out_addr.set_ip("192.168.137." + std::to_string(i));
            out_addr.set_port(10086);
            reply_writer->Write(out_addr);
        }
        std::cout << "Request from " << in_addr.ip() << " " << in_addr.port()
            << " finish!" << std::endl;
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address(std::string(SERVER_IP) + std::string(":") + std::to_string(SERVER_PORT));
    InfoServiceImp service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();

    return 0;
}