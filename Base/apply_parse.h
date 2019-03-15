#ifndef HELPZ_QT_APPLY_PARSE_H
#define HELPZ_QT_APPLY_PARSE_H

#include <iostream>
#include <tuple>
#include <memory>
#include <functional>
#include <type_traits>

#include <QDataStream>

#include "fake_integer_sequence.h"

namespace Helpz {

inline std::unique_ptr<QDataStream> parse_open_device(QIODevice &data_dev, int ds_version)
{
    if (!data_dev.isOpen())
    {
        if (!data_dev.open(QIODevice::ReadOnly))
        {
            throw std::runtime_error(("Can't open msg device:" + data_dev.errorString()).toStdString());
        }
    }

    std::unique_ptr<QDataStream> ds(new QDataStream{&data_dev});
    ds->setVersion(ds_version);
    return std::move(ds);
}

//template<> void parse_out(QDataStream &) {}

template<typename DataStream>
void parse_out(DataStream &) {}

template<typename T, typename... Args>
void parse_out(QDataStream &ds, T& out, Args&... args)
{
    ds >> out;
    if (ds.status() != QDataStream::Ok)
    {
        throw std::runtime_error(std::string("QDataStream parse failed for type: ") + typeid(T).name());
    }
    parse_out(ds, args...);
}

template<typename... Args>
void parse_out(int ds_version, const QByteArray &data, Args&... args)
{
    QDataStream ds(data);
    ds.setVersion(ds_version);
    parse_out(ds, args...);
}

template<typename... Args>
void parse_out(int ds_version, QIODevice &data_dev, Args&... args)
{
    std::unique_ptr<QDataStream> ds = parse_open_device(data_dev, ds_version);
    parse_out(*ds, args...);
}

template<typename T>
T parse(int ds_version, QIODevice &data_dev)
{
    T obj; parse_out(ds_version, data_dev, obj); return obj;
}

template<typename T>
T parse(QDataStream &ds)
{
    T obj; parse_out(ds, obj); return obj;
}

template<typename _Tuple>
void parse(QDataStream &, _Tuple&) {}

template<typename _Tuple, std::size_t x, std::size_t... _Idx>
void parse(QDataStream &ds, _Tuple& __t)
{
    parse_out(ds, std::get<x>(__t)); // typename std::tuple_element<x, _Tuple>::type
    parse<_Tuple, _Idx...>(ds, __t);
}

template <typename RetType, typename _Tuple, typename _Fn, class T, std::size_t... _Idx, typename... Args>
RetType __apply_parse_impl(QDataStream &ds, _Fn __f, T* obj, std::index_sequence<_Idx...>, Args&&... args)
{
    if (ds.status() != QDataStream::Ok)
        ds.resetStatus();

    _Tuple tuple;
    parse<_Tuple, _Idx...>(ds, tuple);

#ifdef __cpp_lib_invoke
    return std::invoke(__f, obj, std::get<_Idx>(std::forward<_Tuple&&>(tuple))..., std::forward<Args&&>(args)...);
#else
#pragma GCC warning "Old invoke"
    auto func = std::bind(__f, obj, std::_Placeholder<_Idx + 1>{}...);
    return std::__invoke(func, std::get<_Idx>(std::forward<_Tuple>(tuple))..., args...);
#endif
}

template <typename RetType, typename _Fn, class T, typename... FArgs, typename... Args>
RetType apply_parse_impl(QDataStream &ds, _Fn __f, T* obj, Args&&... args)
{
//    auto tuple = std::make_tuple(typename std::decay<Args>::type()...);
//    using Tuple = decltype(tuple);
//    using Indices = std::make_index_sequence<std::tuple_size<Tuple>::value>;
    using Tuple = std::tuple<typename std::decay<FArgs>::type...>; // TODO: create tuple without Args, only {FArgs - Args}
    using Indices = std::make_index_sequence<sizeof...(FArgs) - sizeof...(Args)>;

    return __apply_parse_impl<RetType, Tuple>(ds, __f, obj, Indices{}, std::forward<Args&&>(args)...);
}

template<class FT, class T, typename RetType, typename... FArgs, typename... Args>
RetType apply_parse(QDataStream &ds, RetType(FT::*__f)(FArgs...) const, T* obj, Args&&... args)
{
    return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, obj, std::forward<Args&&>(args)...);
}

template<class FT, class T, typename RetType, typename... FArgs, typename... Args>
RetType apply_parse(QDataStream &ds, RetType(FT::*__f)(FArgs...), T* obj, Args&&... args)
{
    return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, obj, std::forward<Args&&>(args)...);
}

template<class FT, class T, typename RetType, typename... FArgs, typename... Args>
RetType apply_parse(QIODevice& data_dev, int ds_version, RetType(FT::*__f)(FArgs...) const, T* obj, Args&&... args)
{
    std::unique_ptr<QDataStream> ds = parse_open_device(data_dev, ds_version);
    return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(*ds, __f, obj, std::forward<Args&&>(args)...);
}

template<class FT, class T, typename RetType, typename... FArgs, typename... Args>
RetType apply_parse(QIODevice& data_dev, int ds_version, RetType(FT::*__f)(FArgs...), T* obj, Args&&... args)
{
    std::unique_ptr<QDataStream> ds = parse_open_device(data_dev, ds_version);
    return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(*ds, __f, obj, std::forward<Args&&>(args)...);
}

template<class FT, class T, typename RetType, typename... FArgs, typename... Args>
RetType apply_parse(const QByteArray &data, int ds_version, RetType(FT::*__f)(FArgs...) const, T* obj, Args&&... args)
{
    QDataStream ds(data);
    ds.setVersion(ds_version);
    return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, obj, std::forward<Args&&>(args)...);
}

template<class FT, class T, typename RetType, typename... FArgs, typename... Args>
RetType apply_parse(const QByteArray &data, int ds_version, RetType(FT::*__f)(FArgs...), T* obj, Args&&... args)
{
    QDataStream ds(data);
    ds.setVersion(ds_version);
    return apply_parse_impl<RetType, decltype(__f), T, FArgs...>(ds, __f, obj, std::forward<Args&&>(args)...);
}

} // namespace Helpz

#endif // HELPZ_QT_APPLY_PARSE_H
