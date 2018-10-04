#include "db_connectioninfo.h"

namespace Helpz {
namespace Database {

ConnectionInfo::ConnectionInfo(const QString &dbName, const QString &login, const QString &pwd, const QString &host, int port, const QString &driver, const QString &connectOptions) :
    port(port), driver(driver), connectOptions(connectOptions), host(host), dbName(dbName), login(login), pwd(pwd) {}

ConnectionInfo::ConnectionInfo(const QSqlDatabase &db) :
    port(db.port()), driver(db.driverName()), connectOptions(db.connectOptions()),
    host(db.hostName()), dbName(db.databaseName()), login(db.userName()), pwd(db.password()) {}

ConnectionInfo::ConnectionInfo() : port(-1) {}

} // namespace Database
} // namespace Helpz
