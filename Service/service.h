#ifndef HELPZ_SERVICE_H
#define HELPZ_SERVICE_H

#include <vector>
#include <memory>
#include <cassert>

#include <QThread>
#include <QMetaMethod>
#include <QLoggingCategory>

#include <Helpz/simplethread.h>
#include <Helpz/logging.h>
#include <Helpz/qtservice.h>

namespace Helpz {
namespace Service {

class Base;
class Object final : public QObject
{
    Q_OBJECT
public:
    Object(Base* base, bool debug);
    ~Object();

    void processCommands(const QStringList &cmdList);

private slots:
    void restart();
private:
    void create_worker();

    typedef ParamThread<Logging> Log_Thread;
    Log_Thread* log_thread_;
    QObject* worker_;
    Base* base_;
};

class Base
{
public:
    static const QLoggingCategory &Log();

    Base();

    Base(int argc, char **argv, const QString &name = QString());

    virtual QObject* create_worker(bool* has_log_message_slot, bool* has_restart_service_slot) = 0;
protected:
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
    QObject* create_worker(bool* has_log_message_slot, bool* has_restart_service_slot) override
    {
        QByteArray method_name;
        for (int n = 0; n < T::staticMetaObject.methodCount(); n++)
        {
            method_name = T::staticMetaObject.method(n).name();
            if (method_name == "logMessage")
            {
                *has_log_message_slot = true;
            }
            else if (method_name == "serviceRestart")
            {
                *has_restart_service_slot = true;
            }

            if (*has_log_message_slot && *has_restart_service_slot)
            {
                break;
            }
        }

        return new T;
    }
};

} // namespace Service
} // namespace Helpz

#endif // HELPZ_SERVICE_H
