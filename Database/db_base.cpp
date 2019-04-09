#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QThread>
#include <QVariant>
#include <QDebug>
#include <QMutexLocker>

#include <iostream>
#include <functional>

#include "db_base.h"

namespace Helpz {

Q_LOGGING_CATEGORY(DBLog, "database")

namespace Database {

/*static*/ QString Base::odbc_driver()
{
#ifdef Q_OS_LINUX
        return "ODBC Driver 13 for SQL Server";
#else
    return "SQL Server";
    return "SQL Server Native Client 11.0";
#endif
}

Base::Base(const Connection_Info& info, const QString &name)
{
    connection_name_ = name;
    create_connection(info);
}

Base::Base(QSqlDatabase& db)
{
    connection_name_ = db.connectionName();
    create_connection(db);
}

Base::~Base()
{
    if (connection_name_.isEmpty())
        return;

    if (QSqlDatabase::contains(connection_name_))
        QSqlDatabase::removeDatabase(connection_name_);
    if (QSqlDatabase::contains(connection_name_))
        close(false);
}

//void Base::clone(Base *other, const QString &name)
//{
//    if (!other)
//        return;
//    other->setConnectionName(name);
//    other->last_connection_.reset(new ConnectionInfo{ db() });
//}

//Base *Base::clone(const QString &name) { return clone<Base>(name); }

QString Base::connection_name() const { return connection_name_; }
void Base::set_connection_name(const QString &name)
{
    close(false);
    connection_name_ = name;
}

QSqlDatabase Base::db_from_info(const Connection_Info &info)
{
    if (connection_name_.isEmpty())
    {
        connection_name_ = QSqlDatabase::defaultConnection;
    }

    if (QSqlDatabase::contains(connection_name_))
    {
        close();
    }

    return info.to_db(connection_name_);
}

bool Base::create_connection()
{
    QSqlDatabase db_obj = database();
    if (db_obj.isValid())
    {
        return create_connection(db_obj);
    }

    if (last_connection_)
    {
        return create_connection(*last_connection_);
    }
    return false;
}

bool Base::create_connection(const Connection_Info& info) { return create_connection(db_from_info(info)); }

bool Base::create_connection(QSqlDatabase db)
{
    if (db.isOpen())
    {
        db.close();
    }

    if (!db.open())
    {
        QSqlError err = db.lastError();
        if (err.type() != QSqlError::NoError)
        {
            qCCritical(DBLog) << err.text() << QSqlDatabase::drivers() << QThread::currentThread() << db.driver()->thread();
        }
        return false;
    }

    if (connection_name_ != db.connectionName())
    {
        if (!connection_name_.isEmpty())
        {
            qCWarning(DBLog) << "Replace database. Old" << connection_name_ << "New:" << db.connectionName();
        }
        connection_name_ = db.connectionName();
    }

    qCDebug(DBLog).noquote() << "Database opened:" << db.databaseName()
                   << QString("%1%2")
                      .arg(db.hostName().isEmpty() ? QString() : db.hostName() + (db.port() == -1 ? QString() : ' ' + QString::number(db.port())))
                      .arg(connection_name_ == QSqlDatabase::defaultConnection ? QString() : ' ' + connection_name_);

    if (last_connection_)
    {
        last_connection_.reset();
    }
    return true;
}

void Base::close(bool store_last)
{
    if (connection_name_.isEmpty())
    {
        return;
    }

    {
        QSqlDatabase db_obj = database();
        if (db_obj.isValid())
        {
            if (db_obj.isOpen())
            {
                db_obj.close();
            }

            if (store_last)
            {
                last_connection_.reset(new Connection_Info{ db_obj });
            }
        }
    }

    QSqlDatabase::removeDatabase(connection_name_);
}

bool Base::is_open() const { return database().isOpen(); }

QSqlDatabase Base::database() const
{
    return connection_name_.isEmpty() ?
                QSqlDatabase() : QSqlDatabase::database(connection_name_, false);
}

bool Base::is_silent() const { return silent_; }
void Base::set_silent(bool sailent) { silent_ = sailent; }

bool Base::create_table(const Helpz::Database::Table &table, const QStringList &types)
{
    if (!table || table.field_names_.size() != types.size())
    {
        return false;
    }

    QStringList columns_info;
    for (int i = 0; i < types.size(); ++i)
    {
        columns_info.push_back(table.field_names_.at(i) + ' ' + types.at(i));
    }

    return exec(QString("create table if not exists %1 (%2)").arg(table.name_).arg(columns_info.join(','))).isActive();
}

QSqlQuery Base::select(const Table& table, const QString &suffix, const QVariantList &values, const std::vector<uint> &field_ids)
{
    QString sql = select_query(table, suffix, field_ids);
    if (sql.isEmpty())
    {
        return QSqlQuery{};
    }
    return exec(sql, values);
}

QString Base::select_query(const Table& table, const QString &suffix, const std::vector<uint> &field_ids) const
{
    if (!table)
    {
        return {};
    }
    return QString("SELECT %1 FROM %2 %3;").arg(escape_fields(table, field_ids).join(',')).arg(table.name_).arg(suffix);
}

bool Base::insert(const Table &table, const QVariantList &values, QVariant *id_out, const QString& suffix, const std::vector<uint> &field_ids, const QString& method)
{
    QString sql = insert_query(table, values.size(), suffix, field_ids, method);
    return !sql.isEmpty() && exec(sql, values, id_out).isActive();
}

QString Base::insert_query(const Table &table, int values_size, const QString& suffix, const std::vector<uint> &field_ids, const QString& method) const
{
    auto escapedFields = escape_fields(table, field_ids);
    if (!table || escapedFields.isEmpty() || escapedFields.size() != values_size)
    {
        return {};
    }

    int q_size = values_size + (values_size - 1);
    QString q_str(q_size, '?');
    ushort* data = reinterpret_cast<ushort*>(q_str.data()) + 1;
    for (int i = 1; i < q_size; i += 2)
    {
        if (i % 2 != 0)
        {
            *data = ',';
            data += 2;
        }
    }

    QString sql = QString(method + " INTO %1(%2) VALUES(%3)").arg(table.name_).arg(escapedFields.join(',')).arg(q_str);
    if (!suffix.isEmpty())
    {
        sql += ' ' + suffix;
    }
    sql += ';';
    return sql;
}

bool Base::replace(const Table &table, const QVariantList &values, QVariant *id_out, const std::vector<uint> &field_ids)
{
    return insert(table, values, id_out, QString(), field_ids, "REPLACE");
}

bool Base::update(const Table &table, const QVariantList &values, const QString &where, const std::vector<uint> &field_ids)
{
    QString sql = update_query(table, values.size(), where, field_ids);
    return !sql.isEmpty() && exec(sql, values).isActive();
}

QString Base::update_query(const Table &table, int values_size, const QString &where, const std::vector<uint> &field_ids) const
{
    auto escapedFields = escape_fields(table, field_ids);
    if (!table || escapedFields.isEmpty() ||
            (where.count('?') + escapedFields.size()) != values_size)
    {
        return {};
    }

    QStringList params;
    for (int i = 0; i < escapedFields.size(); ++i)
    {
        params.push_back(escapedFields.at(i) + "=?");
    }

    QString sql = QString("UPDATE %1 SET %2").arg(table.name_).arg(params.join(','));
    if (!where.isEmpty())
    {
        sql += " WHERE " + where;
    }
    return sql;
}

QSqlQuery Base::del(const QString &table_name, const QString &where, const QVariantList &values)
{
    QString sql = del_query(table_name, where);
    if (!sql.isEmpty())
    {
        return exec(sql, values);
    }

    return QSqlQuery{};
}

QString Base::del_query(const QString &table_name, const QString &where) const
{
    if (table_name.isEmpty())
    {
        return {};
    }

    QString sql = "DELETE FROM " + table_name;
    if (!where.isEmpty())
    {
        sql += " WHERE " + where;
    }
    return sql;
}

quint32 Base::row_count(const QString &table_name, const QString &where, const QVariantList &values)
{
    if (table_name.isEmpty())
    {
        return 0;
    }

    QString sql = "SELECT COUNT(*) FROM " + table_name;
    if (!where.isEmpty())
    {
        sql += " WHERE " + where;
    }

    QSqlQuery query = exec(sql, values);
    return query.next() ? query.value(0).toUInt() : 0;
}

QSqlQuery Base::exec(const QString &sql, const QVariantList &values, QVariant *id_out)
{
    QSqlError lastError;
    ushort attempts_count = 3;
    do
    {
        if (!is_open() && !create_connection())
            continue;

        {
            QSqlQuery query(database());
            query.prepare(sql);

            for (const QVariant& val: values)
                query.addBindValue(val);

            if (query.exec())
            {
                if (id_out)
                    *id_out = query.lastInsertId();

                return query;
            }

            lastError = query.lastError();

            QString errString;
            {
                QTextStream ts(&errString, QIODevice::WriteOnly);
                ts << "DriverMsg: " << lastError.type() << ' ' << lastError.text() << endl
                   << "SQL: " << sql << endl
                   << "Attempt: " << attempts_count << endl
                   << "Data:";

                for (auto&& val: values)
                {
                    ts << " <";
                    if (val.type() == QVariant::ByteArray)
                        ts << "ByteArray(" << val.toByteArray().size() << ')';
                    else
                        ts << val.toString();
                    ts << '>';
                }
            }

            if (is_silent())
                std::cerr << errString.toStdString() << std::endl;
            else if (attempts_count == 1) // if no more attempts
            {
                qCCritical(DBLog).noquote() << errString;
//                    return query; // return with sql error for user
            }
            else
                qCDebug(DBLog).noquote() << errString;
        }

//        if (lastError.type() == QSqlError::ConnectionError || attempts_count == 2)
        {
            close(true);
        }
//        else
//            break;
    }
    while (--attempts_count);

    return QSqlQuery();
}

QStringList Base::escape_fields(const Table &table, const std::vector<uint> &field_ids, QSqlDriver* driver) const
{
    if (!driver)
    {
        driver = database().driver();
    }

    QStringList escapedFields;
    for (uint i = 0; i < (uint)table.field_names_.size(); ++i)
        if (field_ids.empty() || std::find(field_ids.cbegin(), field_ids.cend(), i) != field_ids.cend())
            escapedFields.push_back( driver->escapeIdentifier(table.field_names_.at(i), QSqlDriver::FieldName) );
    return escapedFields;
}

} // namespace Database
} // namespace Helpz
