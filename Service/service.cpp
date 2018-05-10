#include <QDebug>

#include <iostream>
#include <csignal>

#include "qtservice.h"

#include "../Network/simplethread.h"
#include "service.h"

namespace Helpz {
namespace Service {

/*static*/ Q_LOGGING_CATEGORY(Base::Log, "service")

typedef ParamThread<Logging> LogThread;

Base* obj = nullptr;

void term_handler(int)
{
    obj->stop();
    qApp->quit();
    std::cerr << "Termination complete.\n";
    std::exit(0);
}

Object::Object(OneObjectThread *worherThread, bool debug) :
    th{ new LogThread, worherThread }
{
    QObject::connect(worherThread, &OneObjectThread::restart, this, &Object::restart, Qt::QueuedConnection);

    th[0]->start();
    while (Logging::obj == nullptr && !th[0]->wait(5));

    connect(&logg(), &Logging::new_message, worherThread, &OneObjectThread::logMessage);

    logg().debug = debug;
#ifdef Q_OS_UNIX
    logg().syslog = true;
#endif

    th[1]->start();

    qCInfo(Base::Log) << "Server start!";
}

Object::~Object()
{
    qCInfo(Base::Log) << "Server stop...";

    if (th.size())
    {
        if (th.size() >= 2)
            QObject::disconnect(th[1], SIGNAL(finished()), nullptr, nullptr);

        auto it = th.end();

        do {
            it--;
            (*it)->quit();
            if (!(*it)->wait(15000))
                (*it)->terminate();
            delete *it;
        }
        while( it != th.begin() );
        th.clear();
    }
}

void Object::processCommands(const QStringList &cmdList)
{
    auto obj = static_cast<OneObjectThread*>(th.at(1))->obj();
    QMetaObject::invokeMethod(obj, "processCommands", Qt::QueuedConnection,
                Q_ARG(QStringList, cmdList));
}

void Object::restart()
{
    qCInfo(Base::Log) << "Server restart...";
    th[1]->quit();
    if (!th[1]->wait(60000))
        th[1]->terminate();
    th[1]->start();
}

Base::Base(int argc, char **argv) :
    QtService( argc, argv, QCoreApplication::applicationName() )
{
    obj = this;

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
            m_isImmediately = true;
            break;
        }
    }
#endif
}

#ifndef HAS_QT_SERVICE_IMMEDIATELY_CHECK
bool Base::isImmediately() const { return m_isImmediately; }
#endif

void Base::start()
{
    service = std::make_shared<Object>(getWorkerThread(), isImmediately());

    std::signal(SIGTERM, term_handler);
    std::signal(SIGINT, term_handler);
}

void Base::stop()
{
    service.reset();
}

void Base::processCommands(const QStringList &cmdList)
{
    service->processCommands(cmdList);
}

} // namespace Service
} // namespace Helpz
