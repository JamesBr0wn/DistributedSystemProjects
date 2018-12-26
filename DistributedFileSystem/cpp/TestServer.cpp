//
// Created by JamesBrown on 2018/12/26.
//
#include <iostream>
#include <string>
#include <fstream>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include "DataServer.grpc.pb.h"

using std::string;
using std::to_string;
using std::ifstream;
using std::ios;
using std::cout;
using std::endl;
using std::hex;
using std::dec;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using DataServer::DataService;
using DataServer::Block;

char SERVER_ADDRESS[32];

class DataServerImp final: public DataService::Service{
public:
    DataServerImp(string dir):storeDirectory(dir) {}

    Status ReadBlock(ServerContext* context, const Block* request,
                     Block* reply) override {
        string blockName = request->filename() + '_' +
                to_string(request->blocknum());
        ifstream fs(storeDirectory + blockName, ios::in | ios::binary | ios::ate);
        if(fs.is_open()){
            // 读取文件块
            long blockSize = fs.tellg();
            cout << "$ Block " << blockName << " size: " << blockSize << endl;
            if(blockSize != request->blocksize()){
                cout << "# Block " << blockName << " size mismatch!" << endl;
                return Status::CANCELLED;
            }
            fs.seekg(0, ios::beg);
            char* blockData = new char[blockSize];
            fs.read(blockData, blockSize);

            // 验证Hash值
            SHA256_CTX stx;
            unsigned char hash[SHA256_DIGEST_LENGTH+1];
            hash[SHA256_DIGEST_LENGTH] = '\0';
            SHA256_Init(&stx);
            SHA256_Update(&stx, blockData, (size_t)blockSize);
            SHA256_Final(hash, &stx);
            cout << "$ Block " << blockName << " hash: ";
            cout << hex;
            for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
                cout << (unsigned int)hash[i];
            }
            cout << dec << endl;
            if(memcmp(hash, request->blockhash().data(), SHA256_DIGEST_LENGTH) != 0){
                cout << "# Block " << blockName << " hash mismatch!" << endl;
                return Status::CANCELLED;
            }

            // 返回文件块
            reply->set_filename(request->filename());
            reply->set_blocknum(request->blocknum());
            reply->set_blocksize(request->blocksize());
            reply->set_blockdata(blockData, (size_t)blockSize);
            reply->set_blockhash(request->blockhash());

            delete[] blockData;
            fs.close();

            return Status::OK;

        }else{
            cout << "# Block " << blockName << " open failed!" << endl;
            return Status::CANCELLED;
        }

    }
private:
    string storeDirectory;
};

void RunServer() {
    std::string server_address(SERVER_ADDRESS);
    DataServerImp service("BlockStore/");

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
