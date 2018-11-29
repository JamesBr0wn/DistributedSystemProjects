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
using multiple_rpc::Address;
using multiple_rpc::GreetingService;
using multiple_rpc::User;
using multiple_rpc::Address;

class MultipleRPCClient {
public:
    MultipleRPCClient(std::shared_ptr<Channel> channel)
            : info_stub(InfoService::NewStub(channel)) {}

    // Assembles the client's payload, sends it and presents the response back
    // from the server.
    void GetServerInfo() {
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

        while (reader->Read(&reply)) {
            std::cout << "Get server info: "
                      << reply.address() << std::endl;
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
    std::unique_ptr<InfoService::Stub> info_stub;
    std::unique_ptr<GreetingService::Stub> greeting_stub;
};

int main(int argc, char** argv) {
    // Instantiate the client. It requires a channel, out of which the actual RPCs
    // are created. This channel models a connection to an endpoint (in this case,
    // localhost at port 50051). We indicate that the channel isn't authenticated
    // (use of InsecureChannelCredentials()).
    strcpy(SERVER_ADDRESS, argv[1]);
    MultipleRPCClient client(grpc::CreateChannel(
            std::string(SERVER_ADDRESS), grpc::InsecureChannelCredentials()));
    client.GetServerInfo();
    std::cout << "RPC finished." << std::endl;

    return 0;
}