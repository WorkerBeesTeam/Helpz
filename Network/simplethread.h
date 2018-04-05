#ifndef ZIMNIKOV_SIMPLETHREAD_H
#define ZIMNIKOV_SIMPLETHREAD_H

#include <memory>
#include <tuple>

#include <QThread>

namespace Helpz {

template<class T, typename... Args>
class ParamThread : public QThread
{
public:
    typedef std::tuple<Args...> Tuple;

    ParamThread(Args... __args) :
        m_args(new Tuple{__args...}), m_ptr(nullptr) { }
    ~ParamThread() { if (m_args) delete m_args; }

    T* ptr() { return m_ptr; }
    const T* ptr() const { return m_ptr; }

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
            std::unique_ptr<T> objPtr(m_ptr = allocateImpl(Indexes{}));
            SafePtrWatcher watch(&m_ptr);

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
        SafePtrWatcher(T** obj) : m_obj(obj) {}
        ~SafePtrWatcher() { *m_obj = nullptr; }
        T** m_obj;
    };

    Tuple* m_args;
    T* m_ptr;
};

} // namespace Helpz

#endif // SIMPLETHREAD_H

