#ifndef DATABASE_BASE_H
#define DATABASE_BASE_H

#include <QMutex>
#include <QTimer>
#include <QByteArray>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QLoggingCategory>

#include <memory>

namespace Helpz {

Q_DECLARE_LOGGING_CATEGORY(DBLog)

namespace Database {

struct Table {
    QString name;
    QStringList fieldNames;

    bool operator !() const;
};

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

class Base
{
public:
    static QString odbcDriver();

    Base() = default;
    Base(const ConnectionInfo &info, const QString& name = QSqlDatabase::defaultConnection);
    Base(QSqlDatabase &db);
    ~Base();

    void clone(Base* other, const QString& name = QSqlDatabase::defaultConnection);
    Base* clone(const QString& name = QSqlDatabase::defaultConnection);

    template<typename T>
    T* clone(const QString& name = QSqlDatabase::defaultConnection)
    {
        T* object = new T{};
        this->clone(object, name);
        return object;
    }

    QString connectionName() const;
    void setConnectionName(const QString& name);
    QSqlDatabase dbFromInfo(const ConnectionInfo &info);

    bool createConnection();
    bool createConnection(const ConnectionInfo &info);
    bool createConnection(QSqlDatabase db);

    void close(bool store_last = true);
    bool isOpen() const;

    QSqlDatabase db() const;

    struct SilentExec {
        SilentExec(Base* db) : db(db) { db->setSilent(true); }
        ~SilentExec() { db->setSilent(false); }
        Base* db;
    };

    bool isSilent() const;
    void setSilent(bool sailent);

    void addTable(uint idx, const Table& table);
    const Table& getTable(uint idx) const;
    const std::map<uint, Table>& getTables() const;

    bool createTable(uint idx, const QStringList& types);
    bool createTable(const Table& table, const QStringList& types);
    QSqlQuery select(uint idx, const QString& suffix = QString(), const QVariantList &values = QVariantList(), const std::vector<uint> &fieldIds = {});
    QSqlQuery select(const Table &table, const QString& suffix = QString(), const QVariantList &values = QVariantList(), const std::vector<uint>& fieldIds = {});

    bool insert(uint idx, const QVariantList& values, QVariant *id_out = nullptr, const std::vector<uint> &fieldIds = {});
    bool insert(const Table &table, const QVariantList& values, QVariant *id_out = nullptr, const std::vector<uint> &fieldIds = {}, const QString &method = "INSERT");
    bool replace(uint idx, const QVariantList& values, QVariant *id_out = nullptr, const std::vector<uint> &fieldIds = {});
    bool replace(const Table &table, const QVariantList& values, QVariant *id_out = nullptr, const std::vector<uint> &fieldIds = {});

    bool update(uint idx, const QVariantList& values, const QString& where, const std::vector<uint> &fieldIds = {});
    bool update(const Table &table, const QVariantList& values, const QString& where, const std::vector<uint> &fieldIds = {});

    QSqlQuery del(uint idx, const QString& where = QString(), const QVariantList &values = QVariantList());
    QSqlQuery del(const QString& tableName, const QString& where = QString(), const QVariantList &values = QVariantList());

    quint32 row_count(uint idx, const QString& where = QString(), const QVariantList &values = QVariantList());
    quint32 row_count(const QString& tableName, const QString& where = QString(), const QVariantList &values = QVariantList());

    QSqlQuery exec(const QString& sql, const QVariantList &values = QVariantList(), QVariant *id_out = nullptr);
protected:
    void lock(bool locked = true);
private:
    QStringList escapeFields(const Table& table, const std::vector<uint> &fieldIds, QSqlDriver *driver = nullptr);
    QString connName;

    std::map<uint, Table> m_tables;
    static Table m_emptyTable;

    bool m_silent = false;
    QMutex m_mutex;

    std::unique_ptr<ConnectionInfo> m_lastConnection;
    QTimer m_timer;
};

} // namespace Database
} // namespace Helpz

#endif // DATABASE_BASE_H
