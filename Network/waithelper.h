#ifndef HELPZ_NETWORK_WAITHELPER_H
#define HELPZ_NETWORK_WAITHELPER_H

#include <QVariant>

#include <memory>

QT_BEGIN_NAMESPACE
class QTimer;
class QEventLoop;
QT_END_NAMESPACE

namespace Helpz {
namespace Net {

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

typedef std::map<quint16, std::shared_ptr<Helpz::Net::WaitHelper>> WaiterMap;
class Waiter {
public:
    Waiter(quint16 cmd, WaiterMap& waitHelper_map, int msecTimeout = 5000);
    ~Waiter();

    operator bool() const;

    WaiterMap& wait_map;
    std::shared_ptr<Helpz::Net::WaitHelper> helper;
private:
    WaiterMap::iterator it;
};

} // namespace Net
} // namespace Helpz

#endif // HELPZ_NETWORK_WAITHELPER_H
