#pragma once

#include <future>
#include "future.h"

template<typename T>
class CPromise {
public:
    typedef typename CFuture<T>::SharedState SharedState ;

    CPromise();

    CFuture<T> GetFuture();
    void SetValue( const T& newValue );
    void SetException( const std::exception& newException );

private:
    bool isFutureRetrieved;
    std::shared_ptr<std::mutex> criticalSectionLock;
    std::shared_ptr<std::condition_variable> isReadyCV;
    std::shared_ptr<SharedState> sharedState;
};

template<typename T>
CPromise<T>::CPromise()
        : sharedState( new SharedState() ),
          criticalSectionLock( new std::mutex() ),
          isReadyCV( new std::condition_variable() ),
          isFutureRetrieved( false ) {

    sharedState->isReady = false;
    sharedState->isException = false;
}

template<typename T>
CFuture<T> CPromise<T>::GetFuture()
{
    if( !isFutureRetrieved ) {
        isFutureRetrieved = true;
        return CFuture<T>( sharedState, isReadyCV, criticalSectionLock );
    } else {
        throw std::future_errc( std::future_errc::future_already_retrieved );
    }
}

template<typename T>
void CPromise<T>::SetValue( const T& newValue )
{
    std::unique_lock<std::mutex> lock( *criticalSectionLock );
    if( sharedState->isReady ) {
        throw std::future_errc( std::future_errc::promise_already_satisfied );
    }
    sharedState->isReady = true;
    sharedState->storedValue = newValue;
    isReadyCV->notify_one();
}

template<typename T>
void CPromise<T>::SetException( const std::exception& newException )
{
    std::unique_lock<std::mutex> lock( *criticalSectionLock );
    if( sharedState->isReady ) {
        throw std::future_errc( std::future_errc::promise_already_satisfied );
    }
    sharedState->isReady = true;
    sharedState->isException = true;
    sharedState->storedException = newException;
    isReadyCV->notify_one();
}
