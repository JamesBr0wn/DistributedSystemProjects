#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <multiple_rpc.grpc.pb.h>

using grpc::Channel;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ClientContext;
using grpc::Status;
using multiple_rpc::GreetingService;
using multiple_rpc::InfoService;
using multiple_rpc::User;
using multiple_rpc::Address;
using multiple_rpc::Message;

char SERVER_ADDRESS[32];
char INFO_ADDRESS[32];

// Logic and data behind the server's behavior.
class GreetingServiceImp final : public GreetingService::Service {
public:
    GreetingServiceImp(std::shared_ptr<Channel> channel)
            : info_stub(InfoService::NewStub(channel)) {
        Address request;
        Message reply;
        ClientContext context;
        Status status;
        request.set_address(SERVER_ADDRESS);
        status = info_stub->SetServerInfo(&context, request, &reply);
        if (status.ok()) {
            std::cout << reply.message() << std::endl;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            std::cout << "Server set failed" << std::endl;
            exit(-1);
        }
    }

    ~GreetingServiceImp(){
        Address request;
        Message reply;
        ClientContext context;
        Status status;
        request.set_address(SERVER_ADDRESS);
        status = info_stub->UnsetServerInfo(&context, request, &reply);
        if (status.ok()) {
            std::cout << reply.message() << std::endl;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
            std::cout << "Server unset failed" << std::endl;
            exit(-1);
        }
    }

    Status SayHello(ServerContext* context, const User* request,
                         Message* reply) override {
        Address client;
        client.set_address(context->peer());
        std::string prefix("Hello, ");
        std::string postfix(std::string(" from ") + SERVER_ADDRESS + ".");
        reply->set_message(prefix + request->name() + postfix);
        std::cout << std::endl << "Request from user " << request->name() << ":" << client.address() << std::endl;
        std::cout << "Server> " << std::flush;
        return Status::OK;
    }
private:
    std::unique_ptr<InfoService::Stub> info_stub;
};

void RunServer() {
    std::string server_address(SERVER_ADDRESS);
    GreetingServiceImp service(grpc::CreateChannel(
            std::string(INFO_ADDRESS), grpc::InsecureChannelCredentials()));

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Greeting server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    std::string command;
    do{
        std::cout << "Server> ";
        std::cin >> command;

    }while(command != "shutdown");
    server->Shutdown();
}

int main(int argc, char** argv) {
    strcpy(SERVER_ADDRESS, argv[1]);
    strcpy(INFO_ADDRESS, argv[2]);
    RunServer();

    return 0;
}