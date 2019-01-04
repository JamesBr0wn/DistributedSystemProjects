//
// Created by james on 18-12-28.
//

#include "Client.h"

char SERVER_ADDRESS[32];
char CACHE_DIR[32];

class TestClient {
public:
    TestClient(string dir, size_t sz) : clientImp(grpc::CreateChannel(std::string(SERVER_ADDRESS), grpc::InsecureChannelCredentials()), dir, sz){}

//    bool getBlockTest(){
//        BlockInfo request;
//        request.set_filename("block");
//        request.set_blockidx(0);
//        string blockName = request.filename() + '_' + to_string(request.blockidx());
//        ifstream ifs("Temp/" + blockName, ios::in | ios::binary | ios::ate);
//        long blockSize = ifs.tellg();
//        cout << "Block " << blockName << " size: " << blockSize << endl;
//        request.set_blocksize(blockSize);
//        ifs.seekg(0, ios::beg);
//        char * blockData = new char[blockSize];
//        ifs.read(blockData, blockSize);
//
//        SHA256_CTX stx;
//        unsigned char hash[SHA256_DIGEST_LENGTH+1];
//        hash[SHA256_DIGEST_LENGTH] = '\0';
//        SHA256_Init(&stx);
//        SHA256_Update(&stx, blockData, (size_t)blockSize);
//        SHA256_Final(hash, &stx);
//        cout << "Block " << blockName << " hash: ";
//        cout << hex;
//        for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
//            cout << (unsigned int)hash[i];
//        }
//        cout << dec << endl;
//        request.set_blockhash((char*)hash, SHA256_DIGEST_LENGTH);
//        delete[] blockData;
//        ifs.close();
//
//        clientImp.getBlock(request);
//        return true;
//    }
//
//    bool putBlockTest(){
//        BlockInfo request;
//        request.set_filename("block");
//        request.set_blockidx(0);
//        clientImp.putBlock(request);
//        return true;
//    }
//
//    bool rmBlockTest(){
//        BlockInfo request;
//        request.set_filename("block");
//        request.set_blockidx(0);
//        string blockName = request.filename() + '_' + to_string(request.blockidx());
//        ifstream ifs("BlockStore/" + blockName, ios::in | ios::binary | ios::ate);
//        long blockSize = ifs.tellg();
//        cout << "Block " << blockName << " size: " << blockSize << endl;
//        request.set_blocksize(blockSize);
//        ifs.seekg(0, ios::beg);
//        char * blockData = new char[blockSize];
//        ifs.read(blockData, blockSize);
//
//        SHA256_CTX stx;
//        unsigned char hash[SHA256_DIGEST_LENGTH+1];
//        hash[SHA256_DIGEST_LENGTH] = '\0';
//        SHA256_Init(&stx);
//        SHA256_Update(&stx, blockData, (size_t)blockSize);
//        SHA256_Final(hash, &stx);
//        cout << "Block " << blockName << " hash: ";
//        cout << hex;
//        for(long i = 0; i < SHA256_DIGEST_LENGTH; i++){
//            cout << (unsigned int)hash[i];
//        }
//        cout << dec << endl;
//        request.set_blockhash((char*)hash, SHA256_DIGEST_LENGTH);
//        delete[] blockData;
//        ifs.close();
//
//        clientImp.rmBlock(request);
//        return true;
//    }

    bool getTest(string fileName){
        if(clientImp.get(fileName).ok()){
            return true;
        }else{
            return false;
        }
    }

    bool putTest(string fileName){
        if(clientImp.put(fileName).ok()){
            return true;
        }else{
            return false;
        }
    }

    bool rmTest(string fileName){
        if(clientImp.rm(fileName).ok()){
            return true;
        }else{
            return false;
        }
    }

    bool touchTest(string fileName){
        if(clientImp.touch(fileName).ok()){
            return true;
        }else{
            return false;
        }
    }

    bool testOption(int option, string fileName){
        switch(option){
            case 0:
                return getTest(fileName);
            case 1:
                return putTest(fileName);
            case 2:
                return rmTest(fileName);
            case 3:
                return touchTest(fileName);
            default:
                cout << "# Option not valid!" << endl;
                return false;
        }
    }
private:
    ClientImp clientImp;
};

int main(int argc, char** argv){
    strcpy(SERVER_ADDRESS, argv[1]);
    strcpy(CACHE_DIR, argv[2]);
    TestClient client(CACHE_DIR, 1024 * 1024);

//    if(client.putBlockTest()){
//        std::cout << "$ Put succeed!" << std::endl;
//    }else{
//        std::cout << "# Put failed!" << std::endl;
//    }
//    cin >> temp;
//    if(client.getBlockTest()){
//        std::cout << "$ Get succeed!" << std::endl;
//    }else{
//        std::cout << "# Get failed!" << std::endl;
//    }
//    cin >> temp;
//
//    if(client.rmBlockTest()){
//        std::cout << "$ Rm succeed!" << std::endl;
//    }else{
//        std::cout << "# Rm failed!" << std::endl;
//    }
//
    int option;
    string fileName;
    cout << "Please enter your option:" << endl;
    cout << "0. get;\n1. put;\n2. rm;\n3. touch;\n9. exit." << endl;
    cin >> option;
    while(option != 9){
        while(option < 0 || (option > 3 && option != 9)){
            cout << "Option not valid, please try again!" << endl;
            cout << "0. get;\n1. put;\n2. rm;\n3. touch;\n9. exit." << endl;
            cin >> option;
        }
        cout << "Please enter the file name:" << endl;
        cin >> fileName;
        client.testOption(option, fileName);
        cout << "Please enter your option:" << endl;
        cout << "0. get;\n1. put;\n2.rm\n3.touch;\n9. exit." << endl;
        cin >> option;
    };
    return 0;
}
