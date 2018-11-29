#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <multiple_rpc.grpc.pb.h>

char SERVER_ADDRESS[16];

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using multiple_rpc::InfoService;
using multiple_rpc::GreetingService;
using multiple_rpc::Address;
using multiple_rpc::User;
using multiple_rpc::Message;

class MultipleRPCClient {
public:
    MultipleRPCClient(std::shared_ptr<Channel> channel)
            : info_stub(InfoService::NewStub(channel)) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    std::vector<std::string> GetServerInfo() {
        // Data we are sending to the server.
        User request;
        request.set_name("James");

        // Container for the data we expect from the server.
        Address reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        std::unique_ptr<ClientReader<Address> > reader(
                info_stub->GetServerInfo(&context, request));

        std::vector<std::string> greeting_server_list;
        while (reader->Read(&reply)) {
            greeting_server_list.push_back(reply.address());
            std::cout << "Get server info: " << reply.address() << std::endl;
        }
        Status status = reader->Finish();

        // Act upon its status.
        if (status.ok()) {
            std::cout << "GetServerInfo rpc succeeded." << std::endl;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
        return greeting_server_list;
    }

    void SayHello(){
        std::unique_ptr<GreetingService::Stub> greeting_sub;
        std::vector<std::string> greeting_server_list = GetServerInfo();
        for (auto &greeting_server : greeting_server_list) {
            std::shared_ptr<Channel> channel = grpc::CreateChannel(greeting_server, grpc::InsecureChannelCredentials());
            greeting_sub  = GreetingService::NewStub(channel);
            User request;
            request.set_name("James");
            Message reply;
            ClientContext context;
            Status status;
            status = greeting_sub->SayHello(&context, request, &reply);
            if (status.ok()) {
                std::cout << reply.message() << std::endl;
            } else {
                std::cout << status.error_code() << ": " << status.error_message() << std::endl;
                std::cout << "RPC failed" << std::endl;
                exit(-1);
            }
        }
        std::cout << "SayHello rpc succeeded." << std::endl;
    }

private:
    std::unique_ptr<InfoService::Stub> info_stub;

};

int main(int argc, char** argv) {
    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    strcpy(SERVER_ADDRESS, argv[1]);
    MultipleRPCClient client(grpc::CreateChannel(
            std::string(SERVER_ADDRESS), grpc::InsecureChannelCredentials()));
    client.SayHello();
    std::cout << "RPC finished." << std::endl;

    return 0;
}