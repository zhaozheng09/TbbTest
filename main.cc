//g++ -O2 -std=c++11 main.cc -o main -L./build/linux_intel64_gcc_cc5.3.0_libc2.17_kernel3.10.0_release/ -ltbb -lpthread

#include "include/tbb/concurrent_hash_map.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
using namespace std;
using namespace tbb;

typedef typename interface5::concurrent_hash_map<string, int64_t>::const_accessor table_reader_t;
typedef typename interface5::concurrent_hash_map<string, int64_t>::accessor table_writer_t;

interface5::concurrent_hash_map<string, int64_t> test_map; 
table_reader_t reader;
table_writer_t writer;
constexpr uint32_t key_size = 10000000;
constexpr uint32_t thread_num = 100;
string keys[key_size];
atomic_int num;
using namespace chrono;
void Insert(const size_t i) {
    test_map.insert(reader, std::make_pair(keys[i], 5));
}

void Delete(const string& str) {
    test_map.erase(str);
}

void Delete(table_reader_t reader) {
    test_map.erase(reader);
}

void FakeKey(uint32_t s, uint32_t e, vector<string>& strs) {
  for (uint32_t i = s; i < e; ++i) {
    strs.push_back(std::to_string(i));
  }
}

void LoopInsert() {
    for (uint32_t i = 0; i < key_size; ++i) {
      Insert(i);
    }   
}

void LoopFind(vector<string>& strs) {
  for (const auto &str : strs) {
    auto ret = test_map.find(reader, str);
    if (!ret) {
      std::cout << "find err ." << std::endl;
    }
  }
}

void LoopDel(const vector<string>& strs) {
    for (const auto& str : strs) {
        if(test_map.find(reader, str)) {
          test_map.erase(reader);
        }
    }
}

void SingleThread() {
    std::cout << "start fake ." << std::endl;
    vector<string> key;
    FakeKey(0, key_size, key);
    std::cout << "end fake and start insert." << std::endl;
    auto start = chrono::steady_clock::now();    
    LoopInsert();
    std::cout << "end insert and start delete:" << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() << std::endl;
    vector<string> strs;
    for (uint32_t i = 0; i < key_size; ++i) {
        strs.push_back(keys[i]);
    }
    start = chrono::steady_clock::now();    
    LoopFind(strs);
    std::cout << "find over:" << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() << std::endl;

    start = chrono::steady_clock::now();    
    LoopDel(strs);
    std::cout << "end del:" << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() << std::endl;
}

void MultiThread() {
    shared_ptr<thread> ts[thread_num];
    std::cout << "start fake ." << std::endl;
    vector<vector<string> > strs;
    for (uint32_t i = 0; i < thread_num; ++i) {
        strs.push_back(vector<string>());
        FakeKey(i * key_size/thread_num, (i + 1) * key_size/thread_num, strs[i]);
    }
    thread t1(LoopInsert);
    t1.join();
    std::cout << "end fake and start insert." << std::endl;
    auto start = chrono::steady_clock::now();    
    for (uint32_t i = 0; i < thread_num; ++i) {
        ts[i].reset(new thread(LoopDel, strs[i]));
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
        ts[i]->join();
    }
    std::cout << "end insert and start delete:" << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() << std::endl;
    
}

void SimpleTest() {
    test_map.insert(reader, std::pair<string, int64_t>("0", 1));
    if (test_map.find(reader, "0")) {
      std::cout << reader->second << std::endl;
    }

    // release函数会释放tbb表里的bucket上的锁，可以让其他线程操作，否则其他线程会卡死
    reader.release();
    test_map.insert(writer, "0");
    writer->second = 2;
    writer.release();

    if (test_map.find(reader, "0")) {
      std::cout << reader->second << std::endl;
    }
}

int main() {
    //g++ -O2 -std=c++11 main.cc -o main -L./build/linux_intel64_gcc_cc5.3.0_libc2.17_kernel3.10.0_release/ -ltbb -lpthread
    
    //SingleThread();
    
    //MultiThread();
    
    SimpleTest();
    return 0;
}
