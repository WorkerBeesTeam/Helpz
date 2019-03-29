#ifndef HELPZ_DATABASE_TABLE_H
#define HELPZ_DATABASE_TABLE_H

#include <QStringList>

namespace Helpz {
namespace Database {

class Table
{
public:
    bool operator !() const;

    QString name_;
    QStringList field_names_;
};

template<typename T>
QString db_table_name(const QString& db_name = QString())
{
    return db_name.isEmpty() ? T::table_name() : db_name + '.' + T::table_name();
}

template<typename T>
Helpz::Database::Table db_table(const QString& db_name = QString())
{
    return { db_table_name<T>(db_name), T::table_column_names() };
}

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_TABLE_H
