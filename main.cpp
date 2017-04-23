#include <iostream>
#include <future>
#include <thread>


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

template <typename R, typename ...Args>
class my_task<R(Args...)>
{
    std::function<R(Args...)> fn;
    std::promise<R> pr;             // the promise of the result
public:
    template <typename ...Ts>
    explicit my_task(Ts &&... ts) : fn(std::forward<Ts>(ts)...) { }

    template <typename ...Ts>
    void operator()(Ts &&... ts)
    {
        pr.set_value(fn(std::forward<Ts>(ts)...));  // fulfill the promise
    }

    std::future<R> get_future() { return pr.get_future(); }

    // disable copy, default move
};

template<typename F, typename ...Args>
auto my_async(F&& func, Args&&... args) -> std::future<decltype(func(args...))>
{
    auto task = my_task<decltype(func(args...))()>(std::bind(&func, args...));
    auto future = task.get_future();
    std::thread(std::move(task)).detach();
    return std::move(future);
}

int main() {

    int nIterations = 1000000000;
//    auto fut1 = std::async(std::launch::async, foo, true, nIterations);
//    auto fut2 = std::async(std::launch::async, foo, true, nIterations);
    auto fut1 = my_async(foo, true, nIterations);
    auto fut2 = my_async(fooN);
    int sum = 0;
    for( int i = 0; i < nIterations; ++i ){
        sum++;
    }
    std::cout << fut1.get() << ' ' << fut2.get() << " " << sum << std::endl;
    return 0;
}