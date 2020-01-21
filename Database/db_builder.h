#ifndef HELPZ_DATABASE_BUILDER_H
#define HELPZ_DATABASE_BUILDER_H

#include <Helpz/db_base.h>

namespace Helpz {
namespace Database {

template<typename T>
T db_build(const QSqlQuery& query)
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

template<typename T, template<typename> class Container>
QString get_items_list_suffix(const Container<uint32_t>& id_vect, std::size_t column_index = 0)
{
    if (id_vect.empty() || static_cast<std::size_t>(T::table_column_names().size()) <= column_index)
    {
        return {};
    }

    QString suffix = "WHERE ";
    if (!T::table_short_name().isEmpty())
    {
        suffix += T::table_short_name() + '.';
    }

    suffix += T::table_column_names().at(column_index) + " IN (";
    for (uint32_t id: id_vect)
        suffix += QString::number(id) + ',';
    suffix.replace(suffix.size() - 1, 1, QChar(')'));
    return suffix;
}

template<typename T, template<typename> class Container>
QVector<T> db_build_list(Base& db, const Container<uint32_t>& id_vect, const QString& db_name = QString(), std::size_t column_index = 0)
{
    QString suffix = get_items_list_suffix<T>(id_vect, column_index);
    if (suffix.isEmpty())
    {
        return {};
    }
    return db_build_list<T>(db, suffix, db_name);
}

template<typename T, typename ID_T>
T db_build_item(Base& db, ID_T id, const QString& db_name = QString())
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
