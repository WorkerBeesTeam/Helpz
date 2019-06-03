#ifndef HELPZ_FAKE_INTEGER_SEQUENCE_H
#define HELPZ_FAKE_INTEGER_SEQUENCE_H

#include <utility>

#if (__cplusplus < 201402L) && !defined(__cpp_lib_integer_sequence)

namespace fake_std {

template<int...> struct seq { using type = seq; };
template<typename T1, typename T2> struct concat;
template<int... I1, int... I2> struct concat<seq<I1...>, seq<I2...>>: seq<I1..., (sizeof...(I1) + I2)...> {};

template<int N> struct gen_seq;
template<int N> struct gen_seq: concat<typename gen_seq<N/2>::type, typename gen_seq<N-N/2>::type>::type {};
template <> struct gen_seq<0>: seq<>{};
template <> struct gen_seq<1>: seq<0>{};

}

namespace std {
//template <int... N>
//using index_sequence = fake_std::seq<N...>;

//template <int N>
//using make_index_sequence = fake_std::gen_seq<N>;


template< std::size_t ... i >
struct index_sequence
{
    typedef std::size_t value_type;

    typedef index_sequence<i...> type;

    // gcc-4.4.7 doesn't support `constexpr` and `noexcept`.
    static /*constexpr*/ std::size_t size() /*noexcept*/
    {
        return sizeof ... (i);
    }
};


// this structure doubles index_sequence elements.
// s- is number of template arguments in IS.
template< std::size_t s, typename IS >
struct doubled_index_sequence;

template< std::size_t s, std::size_t ... i >
struct doubled_index_sequence< s, index_sequence<i... > >
{
    typedef index_sequence<i..., (s + i)... > type;
};

// this structure incremented by one index_sequence, iff NEED-is true,
// otherwise returns IS
template< bool NEED, typename IS >
struct inc_index_sequence;

template< typename IS >
struct inc_index_sequence<false,IS>{ typedef IS type; };

template< std::size_t ... i >
struct inc_index_sequence< true, index_sequence<i...> >
{
    typedef index_sequence<i..., sizeof...(i)> type;
};



// helper structure for make_index_sequence.
template< std::size_t N >
struct make_index_sequence_impl :
           inc_index_sequence< (N % 2 != 0),
                typename doubled_index_sequence< N / 2,
                           typename make_index_sequence_impl< N / 2> ::type
               >::type
       >
{};

 // helper structure needs specialization only with 0 element.
template<>struct make_index_sequence_impl<0>{ typedef index_sequence<> type; };



// OUR make_index_sequence,  gcc-4.4.7 doesn't support `using`,
// so we use struct instead of it.
template< std::size_t N >
struct make_index_sequence : make_index_sequence_impl<N>::type {};

//index_sequence_for  any variadic templates
template< typename ... T >
struct index_sequence_for : make_index_sequence< sizeof...(T) >{};

}
#endif

#endif // HELPZ_FAKE_INTEGER_SEQUENCE_H
