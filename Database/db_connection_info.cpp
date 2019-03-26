#include "db_connection_info.h"

namespace Helpz {
namespace Database {

Connection_Info::Connection_Info(const QString &db_name, const QString &login, const QString &password, const QString &host, int port, const QString &driver_name, const QString &connect_options) :
    port_(port), driver_name_(driver_name), connect_options_(connect_options), host_(host), db_name_(db_name), login_(login), password_(password) {}

Connection_Info::Connection_Info(const QSqlDatabase &db) :
    port_(db.port()), driver_name_(db.driverName()), connect_options_(db.connectOptions()),
    host_(db.hostName()), db_name_(db.databaseName()), login_(db.userName()), password_(db.password()) {}

Connection_Info::Connection_Info() : port_(-1) {}

QSqlDatabase Connection_Info::to_db(const QString& connection_name) const
{
    QSqlDatabase db = QSqlDatabase::addDatabase( driver_name_, connection_name );
    if (!connect_options_.isEmpty())
    {
        db.setConnectOptions( connect_options_ );
    }
    if (port_ > 0)
    {
        db.setPort( port_ );
    }

    db.setHostName( host_ );
    db.setUserName( login_ );
    db.setPassword( password_ );
    db.setDatabaseName( db_name_ );
    return db;
}

int Connection_Info::port() const { return port_; }
void Connection_Info::set_port(int port) { port_ = port; }

QString Connection_Info::driver_name() const { return driver_name_; }
void Connection_Info::set_driver_name(const QString& driver_name) { driver_name_ = driver_name; }

QString Connection_Info::connect_options() const { return connect_options_; }
void Connection_Info::set_connect_options(const QString& connect_options) { connect_options_ = connect_options; }

QString Connection_Info::host() const { return host_; }
void Connection_Info::set_host(const QString& host) { host_ = host; }

QString Connection_Info::db_name() const { return db_name_; }
void Connection_Info::set_db_name(const QString& db_name) { db_name_ = db_name; }

QString Connection_Info::login() const { return login_; }
void Connection_Info::set_login(const QString& login) { login_ = login; }

QString Connection_Info::password() const { return password_; }
void Connection_Info::set_password(const QString& password) { password_ = password; }

} // namespace Database
} // namespace Helpz
