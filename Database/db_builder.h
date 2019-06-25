#ifndef HELPZ_DATABASE_BUILDER_H
#define HELPZ_DATABASE_BUILDER_H

#include <Helpz/db_base.h>

namespace Helpz {
namespace Database {

template<typename T>
T db_build(QSqlQuery& query)
{
    T result;
    if (query.record().count() >= T::COL_COUNT)
    {
        for (int i = 0; i < T::COL_COUNT; ++i)
        {
            T::value_setter(result, i, query.value(i));
        }
    }
    return result;
}

template<typename T>
QVector<T> db_build_list(Base& db, const QString& suffix = QString(), const QString& db_name = QString())
{
    QVector<T> result_vector;
    QSqlQuery q;
    q = db.select(db_table<T>(db_name), suffix);
    while (q.next())
    {
        result_vector.push_back(db_build<T>(q));
    }
    return result_vector;
}

template<typename T>
QString get_items_list_suffix(const QVector<uint32_t>& id_vect)
{
    if (id_vect.isEmpty())
    {
        return {};
    }

    QString suffix = "WHERE ";
    if (!T::table_short_name().isEmpty())
    {
        suffix += T::table_short_name() + '.';
    }

    suffix += T::table_column_names().first() + " IN (";
    for (uint32_t id: id_vect)
        suffix += QString::number(id) + ',';
    suffix[suffix.size() - 1] = ')';
    return suffix;
}

template<typename T>
QVector<T> db_build_list(Base& db, const QVector<uint32_t>& id_vect, const QString& db_name = QString())
{
    QString suffix = get_items_list_suffix<T>(id_vect);
    if (suffix.isEmpty())
    {
        return {};
    }
    return db_build_list<T>(db, suffix, db_name);
}

template<typename T>
T db_build_item(Base& db, uint32_t id, const QString& db_name = QString())
{
    QString suffix = "WHERE ";
    if (!T::table_short_name().isEmpty())
    {
        suffix += T::table_short_name() + '.';
    }
    suffix += T::table_column_names().first() + '=' + QString::number(id);

    QSqlQuery q = db.select(db_table<T>(db_name), suffix);
    if (q.next())
    {
        return db_build<T>(q);
    }

    return {};
}

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_BUILDER_H
