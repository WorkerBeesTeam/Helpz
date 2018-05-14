#ifndef PROTOTEMPLATE_H
#define PROTOTEMPLATE_H

#include <iostream>

#include <tuple>
#include <functional>
//#include <experimental/tuple>
#include <utility>
#include <type_traits>

#include <QDebug>
#include <QBuffer>
#include <QDataStream>

#include <QLoggingCategory>

namespace Helpz {

template<typename _Tuple>
void parse(QDataStream &, _Tuple&) {}

template<typename _Tuple, std::size_t x, std::size_t... _Idx>
void parse(QDataStream &ds, _Tuple& __t) {
    using T = typename std::tuple_element<x, _Tuple>::type;
    // { qDebug() << "Parse" << typeid(T).name(); }
    T obj;
    ds >> obj;

    if (ds.status() != QDataStream::Ok)
        return;

    std::get<x>(__t) = obj;
    parse<_Tuple, _Idx...>(ds, __t);
}

template<typename _Tuple, std::size_t... _Idx>
bool parseImpl(QDataStream &ds, _Tuple& __t)
{
    QDataStream::Status oldStatus = ds.status();
    if (!ds.device() || !ds.device()->isTransactionStarted())
        ds.resetStatus();

    parse<_Tuple, _Idx...>(ds, __t);

    if (ds.status() != QDataStream::Ok)
        return false;

    if (oldStatus != QDataStream::Ok) {
        ds.resetStatus();
        ds.setStatus(oldStatus);
    }
    return true;
}

template <typename RetType, class T, typename _Fn, typename _Tuple, std::size_t... _Idx>
RetType applyParseImpl(T* obj, _Fn __f, QDataStream &ds, _Tuple&& __t, std::index_sequence<_Idx...>)
{
    if (!parseImpl<_Tuple, _Idx...>(ds, __t))
    {
        const QLoggingCategory category("helpz");
        qCWarning(category) << "Apply parse failed!";

        if constexpr (std::is_same<RetType, void>::value)
            return;
        else if constexpr (std::is_same<RetType, bool>::value)
            return false;
        else if constexpr (std::is_arithmetic<RetType>::value)
            return 0;
        else
            return RetType{};
    }

    auto func = std::bind(__f, obj, std::_Placeholder<_Idx + 1>{}...);
    return std::__invoke(func, std::get<_Idx>(std::forward<_Tuple>(__t))...);
}

template <typename RetType, class T, typename _Fn, typename... Args>
RetType applyParseImpl(T* obj, _Fn __f, QDataStream &ds)
{
    auto tuple = std::make_tuple(typename std::decay<Args>::type()...);

    using Tuple = decltype(tuple);
    using Indices = std::make_index_sequence<std::tuple_size<Tuple>::value>;

    return applyParseImpl<RetType>(obj, __f, ds, std::forward<Tuple>(tuple), Indices{});
}

template<class T, class FT, typename RetType, typename... Args>
RetType applyParse(T* obj, RetType(FT::*__f)(Args...) const, QDataStream &ds)
{
    return applyParseImpl<RetType, T, decltype(__f), Args...>(obj, __f, ds);
}

template<class T, class FT, typename RetType, typename... Args>
RetType applyParse(T* obj, RetType(FT::*__f)(Args...), QDataStream &ds)
{
    return applyParseImpl<RetType, T, decltype(__f), Args...>(obj, __f, ds);
}

namespace Network {

Q_DECLARE_LOGGING_CATEGORY(Log)
Q_DECLARE_LOGGING_CATEGORY(DetailLog)

namespace Cmd {
enum ReservedCommands {
    Zero = 0,

    Ping,
    Pong,

    TextCommand,

    UserCommand = 16
};
}

class ProtoTemplate
{
public:
    enum { ds_ver = QDataStream::Qt_5_6 };
#if (QT_VERSION > QT_VERSION_CHECK(5, 11, 0))
#pragma GCC warning "Think about raise used version or if it the same, then raise qt version in this condition."
#endif

    class Helper {
    public:
        Helper(ProtoTemplate* protoTemplate, quint16 command);
        Helper(Helper&& obj) noexcept;
        ~Helper();

        QByteArray pop_message();

        QDataStream& dataStream();

        template<typename T>
        QDataStream& operator <<(const T& item) { return ds << item; }
    protected:
        QByteArray prepareMessage();
        ProtoTemplate* self;
        quint16 cmd;
        QByteArray data;
        QDataStream ds;
    };

public:
    ProtoTemplate();

    qint64 lastMsgTime() const;
    void updateLastMessageTime();

    bool checkReturned() const;

    Helper send(quint16 cmd);
    void sendCmd(quint16 cmd);
    void sendByte(quint16 cmd, char byte);
    void sendArray(quint16 cmd, const QByteArray &buff);

//    template<typename T>
//    Helper sendT(ushort cmd, const T& obj) {
//        return sendT(cmd) << obj;
//    }

    template<typename T>
    T parse(QDataStream &ds) {
        T obj; ds >> obj; return obj;
    }

//    template<class T, typename RetType, typename... Args>
//    RetType applyParse(T* obj, RetType(T::*func)(Args...), QDataStream &ds) {
//        return RetType();
////        T obj; ds >> obj; return obj;
//    }

    template<class T, typename RetType, typename... Args>
    RetType applyParse(RetType(T::*__f)(Args...), QDataStream &ds)
    {
        return Helpz::applyParse(static_cast<T*>(this), __f, ds);
    }

    template<class T, typename RetType, typename... Args>
    RetType applyParse(RetType(T::*__f)(Args...) const, QDataStream &ds) const
    {
        return Helpz::applyParse(static_cast<T*>(this), __f, ds);
    }

protected:
    virtual void proccessMessage(quint16 cmd, QDataStream& msg) = 0;
    virtual void write(const QByteArray& buff) = 0;

    virtual void proccess_bytes(QIODevice* dev);

    quint16 currentCmd;
    int totalMsgSize; // QByteArray не может быть больше Int
    QByteArray msg;

private:
    qint64 m_lastMsgTime = 0;
    bool m_checkReturned = false;
};

} // namespace Network
} // namespace Helpz

#endif // PROTOTEMPLATE_H
