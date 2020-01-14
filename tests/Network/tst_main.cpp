#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>

#include <Helpz/net_fragmented_message.h>

namespace Helpz
{
class Network_Test : public QObject
{
    Q_OBJECT

public:
    Network_Test()
    {
    }

private Q_SLOTS:

    void initTestCase()
    {
        qDebug() << "init";
        QVERIFY(1 == 1);
    }
    void cleanupTestCase()
    {
        qDebug() << "clean";
    }

    void fragment_message_parts_test()
    {
        char data[100];
        Helpz::Network::Fragmented_Message msg(1, 1, 1, 100);
        msg.data_device_->open(QIODevice::ReadWrite);
        using T = std::vector<std::pair<uint32_t, uint32_t>>;

//        T vect{{0, 100}};
        QCOMPARE((T{{0, 100}}), msg.part_vect_);

        msg.add_data(0, data, 5);
        QCOMPARE((T{{5, 100}}), msg.part_vect_);

        msg.add_data(1, data, 3);
        QCOMPARE((T{{5, 100}}), msg.part_vect_);

        msg.add_data(10, data, 5);
        QCOMPARE((T{{5, 10},{15, 100}}), msg.part_vect_);

        msg.add_data(50, data, 5);
        QCOMPARE((T{{5, 10},{15, 50}, {55, 100}}), msg.part_vect_);

        msg.add_data(95, data, 5);
        QCOMPARE((T{{5, 10},{15, 50}, {55, 95}}), msg.part_vect_);

        msg.add_data(96, data, 3);
        QCOMPARE((T{{5, 10},{15, 50}, {55, 95}}), msg.part_vect_);

        msg.add_data(15, data, 5);
        QCOMPARE((T{{5, 10},{20, 50}, {55, 95}}), msg.part_vect_);

        msg.add_data(5, data, 5);
        QCOMPARE((T{{20, 50}, {55, 95}}), msg.part_vect_);

        msg.add_data(0, data, 30);
        QCOMPARE((T{{30, 50}, {55, 95}}), msg.part_vect_);

        msg.add_data(70, data, 30);
        QCOMPARE((T{{30, 50}, {55, 70}}), msg.part_vect_);

        msg.add_data(30, data, 40);
        QCOMPARE((T{}), msg.part_vect_);
    }
};

} // namespace Dai

QTEST_MAIN(Helpz::Network_Test)

#include "tst_main.moc"
