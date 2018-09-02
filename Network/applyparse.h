#ifndef HELPZ_QT_APPLY_PARSE_H
#define HELPZ_QT_APPLY_PARSE_H

#include <iostream>
#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

#include <QDataStream>

namespace Helpz {

template<typename DataStream>
void parse_out(DataStream &) {}

template<typename T, typename... Args>
void parse_out(QDataStream &ds, T& out, Args&... args) {
    ds >> out;
    if (ds.status() != QDataStream::Ok)
        throw std::runtime_error(std::string("QDataStream parse failed for type: ") + typeid(T).name());
    parse_out(ds, args...);
}

//template<>
//void parse_out(QDataStream &) {}

template<typename T>
T parse(QDataStream &ds) {
    T obj; parse_out(ds, obj); return obj;
}

template<typename _Tuple>
void parse(QDataStream &, _Tuple&) {}

template<typename _Tuple, std::size_t x, std::size_t... _Idx>
void parse(QDataStream &ds, _Tuple& __t) {
    parse_out(ds, std::get<x>(__t)); // typename std::tuple_element<x, _Tuple>::type
    parse<_Tuple, _Idx...>(ds, __t);
}

template <typename RetType, typename _Tuple, typename _Fn, class T, std::size_t... _Idx>
RetType __applyParseImpl(_Fn __f, T* obj, QDataStream &ds, std::index_sequence<_Idx...>)
{
    if (ds.status() != QDataStream::Ok)
        ds.resetStatus();

    _Tuple tuple;
    parse<_Tuple, _Idx...>(ds, tuple);
    return std::invoke(__f, obj, std::get<_Idx>(std::forward<_Tuple&&>(tuple))...);
}

template <typename RetType, typename _Fn, class T, typename... Args>
RetType applyParseImpl(_Fn __f, T* obj, QDataStream &ds)
{
//    auto tuple = std::make_tuple(typename std::decay<Args>::type()...);
//    using Tuple = decltype(tuple);
//    using Indices = std::make_index_sequence<std::tuple_size<Tuple>::value>;
    using Tuple = std::tuple<typename std::decay_t<Args>...>;
    using Indices = std::make_index_sequence<sizeof...(Args)>;

    return __applyParseImpl<RetType, Tuple>(__f, obj, ds, Indices{});
}

template<class FT, class T, typename RetType, typename... Args>
RetType applyParse(RetType(FT::*__f)(Args...) const, T* obj, QDataStream &ds) {
    return applyParseImpl<RetType, decltype(__f), T, Args...>(__f, obj, ds);
}

template<class FT, class T, typename RetType, typename... Args>
RetType applyParse(RetType(FT::*__f)(Args...), T* obj, QDataStream &ds) {
    return applyParseImpl<RetType, decltype(__f), T, Args...>(__f, obj, ds);
}

} // namespace Helpz

#endif // HELPZ_QT_APPLY_PARSE_H
