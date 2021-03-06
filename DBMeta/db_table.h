#ifndef HELPZ_DATABASE_TABLE_H
#define HELPZ_DATABASE_TABLE_H

#include <QStringList>

namespace Helpz {
namespace DB {

class Table
{
public:
    Table(const QString& name, const QString& short_name, const QStringList& field_names);
    Table() = default;
    Table(Table&&) = default;
    Table(const Table&) = default;
    Table& operator=(Table&&) = default;
    Table& operator=(const Table&) = default;

    static QString common_prefix();
    static void set_common_prefix(const QString& prefix);

    bool operator !() const;

    const QString& name() const;
    void set_name(const QString& name);

    const QString& short_name() const;
    void set_short_name(const QString& short_name);

    QStringList& field_names();
    const QStringList& field_names() const;
    void set_field_names(const QStringList& field_names);

private:
    QString name_, short_name_;
    QStringList field_names_;

    static QString common_prefix_;
};

template<typename T>
QString db_table_name(const QString& db_name = QString())
{
    return db_name.isEmpty() ? T::table_name() : db_name + '.' + T::table_name();
}

template<typename T>
Helpz::DB::Table db_table(const QString& db_name = QString())
{
    return { db_table_name<T>(db_name), T::table_short_name(), T::table_column_names() };
}

} // namespace DB
} // namespace Helpz

#endif // HELPZ_DATABASE_TABLE_H
