//
// Created by JamesBrown on 2018/12/26.
//
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include "DataServer.grpc.pb.h"

using std::string;
using std::to_string;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::cout;
using std::endl;
using std::hex;
using std::dec;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using DataServer::DataService;
using DataServer::Block;

char SERVER_ADDRESS[16];

class TestClient{
public:
    TestClient(std::shared_ptr<Channel> channel)
        : _stub(DataService::NewStub(channel)) {}

    bool ReadBlockTest(){
        Block request;
        request.set_filename("block");
        request.set_blocknum(0);
        string blockName = request.filename() + '_' + to_string(request.blocknum());
        ifstream ifs(blockName, ios::in | ios::binary | ios::ate);
        if(ifs.is_open()){
            long blockSize = ifs.tellg();
            cout << "$ Block " << blockName << " size: " << blockSize << endl;
            request.set_blocksize(blockSize);
            ifs.seekg(0, ios::beg);
            char * blockData = new char[blockSize];
            ifs.read(blockData, blockSize);

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
            request.set_blockhash((char*)hash);

            Block reply;
            ClientContext context;
            Status status;
            status = _stub->ReadBlock(&context, request, &reply);

            if (status.ok()) {
                cout << "$ Block " << blockName << " received!" << endl;
                string fileName = "receieved.zip";
                ofstream ofs;
                ofs.open(fileName);
                ofs.write(reply.blockdata().data(), reply.blocksize());
                ofs.close();
                cout << "$ Block " << blockName << " written!" << endl;
            } else {
                cout << "# Block " << blockName << " read failed!" << endl;
                return false;
            }

            delete[] blockData;
            ifs.close();
        }else{
            cout << "# Block " << blockName << " open failed!" << endl;
            return false;
        }

    }
private:
    std::unique_ptr<DataService::Stub> _stub;
};

int main(int argc, char** argv){
    strcpy(SERVER_ADDRESS, argv[1]);
    TestClient client(grpc::CreateChannel(
            std::string(SERVER_ADDRESS), grpc::InsecureChannelCredentials()));
    if(client.ReadBlockTest()){
        std::cout << "$ RPC succeed!" << std::endl;
    }else{
        std::cout << "# RPC failed!" << std::endl;
    }

    return 0;
}