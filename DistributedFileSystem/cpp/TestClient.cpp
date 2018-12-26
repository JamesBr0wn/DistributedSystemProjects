//
// Created by JamesBrown on 2018/12/26.
//
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <grpc++/grpc++.h>
#include <openssl/sha.h>
#include <DataServer.pb.h>
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
using DataServer::BlockInfo;
using DataServer::BlockUnit;

char SERVER_ADDRESS[16];

class TestClient{
public:
    TestClient(std::shared_ptr<Channel> channel)
        : _stub(DataService::NewStub(channel)) {}

    bool ReadBlockTest(){
        BlockInfo request;
        request.set_filename("block");
        request.set_blockidx(0);
        string blockName = request.filename() + '_' + to_string(request.blockidx());
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
            delete[] blockData;
            ifs.close();

            BlockUnit unit;
            ClientContext context;
            std::unique_ptr<ClientReader<BlockUnit>> reader(
                    _stub->ReadBlock(&context, request));
            Status status;

            string fileName = "receieved";
            ofstream ofs;
            unsigned long i = 0;
            ofs.open(fileName);
            while (reader->Read(&unit)) {
                ofs.write(unit.unitdata().data(), unit.unitdata().length());
                cout << "$ Block " << blockName << " read unit " << i << endl;
                i++;
            }
            ofs.close();
            status = reader->Finish();

            if (status.ok()) {
                cout << "$ Block " << blockName << " read finished!" << endl;
                return true;
            } else {
                cout << "# Block " << blockName << " read failed!" << endl;
                return false;
            }
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