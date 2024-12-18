#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <tuple>
#include <typeinfo>

// Register Optimization with Safety (ROS)

namespace ros {
namespace detail {
template <typename T>
concept Msb = std::unsigned_integral<T>;

template <Msb MsbT, MsbT val>
struct msb {
    static constexpr MsbT value = val;
};

template <typename T>
concept Lsb = std::unsigned_integral<T>;

template <Lsb LsbT, LsbT val>
struct lsb {
    static constexpr LsbT value = val;
};

template <typename T>
concept Addr = std::unsigned_integral<T>;

template <Addr AddrT, AddrT val>
struct addr {
    static constexpr AddrT value = val;
};
}

template <typename T, typename enable = void>
struct unwrap_enum {
    using type = T;
};

template <typename T>
struct unwrap_enum<T, typename std::enable_if_t<std::is_enum_v<T>>> {
    using type = std::underlying_type_t<T>;
};

template <typename T>
using unwrap_enum_t = typename unwrap_enum<T>::type;

template <typename T>
concept derivable_unsigned_integral = std::unsigned_integral<unwrap_enum_t<T>>;

template <typename T, detail::msb msb, detail::lsb lsb>
concept FieldSelectable = 
(    derivable_unsigned_integral<T>
) && (
    msb.value <= std::numeric_limits<T>::digits-1 &&
    lsb.value <= std::numeric_limits<T>::digits-1
) && (
    msb.value >= lsb.value
);

template <typename T>
concept FieldType = std::integral<T> || std::is_enum_v<T>;

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

enum class access_type : uint8_t {
    NA    = 0b000'00000,
    RO    = 0b000'00001,
    WO    = 0b000'00010,
    RW    = 0b000'00011,
    RW_0C = 0b000'00111,
    RW_1C = 0b000'01011,
    RW_1S = 0b000'10011,
    R = RO,
    W = WO
};

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
        using value_type_r = typename Field::value_type_r;
        std::cout << "clamp handler with " << static_cast<value_type_r>(v) << std::endl;
        return T{((1 << Field::length) - 1)};
    };
    template <typename Field>
    constexpr FieldErrorHandler<Field> handle = clampHandler<Field>;

    template <typename Register, typename T = typename Register::value_type>
    using RegErrorHandler = typename Register::value_type(*)(T);

    template <typename Register, typename T = typename Register::value_type>
    constexpr RegErrorHandler<Register> maskHandler = [](T v) -> T {
        std::cout << "Attempt to assign read-only bits with " << v << std::endl;
        return T{v & Register::layout};
    };
}

// [TODO]: Add disjoint field support

namespace detail {
template <FieldType T, T val>
struct field_value {
    static constexpr T value = val;
};

template <typename T>
concept RegisterType = std::integral<T>;

template <RegisterType T, T val>
struct register_value {
    static constexpr T value = val;
};
}

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
    using value_type = typename Field::value_type;

    constexpr auto operator= (auto const& rhs) const -> field_assignment_unsafe<Field> {
        static_assert(Field::access_type != access_type::RO, "cannot write read-only field");
        // safe static_case because assignment overload checked type and width validity
        return ros::detail::field_assignment_unsafe<Field>{static_cast<value_type>(rhs)};
    }

    constexpr auto operator= (auto && rhs) const -> field_assignment_unsafe<Field> {
        static_assert(Field::access_type != access_type::RO, "cannot write read-only field");
        // safe static_case because assignment overload checked type and width validity
        return field_assignment_unsafe<Field>{static_cast<value_type>(rhs)};
    }
};

template <typename Register>
struct safe_register_operations_handler {
    using reg = Register;
    using value_type = typename Register::value_type;

    constexpr auto read() const -> ros::detail::register_read<Register> {
        return ros::detail::register_read<Register>{};
    }

    template <typename U, U val>
    requires (std::is_convertible_v<U, value_type>)
    constexpr auto operator= (register_value<U, val>) const -> register_assignment_safe<Register, val> const {
        static_assert(static_cast<value_type>(val & ~Register::layout) == 0, "Attempt to assign read-only bits");
        return ros::detail::register_assignment_safe<Register, val>{};
    }

    template <typename U>
    requires std::integral<U> && std::is_convertible_v<U, value_type>
    constexpr auto operator= (U const& rhs) const -> ros::detail::register_assignment_safe_runtime<Register> {
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<U>::digits, "Unsafe assignment. Assigned value type is too wide.");

        value_type value;
        if (rhs & ~Register::layout) {
            value = ros::error::maskHandler<Register>(rhs);
        } else {
            value = rhs;
        }

        return ros::detail::register_assignment_safe_runtime<Register>{value};
    }

    template <typename U>
    requires std::integral<U> && std::is_convertible_v<U, value_type>
    constexpr auto operator= (U && rhs) const -> ros::detail::register_assignment_safe_runtime<Register> {
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<U>::digits, "Unsafe assignment. Assigned value type is too wide.");

        value_type value;
        if (rhs & ~Register::layout) {
            value = ros::error::maskHandler<Register>(rhs);
        } else {
            value = rhs;
        }
        
        return ros::detail::register_assignment_safe_runtime<Register>{value};
    }

    constexpr auto operator() (std::invocable<value_type> auto f) const -> ros::detail::register_assignment_invocable<decltype(f), Register, Register> {
        return ros::detail::register_assignment_invocable<decltype(f), Register, Register>{f};
    }

    template <typename F, typename RegOpsHandlerT0, typename... RegOpsHandlerTs>
    requires std::invocable<F, typename RegOpsHandlerT0::reg::value_type, typename RegOpsHandlerTs::reg::value_type...>
    constexpr auto operator() (F f, RegOpsHandlerT0 rh0, RegOpsHandlerTs... rhs) const -> ros::detail::register_assignment_invocable<F, Register, typename RegOpsHandlerT0::reg, typename RegOpsHandlerTs::reg...> {
        return ros::detail::register_assignment_invocable<F, Register, typename RegOpsHandlerT0::reg, typename RegOpsHandlerTs::reg...>{f};
    }
};

template <typename Register>
struct unsafe_register_operations_handler {
    using value_type = typename Register::value_type;

    constexpr auto operator= (auto const& rhs) const -> register_assignment_unsafe<Register> {
        // safe static_case because assignment overload checked type and width validity
        return ros::detail::register_assignment_unsafe<Register>{static_cast<value_type>(rhs)};
    }

    constexpr auto operator= (auto && rhs) const -> register_assignment_unsafe<Register> {
        // safe static_case because assignment overload checked type and width validity
        return register_assignment_unsafe<Register>{static_cast<value_type>(rhs)};
    }
};
}

namespace reflect {
struct UniversalType {
    template<typename T>
    operator T() {}
};

template<typename T>
consteval auto get_struct_size(auto ...Members) {
    if constexpr (requires { T{ Members... }; } == false) {
        return sizeof...(Members) - 2; // self and unsafe members
    } else {
        return get_struct_size<T>(Members..., UniversalType{});
    }
}

template <typename T>
constexpr auto forward(T && t) {
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
        auto&& [f0] = forward(t);
        return std::make_tuple(f0);
    }
};

template <typename T>
struct to_tuple_helper<T, 2> {
    constexpr auto operator() (T const& t) const {
        auto&& [f0, f1] = forward(t);
        return std::make_tuple(f0, f1);
    }
};

template <typename T>
struct to_tuple_helper<T, 3> {
    constexpr auto operator() (T const& t) const {
        auto&& [f0, f1, f2] = forward(t);
        return std::make_tuple(f0, f1, f2);
    }
};

template <typename T>
struct to_tuple_helper<T, 4> {
    constexpr auto operator() (T const& t) const {
        auto&& [f0, f1, f2, f3] = forward(t);
        return std::make_tuple(f0, f1, f2, f3);
    }
};

template <typename T>
constexpr auto to_tuple(T const& t) {
    constexpr std::size_t ss = get_struct_size<T>();
    return to_tuple_helper<T, ss>{}(t);
}
}

namespace detail {
template <typename T, typename ...Ts, std::size_t ...Idx>
constexpr auto get_rwm_mask_helper (std::tuple<T, Ts...> const& t, std::index_sequence<Idx...>) -> typename T::value_type_r {
    return (
        (
            (
                static_cast<std::remove_cvref_t<decltype(std::get<Idx>(t))>::value_type_r>(std::get<Idx>(t).access_type) & 
                static_cast<std::remove_cvref_t<decltype(std::get<Idx>(t))>::value_type_r>(ros::access_type::R)
            ) ? 
            std::get<Idx>(t).mask : 0
        ) | ...
    );
};

template <typename reg>
constexpr typename reg::value_type get_rmw_mask (reg const& r) {
    auto tup = reflect::to_tuple(r);
    constexpr std::size_t tup_size = std::tuple_size_v<decltype(tup)>;
    return get_rwm_mask_helper(tup, std::make_index_sequence<tup_size>{});
    // return get_rwm_mask_helper(tup, std::make_index_sequence<reflect::get_struct_size<Reg>()>{});
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

template <typename reg_derived, 
          detail::msb msb, detail::lsb lsb, 
          access_type at, 
          FieldType value_type_f = typename reg_derived::value_type>
requires FieldSelectable<value_type_f, msb, lsb>
struct field {
    using value_type_r = typename reg_derived::value_type;
    using value_type = value_type_f;
    using reg = reg_derived;

    static constexpr access_type access_type = at;

    static constexpr uint8_t length = []() {
        return msb.value == lsb.value ? 1 : msb.value - lsb.value;
    }();

    static constexpr value_type_r mask = []() {
        if (msb.value != lsb.value) {
            if (msb.value == std::numeric_limits<value_type_r>::digits-1) {
                return ~((1u << lsb.value) - 1);
            } else {
                return ((1u << msb.value) - 1) & ~((1u << lsb.value) - 1);
            }
        } else {
            return 1u << msb.value;
        }
    }();

    constexpr field() = default;

    template <typename U, U val>
    requires (std::is_convertible_v<U, value_type>)
    constexpr auto operator= (detail::field_value<U, val>) const -> ros::detail::field_assignment_safe<field, val> {
        static_assert((
            static_cast<value_type_r>(access_type) & 
            static_cast<value_type_r>(access_type::W)) != 0, 
            "cannot write read-only or NA field");
        static_assert(val <= (mask >> lsb.value), "assigned value greater than allowed");
        return ros::detail::field_assignment_safe<field, val>{};
    }

    // constexpr auto operator= (FieldType auto val) const -> ros::detail::field_assignment_safe_runtime<field> {
    //     static_assert(access_type != access_type::RO, "cannot write read-only field");
    //     static_assert(val <= (mask >> lsb.value), "assigned value greater than allowed");
    //     return ros::detail::field_assignment_safe_runtime<field>{val};
    // }

    // field-to-field assignment needs elaboration
    // constexpr auto operator= (auto const& rhs) -> ros::detail::field_assignment_safe<field, decltype(rhs)::value> {
    //     static_assert(access_type != access_type::RO, "cannot write read-only field");
    //     using rhs_type = std::remove_reference_t<decltype(rhs)>;
    //     static_assert(rhs_type::length <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
    //     return ros::detail::field_assignment_safe<field, rhs.value>{};
    // }

    // constexpr auto operator= (auto && rhs) -> ros::detail::field_assignment_safe<field, decltype(rhs)::value> {
    //     static_assert(access_type != access_type::RO, "cannot write read-only field");
    //     using rhs_type = std::remove_reference_t<decltype(rhs)>;
    //     static_assert(rhs_type::length <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
    //     return ros::detail::field_assignment_safe<field, rhs.value>{};
    // }

    // [TODO] create concept
    template <typename T>
    requires (std::unsigned_integral<T> &&
              std::is_convertible_v<T, value_type> &&
              std::numeric_limits<T>::digits >= msb.value - lsb.value)
    constexpr auto operator= (T const& rhs) const -> ros::detail::field_assignment_safe_runtime<field> {
        static_assert((static_cast<value_type_r>(access_type) & static_cast<value_type_r>(access_type::W)) != 0, "cannot write read-only field");
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");

        return ros::detail::field_assignment_safe_runtime<field>{runtime_check(rhs)};
    }

    template <typename T>
    requires (std::unsigned_integral<T> &&
              std::is_convertible_v<T, value_type> &&
              std::numeric_limits<T>::digits >= msb.value - lsb.value)
    constexpr auto operator= (T && rhs) const -> ros::detail::field_assignment_safe_runtime<field> {
        static_assert((static_cast<value_type_r>(access_type) & static_cast<value_type_r>(access_type::W)) != 0, "cannot write read-only field");
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");
        
        return ros::detail::field_assignment_safe_runtime<field>{runtime_check(rhs)};
    }

    template <typename EnumT>
    requires (std::is_enum_v<EnumT>)
    constexpr auto operator= (EnumT val) const -> ros::detail::field_assignment_safe_runtime<field> {
        static_assert((static_cast<value_type_r>(access_type) & static_cast<value_type_r>(access_type::W)) != 0, "cannot write read-only field");
        // static_assert(static_cast<value_type_r>(val) <= (mask >> lsb.value), "assigned value greater than allowed");
        return ros::detail::field_assignment_safe_runtime<field>{runtime_check(val)};
    }

    constexpr auto read() const -> ros::detail::field_read<field> {
        static_assert((static_cast<value_type_r>(access_type) & static_cast<value_type_r>(access_type::R)) != 0, "cannot read write-only or NA field");
        return ros::detail::field_read<field>{};
    }

    constexpr auto operator() (std::invocable<value_type> auto f) const -> ros::detail::field_assignment_invocable<decltype(f), field, field> {
        static_assert((static_cast<value_type_r>(access_type) & static_cast<value_type_r>(access_type::RW)) != 0, "cannot read and write read/write-only or NA field");
        return ros::detail::field_assignment_invocable<decltype(f), field, field>{f};
    }

    template <typename F, typename Field0, typename... Fields>
    requires std::invocable<F, typename Field0::value_type, typename Fields::value_type...>
    constexpr auto operator() (F f, Field0 f0, Fields... fs) const -> ros::detail::field_assignment_invocable<F, field, Field0, Fields...> {
        static_assert((static_cast<value_type_r>(access_type) & static_cast<value_type_r>(access_type::RW)) != 0, "cannot read and write read/write-only or NA field");
        return ros::detail::field_assignment_invocable<F, field, Field0, Fields...>{f};
    }

    static constexpr value_type_r to_reg (value_type_r reg_value, value_type value) {
        return (reg_value & ~mask) | (static_cast<value_type_r>(value) << lsb.value) & mask;
    }

    static constexpr value_type to_field (value_type_r value) {
        return (static_cast<value_type_r>(value) & mask) >> lsb.value;
    }

    static constexpr value_type runtime_check (value_type value) {
        value_type safe_val;
        if (static_cast<value_type_r>(value) <= mask >> lsb.value) {
            safe_val = value;
        } else {
            safe_val = ros::error::handle<field>(value);
        }

        return safe_val;
    }

    static constexpr ros::detail::unsafe_field_operations_handler<field> unsafe{};
};

namespace detail {
// template <typename Reg, unsigned msb, unsigned lsb, access_type AT, typename Reg::value_type val>
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

template <typename Reg, detail::msb msb, detail::lsb lsb, access_type AT, FieldType field_t>
constexpr bool is_field_v<field<Reg, msb, lsb, AT, field_t>> = true;


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
    template <typename... AdjacentAddrs, typename... ValueTypes>
    static std::tuple<ValueTypes...> read(std::tuple<AdjacentAddrs...> addrs);
    template <typename... AdjacentAddrs, typename... ValueTypes>
    static void write(std::tuple<AdjacentAddrs...> addrs, std::tuple<ValueTypes...> values);
};

template <typename reg_derived, detail::RegisterType T, detail::addr addr, typename bus_t>
struct reg {
    using reg_der = reg_derived;
    using value_type = T;
    using bus = bus_t;
    using address = std::integral_constant<std::size_t, addr.value>;

    static constexpr value_type layout = detail::get_rmw_mask(reg_der{});

    static constexpr ros::detail::unsafe_register_operations_handler<reg> unsafe{};
    static constexpr ros::detail::safe_register_operations_handler<reg> self{};
};


template <typename T>
constexpr bool is_reg_v = false;

template <typename r, typename T, typename b, detail::addr addr>
constexpr bool is_reg_v<reg<r, T, addr, b>> = true;


namespace literals {
template <char ...Chars>
constexpr auto operator""_f () {
    using T = std::size_t; // platform max
    constexpr T new_value = ros::detail::to_unsigned_const<T, Chars...>();
    
    // std::cout << "<new value>_f = " << new_value << std::endl;

    return detail::field_value<T, new_value>{};
}

template <char ...Chars>
constexpr auto operator""_r () {
    using T = std::size_t; // platform max
    constexpr T new_value = ros::detail::to_unsigned_const<T, Chars...>();
    
    // std::cout << "<new value>_f = " << new_value << std::endl;

    return detail::register_value<T, new_value>{};
}

template <char ...Chars>
constexpr auto operator""_msb () {
    constexpr unsigned new_value = ros::detail::to_unsigned_const<unsigned, Chars...>();

    return detail::msb<unsigned, new_value>{};
}

template <char ...Chars>
constexpr auto operator""_lsb () {
    constexpr unsigned new_value = ros::detail::to_unsigned_const<unsigned, Chars...>();

    return detail::lsb<unsigned, new_value>{};
}

template <char ...Chars>
constexpr auto operator""_addr () {
    constexpr std::size_t new_value = ros::detail::to_unsigned_const<std::size_t, Chars...>();

    return detail::addr<std::size_t, new_value>{};
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


template <typename...>
struct is_register_assignment_safe : std::false_type {};

template <typename Register, typename Register::value_type val>
struct is_register_assignment_safe<ros::detail::register_assignment_safe<Register, val>> : std::true_type {};

template <typename>
struct is_register_assignment_safe_runtime : std::false_type {};

template <typename Register>
struct is_register_assignment_safe_runtime<ros::detail::register_assignment_safe_runtime<Register>> : std::true_type {};

template <typename>
struct is_register_assignment_unsafe : std::false_type {};

template <typename Field>
struct is_register_assignment_unsafe<ros::detail::register_assignment_unsafe<Field>> : std::true_type {};

template <typename...>
struct is_register_assignment_invocable : std::false_type {};

template <typename F, typename... Registers>
struct is_register_assignment_invocable<ros::detail::register_assignment_invocable<F, Registers...>> : std::true_type {};


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

template <typename>
struct is_register_read {
    static constexpr bool value = false;
};

template <typename Register>
struct is_register_read<ros::detail::register_read<Register>> {
    static constexpr bool value = true;
};


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
        std::tuple_element_t<Idx, TupleInvocableWrites>::type::runtime_check( // safety check
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

namespace detail {
template <typename... Ops>
struct one_field_assignment_per_apply;

template <>
struct one_field_assignment_per_apply<> : std::true_type {};

template <typename Op>
struct one_field_assignment_per_apply<Op> : std::true_type {};

template <typename Op, typename... Ops>
struct one_field_assignment_per_apply<Op, Ops...> {
    using OpField = typename Op::type;
    static constexpr bool more_than_one = (std::is_base_of_v<detail::field_assignment<OpField>, Ops> || ...);
    static constexpr bool value = not more_than_one and one_field_assignment_per_apply<Ops...>::value;
};

template <typename... Ops>
constexpr bool one_field_assignment_per_apply_v = one_field_assignment_per_apply<Ops...>::value;

template<typename ...Ops>
concept OneFieldAssignmentPerApply = one_field_assignment_per_apply_v<Ops...>;

template<typename Op, typename ...Ops>
concept SameRegisterOperations = (std::is_same_v<typename Op::type::reg, typename Ops::type::reg> && ...);

template<typename ...Ops>
concept FieldOperations = (is_field_v<typename Ops::type> && ...);

template<typename Op, typename ...Ops>
concept ApplicableFieldOperations = 
    FieldOperations<Op, Ops...> && 
    SameRegisterOperations<Op, Ops...> && 
    OneFieldAssignmentPerApply<Op, Ops...>;


template <typename... Ops>
struct one_register_assignment_per_apply;

template <>
struct one_register_assignment_per_apply<> : std::true_type {};

template <typename Op>
struct one_register_assignment_per_apply<Op> : std::true_type {};

template <typename Op, typename... Ops>
struct one_register_assignment_per_apply<Op, Ops...> {
    using OpRegister = typename Op::type;
    static constexpr bool more_than_one = (std::is_base_of_v<detail::register_assignment<OpRegister>, Ops> || ...);
    static constexpr bool value = not more_than_one and one_register_assignment_per_apply<Ops...>::value;
};

template <typename... Ops>
constexpr bool one_register_assignment_per_apply_v = one_register_assignment_per_apply<Ops...>::value;

// template<typename ...Ops>
// concept OneRegisterAssignmentPerApply = one_register_assignment_per_apply_v<Ops...>;

template<typename ...Ops>
concept RegisterOperations = (is_reg_v<typename Ops::type> && ...);

template<typename Op, typename ...Ops>
concept ApplicableRegisterOperations = RegisterOperations<Op, Ops...>;

}

// default overload
void apply(...) {};

template<typename Op, typename ...Ops>
requires detail::ApplicableFieldOperations<Op, Ops...>
auto apply(Op op, Ops ...ops) -> return_reads_t<decltype(filter::tuple_filter<is_field_read>(std::make_tuple(op, ops...)))> {
    using value_type = typename Op::type::value_type_r;
    using reg = typename Op::type::reg;
    using bus = typename reg::bus;

    constexpr value_type rmw_mask = reg::layout;

    auto operations = std::make_tuple(op, ops...);

    value_type value{};

    // constexpr bool return_reads = std::disjunction_v<std::is_same<Op, ros::detail::field_read<typename Op::type>>, std::is_same<Ops, ros::detail::field_read<typename Ops::type>>...>;

    // compile-time writes
    auto safe_writes = filter::tuple_filter<is_field_assignment_safe>(operations);
    // runtime writes
    auto safe_writes_runtime = filter::tuple_filter<is_field_assignment_safe_runtime>(operations);
    auto unsafe_writes = filter::tuple_filter<is_field_assignment_unsafe>(operations);

    auto runtime_writes = std::tuple_cat(safe_writes_runtime, unsafe_writes);

    auto invocable_writes = filter::tuple_filter<is_field_assignment_invocable>(operations);

    constexpr value_type comptime_write_mask = detail::get_write_mask<value_type>(safe_writes);
    constexpr value_type runtime_write_mask = detail::get_write_mask<value_type>(runtime_writes);
    constexpr value_type invocable_write_mask = detail::get_write_mask<value_type>(invocable_writes);
    constexpr value_type write_mask = comptime_write_mask | runtime_write_mask | invocable_write_mask;

    if constexpr (write_mask != 0) {
        constexpr bool is_partial_write = ((rmw_mask & write_mask) != rmw_mask);
        constexpr bool has_invocable_writes = std::tuple_size_v<decltype(invocable_writes)> > 0;

        if constexpr (is_partial_write || has_invocable_writes) {
            value = bus::template read<value_type>(reg::address::value);
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

        bus::write(value, reg::address::value);
    } else /* if (return_reads) */ {
        // implicit because if there're no writes, the only possible op is read
        value = bus::template read<value_type>(reg::address::value);
    }

    auto get_read_fields = [&value]<typename ...Ts>(std::tuple<Ts...> reads) /* -> ... */ {
        return std::make_tuple(Ts::type::to_field(value)...);
    };

    return get_read_fields(filter::tuple_filter<is_field_read>(operations));
}


template<typename Op, typename ...Ops>
requires detail::ApplicableRegisterOperations<Op, Ops...>
auto apply(Op op, Ops ...ops) {// -> return_reads_t<decltype(filter::tuple_filter<is_register_read>(std::make_tuple(op, ops...)))> {
    std::cout << "reg apply" << std::endl;

    // 1. evaluate invocable writes (doesn't make sense to sort. the point of sorting
    //    is to potentially optimize bus utilization. read operations interleaved with
    //    writes will not make it possible. on the other hand, there's not enough
    //    weight on the side of keeping temporal values of the arguments until issuing
    //    all of the writes at once. That also will make writes handling more complex.)
    // 2. evaluate reads if any (may make sense to sort)
    // 3. evaluate writes (may make sense to sort)

    // write whole reg
    // checks attempts to write RO fields

    auto operations = std::make_tuple(op, ops...);

    auto invocable_writes = filter::tuple_filter<is_register_assignment_invocable>(operations);

    // compile-time writes
    auto safe_writes = filter::tuple_filter<is_register_assignment_safe>(operations);
    // runtime writes
    auto safe_writes_runtime = filter::tuple_filter<is_register_assignment_safe_runtime>(operations);
    auto unsafe_writes = filter::tuple_filter<is_register_assignment_unsafe>(operations);

    auto runtime_writes = std::tuple_cat(safe_writes_runtime, unsafe_writes);

    if constexpr (std::tuple_size_v<decltype(invocable_writes)> > 0) {
        
    }

    // if there's a write and read for the same register old read
    //   value will be returned

    // first, make all reads for old values
    //   cluster adjucent reads into separate tuples
    //   call read_bundle to each tuple
    
    // second, cluster adjacent writes into separate tuples
    //   call write bundled for each tuple

    // return reads if requiested
}


template <typename T, typename Reg, unsigned msb, unsigned lsb, ros::access_type AT>
concept SafeAssignable = requires {
    requires std::unsigned_integral<T>;
} && ( 
    std::is_convertible_v<typename Reg::value_type, T> &&
    std::numeric_limits<T>::digits >= msb - lsb
);

template <typename T, typename Reg, detail::msb msb, detail::lsb lsb, ros::access_type AT>
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

struct my_reg : ros::reg<my_reg, uint32_t, 0x2000_addr, mmio_bus> {
    enum class FieldState : uint8_t {
        ON,
        OFF
    };

    ros::field<my_reg, 4_msb, 0_lsb, ros::access_type::RW> field0;
    ros::field<my_reg, 12_msb, 4_lsb, ros::access_type::RW> field1;
    ros::field<my_reg, 28_msb, 12_lsb, ros::access_type::RW> field2;
    ros::field<my_reg, 31_msb, 28_lsb, ros::access_type::RW, FieldState> field3;
    // ros::disjoint_field<my_reg, ros::access_type::RW,
    //     ros::field_segment<2_msb, 0_lsb>,
    //     ros::field_segment<5_msb, 4_lsb>
    //     > dj_field;
} r0;

struct my_reg1 : ros::reg<my_reg, uint32_t, 0x2000_addr, mmio_bus> {
    ros::field<my_reg, 31_msb, 0_lsb, ros::access_type::RW> field0;
} r1;


int main() {
    // static_assert(std::is_enum_v<unsigned int>);
    // static_assert(std::is_integral_v<unsigned int>);
    // auto a = ros::unwrap_enum_t<my_reg::FieldState>{};
    // std::cout << typeid(a).name() << std::endl;
    // static_assert(std::is_same_v<ros::unwrap_enum_t<unsigned int>, unsigned int>);
    // static_assert(std::is_same_v<ros::unwrap_enum_t<my_reg::FieldState>, uint8_t>);
    // static_assert(std::is_integral_v<ros::unwrap_enum_t<unsigned int>>);
    uint32_t t = 17;

    // multi-field write/read syntax
    auto [f0, f1] = apply(r0.field0 = 0xf_f,
                          r0.field1.unsafe = 13,
                          r0.field2 = t,
                          r0.field3 = my_reg::FieldState::OFF,
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
    // simple op
    apply(r0.self = 0xf00_r);
    // multi-write
    apply(r0.self = 0xf00_r,
          r1.self = 0xf01_r);
    // simple read
    // auto [r0] = apply(r0.read());
    // write and read
    // auto [r1_val] = 
    apply(r0.self = 0xf00_r,
          r1.self.read());
    // auto [r1_val] = // receives old value of r1 (before write)
    apply(r0.self = 0xf00_r,
          r1.self = 0xf01_r,
          r1.self.read());
    apply(r0.self = t,
          r1.self = 0xbeef);
    // rmw
    apply(r0.self([](auto old_r0) {
        return old_r0 | 0x3;
    }));

    apply(
        r0.self([](auto old_r0, auto r1) {
            return old_r0 & r1 & 0x3;
        }, 
        r0.self, r1.self)
    );


    return 0;
}
