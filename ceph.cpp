#include<iostream>
#include<fstream>
#include<cstring>
#include<string>
#include<iomanip>
#include<thread>
#include<sstream>
#include<openssl/sha.h>
#include<stdio.h>
//#include<pthread.h>

#define BUFFER_SIZE 128*1024
#define HASH33_SIZE 7
#define HASH33_START 57
#define BLOCK_SIZE 67108864 //2147483648
#define OSD_1_2 "192.168.6.14"
#define OSD_3_4 "192.168.6.13"
#define OSD_5_6 "192.168.6.12"
#define OSD_7_8 "192.168.6.10"
#define GB 1073741824

using namespace std;

class FileHashTable;
class Node
{
    string hash256;
    Node *nextBlock;

public:
    Node();
    Node(string);

    friend class FileHashTable;
};//Node

Node :: Node()
{
    this->hash256 = "";
    this->nextBlock = NULL;
}//def cons

Node :: Node(string hash256)
{
    this->hash256 = hash256;
    this->nextBlock = NULL;
}//par cons

class FileHashTable
{
    Node *head;
    string fileName;

public:
    FileHashTable();
    FileHashTable(string);
    void addBlock(FileHashTable **, long int hash, string);
    void addFile(FileHashTable **, string);
    long int getHash(string);
    void store(FileHashTable **);
    void retrieve(FileHashTable **);
    void printFileTable(FileHashTable **);
};

FileHashTable :: FileHashTable()
{
    this->head = NULL;
    this->fileName = "";
}//def cons

FileHashTable :: FileHashTable(string fileName)
{
    this->head = NULL;
    this->fileName = fileName;
}//def cons

long int FileHashTable :: getHash(string fileName)
{
    long int sum =0;
    for(char c: fileName)
        sum += (int)c;
    return sum % GB;
}//getHash

void FileHashTable :: addBlock(FileHashTable **ft, long int hash, string block_Name)
{
    if(ft[hash]->head == NULL)
        ft[hash]->head = new Node(block_Name);
    else
    {
        Node *block;
        for(block = ft[hash]->head; block->nextBlock != NULL; block = block->nextBlock);
        block->nextBlock = new Node(block_Name);
    }//else
}//addBlock

void FileHashTable :: addFile(FileHashTable **ft, string fileName)
{
    long int hash = getHash(fileName); 
    ft[hash] = new FileHashTable(fileName);
}//addFile

string sha256(const string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    return ss.str();
}//sha256

int getHexValue(char a)
{//converting hash hex-char form to actual hex form.
    switch(a)
    {
        case '0': return 0x0;
        case '1': return 0x1;
        case '2': return 0x2;
        case '3': return 0x3;
        case '4': return 0x4;
        case '5': return 0x5;
        case '6': return 0x6;
        case '7': return 0x7;
        case '8': return 0x8;
        case '9': return 0x9;
        case 'a': return 0xA;
        case 'b': return 0xB;
        case 'c': return 0xC;
        case 'd': return 0xD;
        case 'e': return 0xE;
        case 'f': return 0xF;
    }//switch
}//getHexValue

long long int convertHash(string hashValue)
{
    string hash33 = hashValue.substr(HASH33_START, HASH33_SIZE);
    int x;
    long long int final_binary = 0;

    for(int i =0; i < HASH33_SIZE; i++)
    {
        x = getHexValue(hash33[i]);
        final_binary = final_binary << 4;
        final_binary = final_binary | x;        
    }//for
    return final_binary;
}//convertHash

string findDestOSD(long long hashValue)
{//finding OSD no., by 33hash.
    if(hashValue >= 0 and hashValue <(BLOCK_SIZE))
        return OSD_1_2;
    if(hashValue >= (BLOCK_SIZE) and hashValue <(2*BLOCK_SIZE))
        return OSD_3_4;
    if(hashValue >= (2*BLOCK_SIZE) and hashValue <(3*BLOCK_SIZE))
        return OSD_5_6;
    if(hashValue >= (3*BLOCK_SIZE) and hashValue <=(4*BLOCK_SIZE))
        return OSD_7_8;

    return "";
}//findDestOSD

bool sendToOSD(string hashValue, long long newHashValue)
{//identifying receiving OSD's IP and sending the 
 //256hash named file to that OSD.
    string send_scp = "scp /home/cephuser/user/"+ hashValue + " ";

    if(findDestOSD(newHashValue) == OSD_1_2)//sendToOSD1_2
        send_scp = send_scp + OSD_1_2 + ":/home/cephuser/user/";
    else if(findDestOSD(newHashValue) == OSD_3_4)//sendToOSD3_4
        send_scp = send_scp + OSD_3_4 + ":/home/cephuser/user/";
    else if(findDestOSD(newHashValue) == OSD_5_6)//sendToOSD5_6
        send_scp = send_scp + OSD_5_6 + ":/home/cephuser/user/";
    else if(findDestOSD(newHashValue) == OSD_7_8)//sendToOSD7_8
        send_scp = send_scp + OSD_7_8 + ":/home/cephuser/user/";
    else//out of range -prec
        return false;
    //sent successfully
    system(send_scp.c_str());
    return true;
}//sendToOSD

void FileHashTable :: store(FileHashTable **ft)
{
    string fileName;
    fstream fin, fout, fout1, fout2, fout3, fout4;
    char buffer[BUFFER_SIZE];
    
    //block variables
    string hashValue;
    long long newHashValue;

    //sending variables
    //int retry_attempt;
    bool sent;
    bool f1, f2, f3, f4;

    //FileHashTable f;

    f1 = f2 = f3 = f4 = false;
    fout4.open(OSD_7_8, ios::out);
    fout3.open(OSD_5_6, ios::out);
    fout2.open(OSD_3_4, ios::out);
    fout1.open(OSD_1_2, ios::out);

    cout<<"\nEnter File Name : ";
    cin>>fileName;

    addFile(ft, fileName);
    long int hash = getHash(fileName);

    fin.open(fileName, ios::in | ios::binary);
    while(fin)
    {
        //reading 128K from file in a buffer and sending it to hash
        fin.read(buffer, BUFFER_SIZE);
        string *data = new string(buffer);
            
        //sha256 algorithm
        hashValue = sha256(*data);
        newHashValue = convertHash(hashValue);
        cout<<newHashValue<<"\n";
        
        //creating blocks with 256bit hash values as name
        fout.open(hashValue, ios::out | ios::binary);
        fout.write(buffer, fin.gcount());
        fout.close();

        addBlock(ft, hash, hashValue);
      //  printFileTable(ft);

        string s = to_string(newHashValue) + ":" + hashValue;

        if(findDestOSD(newHashValue) == OSD_1_2)//sendToOSD1_2
        {
            fout1<<s<<endl;
            f1 = true;
        }
        else if(findDestOSD(newHashValue) == OSD_3_4)//sendToOSD3_4
        {
            fout2<<s<<endl;
            f2 = true;
        }//else if
        else if(findDestOSD(newHashValue) == OSD_5_6)//sendToOSD5_6
        {
            fout3<<s<<endl;
            f3 = true;
        }//else if
        else if(findDestOSD(newHashValue) == OSD_7_8)//sendToOSD7_8
        {
            fout4<<s<<endl;
            f4 = true;
        }//else if

    }//while
    fin.close();
    fout1.close();
    fout2.close();
    fout3.close();
    fout4.close();

    string send_scp;
    send_scp = "scp /home/cephuser/cephStorage/192.168.6.14 osd1:/home/cephuser/user/";
    if(f1)
        system(send_scp.c_str());
    send_scp = "scp /home/cephuser/cephStorage/192.168.6.13 osd2:/home/cephuser/user/";
    if(f2)
        system(send_scp.c_str());
    send_scp = "scp /home/cephuser/cephStorage/192.168.6.12 osd3:/home/cephuser/user/";
    if(f3)
        system(send_scp.c_str());
    send_scp = "scp /home/cephuser/cephStorage/192.168.6.10 osd4:/home/cephuser/user/";
    if(f4)
        system(send_scp.c_str());
    
    this_thread::sleep_for(2s);
    fin.open("/home/cephuser/user/osd1");
    while(getline(fin, hashValue))
    {
        //ceph put command using hashValue file block name
        string s = "rados -p rbdpool put " + hashValue + " " + hashValue + " --striper";
        cout<<"hello"<<endl;
        system(s.c_str());
    }//while osd1
    fin.close();

    this_thread::sleep_for(2s);
    fin.open("/home/cephuser/user/osd2");
    while(getline(fin, hashValue))
    {
        //ceph put command using hashValue file block name
        string s = "rados -p rbdpool put " + hashValue + " " + hashValue + " --striper";cout<<"hello2"<<endl;
        system(s.c_str());
    }//while osd2
    fin.close();

    this_thread::sleep_for(2s);
    fin.open("/home/cephuser/user/osd3");
    while(getline(fin, hashValue))
    {
        //ceph put command using hashValue file block name
        string s = "rados -p rbdpool put " + hashValue + " " + hashValue + " --striper";cout<<"hello3"<<endl;
        system(s.c_str());
    }//while osd3
    fin.close();
    
    this_thread::sleep_for(2s);
    fin.open("/home/cephuser/user/osd4");
    while(getline(fin, hashValue))
    {
        //ceph put command using hashValue file block name
        string s = "rados -p rbdpool put " + hashValue + " " + hashValue + " --striper";cout<<"hello4"<<endl;
        system(s.c_str());
    }//while osd4

    /*s = "rm /home/cephuser/cephStorage/" + fileName;
    
    for(Node *block = ft[hash]->head; block != NULL; block = block->nextBlock)
    {
        s = "rm /home/cephuser/cephStorage/" + block->hash256;
        system(s.c_str());     
    }//for 
    */
    fin.close();
}//stores the files 

void FileHashTable :: retrieve(FileHashTable **ft)
{
    string fileName;
    long int hash;
    fstream fout, fin;

    cout<<"\nEnter File Name : ";
    cin>>fileName;

    hash = getHash(fileName);
    fout.open(fileName, ios::out | ios::binary);
    string s;
    
    for(Node *block = ft[hash]->head; block != NULL; block = block->nextBlock)
    {
        s = "rados -p rbdpool get " + block->hash256 + " " + block->hash256 + " --striper";
        //ceph ki command from block->hash256
        system(s.c_str());
    }//for

    char buffer[BUFFER_SIZE];
    for(Node *block = ft[hash]->head; block != NULL; block = block->nextBlock)
    {
        fin.open(block->hash256, ios::in | ios::binary);
        while(fin)
        {
            fin.read(buffer, BUFFER_SIZE);
            fout.write(buffer, fin.gcount());
        }//while
        fin.close();
    }//for
    fout.close();

    for(Node *block = ft[hash]->head; block != NULL; block = block->nextBlock)
    {
        s = "rm /home/cephuser/cephStorage/" + block->hash256;
        system(s.c_str());     
    }//for    

}//retrieve

void FileHashTable :: printFileTable(FileHashTable **ft)
{
    Node * block;
    bool flag = false;

    for(long int i =0 ; i<GB ; i++ )
    {
        if(ft[i] != NULL)
        {
            flag = true;
            cout<<"\n"<<ft[i]->fileName;
            for(block = ft[i]->head ; block->nextBlock != NULL ; block = block->nextBlock)
                cout<<block->hash256<<" -> ";
            cout<<block->hash256<<";;;\n";    
        }//if
    }//for

    if(not flag)
        cout<<"\nPlease add files to the table first :)";
}//printFileTable

int main()
{
    int ch;
    FileHashTable **ft = new FileHashTable*[GB];
    FileHashTable f;
    do
    {
        cout<<"\n------------------------------------"
            <<"\n1. Store a file."
            <<"\n2. Retrieve a file."
            <<"\n3. Print File Table."
            <<"\nEnter Choice : ";
        cin>>ch;
    
        switch(ch)
        {
            case 1: f.store(ft);
                break;
            case 2: f.retrieve(ft);
                break;
            case 3: f.printFileTable(ft);
                break;
            default:
                cout<<"\nInvalid Option :(";
                break;
        }//switch
    }while(true);

    return 0;
}//main