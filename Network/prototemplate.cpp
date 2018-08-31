#include <QDataStream>
#include <QDateTime>

#include "prototemplate.h"

namespace Helpz {
namespace Network {

Q_LOGGING_CATEGORY(Log, "net")
Q_LOGGING_CATEGORY(DetailLog, "net.detail")

ProtoTemplate::Helper::Helper(ProtoTemplate *protoTemplate, quint16 command) :
    self(protoTemplate), cmd(command), data(), ds(&data, QIODevice::WriteOnly) {
    ds.setVersion(protoTemplate->ds_ver);
}

ProtoTemplate::Helper::Helper(ProtoTemplate::Helper &&obj) noexcept :
    self(std::move(obj.self)), data(std::move(obj.data)), ds(&data, QIODevice::WriteOnly) {
    ds.device()->seek(obj.ds.device()->pos());
    ds.setVersion(obj.ds.version());

    obj.ds.unsetDevice();
    obj.self = nullptr;
}

ProtoTemplate::Helper::~Helper() {
    ds.unsetDevice();

    if (!self)
        return;

    self->write(prepareMessage());
}

QByteArray ProtoTemplate::Helper::pop_message()
{
    self = nullptr;
    return prepareMessage();
}

QDataStream &ProtoTemplate::Helper::dataStream() { return ds; }

QByteArray ProtoTemplate::Helper::prepareMessage()
{
    if ((cmd >> 15) & 1)
    {
        qCCritical(Log) << "ERROR: Try to send bad cmd with setted last bit" << cmd;
        cmd &= ~(1 << 15);
    }

    bool compress = data.size() >= 1024;
    if (compress)
        cmd |= 1 << 15;

    QByteArray buffer;
    QDataStream buffer_stream(&buffer, QIODevice::WriteOnly);
    buffer_stream << quint16(0) << cmd << (compress ? qCompress(data) : data);
    buffer_stream.device()->seek(0);
    buffer_stream << qChecksum(buffer.constData() + 2, 6);

    qCDebug(DetailLog) << "CMD OUT" << (cmd & ~(1 << 15)) << "SIZE" << data.size() << "WRITE" << buffer.size();
    return buffer;
}

ProtoTemplate::Helper ProtoTemplate::send(quint16 cmd) { return Helper(this, cmd); }

// --------------------------------------------------------------------------------------------------

ProtoTemplate::ProtoTemplate() :
    currentCmd(0), totalMsgSize(0), m_checkReturned(true)
{
}

qint64 ProtoTemplate::lastMsgTime() const { return m_lastMsgTime; }
void ProtoTemplate::updateLastMessageTime() { m_lastMsgTime = QDateTime::currentMSecsSinceEpoch(); }
bool ProtoTemplate::checkReturned() const { return m_checkReturned; }

void ProtoTemplate::sendCmd(quint16 cmd)
{
    if (cmd == Cmd::Ping)
        m_checkReturned = false;
    send(cmd);
}
void ProtoTemplate::sendByte(quint16 cmd, char byte) { send(cmd) << byte; }
void ProtoTemplate::sendArray(quint16 cmd, const QByteArray &buff) { send(cmd) << buff; }

void ProtoTemplate::proccess_bytes(QIODevice *dev)
{
    if (!dev)
        return;

    QDataStream msg_stream(dev);
    msg_stream.setVersion(ds_ver);

    bool checksum_ok;
    quint16 checksum, cmd;
    QByteArray checksum_data(6, Qt::Uninitialized), buffer;

    while (dev->bytesAvailable() >= 8)
    {
        msg_stream.startTransaction();
        msg_stream >> checksum;

        msg_stream.readRawData(checksum_data.data(), 6);
        msg_stream.device()->seek(msg_stream.device()->pos() - 6);
        checksum_ok = checksum == qChecksum(checksum_data, 6);

        msg_stream >> cmd >> buffer;

        qCDebug(DetailLog) << "CMD" << (cmd & ~(1 << 15)) << "SIZE" << buffer.size();

        if (!msg_stream.commitTransaction() && checksum_ok)
            return; // Wait more bytes

        if (!checksum_ok)
        {
            qCWarning(Log) << "BAD Message:" << cmd << buffer.toHex().toUpper()
                        << "Bytes available" << dev->bytesAvailable() << "Tail(8)..." << dev->readAll().left(8).toHex().toUpper()
                        << "Checksum" << checksum << "Expected" << qChecksum(checksum_data, 6);

            if (!dev->atEnd())
                dev->seek(dev->size());
            return;
        }

        if ((cmd >> 15) & 1)
        {
            cmd &= ~(1 << 15);
            buffer = qUncompress(buffer);
        }

        QDataStream user_ds(&buffer, QIODevice::ReadOnly);
        user_ds.setVersion(ds_ver);

        if (cmd < Cmd::UserCommand)
        {
            switch (cmd) {
            case Cmd::Ping:
                sendCmd(Cmd::Pong);
                break;
            case Cmd::Pong:
                m_checkReturned = true;
                break;
            default:
                break;
            }
        }
        try {
            proccessMessage(cmd, user_ds);
        } catch(const std::exception& e) {
            qCCritical(Log) << "EXCEPTION: proccessMessage" << cmd << e.what();
        } catch(...) { qCCritical(Log) << "EXCEPTION Unknown: proccessMessage" << cmd; }
    }
}

} // namespace Network
} // namespace Helpz
