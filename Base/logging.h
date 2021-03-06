#ifndef HELPZ_LOGGING_H
#define HELPZ_LOGGING_H

#include <memory>

#include <QObject>
#include <QDebug>
#include <QMutex>

class QFile;
class QTextStream;

namespace Helpz {

class LogContext
{
public:
    LogContext() = default;
    LogContext(LogContext&&) = default;
    LogContext(const LogContext&) = default;
    LogContext& operator=(LogContext&&) = default;
    LogContext& operator=(const LogContext&) = default;
    LogContext(const QMessageLogContext& ctx);

    QString category() const;

private:
    QString category_;
};

/**
 * @brief Класс для логирования сообщений.
 */
class Logging : public QObject
{
    Q_OBJECT
public:
    Logging(bool debug =
#ifdef QT_DEBUG
            true
#else
            false
#endif
#ifdef Q_OS_UNIX
            , bool syslog = false
#endif
            );
    ~Logging();

    bool debug() const;
    void set_debug(bool val);

#ifdef Q_OS_UNIX
    bool syslog() const;
    void set_syslog(bool val);
#endif

    static Logging* instance();

    QDebug operator <<(const QString &str);

    static QString get_prefix(QtMsgType type, const QString& category, const QString &date_format = "[hh:mm:ss]");
signals:
    void new_message(QtMsgType type, const Helpz::LogContext &ctx, const QString &str);
public slots:
    void init();
private slots:
    void save(QtMsgType type, const Helpz::LogContext &ctx, const QString &str);
private:
    static void handler(QtMsgType type, const QMessageLogContext &ctx, const QString &str);
    static Logging* s_obj;

    bool debug_;
#ifdef Q_OS_UNIX
    bool syslog_;
#endif

    bool initialized_;
    QFile* file_;
    QTextStream* ts_;

    QMutex mutex_;

    friend Helpz::Logging &logg();
};

} // namespace Helpz

Helpz::Logging &logg();

Q_DECLARE_METATYPE(Helpz::LogContext)

#endif // HELPZ_LOGGING_H
