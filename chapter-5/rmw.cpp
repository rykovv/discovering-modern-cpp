#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <tuple>
#include <typeinfo>
#include <expected>

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
constexpr bool is_decimal_digit_v = Char >= '0' && Char <= '9';

template <char ...Chars>
concept DecimalNumber = (is_decimal_digit_v<Chars> && ...);

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
constexpr bool is_x_v = Char == 'x';

template <char Char>
constexpr bool is_0_v = Char == '0';

template <char Char>
struct is_hex_char {
    static constexpr bool value = ros::detail::is_decimal_digit_v<Char> || 
    (Char >= 'A' && Char <= 'F' || Char >= 'a' && Char <= 'f');
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
            T const digit = c > '9' ? c >= 'a' ? c - 'a' + 10 : c - 'A' + 10 : c - '0';
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
template <typename Field>
struct field_assignment;
template <typename Field, typename Field::value_type val>
struct field_assignment_safe;
template <typename Field>
struct field_assignment_safe_runtime;
template <typename Field>
struct field_assignment_unsafe;
template <typename F, typename Field, typename... Fields>
struct field_assignment_invocable;
template <typename Field>
struct field_read;

template <typename Register>
struct register_assignment;
template <typename Register, typename Register::value_type val>
struct register_assignment_safe;
template <typename Register>
struct register_assignment_safe_runtime;
template <typename Register>
struct register_assignment_unsafe;
template <typename F, typename Register, typename... Registers>
struct register_assignment_invocable;
template <typename Register>
struct register_read;

template <typename Field>
struct unsafe_field_operations_handler {
    constexpr auto operator= (auto const& rhs) const -> field_assignment_unsafe<Field> {
        static_assert(Field::access_type != AccessType::RO, "cannot write read-only field");
        // narrowing conversion from int to unsigned int
        return ros::detail::field_assignment_unsafe<Field>(rhs);
    }

    constexpr auto operator= (auto && rhs) const -> field_assignment_unsafe<Field> {
        static_assert(Field::access_type != AccessType::RO, "cannot write read-only field");
        // narrowing conversion from int to unsigned int
        return field_assignment_unsafe<Field>(rhs);
    }
};

template <typename Register>
// [TODO]: decide what to do with RO fields
struct unsafe_register_operations_handler {
    constexpr auto operator= (auto const& rhs) const -> register_assignment_unsafe<Register> {
        // static_assert(Register::access_type != AccessType::RO, "cannot write read-only field");
        // narrowing conversion from int to unsigned int
        return ros::detail::register_assignment_unsafe<Register>(rhs);
    }

    constexpr auto operator= (auto && rhs) const -> register_assignment_unsafe<Register> {
        // static_assert(Register::access_type != AccessType::RO, "cannot write read-only field");
        // narrowing conversion from int to unsigned int
        return register_assignment_unsafe<Register>(rhs);
    }
};
}

namespace reflect {
struct UniversalType {
    template<typename T>
    operator T() {}
};

template<typename T>
consteval auto MemberCounter(auto ...Members) {
    if constexpr (requires { T{ Members... }; } == false) {
        return sizeof...(Members) - 2;
    } else {
        return MemberCounter<T>(Members..., UniversalType{});
    }
}

template <typename T>
constexpr auto forwarder(T && t) {
    return std::forward<T>(t);
}

template<typename T, unsigned S>
struct to_tuple_helper;

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
template <typename T, typename ...Ts, std::size_t ...Idx>
constexpr auto get_rwm_mask_helper (std::tuple<T, Ts...> const& t, std::index_sequence<Idx...>) -> typename T::value_type {
    return ((std::get<Idx>(t).access_type == ros::AccessType::RW ? std::get<Idx>(t).mask : 0) | ...);
};

template <typename Reg>
constexpr typename Reg::value_type get_rmw_mask (Reg const& r) {
    auto tup = reflect::to_tuple(r);
    return get_rwm_mask_helper(tup, std::make_index_sequence<reflect::MemberCounter<Reg>()>{});
}

template <typename Tuple, std::size_t ...Idx>
constexpr auto get_write_mask_helper (Tuple const& tup, std::index_sequence<Idx...>) {
    return (std::tuple_element_t<Idx, Tuple>::type::mask | ...);
};

template <typename T>
constexpr T get_write_mask (std::tuple<> const& tup) {
    return 0;
}

template <typename T, typename... Ts>
constexpr T get_write_mask (std::tuple<Ts...> const& tup) {
    return get_write_mask_helper(tup, std::make_index_sequence<sizeof...(Ts)>{});
}
}

namespace error {
    
    template <typename Field, typename T = typename Field::value_type>
    using FieldErrorHandler = typename Field::value_type(*)(T);

    template <typename Field, typename T = typename Field::value_type>
    constexpr FieldErrorHandler<Field> ignoreHandler = [](T v) -> T {
        std::cout << "ignore handler with " << v << std::endl;
        return T{0};
    };
    template <typename Field, typename T = typename Field::value_type>
    constexpr FieldErrorHandler<Field> clampHandler = [](T v) -> T {
        std::cout << "clamp handler with " << v << std::endl;
        return T{((1 << Field::length::value) - 1)};
    };
    template <typename Field>
    constexpr FieldErrorHandler<Field> handle = clampHandler<Field>;

    template <typename Register, typename T = typename Register::value_type>
    using RegErrorHandler = typename Register::value_type(*)(T);

    template <typename Register, typename T = typename Register::value_type>
    constexpr RegErrorHandler<Register> maskHandler = [](T v) -> T {
        std::cout << "mask handler with " << v << std::endl;
        constexpr T rmw_mask = detail::get_rmw_mask(Register{});
        return T{v & rmw_mask};
    };
}

// [TODO]: Add disjoint field support

template <typename Reg, unsigned msb, unsigned lsb, AccessType AT>
requires FieldSelectable<typename Reg::value_type, msb, lsb>
struct field {
    using value_type = typename Reg::value_type;
    using reg = Reg;
    using bus = typename Reg::bus;
    using length = std::integral_constant<unsigned, msb - lsb>;

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

    constexpr field() = default;

    template <typename U, U val>
    requires (std::is_convertible_v<U, value_type>)
    constexpr auto operator= (std::integral_constant<U, val>) const -> ros::detail::field_assignment_safe<field, val> {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        static_assert(val <= (mask >> lsb), "assigned value greater than allowed");
        return ros::detail::field_assignment_safe<field, val>{};
    }

    // field-to-field assignment needs elaboration
    // constexpr auto operator= (auto const& rhs) -> ros::detail::field_assignment_safe<field, decltype(rhs)::value> {
    //     static_assert(access_type != AccessType::RO, "cannot write read-only field");
    //     using rhs_type = std::remove_reference_t<decltype(rhs)>;
    //     static_assert(rhs_type::length <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
    //     return ros::detail::field_assignment_safe<field, rhs.value>{};
    // }

    // constexpr auto operator= (auto && rhs) -> ros::detail::field_assignment_safe<field, decltype(rhs)::value> {
    //     static_assert(access_type != AccessType::RO, "cannot write read-only field");
    //     using rhs_type = std::remove_reference_t<decltype(rhs)>;
    //     static_assert(rhs_type::length <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
    //     return ros::detail::field_assignment_safe<field, rhs.value>{};
    // }

    // [TODO] create concept
    template <typename T>
    requires (std::is_convertible_v<T, value_type> &&
              std::numeric_limits<T>::digits >= msb - lsb) &&
    requires {requires std::unsigned_integral<T>;}
    constexpr auto operator= (T const& rhs) const -> ros::detail::field_assignment_safe_runtime<field> {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");

        return ros::detail::field_assignment_safe_runtime<field>{check(rhs)};
    }

    template <typename T>
    requires (std::is_convertible_v<T, value_type> &&
              std::numeric_limits<T>::digits >= msb - lsb) &&
    requires {requires std::unsigned_integral<T>;}
    constexpr auto operator= (T && rhs) const -> ros::detail::field_assignment_safe_runtime<field> {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");
        
        return ros::detail::field_assignment_safe_runtime<field>{check(rhs)};
    }

    constexpr auto read() const -> ros::detail::field_read<field> {
        return ros::detail::field_read<field>{};
    }

    constexpr auto operator() (std::invocable<value_type> auto f) const -> ros::detail::field_assignment_invocable<decltype(f), field, field> {
        return ros::detail::field_assignment_invocable<decltype(f), field, field>{f};
    }

    template <typename F, typename Field0, typename... Fields>
    requires std::invocable<F, typename Field0::value_type, typename Fields::value_type...>
    constexpr auto operator() (F f, Field0 f0, Fields... fs) const -> ros::detail::field_assignment_invocable<F, field, Field0, Fields...> {
        return ros::detail::field_assignment_invocable<F, field, Field0, Fields...>{f};
    }

    static constexpr value_type update (value_type old_value, value_type new_value) {
        return (old_value & ~mask) | (new_value & mask);
    }

    static constexpr value_type to_reg (value_type reg_value, value_type value) {
        return (reg_value & ~mask) | (value << lsb) & mask;
    }

    static constexpr value_type to_field (value_type value) {
        return (value & mask) >> lsb;
    }

    static constexpr value_type check (value_type value) {
        value_type safe_val;
        if (value <= mask >> lsb) {
            safe_val = value;
        } else {
            safe_val = ros::error::handle<field>(value);
        }

        return safe_val;
    }

    static constexpr ros::detail::unsafe_field_operations_handler<field> unsafe{};
};

namespace detail {
// template <typename Reg, unsigned msb, unsigned lsb, AccessType AT, typename Reg::value_type val>
template <typename Field>
struct field_assignment {
    using type = Field;
};
template <typename Field, typename Field::value_type val>
struct field_assignment_safe : field_assignment<Field> {
    static constexpr typename Field::value_type value = val;
};

template <typename Field>
struct field_assignment_safe_runtime : field_assignment<Field> {
    using value_type = typename Field::value_type;

    constexpr field_assignment_safe_runtime(value_type v)
      : value{v} {}

    value_type value;
};

template <typename Field>
struct field_assignment_unsafe : field_assignment<Field> {
    using value_type = typename Field::value_type;

    constexpr field_assignment_unsafe(value_type v)
      : value{v} {}

    value_type value;
};

template <typename F, typename FieldOp, typename Field0, typename... Fields>
struct field_assignment_invocable<F, FieldOp, Field0, Fields...> : field_assignment<FieldOp> {
    using fields = std::tuple<Field0, Fields...>;
    
    field_assignment_invocable(F f)
      : lambda_{f}
    {}

    constexpr auto operator() (typename Field0::value_type f0, typename Fields::value_type ...fs) -> typename FieldOp::value_type {
        return lambda_(f0, fs...);
    }

    F lambda_;
};

template <typename Field>
struct field_read {
    using type = Field;
};

template <typename Register>
struct register_assignment {
    using type = Register;
};

template <typename Register, typename Register::value_type val>
struct register_assignment_safe : register_assignment<Register> {
    static constexpr typename Register::value_type value = val;
};

template <typename Register>
struct register_assignment_safe_runtime : register_assignment<Register> {
    using value_type = typename Register::value_type;

    constexpr register_assignment_safe_runtime(value_type v)
      : value{v} {}

    value_type value;
};

template <typename Register>
struct register_assignment_unsafe : register_assignment<Register> {
    using value_type = typename Register::value_type;

    constexpr register_assignment_unsafe(value_type v)
      : value{v} {}

    value_type value;
};

template <typename F, typename RegisterOp, typename Register0, typename... Registers>
struct register_assignment_invocable<F, RegisterOp, Register0, Registers...> : register_assignment<RegisterOp> {
    using registers = std::tuple<Register0, Registers...>;
    
    register_assignment_invocable(F f)
      : lambda_{f}
    {}

    constexpr auto operator() (typename Register0::value_type r0, typename Registers::value_type ...rs) -> typename RegisterOp::value_type {
        return lambda_(r0, rs...);
    }

    F lambda_;
};

template <typename Register>
struct register_read {
    using type = Register;
};
}

template <typename T>
constexpr bool is_field_v = false;

template <typename Reg, unsigned msb, unsigned lsb, AccessType AT>
constexpr bool is_field_v<field<Reg, msb, lsb, AT>> = true;


// template <typename ...Ts>
// concept HasMask = requires {
//     (Ts::mask,...);
// };

// template <typename T, typename ...Ts>
// concept OfSameParentReg = (std::same_as<typename T::reg, typename Ts::reg> && ...);

// template <typename T, typename ...Ts>
// concept Fieldable = HasMask<T, Ts...> && OfSameParentReg<T, Ts...>;

// template <typename Field0, typename ...Fields>
// requires Fieldable<Field0, Fields...>
// struct fields {
//     using value_type = typename Field0::value_type;
//     static constexpr value_type layout = (Field0::mask | ... | Fields::mask);
// };

struct bus {
    template <typename T, typename Addr>
    static T read(Addr address);
    template <typename T, typename Addr>
    static void write(T val, Addr address);
};

template <typename r, typename T, typename b, std::size_t addr>
struct reg {
    using Reg = r;
    using value_type = T;
    using bus = b;
    using address = std::integral_constant<std::size_t, addr>;

    // using f = typename DerivedReg::Fields;
    // using value_type = typename DerivedReg::Fields::value_type;
    // static constexpr value_type layout = DerivedReg::Fields::layout;

    // // static constexpr bool needs_read (value_type new_value) {
    // //     return !(((F0::mask & new_value) > 0) && (((Fs::mask & new_value) > 0) && ...));
    // // }

    // static constexpr value_type update (value_type old_value, value_type new_value) {
    //     return (old_value & ~layout) | (new_value & layout);
    // }

    template <typename U, U val>
    requires (std::is_convertible_v<U, value_type>)
    constexpr auto operator= (std::integral_constant<U, val>) const -> ros::detail::register_assignment_safe<reg, val> {
        // static_assert(access_type != AccessType::RO, "cannot write read-only field");
        // static_assert(val <= (mask >> lsb), "assigned value greater than allowed");
        return ros::detail::register_assignment_safe<reg, val>{};
    }

    // [TODO] create concept
    template <typename U>
    requires std::unsigned_integral<U> && std::is_convertible_v<U, value_type>
    constexpr auto operator= (U const& rhs) const -> ros::detail::register_assignment_safe_runtime<reg> {
        // static_assert(access_type != AccessType::RO, "cannot write read-only field");
        // static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");

        // value_type value;
        // if (rhs <= mask >> lsb) {
        //     value = rhs;
        // } else {
        //     value = ros::error::handle<field>(rhs);
        // }

        return ros::detail::register_assignment_safe_runtime<reg>{rhs};
    }

    template <typename U>
    requires std::unsigned_integral<U> && std::is_convertible_v<U, value_type>
    constexpr auto operator= (T && rhs) const -> ros::detail::register_assignment_safe_runtime<reg> {
        // static_assert(access_type != AccessType::RO, "cannot write read-only field");
        // static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");

        // value_type value;
        // if (rhs <= mask >> lsb) {
        //     value = rhs;
        // } else {
        //     value = ros::error::handle<field>(rhs);
        // }
        
        return ros::detail::register_assignment_safe_runtime<reg>{rhs};
    }

    constexpr auto read() const -> ros::detail::register_read<reg> {
        return ros::detail::register_read<reg>{};
    }

    constexpr auto operator() (std::invocable<value_type> auto f) const -> ros::detail::register_assignment_invocable<decltype(f), reg, reg> {
        return ros::detail::register_assignment_invocable<decltype(f), reg, reg>{f};
    }

    template <typename F, typename Register0, typename... Registers>
    requires std::invocable<F, typename Register0::value_type, typename Registers::value_type...>
    constexpr auto operator() (F f, Register0 f0, Registers... fs) const -> ros::detail::register_assignment_invocable<F, reg, Register0, Registers...> {
        return ros::detail::register_assignment_invocable<F, reg, Register0, Registers...>{f};
    }

    static constexpr ros::detail::unsafe_register_operations_handler<reg> unsafe{};
};

namespace literals {
template <char ...Chars>
constexpr auto operator""_f () {
    using T = std::size_t; // platform max
    constexpr T new_value = ros::detail::to_unsigned_const<T, Chars...>();
    
    // std::cout << "<new value>_f = " << new_value << std::endl;

    return std::integral_constant<T, new_value>{};
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

template <typename ...Is>
using index_sequence_concat_t = typename index_sequence_concat<Is...>::type;

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
struct filtered_index_sequence {
    using type = index_sequence_concat_t<
        conditional_index_sequence_t<
            Predicate<std::tuple_element_t<Is, Tuple>>::value,
            Is
        >...
    >;
};

template <template <typename> class Predicate, typename Tuple, std::size_t... Is>
using filtered_index_sequence_t = filtered_index_sequence<Predicate, Tuple, Is...>::type;

// template <template <typename> class Predicate, typename Tuple, std::size_t... Is>
// constexpr auto filtered_index_sequence_helper(const Tuple& tuple, std::index_sequence<Is...>) {
//     return index_sequence_concat_t<
//         conditional_index_sequence_t<
//             Predicate<std::tuple_element_t<Is, Tuple>>::value,
//             Is
//         >...
//     >{};
// }

// template <template <typename> class Predicate, typename Tuple, std::size_t... Is>
// constexpr auto filtered_index_sequence_helper(const Tuple& tuple, std::index_sequence<Is...>) {
//     return index_sequence_concat_t<
//         conditional_index_sequence_t<
//             Predicate<std::tuple_element_t<Is, Tuple>>::value,
//             Is
//         >...
//     >{};
// }

// template <template <typename> class Predicate, typename Tuple>
// constexpr auto filtered_index_sequence(const Tuple& tuple) {
//     return filtered_index_sequence_helper<Predicate>(tuple, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
// }

// filtered_index_sequence_t<Predicate, Tuple, Is...>

template <typename Tuple, std::size_t... Is>
constexpr auto tuple_filter_apply(const Tuple& tuple, std::index_sequence<Is...>) {
    return std::make_tuple(std::get<Is>(tuple)...);
}

template <template <typename> class Predicate, typename Tuple, std::size_t... Is>
constexpr auto tuple_filter_helper(const Tuple& tuple, std::index_sequence<Is...>) {
    return tuple_filter_apply(tuple, filtered_index_sequence_t<Predicate, Tuple, Is...>{});
}

template <template <typename> class Predicate, typename Tuple>
constexpr auto tuple_filter(const Tuple& tuple) {
    return tuple_filter_helper<Predicate>(tuple, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
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
struct is_field_assignment_safe_runtime {
    static constexpr bool value = false;
};

template <typename Field>
struct is_field_assignment_safe_runtime<ros::detail::field_assignment_safe_runtime<Field>> {
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

template <typename...>
struct is_field_assignment_invocable : std::false_type {};

template <typename F, typename... Fields>
struct is_field_assignment_invocable<ros::detail::field_assignment_invocable<F, Fields...>> : std::true_type {};


template <typename T>
struct return_reads {
    using type = void;
};

template <typename ...FieldReads>
struct return_reads<std::tuple<FieldReads...>> {
    using type = std::tuple<typename FieldReads::type::value_type...>;
};

template <typename ...Ts>
using return_reads_t = typename return_reads<Ts...>::type;


namespace detail {
template <typename T, typename Tuple, std::size_t... Idx>
constexpr T get_write_value_helper(Tuple tup, std::index_sequence<Idx...>) {
    return (std::tuple_element_t<Idx, Tuple>::type::to_reg(T{0}, std::get<Idx>(tup).value) | ...);
}

template <typename T, typename... Ts>
constexpr T get_write_value(T value, T mask, std::tuple<Ts...> const& tup) {
    return (value & ~mask) | 
            get_write_value_helper<T>(tup, std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
constexpr T get_write_value(T value, T mask, std::tuple<> const& tup) {
    return value;
}

template <typename T, typename InvocableWrite, typename TupleFields, std::size_t... Idx>
constexpr T get_invocable_write_fields_helper(T value, InvocableWrite iw, TupleFields tup, std::index_sequence<Idx...>) {
    // get each field
    return iw(std::tuple_element_t<Idx, TupleFields>::to_field(value)...);
}

template <typename T, typename TupleInvocableWrites, std::size_t... Idx>
constexpr T get_invocable_write_value_helper(T value, TupleInvocableWrites tup, std::index_sequence<Idx...>) {
    return (std::tuple_element_t<Idx, TupleInvocableWrites>::type::to_reg( // wrap back everything to reg value
        T{0}, // pass in zero, final value will assigned with a compound mask
        std::tuple_element_t<Idx, TupleInvocableWrites>::type::check( // safety check
            get_invocable_write_fields_helper( // make invocable call with each field value
                value, // original reg value
                std::get<Idx>(tup), // invocable lambda wrapper
                typename std::tuple_element_t<Idx, TupleInvocableWrites>::fields{}, // tuple of fields
                std::make_index_sequence<
                    std::tuple_size_v<
                        typename std::tuple_element_t<Idx, TupleInvocableWrites>::fields
                        >
                    >{}
                )
            )) 
        | ...);
}

template <typename T, typename... InvocableWrites>
constexpr T get_invocable_write_value(T value, T mask, std::tuple<InvocableWrites...> const& tup) {
    return (value & ~mask) | 
            get_invocable_write_value_helper(value, tup, std::make_index_sequence<sizeof...(InvocableWrites)>{});
}

template <typename T>
constexpr T get_invocable_write_value(T value, T mask, std::tuple<> const& tup) {
    return value;
}
}

// namespace ros {

template<typename ...Fields>
void apply(Fields ...fields);

template<>
void apply() {};

template<typename Op, typename ...Ops>
concept Applicable = (
    // [TODO] refine to be a generic operation instead of field
    ros::is_field_v<typename Op::type> && (is_field_v<typename Ops::type> && ...)
) && (
    std::is_same_v<typename Op::type::reg, typename Ops::type::reg> && ...
);

template<typename Op, typename ...Ops>
requires Applicable<Op, Ops...>
auto apply(Op op, Ops ...ops) -> return_reads_t<decltype(filter::tuple_filter<is_field_read>(std::make_tuple(op, ops...)))> {
    using value_type = typename Op::type::value_type;
    using Reg = typename Op::type::reg;
    using bus = typename Reg::bus;

    // [TODO] static_assert on only one assignment operation per field per apply

    auto operations = std::make_tuple(op, ops...);
    value_type value{};

    // constexpr bool return_reads = std::disjunction_v<std::is_same<Op, ros::detail::field_read<typename Op::type>>, std::is_same<Ops, ros::detail::field_read<typename Ops::type>>...>;

    // compile-time writes
    auto safe_writes = filter::tuple_filter<is_field_assignment_safe>(operations);
    // runtime writes
    auto safe_runtime_writes = filter::tuple_filter<is_field_assignment_safe_runtime>(operations);
    auto unsafe_writes = filter::tuple_filter<is_field_assignment_unsafe>(operations);

    auto runtime_writes = std::tuple_cat(safe_runtime_writes, unsafe_writes);

    auto invocable_writes = filter::tuple_filter<is_field_assignment_invocable>(operations);

    constexpr value_type comptime_write_mask = detail::get_write_mask<value_type>(safe_writes);
    constexpr value_type runtime_write_mask = detail::get_write_mask<value_type>(runtime_writes);
    constexpr value_type invocable_write_mask = detail::get_write_mask<value_type>(invocable_writes);
    constexpr value_type write_mask = comptime_write_mask | runtime_write_mask | invocable_write_mask;

    constexpr value_type rmw_mask = detail::get_rmw_mask(Reg{});

    if constexpr (write_mask != 0) {
        constexpr bool is_partial_write = (rmw_mask & write_mask != rmw_mask);
        constexpr bool has_invocable_writes = std::tuple_size_v<decltype(invocable_writes)> > 0;

        if constexpr (is_partial_write | has_invocable_writes) {
            value = bus::template read<value_type>(Reg::address::value);
        }

        // evaluate invocables at the beginning
        // it doesn't make much sense to evaluate it at the end because it will have 
        // newly assigned values. this way just literals could be provided in the lambda
        value = detail::get_invocable_write_value(value, invocable_write_mask, invocable_writes);

        // [TODO] study efficiency of bundling together all writes
        // compile time
        value = detail::get_write_value(value, comptime_write_mask, safe_writes);
        // runtime
        value = detail::get_write_value(value, runtime_write_mask, runtime_writes);

        bus::write(value, Reg::address::value);
    } else /* if (return_reads) */ {
        // implicit because if there're no writes, the only possible op is read
        value = bus::template read<value_type>(Reg::address::value);
    }

    auto get_read_fields = [&value]<typename ...Ts>(std::tuple<Ts...> reads) /* -> ... */ {
        return std::make_tuple(Ts::type::to_field(value)...);
    };

    return get_read_fields(filter::tuple_filter<is_field_read>(operations));
}

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
    auto [val] = apply(ros::detail::field_read<ros::field<Reg, msb, lsb, AT>>{});
    lhs = val;
    return lhs;
}
}



// debug tuple print
template <typename ...Ts, std::size_t ...Idx>
constexpr void print_tuple_helper(std::tuple<Ts...> const& t, std::index_sequence<Idx...> iseq) {
    ((std::cout << std::get<Idx>(t).value << ", "),...);
}

template <typename ...Ts>
constexpr void print_tuple(std::tuple<Ts...> const& t) {
    print_tuple_helper(t, std::make_index_sequence<sizeof...(Ts)>{});
}


struct mmio_bus : ros::bus {
    template <typename T, typename Addr>
    static constexpr T read(Addr address) {
        std::cout << "mmio read called on addr " << std::hex << address << std::endl;
        return T{0xfff30201};
    }
    template <typename T, typename Addr>
    static constexpr void write(T val, Addr address) {
        std::cout << "mmio write called with " << std::hex << val << " on addr " << address << std::endl;
    }
};

using namespace ros::literals;

struct my_reg : ros::reg<my_reg, uint32_t, mmio_bus, 0x2000> {
    ros::field<my_reg, 4, 0, ros::AccessType::RW> field0;
    ros::field<my_reg, 12, 8, ros::AccessType::RW> field1;
    ros::field<my_reg, 20, 16, ros::AccessType::RW> field2;
    ros::field<my_reg, 31, 28, ros::AccessType::RO> field3;
    // ros::disjoint_field<
    //     ros::field<my_reg, 2, 0, ros::AccessType::RW>,
    //     ros::field<my_reg, 5, 4, ros::AccessType::RO>
    //     > dj_field;
} r0;


int main() {
    uint32_t t = 17;

    // multi-field write/read syntax
    auto [f0, f1] = apply(r0.field0 = 0xf_f,
                          r0.field1.unsafe = 13,
                          r0.field2 = t,
                          r0.field0.read(),
                          r0.field2.read());

    // lambda-based rmw
    apply(
        // self-referenced rmw
        r0.field1([&t](auto f1){
            return f1 & t | 0x8;
        }),
        // multi-referenced rmw
        r0.field2([](auto f0, auto f1, auto f2) {
            return f0 + f1 + f2;
        }, 
        r0.field0, r0.field1, r0.field2)
    );

    // [TODO]: Add register operations support
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

    // auto t = ros::reflect::to_tuple(r0);
    // print_tuple(t);

    // single-field read syntax
    uint8_t v;
    // v <= r0.field3;
    
    // return v;
    return v;
}
