#ifndef HELPZ_DATABASE_CONNECTION_INFO_H
#define HELPZ_DATABASE_CONNECTION_INFO_H

#include <QStringList>
#include <QSqlDatabase>

namespace Helpz {
namespace Database {

struct Table {
    QString name_;
    QStringList field_names_;

    bool operator !() const;
};

class Connection_Info {
public:
    Connection_Info(const QString &db_name, const QString &login, const QString &password,
                   const QString &host = "localhost", int port = -1,
                   const QString &driver_name = "QMYSQL", const QString& connect_options = QString());
    Connection_Info(const QSqlDatabase &db);
    Connection_Info(const Connection_Info&) = default;
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

private:
    int port_; // Not uint16 becose -1 default
    QString driver_name_, connect_options_, host_, db_name_, login_, password_;
};

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_CONNECTION_INFO_H
