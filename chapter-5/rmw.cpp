#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <tuple>
#include <typeinfo>

// Register Optimization with Safety (ROS)

namespace ros {
template <typename T, unsigned msb, unsigned lsb>
concept FieldSelectable = requires {
    requires std::unsigned_integral<T>; 
} &&
(
    msb <= std::numeric_limits<T>::digits-1 &&
    lsb <= std::numeric_limits<T>::digits-1
) && (
    msb >= lsb
);

namespace detail {
template <char Char>
struct is_decimal_digit {
    static constexpr bool value = Char - '0' >= 0 && Char - '0' <= 9;
};

template <char Char>
constexpr bool is_decimal_digit_v = ros::detail::is_decimal_digit<Char>::value;

template <char ...Chars>
concept DecimalNumber = (ros::detail::is_decimal_digit_v<Chars> && ...);

template <typename T, char... Chars>
requires DecimalNumber<Chars...>
[[nodiscard]] static constexpr auto to_unsigned_const() -> T {
    constexpr T value = []() {
        constexpr std::array<char, sizeof...(Chars)> chars{Chars...};
        T sum = 0;

        for (char c : chars) {
            T const digit = c - '0';
            sum = (sum * 10) + digit;
        }

        return sum;
    }();

    return value;
}

template <char Char>
struct is_x {
    static constexpr bool value = Char == 'x';
};
template <char Char>
constexpr bool is_x_v = ros::detail::is_x<Char>::value;

template <char Char>
struct is_0 {
    static constexpr bool value = Char == '0';
};
template <char Char>
constexpr bool is_0_v = ros::detail::is_0<Char>::value;

template <char Char>
struct is_hex_char {
    static constexpr bool value = ros::detail::is_decimal_digit_v<Char> || 
    (Char - 'A' >= 0 && Char - 'F' < 6 || Char - 'a' >= 0 && Char - 'f' < 6);
};
template <char Char>
constexpr bool is_hex_char_v = ros::detail::is_hex_char<Char>::value;

template <char Char0, char Char1, char ...Chars>
concept HexadecimalNumber = (
    ros::detail::is_0_v<Char0> &&
    ros::detail::is_x_v<Char1> &&
    (ros::detail::is_hex_char_v<Chars> && ...)
);

template <typename T, char Char0, char Char1, char... Chars>
requires HexadecimalNumber<Char0, Char1, Chars...>
[[nodiscard]] static constexpr auto to_unsigned_const() -> T {
    constexpr T value = []() {
        constexpr std::array<char, sizeof...(Chars)> chars{Chars...};
        T sum = 0;

        for (char c : chars) {
            T const digit = c - '0' > 9? c >= 'a'? c - 'a' + 10 : c - 'A' + 10 : c - '0';
            sum = (sum * 16) + digit;
        }

        return sum;
    }();

    return value;
}
}

enum class AccessType {
    RO,
    RW
};

namespace detail {
// forward declaration of operations
// template <typename Reg, unsigned msb, unsigned lsb, AccessType AT, typename Reg::value_type val>
template <typename Field, typename Field::value_type val>
struct field_assignment_safe;
template <typename Field>
struct field_assignment_unsafe;
template <typename Field>
struct field_read;
}

template <typename Reg, unsigned msb, unsigned lsb, AccessType AT>
requires FieldSelectable<typename Reg::value_type, msb, lsb>
struct field {
    using value_type = typename Reg::value_type;
    using reg = Reg;
    static constexpr unsigned length = msb - lsb;

    static constexpr AccessType access_type = AT;

    static constexpr value_type mask = []() {
        if (msb != lsb) {
            if (msb == std::numeric_limits<value_type>::digits-1) {
                return ~((1 << lsb) - 1);
            } else {
                return ((1 << msb) - 1) & ~((1 << lsb) - 1);
            }
        } else {
            return 1 << msb;
        }
    }();

    constexpr field(value_type v)
        : value{v}
    {}

    constexpr field() : field(0) {};

    template <typename U, U val>
    requires (std::is_convertible_v<U, value_type>)
    constexpr auto operator= (std::integral_constant<U, val>) -> ros::detail::field_assignment_safe<field, val> {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        static_assert(val <= (mask >> lsb), "assigned value greater than allowed");
        value = val;
        return ros::detail::field_assignment_safe<field, val>{};
    }

    constexpr auto operator= (auto const& rhs) -> field & {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        using rhs_type = std::remove_reference_t<decltype(rhs)>;
        static_assert(rhs_type::length <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
        value = rhs.value;
        return *this;
    }

    constexpr auto operator= (auto && rhs) -> field & {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        using rhs_type = std::remove_reference_t<decltype(rhs)>;
        static_assert(rhs_type::length <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
        value = rhs.value;
        return *this;
    }

    static constexpr value_type update (value_type old_value, value_type new_value) {
        return (old_value & ~mask) | (new_value & mask);
    }

    constexpr auto read() const -> ros::detail::field_read<field> {
        // to be substituted with read template expr
        // return *this;
        return ros::detail::field_read<field>{};
    }

    value_type value;
};

namespace detail {
// template <typename Reg, unsigned msb, unsigned lsb, AccessType AT, typename Reg::value_type val>
template <typename Field, typename Field::value_type val>
struct field_assignment_safe {
    using type = Field;
    static constexpr typename Field::value_type value = val;
};

template <typename Field>
struct field_assignment_unsafe {
    using type = Field;
    // should it be static?
    typename Field::value_type value;
};

template <typename Field>
struct field_read {
    using type = Field;
};
}

template <typename T>
constexpr bool is_field_v = false;

template <typename Reg, unsigned msb, unsigned lsb, AccessType AT>
constexpr bool is_field_v<field<Reg, msb, lsb, AT>> = true;


template <typename ...Ts>
concept HasMask = requires {
    (Ts::mask,...);
};

template <typename T, typename ...Ts>
concept OfSameParentReg = (std::same_as<typename T::reg, typename Ts::reg> && ...);

template <typename T, typename ...Ts>
concept Fieldable = HasMask<T, Ts...> && OfSameParentReg<T, Ts...>;

template <typename Field0, typename ...Fields>
requires Fieldable<Field0, Fields...>
struct fields {
    using value_type = typename Field0::value_type;
    static constexpr value_type layout = (Field0::mask | ... | Fields::mask);
};

template <typename T, std::size_t addr>
struct reg {
    using value_type = T;
    static constexpr std::size_t address = addr;

    // using f = typename DerivedReg::Fields;
    // using value_type = typename DerivedReg::Fields::value_type;
    // static constexpr value_type layout = DerivedReg::Fields::layout;

    // // static constexpr bool needs_read (value_type new_value) {
    // //     return !(((F0::mask & new_value) > 0) && (((Fs::mask & new_value) > 0) && ...));
    // // }

    // static constexpr value_type update (value_type old_value, value_type new_value) {
    //     return (old_value & ~layout) | (new_value & layout);
    // }
};

namespace literals {
template <char ...Chars>
constexpr auto operator""_f () {
    using T = std::size_t; // platform max
    constexpr T new_value = ros::detail::to_unsigned_const<T, Chars...>();
    
    std::cout << "<new value>_f = " << new_value << std::endl;

    return std::integral_constant<T, new_value>{};
}
}

template<typename ...Fields>
void apply(Fields ...fields);
 
// template<typename Op, typename ...Ops>
// requires(is_field_v<typename Op::type> && (is_field_v<typename Ops::type> && ...)) &&
//         (std::is_same_v<typename Op::type::reg, typename Ops::type::reg> && ...)
// std::tuple<typename Op::type::value_type, typename Ops::type::value_type...> apply(Op op, Ops ...ops) {
//     typename Op::type::value_type write_mask = (Op::type::access_type == AccessType::RW? Op::type::mask : 0) | ((Ops::type::access_type == AccessType::RW? Ops::type::mask : 0) | ...);
//     std::cout << std::hex << write_mask << std::endl;
//     typename Op::type::value_type rmw_mask = [&]() {
//         auto tup = reflect::to_tuple(Op::type::reg);
//     }();
//     return std::make_tuple(op.value, ops.value...);
// }

// template<typename Field, typename ...Fields>
// requires(is_field_v<Field> && (is_field_v<Fields> && ...)) &&
//         (std::is_same_v<typename Field::reg, typename Fields::reg> && ...)
// std::tuple<typename Field::value_type, typename Fields::value_type...> apply(Field field, Fields ...fields) {
    // optimization cases
    // 1. No read needed
    //   (a) there's only one RW field (goto)
    //   (b) all of the RW fields are written
    // 2. No more than one write to the same field allowed

    // need to learn how to filter a parameter pack
    // filter based on a RW AccessType and examine size_of...

    // check all RW fields of the register are written
    // and partial masks of the fields and compare with the reg layout?
    //   maybe will need to exlude RO fields from the layout
    // need to learn how to iterate over a struct to create a layout first

    // finally enforcement rules

    // to be filtered out by read
//     return std::make_tuple(field.value, fields.value...);
// }

template<>
void apply() {};

template <typename T, typename Reg, unsigned msb, unsigned lsb, ros::AccessType AT>
concept SafeAssignable = requires {
    requires std::unsigned_integral<T>;
} && ( 
    std::is_convertible_v<typename Reg::value_type, T> &&
    std::numeric_limits<T>::digits >= msb - lsb
);

template <typename T, typename Reg, unsigned msb, unsigned lsb, ros::AccessType AT>
requires SafeAssignable<T, Reg, msb, lsb, AT>
constexpr T& operator<= (T & lhs, ros::field<Reg, msb, lsb, AT> const& rhs) {
    // can call apply from here
    lhs = rhs.value;
    return lhs;
}


// how does this work???

namespace reflect {
struct UniversalType {
    template<typename T>
    operator T() {}
};

template<typename T>
consteval auto MemberCounter(auto ...Members) {
    if constexpr (requires { T{ Members... }; } == false)
        return sizeof...(Members) - 1 - 1; // ros::reg has address member
    else
        return MemberCounter<T>(Members..., UniversalType{});
}

template <typename T>
constexpr auto forwarder(T && t) {
    return std::forward<T>(t);
}

template<typename T, unsigned S>
struct to_tuple_helper {};

template <typename T>
struct to_tuple_helper<T, 0> {
    constexpr auto operator() (T const& t) const {
        return std::make_tuple();
    }
};

template <typename T>
struct to_tuple_helper<T, 1> {
    constexpr auto operator() (T const& t) const {
        auto&& [f0] = forwarder(t);
        return std::make_tuple(f0);
    }
};

template <typename T>
struct to_tuple_helper<T, 2> {
    constexpr auto operator() (T const& t) const {
        auto&& [f0, f1] = forwarder(t);
        return std::make_tuple(f0, f1);
    }
};

template <typename T>
struct to_tuple_helper<T, 3> {
    constexpr auto operator() (T const& t) const {
        auto&& [f0, f1, f2] = forwarder(t);
        return std::make_tuple(f0, f1, f2);
    }
};

template <typename T>
struct to_tuple_helper<T, 4> {
    constexpr auto operator() (T const& t) const {
        auto&& [f0, f1, f2, f3] = forwarder(t);
        return std::make_tuple(f0, f1, f2, f3);
    }
};

template <typename T>
constexpr auto to_tuple(T const& t) {
    const unsigned ssize = MemberCounter<T>();
    return to_tuple_helper<T, ssize>{}(t);
}
}

namespace detail {
template <typename T, typename ...Ts, unsigned ...Idx>
constexpr auto get_rwm_mask_helper (std::tuple<T, Ts...> const& t, std::integer_sequence<unsigned, Idx...>) -> typename T::value_type {
    return ((std::get<Idx>(t).access_type == ros::AccessType::RW ? std::get<Idx>(t).mask : 0) | ...);
};

template <typename Reg>
constexpr typename Reg::value_type get_rmw_mask (Reg const& r) {
    auto tup = reflect::to_tuple(r);
    return get_rwm_mask_helper(tup, std::make_integer_sequence<unsigned, reflect::MemberCounter<Reg>()>{});
}
}

namespace filter {
template <typename ...Is>
struct index_sequence_concat;

template <std::size_t... Ls>
struct index_sequence_concat<std::index_sequence<Ls...>> {
    using type = std::index_sequence<Ls...>;
};

template <std::size_t... Ls, std::size_t... Rs>
struct index_sequence_concat<std::index_sequence<Ls...>, std::index_sequence<Rs...>> {
    using type = std::index_sequence<Ls..., Rs...>;
};

template <typename Is0, typename Is1, typename ...Is>
struct index_sequence_concat<Is0, Is1, Is...> {
    using type = typename index_sequence_concat<
        typename index_sequence_concat<Is0, Is1>::type, Is...>
        ::type;
};

template <bool B, std::size_t I>
struct conditional_index_sequence {
    using type = std::index_sequence<>;
};

template <std::size_t I>
struct conditional_index_sequence<true, I> {
    using type = std::index_sequence<I>;
};

template <bool B, std::size_t I>
using conditional_index_sequence_t = typename conditional_index_sequence<B, I>::type;

template <template <typename> class Predicate, typename Tuple, std::size_t... Is>
constexpr auto filtered_index_sequence_helper(const Tuple& tuple, std::index_sequence<Is...>) {
    using r = typename index_sequence_concat<
        conditional_index_sequence_t<
            Predicate<std::tuple_element_t<Is, Tuple>>::value,
            Is
        >...
    >::type;
    return r{};
}

template <template <typename> class Predicate, typename Tuple>
constexpr auto filtered_index_sequence(const Tuple& tuple) {
    return filtered_index_sequence_helper<Predicate>(tuple, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
}

template <typename Tuple, std::size_t... Is>
constexpr auto tuple_filter_helper(const Tuple& tuple, std::index_sequence<Is...>) {
    return std::make_tuple(std::get<Is>(tuple)...);
}

template <template <typename> class Predicate, typename Tuple>
auto tuple_filter(const Tuple& tuple) {
    return tuple_filter_helper(tuple, filtered_index_sequence<Predicate>(tuple));
}
}

template <typename>
struct is_field_read {
    static constexpr bool value = false;
};

template <typename Field>
struct is_field_read<ros::detail::field_read<Field>> {
    static constexpr bool value = true;
};

template <typename Field>
constexpr bool is_field_read_v = is_field_read<Field>::value;

template <typename>
struct is_field_assignment_safe {
    static constexpr bool value = false;
};

template <typename Field, typename Field::value_type val>
struct is_field_assignment_safe<ros::detail::field_assignment_safe<Field, val>> {
    static constexpr bool value = true;
};

template <typename>
struct is_field_assignment_unsafe {
    static constexpr bool value = false;
};

template <typename Field>
struct is_field_assignment_unsafe<ros::detail::field_assignment_unsafe<Field>> {
    static constexpr bool value = true;
};


// namespace ros {
template<typename Op, typename ...Ops>
requires(ros::is_field_v<typename Op::type> && (is_field_v<typename Ops::type> && ...)) &&
        (std::is_same_v<typename Op::type::reg, typename Ops::type::reg> && ...)
std::tuple<typename Op::type::value_type, typename Ops::type::value_type...> apply(Op op, Ops ...ops) {
    using value_type = typename Op::type::value_type;
    using Reg = typename Op::type::reg;

    // [TODO]: need to check if op is a write
    constexpr value_type write_mask = (Op::type::access_type == AccessType::RW? Op::type::mask : 0) | ((Ops::type::access_type == AccessType::RW? Ops::type::mask : 0) | ...);    
    constexpr value_type rmw_mask = detail::get_rmw_mask(Reg{});
    // std::cout << std::hex << write_mask << std::endl;
    // std::cout << std::hex << rmw_mask << std::endl;
    constexpr bool is_partial_write = (rmw_mask & write_mask != rmw_mask);
    // std::cout << std::hex << is_partial_write << std::endl;
    constexpr bool return_reads = std::conjunction_v<std::is_same<Op, ros::detail::field_read<typename Op::type>>, std::is_same<Ops, ros::detail::field_read<typename Ops::type>>...>;
    // std::cout << std::hex << needs_read << std::endl;

    // cases:
    // i)   writes only -- return empty tuple
    // ii)  reads only  -- return tuple with corresponding values
    // iii) writes and reads combined -- return tuple with corresponding values

    // basic flows:
    // has writes?
    //   partial write? (needs read)
    // has reads?
    //   combined writes and reads

    // operations
    // (1) perform read for partial write
    // filter out write ops
    auto safe_writes = filter::tuple_filter<is_field_assignment_safe>(std::make_tuple(op, ops...));
    auto unsafe_writes = filter::tuple_filter<is_field_assignment_unsafe>(std::make_tuple(op, ops...));
    
    // combine writes into one value

    if constexpr (is_partial_write) {
        // read

    }

    // (2) perform write

    // (3) prepare read returns
    if constexpr (return_reads) {
        // get read ops
        auto reads = filter::tuple_filter<is_field_read>(std::make_tuple(op, ops...));
    }

    return std::make_tuple(op.value, ops.value...);
}
}

template <typename ...Ts, unsigned ...Idx>
constexpr void print_tuple_helper(std::tuple<Ts...> const& t, std::integer_sequence<unsigned, Idx...> iseq) {
    ((std::cout << std::get<Idx>(t).value << ", "),...);
}

template <typename ...Ts>
constexpr void print_tuple(std::tuple<Ts...> const& t) {
    print_tuple_helper(t, std::make_integer_sequence<unsigned, sizeof...(Ts)>{});
}

using namespace ros::literals;

struct my_reg : ros::reg<uint32_t, 0x2000> {
    ros::field<my_reg, 4, 0, ros::AccessType::RW> field0;
    ros::field<my_reg, 12, 8, ros::AccessType::RW> field1;
    ros::field<my_reg, 20, 16, ros::AccessType::RW> field2;
    ros::field<my_reg, 31, 28, ros::AccessType::RW> field3;
};


int main() {

    // std::cout << structured::MemberCounter<my_reg>() << std::endl;
    
    my_reg r0;

    // multi-field write syntax
    apply(r0.field0 = 0xf_f,
          r0.field1 = 12_f);

    // multi-field read syntax
    // auto [f2, f3] = apply(r0.field2.read(),
    //                       r0.field3.read());

    // multi-field write/read syntax
    // auto [f2, f3] = apply(r0.field0 = 0xf_f,
    //                       r0.field1 = 12_f,
    //                       r0.field2.read(),
    //                       r0.field3.read());

    // read
    // uint16_t value;
    // apply(r0.field0.read(value));
    // read-modify-write
    // apply(r0.field0([](auto v) {
    //     return v*2;
    // }));

    // need to learn how to iterate over a struct
    // write whole reg
    // checks attempts to write RO fields
    // apply(r0 = 2032_r);
    // read while reg
    // uint32_t value;
    // apply(r0.read(value));
    // read-modify-write
    // apply(r0([](auto v) {
    //     return v*2;
    // }));

    auto t = ros::reflect::to_tuple(r0);
    print_tuple(t);

    // single-field read syntax
    uint8_t v;
    // v <= r0.field0;
    
    // return v;
    return v;
}
