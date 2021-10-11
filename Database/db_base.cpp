#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QThread>
#include <QVariant>
#include <QDebug>
#include <QMutexLocker>
#include <QRandomGenerator>

#include <iostream>
#include <functional>

#include "db_base.h"

namespace Helpz {

Q_LOGGING_CATEGORY(DBLog, "database", QtInfoMsg)

namespace DB {

/*static*/ QString Base::odbc_driver()
{
#ifdef Q_OS_LINUX
        return "ODBC Driver 13 for SQL Server";
#else
    return "SQL Server";
    return "SQL Server Native Client 11.0";
#endif
}

thread_local Base base_instance;
/*static*/ Base& Base::get_thread_local_instance() 
{
    if (!base_instance.is_open() && !base_instance.create_connection())
        qCCritical(DBLog) << "Get database instance for not open driver" << base_instance.connection_name_;

    return base_instance;
}

/*static*/ QString Base::get_q_array(int fields_count, int row_count)
{
    int curr_row = 0;
    QString q_str((fields_count * 2 + 2) * row_count - 1, '?');
    ushort* data = reinterpret_cast<ushort*>(q_str.data());
    ushort* data_end = reinterpret_cast<ushort*>(q_str.data()) + q_str.size();
    while (data < data_end)
    {
        if (curr_row >= fields_count)
        {
            curr_row = 0;

            *data++ = ')';
            if (data == data_end)
                break;
            *data++ = ',';
        }

        *data = curr_row == 0 ? '(' : ',';
        data += 2;
        ++curr_row;
    }

    return q_str;
}

QString gen_thread_based_name()
{
    return QString("hz_db_%1_%2").arg(reinterpret_cast<qintptr>(QThread::currentThreadId())).arg(QRandomGenerator::system()->generate());
}

Base::Base(const Connection_Info& info, const QString &name) :
    connection_name_(name), info_(info)
{
    if (connection_name_.isEmpty())
    {
        connection_name_ = gen_thread_based_name();
    }
}

Base::Base(QSqlDatabase& db, const QString &prefix) :
    Base{Connection_Info(db, prefix), db.connectionName()}
{
}

Base::~Base()
{
    if (connection_name_.isEmpty())
        return;

    if (QSqlDatabase::contains(connection_name_))
        QSqlDatabase::removeDatabase(connection_name_);
    if (QSqlDatabase::contains(connection_name_))
        close();
}

QString Base::connection_name() const { return connection_name_; }
void Base::set_connection_name(const QString &name, const QString &prefix)
{
    close();

    connection_name_ = name;
    info_ = Connection_Info{database(), prefix};

    if (connection_name_.isEmpty())
    {
        connection_name_ = gen_thread_based_name();
    }
}

Connection_Info Base::connection_info() const { return info_; }
void Base::set_connection_info(const Connection_Info& info)
{
    close();
    info_ = info;
}

bool Base::create_connection()
{
    QSqlDatabase db_obj = database();
    if (db_obj.isValid())
        return create_connection(db_obj);

    return create_connection(info_.to_db(connection_name_));
}

bool Base::create_connection(QSqlDatabase db)
{
    if (db.isOpen())
        db.close();

    if (!db.open())
    {
        QSqlError err = db.lastError();
        if (err.type() != QSqlError::NoError)
        {
            _last_error = err.text();
            qCCritical(DBLog) << _last_error << QSqlDatabase::drivers() << QThread::currentThread() << db.driver()->thread();
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
    return true;
}

void Base::close()
{
    if (!connection_name_.isEmpty())
    {    
        if (database().isValid())
        {
            QSqlDatabase db_obj = database();
            if (db_obj.isOpen())
            {
                db_obj.close();
            }
        }

        QSqlDatabase::removeDatabase(connection_name_);
    }
}

bool Base::is_open() const { return database().isOpen(); }

QSqlDatabase Base::database() const
{
    return connection_name_.isEmpty() ?
                QSqlDatabase() : QSqlDatabase::database(connection_name_, false);
}

bool Base::is_silent() const { return silent_; }
void Base::set_silent(bool sailent) { silent_ = sailent; }

const QString &Base::last_error() const { return _last_error; }

bool Base::create_table(const Helpz::DB::Table &table, const QStringList &types)
{
    if (!table || table.field_names().size() != types.size())
    {
        return false;
    }

    QStringList columns_info;
    for (int i = 0; i < types.size(); ++i)
    {
        columns_info.push_back(table.field_names().at(i) + ' ' + types.at(i));
    }

    return exec(QString("create table if not exists %1 (%2)").arg(table.name()).arg(columns_info.join(','))).isActive();
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

    QString table_name = table.name();
    if (!table.short_name().isEmpty())
    {
        table_name += ' ' + table.short_name();
    }

    return QString("SELECT %1 FROM %2 %3;").arg(escape_fields(table, field_ids, true).join(',')).arg(table_name).arg(suffix);
}

bool Base::insert(const Table &table, const QVariantList &values, QVariant *id_out, const QString& suffix, const std::vector<uint> &field_ids, const QString& method)
{
    QString sql = insert_query(table, values.size(), suffix, field_ids, method);
    return !sql.isEmpty() && exec(sql, values, id_out).isActive();
}

QString Base::insert_query(const Table &table, int values_size, const QString& suffix, const std::vector<uint> &field_ids, const QString& method) const
{
    auto escapedFields = escape_fields(table, field_ids);
    if (!table || escapedFields.isEmpty() || (suffix.count('?') + escapedFields.size()) != values_size)
    {
        return {};
    }

    int q_size = escapedFields.size() + (escapedFields.size() - 1);
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

    QString sql = QString(method + " INTO %1(%2) VALUES(%3)").arg(table.name()).arg(escapedFields.join(',')).arg(q_str);
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

QSqlQuery Base::update(const Table &table, const QVariantList &values, const QString &where, const std::vector<uint> &field_ids)
{
    QString sql = update_query(table, values.size(), where, field_ids);
    return exec(sql, values);
}

QString Base::update_query(const Table &table, int values_size, const QString &where, const std::vector<uint> &field_ids) const
{
    auto escapedFields = escape_fields(table, field_ids);
    if (!table || escapedFields.isEmpty()
        || (where.count('?') + escapedFields.size()) != values_size)
    {
        return {};
    }

    QStringList params;
    for (int i = 0; i < escapedFields.size(); ++i)
    {
        params.push_back(escapedFields.at(i) + "=?");
    }

    QString sql = QString("UPDATE %1 SET %2").arg(table.name()).arg(params.join(','));
    if (!where.isEmpty())
        sql += " WHERE " + where;
    return sql;
}

QSqlQuery Base::del(const QString &table_name, const QString &where, const QVariantList &values)
{
    QString sql = del_query(table_name, where);
    if (!sql.isEmpty())
        return exec(sql, values);

    return QSqlQuery{};
}

QString Base::del_query(const QString &table_name, const QString &where) const
{
    if (table_name.isEmpty())
        return {};

    QString sql = "DELETE FROM " + table_name;
    if (!where.isEmpty())
        sql += " WHERE " + where;
    return sql;
}

QSqlQuery Base::truncate(const QString &table_name)
{
    return exec(truncate_query(table_name));
}

QString Base::truncate_query(const QString &table_name) const
{
    return "TRUNCATE TABLE " + table_name + ';';
}

uint32_t Base::row_count(const QString &table_name, const QString &where, const QVariantList &values)
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
    if (sql.isEmpty())
        return QSqlQuery();

    ushort attempts_count = 3;
    do
    {
        _last_error.clear();

        if (!is_open() && !create_connection())
            continue;

        {
            bool res;
            QSqlQuery query(database());

            if (values.empty())
                res = query.exec(sql);
            else
            {
                query.prepare(sql);

                for (const QVariant& val: values)
                    query.addBindValue(val);
                res = query.exec();
            }

            if (res)
            {
                if (id_out)
                    *id_out = query.lastInsertId();

                return query;
            }

            QString error_str;
            {
                QSqlError sql_error = query.lastError();
                _last_error = sql_error.text();

                QTextStream ts(&error_str, QIODevice::WriteOnly);
                ts << "DriverMsg: " << sql_error.type() << ' ' << _last_error
                   << "\nSQL: " << sql
                   << "\nAttempt: " << attempts_count
                   << "\nData:";

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
                std::cerr << error_str.toStdString() << std::endl;
            else if (attempts_count == 1) // if no more attempts
            {
                qCCritical(DBLog).noquote() << error_str;
//                    return query; // return with sql error for user
            }
            else
                qCDebug(DBLog).noquote() << error_str;
        }

//        if (lastError.type() == QSqlError::ConnectionError || attempts_count == 2)
        {
            close();
        }
//        else
//            break;
    }
    while (--attempts_count);

    return QSqlQuery();
}

QStringList escape_fields_impl(QSqlDriver *driver, const QString& table_short_name, const QStringList& fields)
{
    QStringList escaped_fields;
    for (const QString& field_name: fields)
        escaped_fields.push_back( table_short_name.isEmpty() ?
            driver->escapeIdentifier(field_name, QSqlDriver::FieldName) :
            driver->escapeIdentifier(table_short_name + field_name, QSqlDriver::FieldName)
        );
    return escaped_fields;
}

QStringList Base::escape_fields(const Table &table, const std::vector<uint> &field_ids, bool use_short_name, QSqlDriver* driver) const
{
    if (!driver)
        driver = database().driver();

    QString table_short_name;
    if (use_short_name && !table.short_name().isEmpty())
        table_short_name = table.short_name() + '.';

    if (field_ids.empty())
        return escape_fields_impl(driver, table_short_name, table.field_names());

    QStringList fields;
    for (uint i: field_ids)
        if (i < table.field_names().size())
            fields.push_back(table.field_names().at(i));
    return escape_fields_impl(driver, table_short_name, fields);
}

} // namespace DB
} // namespace Helpz
