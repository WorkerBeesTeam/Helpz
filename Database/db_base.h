#ifndef HELPZ_DATABASE_BASE_H
#define HELPZ_DATABASE_BASE_H

#include <memory>

#include <QByteArray>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlQuery>
#include <QLoggingCategory>

#include <Helpz/db_connection_info.h>
#include <Helpz/db_table.h>

namespace Helpz {

Q_DECLARE_LOGGING_CATEGORY(DBLog)

namespace Database {

class Base
{
public:
    static QString odbc_driver();

    Base() = default;
    Base(const Connection_Info &info, const QString& name = QSqlDatabase::defaultConnection);
    Base(QSqlDatabase &db);
    ~Base();

//    void clone(Base* other, const QString& name = QSqlDatabase::defaultConnection);
//    Base* clone(const QString& name = QSqlDatabase::defaultConnection);

//    template<typename T>
//    T* clone(const QString& name = QSqlDatabase::defaultConnection)
//    {
//        T* object = new T{};
//        this->clone(object, name);
//        return object;
//    }

    QString connection_name() const;
    void set_connection_name(const QString& name);
    QSqlDatabase db_from_info(const Connection_Info &info);

    bool create_connection();
    bool create_connection(const Connection_Info &info);
    bool create_connection(QSqlDatabase db);

    void close(bool store_last = true);
    bool is_open() const;

    QSqlDatabase database() const;

    struct SilentExec
    {
        SilentExec(Base* db) : db(db) { db->set_silent(true); }
        ~SilentExec() { db->set_silent(false); }
        Base* db;
    };

    bool is_silent() const;
    void set_silent(bool sailent);

    bool create_table(const Table& table, const QStringList& types);

    QSqlQuery select(const Table &table, const QString& suffix = QString(), const QVariantList &values = QVariantList(), const std::vector<uint>& field_ids = {});
    QString select_query(const Table& table, const QString &suffix = {}, const std::vector<uint> &field_ids = {}) const;

    bool insert(const Table &table, const QVariantList& values, QVariant *id_out = nullptr, const QString& suffix = QString(), const std::vector<uint> &field_ids = {}, const QString &method = "INSERT");
    QString insert_query(const Table& table, int values_size, const QString& suffix = QString(), const std::vector<uint>& field_ids = {}, const QString& method = "INSERT") const;
    bool replace(const Table &table, const QVariantList& values, QVariant *id_out = nullptr, const std::vector<uint> &field_ids = {});

    bool update(const Table &table, const QVariantList& values, const QString& where, const std::vector<uint> &field_ids = {});
    QString update_query(const Table& table, int values_size, const QString& where, const std::vector<uint>& field_ids = {}) const;

    QSqlQuery del(const QString& table_name, const QString& where = QString(), const QVariantList &values = QVariantList());
    QString del_query(const QString& table_name, const QString& where = QString()) const;

    quint32 row_count(const QString& table_name, const QString& where = QString(), const QVariantList &values = QVariantList());

    QSqlQuery exec(const QString& sql, const QVariantList &values = QVariantList(), QVariant *id_out = nullptr);
private:
    QStringList escape_fields(const Table& table, const std::vector<uint> &field_ids, bool use_short_name = false, QSqlDriver *driver = nullptr) const;

    bool silent_ = false;
    QString connection_name_;

    std::unique_ptr<Connection_Info> last_connection_;
};

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_BASE_H
