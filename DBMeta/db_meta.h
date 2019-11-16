#ifndef HELPZ_DATABASE_META_H
#define HELPZ_DATABASE_META_H

#include <type_traits>

#include <QVariant>

namespace Helpz {
namespace Database {

template<typename T, typename FT, typename AT>
void db_set_value_from_variant(T& obj, void (FT::*setter)(AT), const QVariant& value)
{
    using VALUE_TYPE = typename std::decay<AT>::type;
    (obj.*setter)(value.value<VALUE_TYPE>());
}

template<typename T, typename FT, typename AT>
void db_set_value_from_variant(T& obj, AT FT::*member_value, const QVariant& value)
{
    using VALUE_TYPE = typename std::decay<AT>::type;
    (obj.*member_value) = value.value<VALUE_TYPE>();
}

} // namespace Database
} // namespace Helpz

#define DB_A(A) A, A(), set_##A
#define DB_AM(A) A, A, A
#define DB_AMN(A) A, A ? obj.A : QVariant(), A
#define DB_AN(A) A, A() ? obj.A() : QVariant(), set_##A
#define DB_AT(A) A, A##_to_db(), set_##A##_from_db
#define DB_ANS(A) A, A().isEmpty() ? "" : obj.A(), set_##A

#define HELPZ_DB_META(T, t_name, t_short_name, N, ...)          \
    public:                                                     \
    enum Database_Columns {                                     \
        HELPZ_DB_COL_NAME_##N (__VA_ARGS__), COL_COUNT          \
    };                                                          \
    static QString table_name()                                 \
    {                                                           \
        return t_name;                                          \
    }                                                           \
    static QString table_short_name()                           \
    {                                                           \
        return t_short_name;                                    \
    }                                                           \
    static QStringList table_column_names()                     \
    {                                                           \
        return { HELPZ_DB_QUOTE_##N (__VA_ARGS__) };            \
    }                                                           \
    static QVariant value_getter(const T& obj,                  \
                                 std::size_t pos)               \
    {                                                           \
        switch (pos)                                            \
        {                                                       \
        HELPZ_DB_GETTER_CASE_##N (obj, __VA_ARGS__)             \
        default: break;                                         \
        }                                                       \
        return {};                                              \
    }                                                           \
    static void value_setter(T& obj, std::size_t pos,           \
                             const QVariant& value)             \
    {                                                           \
        switch (pos) {                                          \
        HELPZ_DB_SETTER_CASE_##N (T, obj, value, __VA_ARGS__)   \
        default: break;                                         \
        }                                                       \
    }                                                           \
    private:

#define HELPZ_DB_COL_NAME_1(A, GT, ST, ...)   COL_##A
#define HELPZ_DB_COL_NAME_2(A, GT, ST, ...)   COL_##A, HELPZ_DB_COL_NAME_1(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_3(A, GT, ST, ...)   COL_##A, HELPZ_DB_COL_NAME_2(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_4(A, GT, ST, ...)   COL_##A, HELPZ_DB_COL_NAME_3(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_5(A, GT, ST, ...)   COL_##A, HELPZ_DB_COL_NAME_4(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_6(A, GT, ST, ...)   COL_##A, HELPZ_DB_COL_NAME_5(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_7(A, GT, ST, ...)   COL_##A, HELPZ_DB_COL_NAME_6(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_8(A, GT, ST, ...)   COL_##A, HELPZ_DB_COL_NAME_7(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_9(A, GT, ST, ...)   COL_##A, HELPZ_DB_COL_NAME_8(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_10(A, GT, ST, ...)  COL_##A, HELPZ_DB_COL_NAME_9(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_11(A, GT, ST, ...)  COL_##A, HELPZ_DB_COL_NAME_10(__VA_ARGS__)

#define HELPZ_DB_QUOTE_1(A, GT, ST)           #A
#define HELPZ_DB_QUOTE_2(A, GT, ST, ...)      #A, HELPZ_DB_QUOTE_1(__VA_ARGS__)
#define HELPZ_DB_QUOTE_3(A, GT, ST, ...)      #A, HELPZ_DB_QUOTE_2(__VA_ARGS__)
#define HELPZ_DB_QUOTE_4(A, GT, ST, ...)      #A, HELPZ_DB_QUOTE_3(__VA_ARGS__)
#define HELPZ_DB_QUOTE_5(A, GT, ST, ...)      #A, HELPZ_DB_QUOTE_4(__VA_ARGS__)
#define HELPZ_DB_QUOTE_6(A, GT, ST, ...)      #A, HELPZ_DB_QUOTE_5(__VA_ARGS__)
#define HELPZ_DB_QUOTE_7(A, GT, ST, ...)      #A, HELPZ_DB_QUOTE_6(__VA_ARGS__)
#define HELPZ_DB_QUOTE_8(A, GT, ST, ...)      #A, HELPZ_DB_QUOTE_7(__VA_ARGS__)
#define HELPZ_DB_QUOTE_9(A, GT, ST, ...)      #A, HELPZ_DB_QUOTE_8(__VA_ARGS__)
#define HELPZ_DB_QUOTE_10(A, GT, ST, ...)     #A, HELPZ_DB_QUOTE_9(__VA_ARGS__)
#define HELPZ_DB_QUOTE_11(A, GT, ST, ...)     #A, HELPZ_DB_QUOTE_10(__VA_ARGS__)

#define HELPZ_DB_GETTER_CASE_IMPL(A, O, F)    case COL_##A: return O.F;
#define HELPZ_DB_GETTER_CASE_1(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)
#define HELPZ_DB_GETTER_CASE_2(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_1(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_3(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_2(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_4(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_3(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_5(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_4(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_6(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_5(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_7(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_6(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_8(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_7(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_9(O, A, GT, ST, ...)     HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_8(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_10(O, A, GT, ST, ...)    HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_9(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_11(O, A, GT, ST, ...)    HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_10(O, __VA_ARGS__)

#define HELPZ_DB_SETTER_CASE_IMPL(A, T, O, F, V)    case COL_##A: ::Helpz::Database::db_set_value_from_variant(O, &T::F, V); break;
#define HELPZ_DB_SETTER_CASE_1(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)
#define HELPZ_DB_SETTER_CASE_2(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_1(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_3(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_2(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_4(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_3(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_5(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_4(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_6(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_5(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_7(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_6(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_8(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_7(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_9(T, O, V, A, GT, ST, ...)     HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_8(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_10(T, O, V, A, GT, ST, ...)    HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_9(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_11(T, O, V, A, GT, ST, ...)    HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_10(T, O, V, __VA_ARGS__)

#endif // HELPZ_DATABASE_META_H
