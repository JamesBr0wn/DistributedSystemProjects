#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <cstring>

#include <grpc++/grpc++.h>
#include <multiple_rpc.grpc.pb.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using multiple_rpc::InfoService;
using multiple_rpc::User;
using multiple_rpc::Address;

char SERVER_ADDRESS[32];

// Logic and data behind the server's behavior.
class InfoServiceImp final : public InfoService::Service {
public:
    Status GetServerInfo(ServerContext* context, const User* request,
                         ServerWriter<Address>* reply_writer) override {
        Address in_addr, out_addr;
        in_addr.set_address(context->peer());
        std::cout << "Request from " << in_addr.address()
            << " start!" << std::endl;
        for(int i = 0; i < 3; i++){
            out_addr.set_address("127.0.0.1:8888");

            reply_writer->Write(out_addr);
        }
        std::cout << "Request from " << in_addr.address()
            << " finish!" << std::endl;
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address(SERVER_ADDRESS);
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
    strcpy(SERVER_ADDRESS, argv[1]);
    RunServer();

    return 0;
}