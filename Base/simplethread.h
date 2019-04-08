#ifndef HELPZ_SIMPLETHREAD_H
#define HELPZ_SIMPLETHREAD_H

#include <memory>
#include <tuple>
#include <type_traits>
#include <atomic>
#include <future>

#include <QThread>

#include "fake_integer_sequence.h"

namespace Helpz {

template<class T, typename... Args>
class Thread_Base : public QThread
{
public:
    Thread_Base(Args... __args) :
        args_(new Tuple{__args...}) {}

    ~Thread_Base()
    {
        if (args_)
            delete args_;
    }

protected:
    virtual void started(T*) {}

    template<std::size_t N>
    using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;
    template<std::size_t N>
    constexpr ArgType<N>& arg() const
    {
        return std::get<N>(*args_);
    }

    template<std::size_t... Idx>
    T* allocate_impl(std::index_sequence<Idx...>) const
    {
        return new T{ std::get<Idx>(std::forward<Tuple>(*args_))... };
    }

    virtual void run() override
    {
        std::unique_ptr<T> objPtr(allocate_impl(Indexes{}));
        started(objPtr.get());
        delete args_; args_ = nullptr;
        exec();
    }

    using Tuple = std::tuple<typename std::decay<Args>::type...>;
private:
    using Indexes = std::make_index_sequence<std::tuple_size<Tuple>::value>;
    Tuple* args_;
};

class Thread_Promises
{
public:
    Thread_Promises()
    {
        future_ = promise_.get_future();
        shared_future_ = future_.share();
    }

    void set_value(bool value)
    {
        promise_.set_value(value);
    }

    void set_exception(std::exception_ptr exception_ptr)
    {
        promise_.set_exception(exception_ptr);
    }

    std::shared_future<bool> get_future()
    {
        return shared_future_;
    }

private:
    std::promise<bool> promise_;
    std::future<bool> future_;
    std::shared_future<bool> shared_future_;
};

template<class T, typename... Args>
class ParamThread : public Thread_Base<T, Args...>
{
public:
    ParamThread(Args... __args) :
        Thread_Base<T, Args...>::Thread_Base(__args...),
        ptr_(nullptr),
        promises_(new Thread_Promises{})
    {}
    ~ParamThread()
    {
        if (promises_)
            delete promises_;
    }

    T* ptr()
    {
        T* obj = ptr_.load();
        if (!obj && promises_)
        {
            if (!promises_->get_future().get())
                throw std::runtime_error("bad thread start result");
            obj = ptr_.load();
        }
        return obj;
    }

protected:
    virtual void started(T* obj) override
    {
        ptr_.store(obj);
        promises_->set_value(obj != nullptr);
        delete promises_; promises_ = nullptr;
    }

    virtual void run() override
    {
        try
        {
            Thread_Base<T, Args...>::run();
            ptr_.store(nullptr);
        }
        catch(...)
        {
            if (!promises_)
                throw std::current_exception();
            promises_->set_exception(std::current_exception());
            delete promises_; promises_ = nullptr;
        }
    }

private:
    std::atomic<T*> ptr_;
    Thread_Promises* promises_;
};

} // namespace Helpz

#endif // HELPZ_SIMPLETHREAD_H

