#include <iostream>
#include "future.h"
#include "promise.h"
#include <thread>
#include <vector>

int foo(bool b, int nIterations ){
    if( !b ){
        return 0;
    }
    int sum = 0;
    for( int i = 0; i < nIterations; ++i ){
        sum++;
    }
    return sum;
}

int fooN(){
    int sum = 0;
    for( int i = 0; i < 1000000000; ++i ){
        sum++;
    }
    return sum;
}

template <typename> class my_task;

template <typename R, typename ...Args>
class my_task<R(Args...)>
{
    std::function<R(Args...)> fn;
    CPromise<R> pr;             // the promise of the result
public:
    template <typename ...Ts>
    explicit my_task(Ts &&... ts) : fn(std::forward<Ts>(ts)...) { }

    template <typename ...Ts>
    void operator()(Ts &&... ts) {
        try {
            pr.SetValue(fn(std::forward<Ts>(ts)...));  // fulfill the promise
        } catch ( std::exception* exception){
            std::cout << exception->what() << std::endl ;
        }
    }

    std::shared_ptr<CFuture<R> > get_future() { return std::make_shared<CFuture<R> >(pr.GetFuture()); }

    // disable copy, default move
};

class CAsyncRunner {
public:
    CAsyncRunner( int _poolSize) {
        poolSize = _poolSize;
    }

    ~CAsyncRunner(){
        for(int i = 0; i < threadPool.size(); ++i){
            threadPool[i].join();
        }
    }

    template<typename F, typename ...Args>
    auto asyncRun(F &&func, bool isSync, Args &&... args) -> CFuture<decltype(func(args...))> {
        auto task = my_task<decltype(func(args...))()>(std::bind(&func, args...));
        auto future = task.get_future();
        if( !isSync ) {
            if(threadPool.size() < poolSize){
                threadPool.emplace_back(std::move(task));
                futures.push_back(future);
                std::cout << "async" << std::endl;
                return *future;
            } else {
                for (int i = 0; i < threadPool.size(); ++i) {
                    if (threadPool[i].joinable() && futures[i]->IsReady()) {
                        threadPool[i].join();
                        threadPool.emplace(threadPool.begin() + i, std::move(task));
                        futures[i] = future;
                        std::cout << "async" << std::endl;
                        return *future;
                    }
                }
            }
        }
        std::cout << "sync" << std::endl;
        std::thread sync(std::move(task));
        sync.join();
        return *future;
    }

private:
    std::vector<std::thread> threadPool;
    std::vector<std::shared_ptr<MyFutureBase> > futures;
    int poolSize;
};

int main() {

    int nIterations = 100000;
//    auto fut1 = std::async(std::launch::async, foo, true, nIterations);
//    auto fut2 = std::async(std::launch::async, foo, true, nIterations);
    CAsyncRunner asyncRunner(2);
    auto fut1 = asyncRunner.asyncRun(foo, true, true, nIterations);
    auto fut3 = asyncRunner.asyncRun(foo, false, true, nIterations);
    auto fut4 = asyncRunner.asyncRun(foo, false, true, nIterations);
    auto fut2 = asyncRunner.asyncRun(fooN, false);
    asyncRunner.~CAsyncRunner();
//    int sum = 0;
//    for( int i = 0; i < nIterations; ++i ){
//        sum++;
//    }
    std::cout << fut1.GetValue() << ' ' << fut2.GetValue() << " " << fut3.GetValue() << " "
              << fut4.GetValue() << " "<< std::endl;
    return 0;
}