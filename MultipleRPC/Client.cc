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
using multiple_rpc::ProxyService;
using multiple_rpc::GreetingService;
using multiple_rpc::Address;
using multiple_rpc::User;
using multiple_rpc::Message;

class MultipleRPCClient {
public:
    MultipleRPCClient(std::shared_ptr<Channel> channel)
            : proxy_stub(ProxyService::NewStub(channel)) {}
            
    std::vector<std::string> GetServerInfo() {
        User request;
        request.set_name("James");

        Address reply;

        ClientContext context;
        
        std::unique_ptr<ClientReader<Address> > reader(
                proxy_stub->GetServerInfo(&context, request));

        std::vector<std::string> greeting_server_list;
        while (reader->Read(&reply)) {
            greeting_server_list.push_back(reply.address());
            std::cout << "Get server info: " << reply.address() << std::endl;
        }
        Status status = reader->Finish();

        if (status.ok()) {
            std::cout << "GetServerInfo rpc succeeded." << std::endl;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
        return greeting_server_list;
    }

    void SayHello(){
        User request;
        request.set_name("James");

        Message reply;
        
        ClientContext context;

        std::unique_ptr<ClientReader<Message> > reader(
                proxy_stub->SayHello(&context, request));

        std::vector<std::string> greeting_server_list;
        while (reader->Read(&reply)) {
            std::cout << reply.message() << std::endl;
        }
        Status status = reader->Finish();

        if (status.ok()) {
            std::cout << "SayHello rpc succeeded." << std::endl;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
    }

private:
    std::unique_ptr<ProxyService::Stub> proxy_stub;
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