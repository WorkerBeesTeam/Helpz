#ifndef SERVICE_H
#define SERVICE_H

#include <vector>
#include <memory>
#include <assert.h>

#include <QThread>
#include <QMetaMethod>
#include <QLoggingCategory>

#include "qtservice.h"
#include "logging.h"

namespace Helpz {
namespace Service {

class OneObjectThread : public QThread {
    Q_OBJECT
protected:
    QObject *m_obj = nullptr;
public:
    QObject *obj() const { return m_obj; }
signals:
    void logMessage(QtMsgType type, const Helpz::LogContext &ctx, const QString &str);
    void restart();
};

class Object final : public QObject {
public:
    Object(OneObjectThread* worherThread, bool debug);
    ~Object();

    void processCommands(const QStringList &cmdList);

private:
    void restart();
    std::vector<QThread*> th;
};

class Base : public QtService<QCoreApplication>
{
public:
    static const QLoggingCategory &Log();

    Base(int argc, char **argv);
#ifndef HAS_QT_SERVICE_IMMEDIATELY_CHECK
    bool isImmediately() const;
#endif
protected:
    virtual OneObjectThread* getWorkerThread() = 0;
private:
    void start() override;
    void stop() override;

    void processCommands(const QStringList &cmdList) override;

    friend void term_handler(int);

#ifndef HAS_QT_SERVICE_IMMEDIATELY_CHECK
    bool m_isImmediately = false;
#endif

    std::shared_ptr<Object> service;
};

template<class T>
class WorkerThread : public OneObjectThread {
    void run() override
    {
        std::unique_ptr<T> workObj(new T);
        m_obj = workObj.get();

        for (int n = 0; n < T::staticMetaObject.methodCount(); n++)
        {
            auto&& method = T::staticMetaObject.method(n).name();
            if (method == "logMessage")
                connect(this, SIGNAL(logMessage(QtMsgType, Helpz::LogContext, QString)), workObj.get(),
                        SLOT(logMessage(QtMsgType, Helpz::LogContext, QString)), Qt::QueuedConnection);
            else if (method == "serviceRestart")
                connect(workObj.get(), SIGNAL(serviceRestart()), this, SIGNAL(restart()), Qt::QueuedConnection);
        }

        exec();
    }
};

template<class T>
class Impl : public Base
{
public:
    using Base::Base;

    static Impl<T>& instance(int argc = 0, char **argv = nullptr)
    {
        static Impl<T> service(argc, argv);
        return service;
    }
private:
    OneObjectThread* getWorkerThread() override
    {
        return new WorkerThread<T>;
    }
};

} // namespace Service
} // namespace Helpz

#endif // SERVICE_H
