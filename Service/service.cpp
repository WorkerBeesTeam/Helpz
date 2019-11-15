#include <QDebug>

#include <iostream>
#include <csignal>

#include <execinfo.h>
#include <unistd.h>

#include "qtservice.h"
#include "service.h"

namespace Helpz {
namespace Service {

/*static*/ Q_LOGGING_CATEGORY(Base::Log, "service")

Base* g_obj = nullptr;

void term_handler(int signal)
{
    int exit_code;
    if (signal == SIGSEGV || signal == SIGFPE)
    {
        void *array[10];
        size_t size = backtrace(array, 10);

        // print out all the frames to stderr
        fprintf(stderr, "Error: signal %d:\n", signal);
        backtrace_symbols_fd(array, size, STDERR_FILENO);
        exit_code = 1;
    }
    else
        exit_code = 0;

    if (g_obj)
        g_obj->stop();
    qApp->exit(exit_code);
    std::cerr << "Termination complete.\n";
    std::exit(exit_code);
}

Object::Object(Base *base, bool debug) :
    log_thread_(new Log_Thread),
    base_(base)
{
    log_thread_->start();
    log_thread_->ptr();

    logg().set_debug(debug);
#ifdef Q_OS_UNIX
    logg().set_syslog(true);
#endif

    create_worker();
    qCInfo(Base::Log) << "Server start!";
}

Object::~Object()
{
    qCInfo(Base::Log) << "Server stop...";
    disconnect(&logg(), &Logging::new_message, nullptr, nullptr);

    delete worker_;

    log_thread_->quit();
    if (!log_thread_->wait(15000))
        log_thread_->terminate();
    delete log_thread_;
}

void Object::processCommands(const QStringList &cmdList)
{
    QMetaObject::invokeMethod(worker_, "processCommands", Qt::QueuedConnection,
                              Q_ARG(QStringList, cmdList));
}

void Object::restart()
{
    qCInfo(Base::Log) << "Server restart...";

    delete worker_;
    create_worker();
}

void Object::create_worker()
{
    bool has_log_message_slot = false, has_restart_service_slot = false;
    worker_ = base_->create_worker(&has_log_message_slot, &has_restart_service_slot);

    if (has_log_message_slot)
    {
        connect(&logg(), SIGNAL(new_message(QtMsgType,Helpz::LogContext,QString)),
                worker_, SLOT(logMessage(QtMsgType,Helpz::LogContext,QString)));
    }

    if (has_restart_service_slot)
    {
        connect(worker_, SIGNAL(serviceRestart()), this, SLOT(restart()), Qt::QueuedConnection);
    }
}

Base::Base()
{
    g_obj = this;
}

void Base::start()
{
    service_ = std::make_shared<Object>(this, is_immediately());

    std::signal(SIGTERM, term_handler);
    std::signal(SIGINT, term_handler);

    if (qEnvironmentVariableIsSet("HELPZ_DISABLE_SIGNAL_CATCH"))
    {
        qCWarning(Base::Log) << "SEGV and FPE signal catch disabled";
    }
    else
    {
        std::signal(SIGSEGV, term_handler);
        std::signal(SIGFPE, term_handler);
    }
}

void Base::stop()
{
    service_.reset();
}

void Base::processCommands(const QStringList &cmdList)
{
    service_->processCommands(cmdList);
}

} // namespace Service
} // namespace Helpz
