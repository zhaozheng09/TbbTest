#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <memory>

using namespace std;

class OpNameNodeList;

struct OpNameNode {
    OpNameNode(std::string strIn, OpNameNodeList* listIn):pref(nullptr),next(nullptr),str(strIn), list(listIn), using_(false) {}
    OpNameNode *pref;
    OpNameNode *next;
    string str;
    OpNameNodeList* list;
    mutex m;
    atomic_bool using_;
    atomic_bool deleted_;
    void Wait() {
      while(using_.load()) {};
      Use();
    }
    void Free() {
      using_.store(false);
    }
    void Use() {using_.store(true);}
    bool IsUsed()const { return using_.load();}
};

class OpNameNodeList {
public:
    void AddNode(OpNameNode* node) {
      m_.lock();
      if (head_ == nullptr) {
        head_ = node;
        head_->pref = nullptr;
        head_->next = nullptr;
        m_.unlock();
        return ;
      }
      node->next = head_;
      node->pref = nullptr;
      head_->pref = node;
      head_ = node;
      m_.unlock();
    }
    void DelNode(OpNameNode* node) {
      while((node->next && node->next->IsUsed())) {}
      if (node->next) {
        node->next->Wait();
      }
      if (node->pref) {
        node->pref->Wait();
      }
      node->Wait();
      if (node->next) {
        node->next->pref = node->pref;
        node->next->Free();
      }
      if (node->pref) {
        node->pref->next = node->next;
        node->pref->Free();
      }
      if(node == head_) {
        head_ = node->next;
      }
      node->Free();
    }
    void Print() {
      OpNameNode *p = head_;
      string str = "";
      while(p) {
        str += (p->str+","); 
        p = p->next;
      }
      std::cout << "hang node:" << str << std::endl;
    }
    OpNameNode* head_ = nullptr;
    std::mutex m_;
};

OpNameNodeList op_list;
//unordered_map<uint32_t, OpNameNode*> ops;
vector<OpNameNode*> ops;
std::mutex m;
atomic_int num(0);
void Push() {
   OpNameNode* pNode = new OpNameNode("aaa", &op_list);
   op_list.AddNode(pNode); 
   m.lock();
   ops.push_back(pNode);
   m.unlock();
}

void Pop2() {
   for (const auto& iter : ops) {
     op_list.DelNode(iter);
     //delete iter;
   }
}

void Pop(const uint32_t s, const uint32_t e) {
   for (uint32_t i = s; i < e; ++i) {
     op_list.DelNode(ops[i]);
     delete ops[i];
   }
}
constexpr uint32_t len = 10000000;
void while_push() {
    for (uint32_t i = 0; i < len; ++i) {
      Push();
    }
}

void while_print() {
    for (uint32_t i = 0; i < len; ++i) {
      op_list.Print();
      sleep(10);
    }
}

constexpr uint32_t thread_num = 10;

void SingleThread() {
    auto start = chrono::steady_clock::now();    
    while_push(); 
    std::cout << "end insert and start delete:" << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() << std::endl;
    start = chrono::steady_clock::now();    
    Pop(0, ops.size()); 
    std::cout << "end insert and start delete:" << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() << std::endl;
}

void MultiThread() {
     auto start = chrono::steady_clock::now();    
while_push(); 
    std::cout << "end insert and start delete:" << chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() << std::endl;
    start = chrono::steady_clock::now();    
    shared_ptr<thread> ts[thread_num];
    for (uint32_t i = 0; i < thread_num; ++i) {
      ts[i].reset(new thread(Pop, i * ops.size()/thread_num, (i + 1) * ops.size()/thread_num));
    }
    for (uint32_t i = 0; i < thread_num; ++i) {
      ts[i]->join();
    }
}

int main() {
    //SingleThread();
    MultiThread();
    return 0;
}


