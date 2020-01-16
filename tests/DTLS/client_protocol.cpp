#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>

#include "client_protocol.h"

namespace Helpz {

/*static*/ std::shared_ptr<Helpz::Network::Protocol> Client_Protocol::create(const std::string &/*app_protocol*/)
{
    return std::shared_ptr<Helpz::Network::Protocol>(new Client_Protocol{});
}

std::future<void> Client_Protocol::test_simple_message(const QString &text)
{
    static std::promise<void> promise;
    promise = std::promise<void>();

    send(MSG_SIMPLE)
            .answer([](QIODevice& /*dev*/)
    {
        promise.set_exception(std::make_exception_ptr(std::logic_error("No answer in this test")));
    }).timeout([]()
    {
        promise.set_value();
        std::cout << "MSG_SIMPLE timeout test" << std::endl;
    }, std::chrono::seconds(1)) << text;

    return promise.get_future();
}

std::future<QString> Client_Protocol::test_message_with_answer(quint32 value2)
{
    static std::promise<QString> promise;
    promise = std::promise<QString>();

    send(MSG_ANSWERED).answer([this](QIODevice& data_dev)
    {
        QString answer_text;
        parse_out(data_dev, answer_text);

        promise.set_value(answer_text);
        std::cout << "Answer text: " << answer_text.toStdString() << std::endl;
    }).timeout([]()
    {
        promise.set_exception(std::make_exception_ptr(std::logic_error("No timeout in this test")));
        std::cout << "MSG_ANSWERED timeout" << std::endl;
    }, std::chrono::seconds(15), std::chrono::milliseconds{3000}) << bool(true) << value2;

    return promise.get_future();
}

QByteArray Client_Protocol::test_send_file()
{
    QString file_name = qApp->applicationDirPath() + "/test_file.dat"; //"/usr/lib/libcc1.so.0.0.0";
    QFile::remove(file_name);
    std::unique_ptr<QFile> device(new QFile(file_name));
    if (!device->open(QIODevice::ReadWrite))
        throw std::runtime_error("Can't open file: " + device->errorString().toStdString());

    while (device->size() < 100000000)
    {
        std::unique_ptr<char[]> data(new char[4096]);
        device->write(data.get(), 4096);
    }
    device->seek(0);

    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!hash.addData(device.get()))
        throw std::runtime_error("Can't get file hash.");
    QByteArray hash_result = hash.result();
    device->seek(0);

    send(MSG_FILE_META) << file_name << static_cast<uint32_t>(device->size()) << hash_result;
    send(MSG_FILE).set_data_device(std::move(device));
    return hash_result;
}

void Client_Protocol::ready_write()
{
    std::cout << "Connected" << std::endl;

//    test_send_file();
//    test_simple_message();
//    test_message_with_answer();
}

void Client_Protocol::process_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    std::cout << "process_message #" << int(msg_id) << ' ' << int(cmd) << " size " << data_dev.size() << std::endl;
}

void Client_Protocol::process_answer_message(uint8_t msg_id, uint8_t cmd, QIODevice &data_dev)
{
    std::cout << "process_answer_message #" << int(msg_id) << ' ' << int(cmd) << " size " << data_dev.size() << std::endl;
}

} // namespace Helpz
