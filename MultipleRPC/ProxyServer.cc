#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstring>

#include <grpc++/grpc++.h>
#include <multiple_rpc.grpc.pb.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using multiple_rpc::ProxyService;
using multiple_rpc::GreetingService;
using multiple_rpc::User;
using multiple_rpc::Address;
using multiple_rpc::Message;

char SERVER_ADDRESS[32];

// Logic and data behind the server's behavior.
class ProxyServiceImp final : public ProxyService::Service {
public:
    Status GetServerInfo(ServerContext* context, const User* request,
                         ServerWriter<Address>* reply_writer) override {
        Address client, server;
        client.set_address(context->peer());
        std::cout << "Request from user " << request->name() << ":" << client.address() << std::endl;
        for(int i = 0; i < server_list.size(); i++){
            reply_writer->Write(server_list[i]);
        }
        return Status::OK;
    }

    Status SetServerInfo(ServerContext* context, const Address* request,
                        Message* message) override{
        server_list.push_back(*request);
        message->set_message("Server " + request->address() + " set successful");
        std::cout <<  message->message() << std::endl;
        return Status::OK;
    }

    Status UnsetServerInfo(ServerContext* context, const Address* request,
                         Message* message) override{
        for(auto iter = server_list.begin(); iter != server_list.end(); iter++){
            if(iter->address() == request->address()){
                server_list.erase(iter);
                message->set_message("Server " + request->address() + " unset successful");
                std::cout <<  message->message() << std::endl;
                return Status::OK;
            }
        }
        return Status::CANCELLED;
    }

    Status SayHello(ServerContext* context, const User* request,
                    ServerWriter<Message>* reply_writer) override{
        Address client;
        client.set_address(context->peer());
        std::cout << "Request from user " << request->name() << ":" << client.address() << std::endl;
        std::unique_ptr<GreetingService::Stub> greeting_sub;
        for (auto &greeting_server : server_list) {
            std::shared_ptr<Channel> channel = grpc::CreateChannel(greeting_server.address(), grpc::InsecureChannelCredentials());
            std::cout << greeting_server.address() << std::endl;
            greeting_sub  = GreetingService::NewStub(channel);
            User user;
            user.set_name(request->name());
            Message message;
            ClientContext context;
            Status status;
            status = greeting_sub->SayHello(&context, *request, &message);
            if (status.ok()) {
                std::cout << message.message() << std::endl;
                reply_writer->Write(message);
            } else {
                return Status::CANCELLED;
            }
        }
        return Status::OK;
    }

private:
    std::vector<Address> server_list;
};

void RunServer() {
    std::string server_address(SERVER_ADDRESS);
    ProxyServiceImp service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Info server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

int main(int argc, char** argv) {
    strcpy(SERVER_ADDRESS, argv[1]);
    RunServer();

    return 0;
}