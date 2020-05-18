#ifndef HELPZ_DATABASE_META_H
#define HELPZ_DATABASE_META_H

#include <type_traits>

#include <QVariant>

#include <Helpz/db_table.h>

namespace Helpz {
namespace DB {

template<typename T>
bool is_table_use_common_prefix() { return true; }

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

} // namespace DB
} // namespace Helpz

#define DB_A(A) A, A(), set_##A
#define DB_AM(A) A, A, A
#define DB_AMN(A) A, A ? obj.A : QVariant(), A
#define DB_AN(A) A, A() ? obj.A() : QVariant(), set_##A
#define DB_AT(A) A, A##_to_db(), set_##A##_from_db
#define DB_ANS(A) A, A().isEmpty() ? "" : obj.A(), set_##A

#define HELPZ_DB_META_PASTER(T, t_name, t_short_name, N, ...)   \
    public:                                                     \
    enum Database_Columns {                                     \
        HELPZ_DB_COL_NAME_##N (__VA_ARGS__), COL_COUNT          \
    };                                                          \
    static QString table_name()                                 \
    {                                                           \
        using namespace Helpz::DB;                              \
        if (is_table_use_common_prefix<T>())                    \
            return Table::common_prefix() + t_name;             \
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
    static QVariantList to_variantlist(const T& obj)            \
    {                                                           \
        QVariantList variantlist;                               \
        for (std::size_t i = 0; i < T::COL_COUNT; ++i)          \
        {                                                       \
            variantlist.push_back(T::value_getter(obj, i));     \
        }                                                       \
        return variantlist;                                     \
    }                                                           \
    private:

#define HELPZ_DB_META_EVALUATOR(T, t_name, t_short_name, N, ...) HELPZ_DB_META_PASTER(T, t_name, t_short_name, N, __VA_ARGS__)
#define HELPZ_DB_META(T, t_name, t_short_name, ...) HELPZ_DB_META_EVALUATOR(T, t_name, t_short_name, HELPZ_DB_NARG(__VA_ARGS__), __VA_ARGS__)

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
#define HELPZ_DB_COL_NAME_12(A, GT, ST, ...)  COL_##A, HELPZ_DB_COL_NAME_11(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_13(A, GT, ST, ...)  COL_##A, HELPZ_DB_COL_NAME_12(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_14(A, GT, ST, ...)  COL_##A, HELPZ_DB_COL_NAME_13(__VA_ARGS__)
#define HELPZ_DB_COL_NAME_15(A, GT, ST, ...)  COL_##A, HELPZ_DB_COL_NAME_14(__VA_ARGS__)

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
#define HELPZ_DB_QUOTE_12(A, GT, ST, ...)     #A, HELPZ_DB_QUOTE_11(__VA_ARGS__)
#define HELPZ_DB_QUOTE_13(A, GT, ST, ...)     #A, HELPZ_DB_QUOTE_12(__VA_ARGS__)
#define HELPZ_DB_QUOTE_14(A, GT, ST, ...)     #A, HELPZ_DB_QUOTE_13(__VA_ARGS__)
#define HELPZ_DB_QUOTE_15(A, GT, ST, ...)     #A, HELPZ_DB_QUOTE_14(__VA_ARGS__)

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
#define HELPZ_DB_GETTER_CASE_12(O, A, GT, ST, ...)    HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_11(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_13(O, A, GT, ST, ...)    HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_12(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_14(O, A, GT, ST, ...)    HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_13(O, __VA_ARGS__)
#define HELPZ_DB_GETTER_CASE_15(O, A, GT, ST, ...)    HELPZ_DB_GETTER_CASE_IMPL(A, O, GT)   HELPZ_DB_GETTER_CASE_14(O, __VA_ARGS__)

#define HELPZ_DB_SETTER_CASE_IMPL(A, T, O, F, V)    case COL_##A: ::Helpz::DB::db_set_value_from_variant(O, &T::F, V); break;
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
#define HELPZ_DB_SETTER_CASE_12(T, O, V, A, GT, ST, ...)    HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_11(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_13(T, O, V, A, GT, ST, ...)    HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_12(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_14(T, O, V, A, GT, ST, ...)    HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_13(T, O, V, __VA_ARGS__)
#define HELPZ_DB_SETTER_CASE_15(T, O, V, A, GT, ST, ...)    HELPZ_DB_SETTER_CASE_IMPL(A, T, O, ST, V)   HELPZ_DB_SETTER_CASE_14(T, O, V, __VA_ARGS__)


#define HELPZ_DB_NARG(...) \
         HELPZ_DB_NARG_(__VA_ARGS__,HELPZ_DB_RSEQ_N())
#define HELPZ_DB_NARG_(...) \
         HELPZ_DB_ARG_N(__VA_ARGS__)
#define HELPZ_DB_ARG_N( \
          _1_1,_1_2,_1_3, _2_1,_2_2,_2_3, _3_1,_3_2,_3_3, _4_1,_4_2,_4_3, _5_1,_5_2,_5_3, _6_1,_6_2,_6_3, _7_1,_7_2,_7_3, _8_1,_8_2,_8_3, _9_1,_9_2,_9_3, \
         _10_1,_10_2,_10_3, _11_1,_11_2,_11_3, _12_1,_12_2,_12_3, _13_1,_13_2,_13_3, _14_1,_14_2,_14_3, _15_1,_15_2,_15_3, N, ...) N
#define HELPZ_DB_RSEQ_N() \
         15,15,15,14,14,14,13,13,13,12,12,12,11,11,11,10,10,10, \
         9,9,9,8,8,8,7,7,7,6,6,6,5,5,5,4,4,4,3,3,3,2,2,2,1,1,1,0

// ----------------------------------- Get number of ... __VA_ARGS__
#define HELPZ_PP_NARG(...) \
         HELPZ_PP_NARG_(__VA_ARGS__,HELPZ_PP_RSEQ_N())
#define HELPZ_PP_NARG_(...) \
         HELPZ_PP_ARG_N(__VA_ARGS__)
#define HELPZ_PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define HELPZ_PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

// ----------------------------------- For each

// Make a FOREACH macro
#define HELPZ_FE_1(WHAT, X) WHAT(X)
#define HELPZ_FE_2(WHAT, X, ...) WHAT(X)HELPZ_FE_1(WHAT, __VA_ARGS__)
#define HELPZ_FE_3(WHAT, X, ...) WHAT(X)HELPZ_FE_2(WHAT, __VA_ARGS__)
#define HELPZ_FE_4(WHAT, X, ...) WHAT(X)HELPZ_FE_3(WHAT, __VA_ARGS__)
#define HELPZ_FE_5(WHAT, X, ...) WHAT(X)HELPZ_FE_4(WHAT, __VA_ARGS__)
//... repeat as needed

#define HELPZ_GET_MACRO(_1,_2,_3,_4,_5,NAME,...) NAME
#define HELPZ_FOR_EACH(action,...) \
  HELPZ_GET_MACRO(__VA_ARGS__,HELPZ_FE_5,HELPZ_FE_4,HELPZ_FE_3,HELPZ_FE_2,HELPZ_FE_1)(action,__VA_ARGS__)


//#define STRUCT_MEM(name) char *name;
//#define DEF_STRUCT(name, ...) extern const struct name { HELPZ_FOR_EACH(STRUCT_MEM, __VA_ARGS__) };
// DEF_STRUCT(myStruct, a = "first", b = "second", c = "third")

// result after preprocessing:
// extern const struct myStruct { char *a = "first";char *b = "second";char *c = "third"; };

#endif // HELPZ_DATABASE_META_H
