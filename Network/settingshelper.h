#ifndef ZIMNIKOV_SETTINGSHELPER_H
#define ZIMNIKOV_SETTINGSHELPER_H

#include <QSettings>

#include <functional>

#include <Helpz/simplethread.h>

namespace Helpz {

template<typename _Tp> struct CharArrayToQString { typedef _Tp type; };
template<> struct CharArrayToQString<const char*> { typedef QString type; };
template<> struct CharArrayToQString<char*> { typedef QString type; };

template<typename T>
struct Param {
    using Type = typename CharArrayToQString<typename std::decay<T>::type>::type;
    typedef std::function<QVariant(const QVariant&/*value*//*, bool get_algo*/)> NormalizeFunc;

    Param(const QString& name, const T& default_value, NormalizeFunc normalize_function = nullptr) :
        name(name), default_value(default_value), normalize(normalize_function) {}

    QString name;
    QVariant default_value;
    NormalizeFunc normalize;
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
    T* ptr() { return applyToObj<T>(m_idx); }

    template<typename T>
    std::shared_ptr<T> shared_ptr() { return std::shared_ptr<T>{ ptr<T>() }; }

    template<typename T>
    std::unique_ptr<T> unique_ptr() { return std::unique_ptr<T>{ ptr<T>() }; }
protected:
    template<typename T>
    const T& getValue(const T& val) const { return val; }
    template<typename _Pt>
    typename Param<_Pt>::Type getValue(const Param<_Pt>& param)
    {
        using T = typename Param<_Pt>::Type;
        if (s->contains(param.name))
        {
            QVariant value = s->value(param.name, param.default_value);
            if (param.normalize)
                value = param.normalize(value);
            return qvariant_cast<T>(value);
        }
        s->setValue(param.name, param.default_value);

        if (param.normalize)
            return qvariant_cast<T>(param.normalize(param.default_value));
        return qvariant_cast<T>(param.default_value);
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
    Type* applyToObj(const std::index_sequence<_Idx...>&)
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
    Type* operator ()(QSettings* settings, const QString& begin_group, Args... param)
    {
        SettingsHelper<Args...> helper(settings, begin_group, param...);
        return helper.template ptr<Type>();
    }
};

template<typename T, typename... Args>
using SettingsThreadHelper = SettingsHelperPtr<ParamThread<T, Args...>>;

} // namespace Helpz

#endif // ZIMNIKOV_SETTINGSHELPER_H
