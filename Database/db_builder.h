#ifndef HELPZ_DATABASE_BUILDER_H
#define HELPZ_DATABASE_BUILDER_H

#include <memory>
#include <type_traits>

#include <Helpz/db_base.h>

namespace Helpz {
namespace DB {

/* Examples:
 * Device device; db_fill_item(query, device);
 * Device device = db_build<Device>(query);
 * QVector<Device> devices = db_build_list<Device>(db);
 * auto devices = db_build_list<std::share_ptr<Device>>(db);
 *
 * std::set<Device> devices = db_build_list<Device, std::set>(db);
 */

template<typename T>
void db_fill_item(const QSqlQuery& query, T& item)
{
    const int count = std::min<int>(T::COL_COUNT, query.record().count());
    for (int i = 0; i < count; ++i)
        T::value_setter(item, i, query.value(i));
}

template<typename T>
void db_build_impl(const QSqlQuery& query, T& item)
{
    db_fill_item<T>(query, item);
}

template<typename T>
void db_build_impl(const QSqlQuery& query, std::shared_ptr<T>& item)
{
    item = std::make_shared<T>();
    db_fill_item<T>(query, *item);
}

template<typename T>
T db_build(const QSqlQuery& query)
{
    T result;
    db_build_impl(query, result);
    return result;
}

template<typename C, typename = void>
struct has_reserve : std::false_type {};

template<typename C>
struct has_reserve< C, std::enable_if_t<
                         std::is_same<
                           decltype( std::declval<C>().reserve( std::declval<typename C::size_type>() ) ),
                           void
                         >::value
                       > >
  : std::true_type {};

template< typename C >
std::enable_if_t< !has_reserve< C >::value >
optional_reserve( C&, std::size_t ) {}

template< typename C >
std::enable_if_t< has_reserve< C >::value >
optional_reserve( C& c, std::size_t n )
{
    c.reserve( c.size() + n );
}

template<typename T, template<typename...> class Container = QVector>
Container<T> db_build_list(QSqlQuery& q)
{
    Container<T> c;
    while (q.next())
    {
        optional_reserve(c, 1);
        c.insert( c.end(), db_build<T>(q) );
    }
    return c;
}


template<typename T>
struct remove_smart_pointer { using type = T; };

template<typename T>
struct remove_smart_pointer<std::shared_ptr<T>> { using type = typename T::element_type; };

template<typename T, template<typename...> class Container = QVector>
Container<T> db_build_list(Base& db, const QString& suffix = QString(), const QVariantList &values = QVariantList(), const QString& db_name = QString())
{
    using Item_T = typename remove_smart_pointer<T>::type;
    QSqlQuery q = db.select(db_table<Item_T>(db_name), suffix, values);
    return db_build_list<T, Container>(q);
}

template<typename T, template<typename...> class Container>
QString db_get_items_list_suffix(const Container<uint32_t>& id_vect, std::size_t column_index = 0)
{
    if (id_vect.empty() || static_cast<std::size_t>(T::table_column_names().size()) <= column_index)
        return {};

    QString suffix = "WHERE ";
    if (!T::table_short_name().isEmpty())
        suffix += T::table_short_name() + '.';

    suffix += T::table_column_names().at(column_index) + " IN (";
    for (uint32_t id: id_vect)
        suffix += QString::number(id) + ',';
    suffix.replace(suffix.size() - 1, 1, QChar(')'));
    return suffix;
}

template<typename T, template<typename...> class Ret_Container = QVector, template<typename...> class Container>
Ret_Container<T> db_build_list(Base& db, const Container<uint32_t>& id_vect, const QString& db_name = QString(), std::size_t column_index = 0)
{
    using Item_T = typename remove_smart_pointer<T>::type;
    const QString suffix = db_get_items_list_suffix<Item_T>(id_vect, column_index);
    if (suffix.isEmpty())
        return {};
    return db_build_list<T, Ret_Container>(db, suffix, {}, db_name);
}

template<typename T, typename ID_T>
T db_build_item(Base& db, ID_T id, const QString& db_name = QString())
{
    using Item_T = typename remove_smart_pointer<T>::type;

    QString suffix = "WHERE ";
    if (!T::table_short_name().isEmpty())
        suffix += Item_T::table_short_name() + '.';
    suffix += Item_T::table_column_names().first() + '=' + QString::number(id);

    QSqlQuery q = db.select(db_table<Item_T>(db_name), suffix);
    if (q.next())
        return db_build<T>(q);

    return {};
}

template<typename T, template<typename...> class Container, typename... Args>
QString get_db_field_in_sql(const QString& field_name, const Container<T>& id_list, Args... ids)
{
    QString sql = field_name;
    sql += (sizeof...(ids) + id_list.size() == 1) ? "=" : " IN (";

    for (const uint32_t id: id_list)
    {
        sql += QString::number(id);
        sql += ',';
    }

    if constexpr (sizeof...(ids))
        sql += ((QString::number(ids) + ',') + ...);

    if (sizeof...(ids) + id_list.size() == 1)
        sql.remove(sql.size() - 1, 1);
    else
        sql.replace(sql.size() - 1, 1, QChar(')'));

    return sql;
}

} // namespace DB
} // namespace Helpz

#endif // HELPZ_DATABASE_BUILDER_H
