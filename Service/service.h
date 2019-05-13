#ifndef SERVICE_H
#define SERVICE_H

#include <vector>
#include <memory>
#include <cassert>

#include <QThread>
#include <QMetaMethod>
#include <QLoggingCategory>

#include <Helpz/logging.h>
#include <Helpz/qtservice.h>

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

class Base
{
public:
    static const QLoggingCategory &Log();

    Base();

    Base(int argc, char **argv, const QString &name = QString());

protected:
    virtual OneObjectThread* getWorkerThread() = 0;
    virtual bool is_immediately() const = 0;
    void start();
    void stop();
    void processCommands(const QStringList &cmdList);
private:

    friend void term_handler(int);

    std::shared_ptr<Object> service_;
};

template<typename Application>
class Base_Template : public QtService<Application>, public Base
{
public:
    Base_Template(int argc, char **argv, const QString& name) :
        QtService<Application>( argc, argv, name.isEmpty() ? QCoreApplication::applicationName() : name ),
        Base()
    {
        assert( !QtService<Application>::serviceName().isEmpty() );

    #ifdef Q_OS_WIN32
        setStartupType(QtServiceController::AutoStartup);
    #endif

    #ifndef HAS_QT_SERVICE_IMMEDIATELY_CHECK
        for(int i = 1; i < argc; ++i)
        {
            QString a(argv[i]);
            if (a == QLatin1String("-e") || a == QLatin1String("-exec"))
            {
                isImmediately_ = true;
                break;
            }
        }
    #endif
    }

#ifndef HAS_QT_SERVICE_IMMEDIATELY_CHECK
    bool isImmediately() const { return isImmediately_; }
#endif

    bool is_immediately() const override { return Base_Template<Application>::isImmediately(); }
private:
    void start() override { Base::start(); }
    void stop() override { Base::stop(); }
    void processCommands(const QStringList &cmdList) override { Base::processCommands(cmdList); }

#ifndef HAS_QT_SERVICE_IMMEDIATELY_CHECK
    bool isImmediately_ = false;
#endif
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

template<class T, typename Application = QCoreApplication>
class Impl : public Base_Template<Application>
{
public:
    using Base_Template<Application>::Base_Template;

    static Impl<T, Application>& instance(int argc = 0, char **argv = nullptr, const QString &name = QString())
    {
        static Impl<T, Application> service(argc, argv, name);
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
