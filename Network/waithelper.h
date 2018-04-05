#ifndef ZIMNIKOV_NETWORK_WAITHELPER_H
#define ZIMNIKOV_NETWORK_WAITHELPER_H

#include <QVariant>

#include <memory>

QT_BEGIN_NAMESPACE
class QTimer;
class QEventLoop;
QT_END_NAMESPACE

namespace Helpz {
namespace Network {

class WaitHelper {
public:
    WaitHelper(int msecTimeout = 5000);
    ~WaitHelper();

    bool wait();
    void finish(const QVariant& value = QVariant());

    QVariant result() const;
private:
    QVariant m_result;
    QEventLoop* m_lock;
    QTimer* m_timer;
};

typedef std::map<quint16, std::shared_ptr<Helpz::Network::WaitHelper>> WaiterMap;
class Waiter {
public:
    Waiter(quint16 cmd, WaiterMap& waitHelper_map, int msecTimeout = 5000);
    ~Waiter();

    operator bool() const;

    WaiterMap& wait_map;
    std::shared_ptr<Helpz::Network::WaitHelper> helper;
private:
    WaiterMap::iterator it;
};

} // namespace Network
} // namespace Helpz

#endif // ZIMNIKOV_NETWORK_WAITHELPER_H
