#ifndef HELPZ_DATABASE_CONNECTION_INFO_H
#define HELPZ_DATABASE_CONNECTION_INFO_H

#include <QStringList>
#include <QSqlDatabase>

namespace Helpz {
namespace Database {

class Connection_Info
{
    static Connection_Info common_;
public:

    static const Connection_Info& common();
    static void set_common(const Connection_Info& info);

    Connection_Info(const QString &db_name, const QString &login, const QString &password,
                   const QString &host = "localhost", int port = -1, const QString& prefix = QString(),
                   const QString &driver_name = "QMYSQL", const QString& connect_options = QString());
    Connection_Info(const QSqlDatabase &db, const QString& prefix /*= common().prefix()*/);
    Connection_Info(Connection_Info&&) = default;
    Connection_Info(const Connection_Info&) = default;
    Connection_Info& operator=(Connection_Info&&) = default;
    Connection_Info& operator=(const Connection_Info&) = default;
    Connection_Info();

    QSqlDatabase to_db(const QString& connection_name = QSqlDatabase::defaultConnection) const;

    int port() const;
    void set_port(int port);

    QString driver_name() const;
    void set_driver_name(const QString& driver_name);

    QString connect_options() const;
    void set_connect_options(const QString& connect_options);

    QString host() const;
    void set_host(const QString& host);

    QString db_name() const;
    void set_db_name(const QString& db_name);

    QString login() const;
    void set_login(const QString& login);

    QString password() const;
    void set_password(const QString& password);

    QString prefix() const;
    void set_prefix(const QString& prefix);

private:
    int port_; // Not uint16 becose -1 default
    QString driver_name_, connect_options_, host_, db_name_, login_, password_, prefix_;
};

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_CONNECTION_INFO_H
