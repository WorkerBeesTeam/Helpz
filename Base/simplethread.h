#ifndef HELPZ_SIMPLETHREAD_H
#define HELPZ_SIMPLETHREAD_H

#include <memory>
#include <tuple>
#include <atomic>

#include <QThread>

#include "fake_integer_sequence.h"

namespace Helpz {

template<class T, typename... Args>
class ParamThread : public QThread
{
public:
    typedef std::tuple<Args...> Tuple;

    ParamThread(Args... __args) :
        m_args(new Tuple{__args...}), ptr_(nullptr) { }
    ~ParamThread() { if (m_args) delete m_args; }

    T* ptr() { return ptr_.load(); }
    const T* ptr() const { return ptr_.load(); }

    virtual void started() {}

    template<std::size_t N>
    using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;
protected:
    template<std::size_t N>
    constexpr ArgType<N>& arg() const {
        return std::get<N>(*m_args);
    }

    template<std::size_t... Idx>
    T* allocateImpl(std::index_sequence<Idx...>) const {
        return new T{ std::get<Idx>(std::forward<Tuple>(*m_args))... };
    }

    virtual void run() override
    {
        try {
            using Indexes = std::make_index_sequence<std::tuple_size<Tuple>::value>;
            std::unique_ptr<T> objPtr(allocateImpl(Indexes{}));
            ptr_.store(objPtr.get());
            SafePtrWatcher watch(&ptr_);

            started();
            delete m_args; m_args = nullptr;

            exec();
        } catch(const QString& msg) {
            qWarning("%s", qPrintable(msg));
        } catch(const std::exception& e) {
            qWarning("%s", e.what());
        } catch(...) {}
    }

private:
    struct SafePtrWatcher {
        SafePtrWatcher(std::atomic<T*>* ptr) : ptr_(ptr) {}
        ~SafePtrWatcher() { ptr_->store(nullptr); }
        std::atomic<T*>* ptr_;
    };

    Tuple* m_args;
    std::atomic<T*> ptr_;
};

} // namespace Helpz

#endif // HELPZ_SIMPLETHREAD_H

