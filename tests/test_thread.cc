#include "sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count = 0;
//sylar::RWMutex s_mutex;
sylar::Mutex s_mutex;

void fun1(){
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << " this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id: " << sylar::Thread::GetThis()->getId();
    for(int i = 0; i < 100000; ++i){
//        sylar::RWMutex::WriteLock lock(s_mutex);
        sylar::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2(){
    while(true){
        SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3(){
    while(true){
        SYLAR_LOG_INFO(g_logger) << "===========================================================================";
    }
}

int main(int argc, char** argv){
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/chihuicong/server/conf/log2.yml");
    sylar::Config::LoadFromYaml(root);
    std::vector<sylar::Thread::ptr> thrs;
    for(int i = 0; i < 2; ++i){
        sylar::Thread::ptr thr(new sylar::Thread(&fun2, "name_" + std::to_string(i * 2)));
        sylar::Thread::ptr thr2(new sylar::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for(size_t i = 0; i < thrs.size(); ++i){
        thrs[i]->join();
    }

    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count = " << count;

    return 0;
}

//#include <iostream>
//#include <pthread.h>

//static int count = 0;

//void* fun1(void *){
//    for(int i = 0; i < 500000; i++){
//        --count;
//        std::cout << "线程1: " << count << std::endl;
//    }
//    return 0;
//}

//void* fun2(void *){
//    for(int i = 0; i < 500000; i++){
//        ++count;
//        std::cout << "线程2: " << count << std::endl;
//    }
//    return 0;
//}

//int main(){
//    pthread_t pid1, pid2;
//    pthread_create(&pid1, nullptr, &fun1, nullptr);
//    pthread_create(&pid2, nullptr, &fun2, nullptr);
//    pthread_join(pid1, nullptr);
//    pthread_join(pid2, nullptr);
//    std::cout << "主线程" << std::endl;
//    std::cout << "count: " << count << std::endl;
//    return 0;
//}
