#pragma once

#include <condition_variable>
#include <atomic>

class MyFutureBase{
public:
    virtual bool IsReady() = 0;
};

template<typename T> class CPromise;

template<typename T>
class CFuture : public MyFutureBase {
public:
    friend class CPromise<T>;

    T GetValue();
    bool IsReady();
    bool TryGetValue( T& newValue );
private:
    struct SharedState {
        std::atomic_bool isReady;
        bool isException;
        T storedValue;
        std::exception storedException;
    };

    bool isValid;
    std::shared_ptr<std::mutex> criticalSectionLock;
    std::shared_ptr<std::condition_variable> isReadyCV;
    std::shared_ptr<SharedState> sharedState;

    CFuture( std::shared_ptr<SharedState> promiseSharedState,
              std::shared_ptr<std::condition_variable> isReadyCV,
              std::shared_ptr<std::mutex> criticalSectionLock );

    void checkValidity();
    void checkException();
};

template<typename T>
T CFuture<T>::GetValue()
{
    checkValidity();
    std::unique_lock<std::mutex> lock( *criticalSectionLock );
    while( !sharedState->isReady ) {
        isReadyCV->wait( lock );
    }
    isValid = false;
    checkException();
    return sharedState->storedValue;
}

template<typename T>
bool CFuture<T>::TryGetValue( T& newValue )
{
    checkValidity();
    std::unique_lock<std::mutex> lock( *criticalSectionLock );
    if( sharedState->isReady ) {
        isValid = false;
        checkException();
        newValue = sharedState->storedValue;
        return true;
    } else {
        return false;
    }
}

template<typename T>
CFuture<T>::CFuture( std::shared_ptr<SharedState> promiseSharedState,
                       std::shared_ptr<std::condition_variable> isReadyCV,
                       std::shared_ptr<std::mutex> criticalSectionLock )
        : sharedState( promiseSharedState ),
          isReadyCV( isReadyCV ),
          criticalSectionLock( criticalSectionLock),
          isValid( true) {}

template<typename T>
void CFuture<T>::checkValidity()
{
    if( !isValid ) {
        throw std::exception( std::exception() );
    }
}

template<typename T>
void CFuture<T>::checkException()
{
    if( sharedState->isException ) {
        throw sharedState->storedException;
    }
}

template<typename T>
bool CFuture<T>::IsReady() {
    return sharedState->isReady;
}
