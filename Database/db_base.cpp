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

bool Table::operator !() const {
    return name.isEmpty() || fieldNames.empty();
}

ConnectionInfo::ConnectionInfo(const QString &dbName, const QString &login, const QString &pwd, const QString &host, int port, const QString &driver, const QString &connectOptions) :
    port(port), driver(driver), connectOptions(connectOptions), host(host), dbName(dbName), login(login), pwd(pwd) {}

ConnectionInfo::ConnectionInfo(const QSqlDatabase &db) :
    port(db.port()), driver(db.driverName()), connectOptions(db.connectOptions()),
    host(db.hostName()), dbName(db.databaseName()), login(db.userName()), pwd(db.password()) {}

// ------------------------------------------------------------------------------------------------

Table Base::m_emptyTable;

/*static*/ QString Base::odbcDriver()
{
#ifdef Q_OS_LINUX
        return "ODBC Driver 13 for SQL Server";
#else
    return "SQL Server";
    return "SQL Server Native Client 11.0";
#endif
}

Base::Base(const ConnectionInfo &info, const QString &name)
{
    connName = name;
    createConnection(info);
}

Base::Base(QSqlDatabase& db)
{
    connName = db.connectionName();
    createConnection(db);
}

Base::~Base()
{
    if (connName.isEmpty())
        return;

    if (QSqlDatabase::contains(connName))
        QSqlDatabase::removeDatabase(connName);
    if (QSqlDatabase::contains(connName))
        close(false);
}

void Base::clone(Base *other, const QString &name)
{
    if (!other)
        return;
    other->setConnectionName(name);
    other->m_lastConnection.reset(new ConnectionInfo{ db() });
}

Base *Base::clone(const QString &name) { return clone<Base>(name); }

QString Base::connectionName() const { return connName; }
void Base::setConnectionName(const QString &name)
{
    close(false);
    connName = name;
}

QSqlDatabase Base::dbFromInfo(const ConnectionInfo &info)
{
    if (connName.isEmpty())
        connName = QSqlDatabase::defaultConnection;

    QSqlDatabase db = QSqlDatabase::addDatabase( info.driver, connName );
    qCCritical(DBLog) << "DB INFO: " << connName << (qintptr)QThread::currentThread() << (qintptr)db.driver()->thread();
    if (!info.connectOptions.isEmpty())
        db.setConnectOptions( info.connectOptions );
    db.setHostName( info.host );
    if (info.port > 0)
        db.setPort( info.port );

    db.setUserName( info.login );
    db.setPassword( info.pwd );
    db.setDatabaseName( info.dbName );
    return db;
}

bool Base::createConnection()
{
    QSqlDatabase db_obj = db();
    if (db_obj.isValid())
        return createConnection(db_obj);

    if (m_lastConnection)
        return createConnection(*m_lastConnection);
    return false;
}

bool Base::createConnection(const ConnectionInfo &info) { return createConnection(dbFromInfo(info)); }

bool Base::createConnection(QSqlDatabase db)
{
    if (db.isOpen())
        db.close();

    if (QThread::currentThread() != db.driver()->thread())
        qCCritical(DBLog) << "OPEN DB: " << connName << (qintptr)QThread::currentThread() << (qintptr)db.driver()->thread();

    if (!db.open()) {
        QSqlError err = db.lastError();
        if (err.type() != QSqlError::NoError)
            qCCritical(DBLog) << err.text() << QSqlDatabase::drivers() << QThread::currentThread() << db.driver()->thread();
        return false;
    }

    if (connName != db.connectionName())
    {
        if (!connName.isEmpty())
            qCWarning(DBLog) << "Replace database. Old" << connName << "New:" << db.connectionName();
        connName = db.connectionName();
    }

    qCDebug(DBLog).noquote() << "Database opened:" << db.databaseName()
                   << QString("%1%2")
                      .arg(db.hostName().isEmpty() ? QString() : db.hostName() + (db.port() == -1 ? QString() : ' ' + QString::number(db.port())))
                      .arg(connName == QSqlDatabase::defaultConnection ? QString() : ' ' + connName);

    if (m_lastConnection)
        m_lastConnection.reset();
    return true;
}

void Base::close(bool store_last)
{
    if (connName.isEmpty())
        return;
    {
        QSqlDatabase db_obj = db();
        if (db_obj.isValid())
        {
            if (db_obj.isOpen())
                db_obj.close();

            if (store_last)
                m_lastConnection.reset(new ConnectionInfo{ db_obj });
        }
    }

    QSqlDatabase::removeDatabase(connName);
}

bool Base::isOpen() const { return db().isOpen(); }

QSqlDatabase Base::db() const {
    return connName.isEmpty() ? QSqlDatabase() : QSqlDatabase::database(connName, false);
}

bool Base::isSilent() const { return m_silent; }
void Base::setSilent(bool sailent) { m_silent = sailent; }

void Base::addTable(uint idx, const Table &table)
{
    m_tables[idx] = table;
}

const Table& Base::getTable(uint idx) const
{
    auto it = m_tables.find(idx);
    if (it != m_tables.cend())
        return it->second;
    return m_emptyTable;
}

const std::map<uint, Table> &Base::getTables() const { return m_tables; }

bool Base::createTable(uint idx, const QStringList &types)
{
    return Base::createTable(getTable(idx), types);
}

bool Base::createTable(const Helpz::Database::Table &table, const QStringList &types)
{
    if (!table || table.fieldNames.size() != types.size())
        return false;

    QStringList columns_info;
    for (int i = 0; i < types.size(); ++i)
        columns_info.push_back(table.fieldNames.at(i) + ' ' + types.at(i));

    return exec(QString("create table if not exists %1 (%2)").arg(table.name).arg(columns_info.join(','))).isActive();
}

QSqlQuery Base::select(uint idx, const QString& suffix, const QVariantList &values, const std::vector<uint> &fieldIds)
{
    return Base::select(getTable(idx), suffix, values, fieldIds);
}

QSqlQuery Base::select(const Table& table, const QString &suffix, const QVariantList &values, const std::vector<uint> &fieldIds)
{
    if (!table)
        return QSqlQuery();

    return exec(QString("SELECT %1 FROM %2 %3;")
                .arg(escapeFields(table, fieldIds).join(',')).arg(table.name).arg(suffix), values);
}

bool Base::insert(uint idx, const QVariantList &values, QVariant *id_out, const std::vector<uint> &fieldIds)
{
    return Base::insert(getTable(idx), values, id_out, fieldIds);
}

bool Base::insert(const Table &table, const QVariantList &values, QVariant *id_out, const std::vector<uint> &fieldIds, const QString& method)
{
    auto escapedFields = escapeFields(table, fieldIds);
    if (!table || escapedFields.isEmpty() || escapedFields.size() != values.size())
        return false;

    QStringList q_list;
    for(int i = 0; i < values.size(); ++i)
        q_list.push_back(QChar('?'));

    QString sql = QString(method + " INTO %1(%2) VALUES(%3);").arg(table.name).arg(escapedFields.join(',')).arg(q_list.join(','));
    return exec(sql, values, id_out).isActive();
}

bool Base::replace(uint idx, const QVariantList &values, QVariant *id_out, const std::vector<uint> &fieldIds)
{
    return replace(getTable(idx), values, id_out, fieldIds);
}

bool Base::replace(const Table &table, const QVariantList &values, QVariant *id_out, const std::vector<uint> &fieldIds)
{
    return insert(table, values, id_out, fieldIds, "REPLACE");
}

bool Base::update(uint idx, const QVariantList &values, const QString &where, const std::vector<uint> &fieldIds)
{
    return update(getTable(idx), values, where, fieldIds);
}

bool Base::update(const Table &table, const QVariantList &values, const QString &where, const std::vector<uint> &fieldIds)
{
    auto escapedFields = escapeFields(table, fieldIds);
    if (!table || escapedFields.isEmpty() ||
            (where.count('?') + escapedFields.size()) != values.size())
        return false;

    QStringList params;
    for (int i = 0; i < escapedFields.size(); ++i)
        params.push_back(escapedFields.at(i) + "=?");

    auto sql = QString("UPDATE %1 SET %2").arg(table.name).arg(params.join(','));
    if (!where.isEmpty())
        sql += " WHERE " + where;
    return exec(sql, values).isActive();
}

QSqlQuery Base::del(uint idx, const QString &where, const QVariantList &values)
{
    return Base::del(getTable(idx).name, where, values);
}

QSqlQuery Base::del(const QString &tableName, const QString &where, const QVariantList &values)
{
    if (tableName.isEmpty())
        return QSqlQuery();

    QString sql = "DELETE FROM " + tableName;
    if (!where.isEmpty())
        sql += " WHERE " + where;

    return exec(sql, values);
}

quint32 Base::row_count(uint idx, const QString &where, const QVariantList &values)
{
    return Base::row_count(getTable(idx).name, where, values);
}

quint32 Base::row_count(const QString &tableName, const QString &where, const QVariantList &values)
{
    if (tableName.isEmpty())
        return 0;

    QString sql = "SELECT COUNT(*) FROM " + tableName;
    if (!where.isEmpty())
        sql += " WHERE " + where;

    QSqlQuery query = exec(sql, values);
    return query.next() ? query.value(0).toUInt() : 0;
}

QSqlQuery Base::exec(const QString &sql, const QVariantList &values, QVariant *id_out)
{
    QMutexLocker lock(&m_mutex);

    if (!m_timer.isActive())
    {
        m_timer.setTimerType(Qt::VeryCoarseTimer);
        m_timer.setSingleShot(true);
        m_timer.setInterval(15 * 60000);
        m_timer.connect(&m_timer, &QTimer::timeout, std::bind(&Base::close, this, true));
    }
    if (m_timer.thread() == QThread::currentThread())
        m_timer.start();
    else
    {
        qCCritical(DBLog) << "Diffrent thread";
        QMetaObject::invokeMethod(&m_timer, "start", Qt::BlockingQueuedConnection);
    }

    QSqlError lastError;
    ushort attempts_count = 3;
    do
    {
        if (!isOpen() && !createConnection())
            continue;

        {
            QSqlDatabase db_obj = db();
            bool diffrent_thread = db_obj.driver()->thread() != QThread::currentThread();
            if (diffrent_thread)
                qCCritical(DBLog) << "Diffrent thread call. Current:" << QThread::currentThread() << "Driver:" << db_obj.driver()->thread() << sql;

            QSqlQuery query(db_obj);
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

            if (isSilent())
                std::cerr << errString.toStdString() << std::endl;
            else if (attempts_count == 1) // if no more attempts
            {
                qCCritical(DBLog).noquote() << errString;
//                    return query; // return with sql error for user
            }
            else
                qCDebug(DBLog).noquote() << errString;
        }

        if (lastError.type() == QSqlError::ConnectionError || attempts_count == 2)
            close();
//        else
//            break;
    }
    while (--attempts_count);

    return QSqlQuery();
}

void Base::lock(bool locked)
{
    if (locked)
        m_mutex.lock();
    else
        m_mutex.unlock();
}

QStringList Base::escapeFields(const Table &table, const std::vector<uint> &fieldIds, QSqlDriver* driver)
{
    if (!driver)
        driver = db().driver();

    QStringList escapedFields;
    for (uint i = 0; i < (uint)table.fieldNames.size(); ++i)
        if (fieldIds.empty() || std::find(fieldIds.cbegin(), fieldIds.cend(), i) != fieldIds.cend())
            escapedFields.push_back( driver->escapeIdentifier(table.fieldNames.at(i), QSqlDriver::FieldName) );
    return escapedFields;
}

} // namespace Database
} // namespace Helpz
