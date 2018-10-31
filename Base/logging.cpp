#include <QCoreApplication>
#include <QMutexLocker>
#include <QTextStream>
#include <QFile>
#include <QDateTime>

#include <iostream>

#ifdef Q_OS_UNIX
#include <sys/syslog.h>
#endif

#include "logging.h"

namespace Helpz {

Logging* Logging::obj = nullptr;

Logging::Logging() :
#ifdef QT_DEBUG
    debug(true),
#else
    debug(false),
#endif
#ifdef Q_OS_UNIX
    syslog(false),
#endif
    initialized(false),
    file(nullptr),
    ts(nullptr)
{
    obj = this;

    qRegisterMetaType<QtMsgType>("QtMsgType");
    qRegisterMetaType<Helpz::LogContext>("Helpz::LogContext");

    connect(this, &Logging::new_message, this, &Logging::save);
    qInstallMessageHandler(handler);
}

Logging::~Logging()
{
    qInstallMessageHandler(qt_message_output);

    if (initialized)
    {
#ifdef Q_OS_UNIX
        if (syslog)
            closelog ();
        else
#endif
        {
            delete ts;
            file->close();
            delete file;
        }
    }
}

QDebug Logging::operator <<(const QString &str)
{
    return qDebug() << str;
//    qt_message_output(QtDebugMsg, ctx, str);
//    QMetaObject::invokeMethod(this, "save", Qt::QueuedConnection,
//                              Q_ARG(QString, str));
    //    return *this;
}

QString Logging::get_prefix(QtMsgType type, const QMessageLogContext *ctx, const QString& date_format)
{
    QChar type_char;
    switch (type) {
    default:
    case QtInfoMsg: type_char = 'I'; break;
    case QtDebugMsg: type_char = 'D'; break;
    case QtWarningMsg: type_char = 'W'; break;
    case QtCriticalMsg: type_char = 'C'; break;
    case QtFatalMsg: type_char = 'F'; break;
    }

    return QDateTime::currentDateTime().toString(date_format) + '[' + ctx->category + "][" + type_char + "] ";
}

/*static*/ void Logging::handler(QtMsgType type, const QMessageLogContext &ctx, const QString &str)
{
    if (!obj->debug && type == QtDebugMsg) return;

#ifdef Q_OS_UNIX
    if (!obj->syslog || obj->debug)
#endif
    qt_message_output(type, ctx, get_prefix(type, &ctx) + str);

    auto logContext = std::make_shared<QMessageLogContext>( ctx.file, ctx.line, ctx.function, ctx.category );
    emit obj->new_message(type, logContext, str);
}

void Logging::init()
{
#ifdef Q_OS_UNIX
    if (syslog)
    {
        //Set our Logging Mask and open the Log
        setlogmask(LOG_UPTO(LOG_NOTICE));
        openlog(qPrintable(QCoreApplication::applicationName()),
                LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);

        //Close Standard File Descriptors
//        close(STDIN_FILENO);
//        close(STDOUT_FILENO);
//        close(STDERR_FILENO);
    }
    else
#endif
    {
        file = new QFile(qApp->applicationDirPath() + "/" + qAppName() + ".log");
        if (file->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
            ts = new QTextStream(file);
    }
    initialized = true;
}

void Logging::save(QtMsgType type, const LogContext &ctx, const QString &str)
{
    QMutexLocker lock(&mutex);

#ifdef Q_OS_UNIX
    if (syslog)
    {
        int level;
        switch (type) {
        default:
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
        case QtInfoMsg: level = LOG_INFO; break;
#endif
        case QtDebugMsg: level = LOG_DEBUG; break;
        case QtWarningMsg: level = LOG_WARNING; break;
        case QtCriticalMsg: level = LOG_ERR; break;
        case QtFatalMsg: level = LOG_CRIT; break;
        }
        ::syslog(level, "[%s] %s", ctx->category, qPrintable(str));
    }
    else
#endif
        if (ts)
    {
        *ts << (get_prefix(type, ctx.get(), "[hh:mm:ss dd.MM]") + str) << endl;
        ts->flush();
    }
}
} // namespace Helpz

Helpz::Logging &logg()
{
    return *Helpz::Logging::obj;
}
