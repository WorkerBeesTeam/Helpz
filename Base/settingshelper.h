#ifndef HELPZ_SETTINGSHELPER_H
#define HELPZ_SETTINGSHELPER_H

#include <functional>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

#include <QSettings>

#include <Helpz/zstring.h>
#include <Helpz/simplethread.h>

template<> inline std::string qvariant_cast<std::string>(const QVariant& value) {
    return value.toString().toStdString();
}

template<> inline std::vector<std::string> qvariant_cast<std::vector<std::string>>(const QVariant& value)
{
    std::vector<std::string> res;
    std::string text = value.toString().toStdString();
    boost::split(res, text, [](char c) { return c == ','; });
    for (std::string& str: res)
        Helpz::String::trim(str);
    return res;
}

template<> inline std::chrono::seconds qvariant_cast<std::chrono::seconds>(const QVariant& value) {
    return std::chrono::seconds{value.toLongLong()};
}

template<> inline std::chrono::milliseconds qvariant_cast<std::chrono::milliseconds>(const QVariant& value) {
    return std::chrono::milliseconds{value.toLongLong()};
}

namespace Helpz {

template<typename T> inline QVariant QValueNormalize(const T& default_value) { return default_value; }
template<> inline QVariant QValueNormalize<std::string>(const std::string& default_value) { return QString::fromStdString(default_value); }
template<> inline QVariant QValueNormalize<std::vector<std::string>>(const std::vector<std::string>& default_value) { return QString::fromStdString(boost::join(default_value, ",")); }
template<> inline QVariant QValueNormalize<std::chrono::seconds>(const std::chrono::seconds& default_value) { return QVariant{static_cast<qlonglong>(default_value.count())}; }
template<> inline QVariant QValueNormalize<std::chrono::milliseconds>(const std::chrono::milliseconds& default_value) { return QVariant{static_cast<qlonglong>(default_value.count())}; }

template<typename _Tp> struct CharArrayToQString { typedef _Tp type; };
template<> struct CharArrayToQString<const char*> { typedef QString type; };
template<> struct CharArrayToQString<char*> { typedef QString type; };

template<typename T>
struct Param {
    using Type = typename CharArrayToQString<typename std::decay<T>::type>::type;
    typedef std::function<QVariant(const QVariant&/*value*//*, bool get_algo*/)> NormalizeFunc;

    Param(const QString& name, const T& default_value, NormalizeFunc normalize_function = nullptr) :
        name_(name), default_value_(QValueNormalize<T>(default_value)), normalize_(normalize_function)
    {}

    QString name_;
    QVariant default_value_;
    NormalizeFunc normalize_;
};

template<> struct CharArrayToQString<Param<const char*>> { typedef QString type; };
template<> struct CharArrayToQString<Param<char*>> { typedef QString type; };
template<typename _Tp>
struct CharArrayToQString<Param<_Tp>> {
    typedef typename Param<_Tp>::Type type;
};

template<typename... Args>
class SettingsHelper
{
public:
    using Tuple = std::tuple<typename CharArrayToQString<typename std::decay<Args>::type>::type...>;

    SettingsHelper(QSettings* settings, const QString& begin_group, Args... param) :
        s(settings), end_group(!begin_group.isEmpty())
    {
        if (end_group)
            s->beginGroup(begin_group);

        auto p_tuple = std::make_tuple(param...);
        using ParamTuple = decltype(p_tuple);

        parseTuple<ParamTuple>(std::forward<ParamTuple>(p_tuple), m_idx);
    }
    ~SettingsHelper()
    {
        if (end_group)
            s->endGroup();
    }

    Tuple operator ()() { return m_args; }

    template<typename T>
    T obj() { return apply_to_obj<T>(m_idx); }

    template<typename T>
    T* ptr() { return apply_to_obj_ptr<T>(m_idx); }

    template<typename T>
    std::shared_ptr<T> shared_ptr() { return std::shared_ptr<T>{ ptr<T>() }; }

    template<typename T>
    std::unique_ptr<T> unique_ptr() { return std::unique_ptr<T>{ ptr<T>() }; }
protected:
    template<typename T>
    const T& getValue(const T& val) const { return val; }
//    template<typename T> T&& getValue(T&& val) const { return val; }
    template<typename _Pt>
    typename Param<_Pt>::Type getValue(const Param<_Pt>& param)
    {
        using T = typename Param<_Pt>::Type;
        if (s->contains(param.name_))
        {
            QVariant value = s->value(param.name_, param.default_value_);
            if (param.normalize_)
                value = param.normalize_(value);
            return qvariant_cast<T>(value);
        }
        s->setValue(param.name_, param.default_value_);

        if (param.normalize_)
            return qvariant_cast<T>(param.normalize_(param.default_value_));
        return qvariant_cast<T>(param.default_value_);
    }

    template<typename _PTuple>
    void parse(_PTuple&) {}
    template<typename _PTuple, std::size_t N, std::size_t... _Idx>
    void parse(_PTuple& __pt) {
        std::get<N>(m_args) = getValue(std::get<N>(__pt));
        parse<_PTuple, _Idx...>(__pt);
    }

    template <typename _PTuple, std::size_t... _Idx>
    void parseTuple(_PTuple&& __pt, const std::index_sequence<_Idx...>&)
    {
        parse<_PTuple, _Idx...>(__pt);
    }

    template <typename Type, std::size_t... _Idx>
    Type apply_to_obj(const std::index_sequence<_Idx...>&)
    {
        return Type{std::get<_Idx>(std::forward<Tuple>(m_args))...};
    }

    template <typename Type, std::size_t... _Idx>
    Type* apply_to_obj_ptr(const std::index_sequence<_Idx...>&)
    {
        return new Type{std::get<_Idx>(std::forward<Tuple>(m_args))...};
    }

    Tuple m_args;

    using Indices = std::make_index_sequence<std::tuple_size<Tuple>::value>;
    Indices m_idx;
private:
    QSettings* s;
    bool end_group;
};

template<typename T>
class SettingsHelperPtr {
public:
    using Type = T;

    template<typename... Args>
    Type* operator ()(QSettings* settings, const QString& begin_group, Args... param) {
        return this->template make<Args...>(settings, begin_group, param...);
    }

    template<typename... Args>
    Type* make(QSettings* settings, const QString& begin_group, Args... param)
    {
        SettingsHelper<Args...> helper(settings, begin_group, param...);
        return helper.template ptr<Type>();
    }
};

template<typename T, typename... Args>
using SettingsThreadHelper = SettingsHelperPtr<ParamThread<T, Args...>>;

} // namespace Helpz

#endif // HELPZ_SETTINGSHELPER_H
