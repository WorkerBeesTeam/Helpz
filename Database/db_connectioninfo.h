#ifndef HELPZ_DATABASE_CONNECTIONINFO_H
#define HELPZ_DATABASE_CONNECTIONINFO_H

#include <QSqlDatabase>

namespace Helpz {
namespace Database {

struct ConnectionInfo {
    ConnectionInfo(const QString &dbName, const QString &login, const QString &pwd,
                   const QString &host = "localhost", int port = -1, const QString &driver = "QMYSQL",
                   const QString& connectOptions = QString());
    ConnectionInfo(const QSqlDatabase &db);

    int port;
    QString driver;
    QString connectOptions;
    QString host;
    QString dbName;
    QString login;
    QString pwd;
};

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_CONNECTIONINFO_H
