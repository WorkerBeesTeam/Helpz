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
    if (q.isActive())
    {
        while (q.next())
        {
            result_vector.push_back(db_build<T>(q));
        }
    }
    return result_vector;
}

} // namespace Database
} // namespace Helpz

#endif // HELPZ_DATABASE_BUILDER_H
