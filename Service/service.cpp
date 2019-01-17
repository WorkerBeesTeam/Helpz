#include <QDebug>

#include <iostream>
#include <csignal>

#include "qtservice.h"

#include <Helpz/simplethread.h>
#include "service.h"

namespace Helpz {
namespace Service {

/*static*/ Q_LOGGING_CATEGORY(Base::Log, "service")

typedef ParamThread<Logging> LogThread;

Base* g_obj = nullptr;

void term_handler(int)
{
    g_obj->stop();
    qApp->quit();
    std::cerr << "Termination complete.\n";
    std::exit(0);
}

Object::Object(OneObjectThread *worherThread, bool debug) :
    th_{ new LogThread, worherThread }
{
    QObject::connect(worherThread, &OneObjectThread::restart, this, &Object::restart, Qt::QueuedConnection);

    th_[0]->start();
    while (Logging::instance() == nullptr && !th_[0]->wait(5));

    connect(&logg(), &Logging::new_message, worherThread, &OneObjectThread::logMessage);

    logg().set_debug(debug);
#ifdef Q_OS_UNIX
    logg().set_syslog(true);
#endif

    th_[1]->start();

    qCInfo(Base::Log) << "Server start!";
}

Object::~Object()
{
    qCInfo(Base::Log) << "Server stop...";
    disconnect(&logg(), &Logging::new_message, nullptr, nullptr);

    if (th_.size())
    {
        if (th_.size() >= 2)
            QObject::disconnect(th_[1], SIGNAL(finished()), nullptr, nullptr);

        auto it = th_.end();

        do {
            it--;
            (*it)->quit();
            if (!(*it)->wait(15000))
                (*it)->terminate();
            delete *it;
        }
        while( it != th_.begin() );
        th_.clear();
    }
}

void Object::processCommands(const QStringList &cmdList)
{
    auto obj = static_cast<OneObjectThread*>(th_.at(1))->obj();
    QMetaObject::invokeMethod(obj, "processCommands", Qt::QueuedConnection,
                Q_ARG(QStringList, cmdList));
}

void Object::restart()
{
    qCInfo(Base::Log) << "Server restart...";
    th_[1]->quit();
    if (!th_[1]->wait(60000))
        th_[1]->terminate();
    th_[1]->start();
}

Base::Base(int argc, char **argv) :
    QtService( argc, argv, QCoreApplication::applicationName() )
{
    g_obj = this;

    assert( !QCoreApplication::applicationName().isEmpty() );

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
bool Base::isImmediately() const { return isImmediately_; }
#endif

void Base::start()
{
    service_ = std::make_shared<Object>(getWorkerThread(), isImmediately());

    std::signal(SIGTERM, term_handler);
    std::signal(SIGINT, term_handler);
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
