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

#include "applyparse.h"

namespace Helpz {
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
#if (QT_VERSION > QT_VERSION_CHECK(5, 11, 2))
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

//    template<typename T>
//    T parse(QDataStream &ds) {
//        T obj; ds >> obj; return obj;
//    }

//    template<class T, typename RetType, typename... Args>
//    RetType applyParse(T* obj, RetType(T::*func)(Args...), QDataStream &ds) {
//        return RetType();
////        T obj; ds >> obj; return obj;
//    }

    template<typename RetType, class T, typename... Args>
    RetType applyParse(RetType(T::*__f)(Args...), QDataStream &ds) {
        return applyParseImpl<RetType, decltype(__f), T, Args...>(__f, static_cast<T*>(this), ds);
    }

    template<typename RetType, class T, typename... Args>
    RetType applyParse(RetType(T::*__f)(Args...) const, QDataStream &ds) const {
        return applyParseImpl<RetType, decltype(__f), T, Args...>(__f, static_cast<T*>(this), ds);
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
