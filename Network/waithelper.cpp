#include <QTimer>
#include <QEventLoop>

#include "waithelper.h"

namespace Helpz {
namespace Network {

WaitHelper::WaitHelper(int msecTimeout) {
    m_lock = new QEventLoop;
    m_timer = new QTimer;
    QObject::connect(m_timer, &QTimer::timeout, m_lock, &QEventLoop::quit);
    m_timer->setInterval(msecTimeout);
}

WaitHelper::~WaitHelper()
{
    if (m_timer->isActive())
        m_timer->stop();
    delete m_timer;

    if (m_lock->isRunning())
        m_lock->quit();
    delete m_lock;
}

bool WaitHelper::wait() {
    m_timer->start();
    int ret_code = m_lock->exec();
    m_timer->stop();
    return ret_code == 1;
}

void WaitHelper::finish(const QVariant &value) {
    m_result = value;
    m_lock->exit(1);
}

QVariant WaitHelper::result() const { return m_result; }

// ---------------------------------------------------------------------
Waiter::Waiter(quint16 cmd, Helpz::Network::WaiterMap &waitHelper_map, int msecTimeout) :
    wait_map(waitHelper_map), it(wait_map.end())
{
    if (wait_map.find(cmd) != wait_map.cend())
        return;

    auto waiter = std::make_shared<Helpz::Network::WaitHelper>(msecTimeout);
    auto empl = wait_map.emplace(cmd, waiter);
    if (empl.second)
    {
        helper = waiter;
        it = empl.first;
    }
}

Waiter::~Waiter() {
    if (it != wait_map.end())
    wait_map.erase(it);
}

Waiter::operator bool() const {
    return helper && it != wait_map.end();
}

} // namespace Network
} // namespace Helpz
