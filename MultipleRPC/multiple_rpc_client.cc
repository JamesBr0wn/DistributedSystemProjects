#include <iostream>
#include <memory>
#include <string>

#include <grpc++/grpc++.h>
#include <multiple_rpc.grpc.pb.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2333
#define CLIENT_IP "127.0.0.1"
#define CLIENT_PORT 10086

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using multiple_rpc::InfoService;
using multiple_rpc::InfoService;
using multiple_rpc::Address;

class MultipleRPCClient {
public:
    MultipleRPCClient(std::shared_ptr<Channel> channel)
            : stub_(InfoService::NewStub(channel)) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    void GetServerInfo() {
        // Data we are sending to the server.
        Address request;
        request.set_ip(CLIENT_IP);
        request.set_port(CLIENT_PORT);

        // Container for the data we expect from the server.
        Address reply;

        // Context for the client. It could be used to convey extra information to
        // the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        std::unique_ptr<ClientReader<Address> > reader(
                stub_->GetServerInfo(&context, request));

        while (reader->Read(&reply)) {
            std::cout << "Found server info called "
                      << reply.ip() << ":"
                      << reply.port() << std::endl;
        }
        Status status = reader->Finish();

        // Act upon its status.
        if (status.ok()) {
            std::cout << "GetServerInfo rpc succeeded." << std::endl;
        } else {
            std::cout << status.error_code() << ": " << status.error_message()
                      << std::endl;
        }
    }

private:
    std::unique_ptr<InfoService::Stub> stub_;
};

int main(int argc, char** argv) {
    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    MultipleRPCClient client(grpc::CreateChannel(
            std::string(SERVER_IP) + std::string(":") + std::to_string(SERVER_PORT), grpc::InsecureChannelCredentials()));
    client.GetServerInfo();
    std::cout << "RPC finished." << std::endl;

    return 0;
}