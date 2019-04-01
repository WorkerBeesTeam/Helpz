#ifndef SERVICE_H
#define SERVICE_H

#include <vector>
#include <memory>
#include <assert.h>

#include <QThread>
#include <QMetaMethod>
#include <QLoggingCategory>

#include <Helpz/logging.h>
#include "qtservice.h"

namespace Helpz {
namespace Service {

class OneObjectThread : public QThread {
    Q_OBJECT
protected:
    QObject *obj_ = nullptr;
public:
    QObject *obj() const { return obj_; }
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
    std::vector<QThread*> th_;
};

class Base : public QtService<QCoreApplication>
{
public:
    static const QLoggingCategory &Log();

    Base(int argc, char **argv, const QString &name = QString());
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
    bool isImmediately_ = false;
#endif

    std::shared_ptr<Object> service_;
};

template<class T>
class WorkerThread : public OneObjectThread {
    void run() override
    {
        std::unique_ptr<T> workObj(new T);
        obj_ = workObj.get();

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

    static Impl<T>& instance(int argc = 0, char **argv = nullptr, const QString &name = QString())
    {
        static Impl<T> service(argc, argv, name);
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
