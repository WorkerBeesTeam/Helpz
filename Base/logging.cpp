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

LogContext::LogContext(const QMessageLogContext& ctx) :
    category_(ctx.category)
{
}

QString LogContext::category() const
{
    return category_;
}

// --------------------------------------------------------

Logging* Logging::s_obj = nullptr;

Logging::Logging(bool debug
#ifdef Q_OS_UNIX
                 , bool syslog
#endif
                 ) :
    debug_(debug),
#ifdef Q_OS_UNIX
    syslog_(syslog),
#endif
    initialized_(false),
    file_(nullptr),
    ts_(nullptr)
{
    s_obj = this;

    qRegisterMetaType<QtMsgType>("QtMsgType");
    qRegisterMetaType<Helpz::LogContext>("Helpz::LogContext");

    connect(this, &Logging::new_message, this, &Logging::save);
    qInstallMessageHandler(handler);
}

Logging::~Logging()
{
    qInstallMessageHandler(qt_message_output);

    if (initialized_)
    {
#ifdef Q_OS_UNIX
        if (syslog_)
            closelog ();
        else
#endif
        {
            delete ts_;
            file_->close();
            delete file_;
        }
    }
}

bool Logging::debug() const { return debug_; }
void Logging::set_debug(bool val) { debug_ = val; }

#ifdef Q_OS_UNIX
bool Logging::syslog() const { return syslog_; }
void Logging::set_syslog(bool val) { syslog_ = val; }
#endif

/*static*/ Logging *Logging::instance() { return s_obj; }

QDebug Logging::operator <<(const QString &str)
{
    return qDebug() << str;
//    qt_message_output(QtDebugMsg, ctx, str);
//    QMetaObject::invokeMethod(this, "save", Qt::QueuedConnection,
//                              Q_ARG(QString, str));
    //    return *this;
}

QString Logging::get_prefix(QtMsgType type, const QString& category, const QString& date_format)
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

    return QDateTime::currentDateTime().toString(date_format) + '[' + category + "][" + type_char + "] ";
}

/*static*/ void Logging::handler(QtMsgType type, const QMessageLogContext &ctx, const QString &str)
{
    if (!s_obj->debug() && type == QtDebugMsg) return;

#ifdef Q_OS_UNIX
    if (!s_obj->syslog() || s_obj->debug())
#endif
    qt_message_output(type, ctx, get_prefix(type, ctx.category) + str);

    emit s_obj->new_message(type, {ctx}, str);
}

void Logging::init()
{
    if (initialized_)
    {
        return;
    }
#ifdef Q_OS_UNIX
    if (syslog_)
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
        file_ = new QFile(qApp->applicationDirPath() + "/" + qAppName() + ".log");
        if (file_->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
            ts_ = new QTextStream(file_);
    }
    initialized_ = true;
}

void Logging::save(QtMsgType type, const LogContext &ctx, const QString &str)
{
    QMutexLocker lock(&mutex_);

#ifdef Q_OS_UNIX
    if (syslog_)
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
        ::syslog(level, "[%s] %s", ctx.category().toLocal8Bit().constData(), qPrintable(str));
    }
    else
#endif
        if (ts_)
    {
        *ts_ << (get_prefix(type, ctx.category(), "[hh:mm:ss dd.MM]") + str) << endl;
        ts_->flush();
    }
}

} // namespace Helpz

Helpz::Logging &logg()
{
    return *Helpz::Logging::instance();
}
