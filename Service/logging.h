#ifndef LOG_H
#define LOG_H

#include <memory>

#include <QObject>
#include <QDebug>
#include <QMutex>

class QFile;
class QTextStream;

namespace Helpz {

typedef std::shared_ptr<QMessageLogContext> LogContext;
/**
 * @brief Класс для логирования сообщений.
 */
class Logging : public QObject
{
    Q_OBJECT
public:
    Logging();
    ~Logging();

    bool debug;
#ifdef Q_OS_UNIX
    bool syslog;
#endif

    QDebug operator <<(const QString &str);
    static Logging* obj;

    static QString get_prefix(QtMsgType type, const QMessageLogContext *ctx, const QString &date_format = "[hh:mm:ss]");
signals:
    void new_message(QtMsgType type, const Helpz::LogContext &ctx, const QString &str);
public slots:
    void init();
private slots:
    void save(QtMsgType type, const Helpz::LogContext &ctx, const QString &str);
private:
    static void handler(QtMsgType type, const QMessageLogContext &ctx, const QString &str);

    bool initialized;
    QFile* file;
    QTextStream* ts;

    QMutex mutex;

    friend Logging &logg();
};

} // namespace Helpz

Helpz::Logging &logg();

Q_DECLARE_METATYPE(Helpz::LogContext)

#endif // LOG_H
