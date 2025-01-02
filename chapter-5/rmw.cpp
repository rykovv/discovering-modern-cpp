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

template <std::unsigned_integral MsbT, MsbT val>
struct msb {
    static constexpr MsbT value = val;
};

template <std::unsigned_integral LsbT, LsbT val>
struct lsb {
    static constexpr LsbT value = val;
};

template <std::unsigned_integral AddrT, AddrT val>
struct addr {
    static constexpr AddrT value = val;
};
} // namespace ros::detail

namespace detail {
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
concept field_selectable = 
(    derivable_unsigned_integral<T>
) && (
    msb.value <= std::numeric_limits<T>::digits-1 &&
    lsb.value <= std::numeric_limits<T>::digits-1
) && (
    msb.value >= lsb.value
);
} // namespace ros::detail

namespace detail {

template <char Char>
constexpr bool is_decimal_digit_v = Char >= '0' && Char <= '9';

template <char Char> constexpr bool is_delimiter_v = Char == '\'';

template <char ...Chars>
concept decimal_number =
    ((is_decimal_digit_v<Chars> or is_delimiter_v<Chars>) and ...);

constexpr static auto is_delimiter = [](char const c) -> bool {
    return c == '\'';
};

template <typename T, char... Chars>
requires decimal_number <Chars...>
[[nodiscard]] static constexpr auto to_constant() -> T {
    constexpr T value = []() {
        constexpr std::array<char, sizeof...(Chars)> chars{Chars...};
        T sum = 0;

        for (char c : chars) {
            if (not is_delimiter(c)) {
                T const digit = c - '0';
                sum = (sum * 10) + digit;
            }
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
constexpr bool is_hex_char_v =
    (Char >= 'A' && Char <= 'F' || Char >= 'a' && Char <= 'f');

template <char Char>
constexpr bool is_hex_digit_v =
    is_decimal_digit_v<Char> || is_hex_char_v<Char> || is_delimiter_v<Char>;

template <char Char0, char Char1, char... Chars>
concept hex_number =
    is_0_v<Char0> && is_x_v<Char1> && (is_hex_digit_v<Chars> && ...);

template <typename T, char Char0, char Char1, char... Chars>
requires hex_number<Char0, Char1, Chars...>
[[nodiscard]] static constexpr auto to_constant() -> T {
    constexpr T value = []() {
        constexpr std::array<char, sizeof...(Chars)> chars{Chars...};
        T sum = 0;

        for (char c : chars) {
            if (not is_delimiter(c)) {
                /* The difference between upper and lower case in ascii table is
                    * the presence of the 6th bit */
                T const digit =
                    c <= '9' ? c - '0' : (c & 0b1101'1111) - 'A' + 10;
                sum = (sum * 16) + digit;
            }
        }

        return sum;
    }();

    return value;
}
} // namespace ros::detail

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
    using field_error_handler = T(*)(T);

    template <typename Field, typename T = typename Field::value_type>
    constexpr field_error_handler<Field> ignore_handler = [](T v) -> T {
        std::cout << "ignore handler with " << v << std::endl;
        return T{0};
    };
    template <typename Field, typename T = typename Field::value_type>
    constexpr field_error_handler<Field> clamp_handler = [](T v) -> T {
        using value_type_r = typename Field::value_type_r;
        std::cout << "clamp handler with " << static_cast<value_type_r>(v) << std::endl;
        return T{((1 << Field::length) - 1)};
    };
    template <typename Field>
    constexpr field_error_handler<Field> handle_field_error = clamp_handler<Field>;

    template <typename Register, typename T = typename Register::value_type>
    using register_error_handler = T(*)(T);

    template <typename Register, typename T = typename Register::value_type>
    constexpr register_error_handler<Register> mask_handler = [](T v) -> T {
        std::cout << "Attempt to assign read-only bits with " << v << std::endl;
        return T{v & Register::layout};
    };
    template <typename Register>
    constexpr field_error_handler<Register> handle_register_error = mask_handler<Register>;

} // namespace ros::error

// [TODO]: Add disjoint field support

namespace detail {
template <typename T>
concept field_type = std::integral<T> || std::is_enum_v<T>;

template <field_type T, T val>
struct field_value {
    static constexpr T value = val;
};

template <typename T>
concept register_type = std::integral<T>;

template <register_type T, T val>
struct register_value {
    static constexpr T value = val;
};
}

namespace detail {
// forward declaration of operations
template <typename Field>
struct field_assignment;
template <typename Field, typename Field::value_type val>
struct field_assignment_ct; // compile-time
template <typename Field>
struct field_assignment_rt; // runtime
template <typename F, typename Field, typename... Fields>
struct field_assignment_invocable;
template <typename Field>
struct field_read;

template <typename Register>
struct register_assignment;
template <typename Register, typename Register::value_type val>
struct register_assignment_ct; // compile-time
template <typename Register>
struct register_assignment_rt; // runtime
template <typename F, typename Register, typename... Registers>
struct register_assignment_invocable;
template <typename Register>
struct register_read;

template <typename Field>
struct unsafe_field_operations_handler {
    using value_type = typename Field::value_type;

    constexpr auto operator= (auto const& rhs) const -> field_assignment_rt<Field> {
        static_assert(Field::access != access_type::RO, "cannot write read-only field");
        // safe static_case because assignment overload checked type and width validity
        return ros::detail::field_assignment_rt<Field>{static_cast<value_type>(rhs)};
    }

    constexpr auto operator= (auto && rhs) const -> field_assignment_rt<Field> {
        static_assert(Field::access != access_type::RO, "cannot write read-only field");
        // safe static_case because assignment overload checked type and width validity
        return field_assignment_rt<Field>{static_cast<value_type>(rhs)};
    }

    // TODO: add compile time unsafe operations
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
    constexpr auto operator= (register_value<U, val>) const -> register_assignment_ct<Register, val> const {
        static_assert(static_cast<value_type>(val & ~Register::layout) == 0, "Attempt to assign read-only bits");
        return ros::detail::register_assignment_ct<Register, val>{};
    }

    template <typename U>
    requires std::integral<U> && std::is_convertible_v<U, value_type>
    constexpr auto operator= (U const& rhs) const -> ros::detail::register_assignment_rt<Register> {
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<U>::digits, "Unsafe assignment. Assigned value type is too wide.");

        value_type value;
        if (rhs & ~Register::layout) {
            value = ros::error::handle_register_error<Register>(rhs);
        } else {
            value = rhs;
        }

        return ros::detail::register_assignment_rt<Register>{value};
    }

    template <typename U>
    requires std::integral<U> && std::is_convertible_v<U, value_type>
    constexpr auto operator= (U && rhs) const -> ros::detail::register_assignment_rt<Register> {
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<U>::digits, "Unsafe assignment. Assigned value type is too wide.");

        value_type value;
        if (rhs & ~Register::layout) {
            value = ros::error::handle_register_error<Register>(rhs);
        } else {
            value = rhs;
        }
        
        return ros::detail::register_assignment_rt<Register>{value};
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

    constexpr auto operator= (auto const& rhs) const -> register_assignment_rt<Register> {
        // safe static_case because assignment overload checked type and width validity
        return ros::detail::register_assignment_rt<Register>{static_cast<value_type>(rhs)};
    }

    constexpr auto operator= (auto && rhs) const -> register_assignment_rt<Register> {
        // safe static_case because assignment overload checked type and width validity
        return register_assignment_rt<Register>{static_cast<value_type>(rhs)};
    }

    // TODO: Add compile time unsafe operations support
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
                static_cast<std::remove_cvref_t<decltype(std::get<Idx>(t))>::value_type_r>(std::get<Idx>(t).access) & 
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

template <typename Tup, std::size_t ...Idx>
constexpr bool check_rw_fields_helper (access_type at, Tup const& t, std::index_sequence<Idx...>) {
    return (
        (
            (
                static_cast<uint8_t>(std::tuple_element_t<Idx, Tup>::access) & 
                static_cast<uint8_t>(ros::access_type::RW)
            ) == static_cast<uint8_t>(at)
        ) | ...
    );
};

template <typename reg>
constexpr bool check_wo_fields (reg const& r) {
    auto tup = reflect::to_tuple(r);
    constexpr std::size_t tup_size = std::tuple_size_v<decltype(tup)>;
    return check_rw_fields_helper(ros::access_type::WO, tup, std::make_index_sequence<tup_size>{});
}

template <typename reg>
constexpr bool check_ro_fields (reg const& r) {
    auto tup = reflect::to_tuple(r);
    constexpr std::size_t tup_size = std::tuple_size_v<decltype(tup)>;
    return check_rw_fields_helper(ros::access_type::RO, tup, std::make_index_sequence<tup_size>{});
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
          detail::field_type value_type_f = typename reg_derived::value_type>
requires detail::field_selectable<value_type_f, msb, lsb>
struct field {
    using value_type_r = typename reg_derived::value_type;
    using value_type = value_type_f;
    using reg = reg_derived;

    static constexpr access_type access = at;

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
    constexpr auto operator= (detail::field_value<U, val>) const -> ros::detail::field_assignment_ct<field, val> {
        static_assert((
            static_cast<value_type_r>(access) & 
            static_cast<value_type_r>(access_type::W)) != 0, 
            "cannot write read-only or NA field");
        static_assert(val <= (mask >> lsb.value), "assigned value greater than allowed");
        return ros::detail::field_assignment_ct<field, val>{};
    }

    // constexpr auto operator= (field_type auto val) const -> ros::detail::field_assignment_rt<field> {
    //     static_assert(access_type != access_type::RO, "cannot write read-only field");
    //     static_assert(val <= (mask >> lsb.value), "assigned value greater than allowed");
    //     return ros::detail::field_assignment_rt<field>{val};
    // }

    // field-to-field assignment needs elaboration
    // constexpr auto operator= (auto const& rhs) -> ros::detail::field_assignment_ct<field, decltype(rhs)::value> {
    //     static_assert(access_type != access_type::RO, "cannot write read-only field");
    //     using rhs_type = std::remove_reference_t<decltype(rhs)>;
    //     static_assert(rhs_type::length <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
    //     return ros::detail::field_assignment_ct<field, rhs.value>{};
    // }

    // constexpr auto operator= (auto && rhs) -> ros::detail::field_assignment_ct<field, decltype(rhs)::value> {
    //     static_assert(access_type != access_type::RO, "cannot write read-only field");
    //     using rhs_type = std::remove_reference_t<decltype(rhs)>;
    //     static_assert(rhs_type::length <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
    //     return ros::detail::field_assignment_ct<field, rhs.value>{};
    // }

    // [TODO] create concept
    template <typename T>
    requires (std::unsigned_integral<T> &&
              std::is_convertible_v<T, value_type> &&
              std::numeric_limits<T>::digits >= msb.value - lsb.value)
    constexpr auto operator= (T const& rhs) const -> ros::detail::field_assignment_rt<field> {
        static_assert((static_cast<value_type_r>(access) & static_cast<value_type_r>(access_type::W)) != 0, "cannot write read-only field");
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");

        return ros::detail::field_assignment_rt<field>{runtime_check(rhs)};
    }

    template <typename T>
    requires (std::unsigned_integral<T> &&
              std::is_convertible_v<T, value_type> &&
              std::numeric_limits<T>::digits >= msb.value - lsb.value)
    constexpr auto operator= (T && rhs) const -> ros::detail::field_assignment_rt<field> {
        static_assert((static_cast<value_type_r>(access) & static_cast<value_type_r>(access_type::W)) != 0, "cannot write read-only field");
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");
        
        return ros::detail::field_assignment_rt<field>{runtime_check(rhs)};
    }

    template <typename EnumT>
    requires (std::is_enum_v<EnumT>)
    constexpr auto operator= (EnumT val) const -> ros::detail::field_assignment_rt<field> {
        static_assert((static_cast<value_type_r>(access) & static_cast<value_type_r>(access_type::W)) != 0, "cannot write read-only field");
        // static_assert(static_cast<value_type_r>(val) <= (mask >> lsb.value), "assigned value greater than allowed");
        return ros::detail::field_assignment_rt<field>{runtime_check(val)};
    }

    constexpr auto read() const -> ros::detail::field_read<field> {
        static_assert((static_cast<value_type_r>(access) & static_cast<value_type_r>(access_type::R)) != 0, "cannot read write-only or NA field");
        return ros::detail::field_read<field>{};
    }

    constexpr auto operator() (std::invocable<value_type> auto f) const -> ros::detail::field_assignment_invocable<decltype(f), field, field> {
        static_assert((static_cast<value_type_r>(access) & static_cast<value_type_r>(access_type::RW)) != 0, "cannot read and write read/write-only or NA field");
        return ros::detail::field_assignment_invocable<decltype(f), field, field>{f};
    }

    template <typename F, typename Field0, typename... Fields>
    requires std::invocable<F, typename Field0::value_type, typename Fields::value_type...>
    constexpr auto operator() (F f, Field0 f0, Fields... fs) const -> ros::detail::field_assignment_invocable<F, field, Field0, Fields...> {
        static_assert((static_cast<value_type_r>(access) & static_cast<value_type_r>(access_type::RW)) != 0, "cannot read and write read/write-only or NA field");
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
            safe_val = ros::error::handle_field_error<field>(value);
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
struct field_assignment_ct : field_assignment<Field> {
    static constexpr typename Field::value_type value = val;
};

template <typename Field>
struct field_assignment_rt : field_assignment<Field> {
    using value_type = typename Field::value_type;

    constexpr field_assignment_rt(value_type v)
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
struct register_assignment_ct : register_assignment<Register> {
    static constexpr typename Register::value_type value = val;
};

template <typename Register>
struct register_assignment_rt : register_assignment<Register> {
    using value_type = typename Register::value_type;

    constexpr register_assignment_rt(value_type v)
      : value{v} {}

    value_type value;
};

template <typename F, typename RegisterOp, typename Register0, typename... Registers>
struct register_assignment_invocable<F, RegisterOp, Register0, Registers...> : register_assignment<RegisterOp> {
    using registerOp = RegisterOp;
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

template <typename Reg, detail::msb msb, detail::lsb lsb, access_type AT, detail::field_type field_t>
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

template <typename reg_derived, detail::register_type T, detail::addr addr, typename bus_t>
struct reg {
    using reg_der = reg_derived;
    using value_type = T;
    using bus = bus_t;
    using address = std::integral_constant<std::size_t, addr.value>;

    static constexpr value_type layout = detail::get_rmw_mask(reg_der{});
    static constexpr bool has_wo_field = detail::check_wo_fields(reg_der{});
    static constexpr bool has_ro_field = detail::check_ro_fields(reg_der{});

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
    constexpr T new_value = ros::detail::to_constant<T, Chars...>();
    
    // std::cout << "<new value>_f = " << new_value << std::endl;

    return detail::field_value<T, new_value>{};
}

template <char ...Chars>
constexpr auto operator""_r () {
    using T = std::size_t; // platform max
    constexpr T new_value = ros::detail::to_constant<T, Chars...>();
    
    // std::cout << "<new value>_f = " << new_value << std::endl;

    return detail::register_value<T, new_value>{};
}

template <char ...Chars>
constexpr auto operator""_msb () {
    constexpr unsigned new_value = ros::detail::to_constant<unsigned, Chars...>();

    return detail::msb<unsigned, new_value>{};
}

template <char ...Chars>
constexpr auto operator""_lsb () {
    constexpr unsigned new_value = ros::detail::to_constant<unsigned, Chars...>();

    return detail::lsb<unsigned, new_value>{};
}

template <char ...Chars>
constexpr auto operator""_addr () {
    constexpr std::size_t new_value = ros::detail::to_constant<std::size_t, Chars...>();

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
struct is_field_assignment_ct {
    static constexpr bool value = false;
};

template <typename Field, typename Field::value_type val>
struct is_field_assignment_ct<ros::detail::field_assignment_ct<Field, val>> {
    static constexpr bool value = true;
};

template <typename>
struct is_field_assignment_rt {
    static constexpr bool value = false;
};

template <typename Field>
struct is_field_assignment_rt<ros::detail::field_assignment_rt<Field>> {
    static constexpr bool value = true;
};

template <typename...>
struct is_field_assignment_invocable : std::false_type {};

template <typename F, typename... Fields>
struct is_field_assignment_invocable<ros::detail::field_assignment_invocable<F, Fields...>> : std::true_type {};


template <typename...>
struct is_register_assignment_ct : std::false_type {};

template <typename Register, typename Register::value_type val>
struct is_register_assignment_ct<ros::detail::register_assignment_ct<Register, val>> : std::true_type {};

template <typename>
struct is_register_assignment_rt : std::false_type {};

template <typename Register>
struct is_register_assignment_rt<ros::detail::register_assignment_rt<Register>> : std::true_type {};

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

template<typename InvocableWrite, std::size_t ...Is>
constexpr void evaluate_invocable_assignment_helper(InvocableWrite iw, std::index_sequence<Is...>) {
    using registerOp = typename InvocableWrite::registerOp;
    using busOp = registerOp::bus;
    using registers = typename InvocableWrite::registers;

    busOp::write(iw(
        // call bus::read that corresponds to each register
        std::tuple_element_t<Is, registers>::bus::template read<
            typename std::tuple_element_t<Is, registers>::value_type
        >(std::tuple_element_t<Is, registers>::address::value)
        ...), 
        registerOp::address::value
    );
}

template<typename InvocableWrite>
constexpr void evaluate_invocable_assignment(InvocableWrite iw) {
    using registers = typename InvocableWrite::registers;
    evaluate_invocable_assignment_helper(iw, std::make_index_sequence<std::tuple_size_v<registers>>{});
}

template<typename ...InvocableWrites, std::size_t ...Is>
constexpr void evaluate_invocable_assignments_helper(std::tuple<InvocableWrites...> writes, std::index_sequence<Is...>) {
    (evaluate_invocable_assignment(std::get<Is>(writes)), ...);
}

template<typename ...InvocableWrites>
constexpr void evaluate_invocable_assignments(std::tuple<InvocableWrites...> writes) {
    constexpr bool ro_write_attempt = (InvocableWrites::type::has_ro_field or ...);
    static_assert(not ro_write_attempt, "Attemp to write read-only register");
    evaluate_invocable_assignments_helper(writes, std::make_index_sequence<sizeof...(InvocableWrites)>{});
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
concept one_assignment_per_field = one_field_assignment_per_apply_v<Ops...>;

template<typename Op, typename ...Ops>
concept same_register = (std::is_same_v<typename Op::type::reg, typename Ops::type::reg> && ...);

template<typename ...Ops>
concept field_operations = (is_field_v<typename Ops::type> && ...);

template<typename Op, typename ...Ops>
concept field_constraints = 
    field_operations<Op, Ops...> and 
    same_register<Op, Ops...> and
    one_assignment_per_field<Op, Ops...>;


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

template<typename ...Ops>
concept one_assignment_per_register = one_register_assignment_per_apply_v<Ops...>;

template<typename ...Ops>
concept register_operations = (is_reg_v<typename Ops::type> && ...);

template<typename Op, typename ...Ops>
concept register_constraints = 
    register_operations<Op, Ops...> and
    one_assignment_per_register<Op, Ops...>;
}

// default overload
void apply() {};

template<typename Op, typename ...Ops>
requires detail::field_constraints<Op, Ops...>
auto apply(Op op, Ops ...ops) -> return_reads_t<decltype(filter::tuple_filter<is_field_read>(std::make_tuple(op, ops...)))> {
    using value_type = typename Op::type::value_type_r;
    using reg = typename Op::type::reg;
    using bus = typename reg::bus;

    constexpr value_type rmw_mask = reg::layout;

    auto operations = std::make_tuple(op, ops...);

    value_type value{};

    // compile-time writes
    auto writes_ct = filter::tuple_filter<is_field_assignment_ct>(operations);
    // runtime writes
    auto writes_rt = filter::tuple_filter<is_field_assignment_rt>(operations);
    auto writes_inv = filter::tuple_filter<is_field_assignment_invocable>(operations);

    constexpr value_type write_mask_ct = detail::get_write_mask<value_type>(writes_ct);
    constexpr value_type write_mask_rt = detail::get_write_mask<value_type>(writes_rt);
    constexpr value_type write_mask_inv = detail::get_write_mask<value_type>(writes_inv);
    constexpr value_type write_mask = write_mask_ct | write_mask_rt | write_mask_inv;

    if constexpr (write_mask != 0) {
        static_assert(not reg::has_ro_field, "Attempt to write read-only register");

        constexpr bool is_partial_write = ((rmw_mask & write_mask) != rmw_mask);
        constexpr bool has_invocable_writes = std::tuple_size_v<decltype(writes_inv)> > 0;

        if constexpr (is_partial_write || has_invocable_writes) {
            static_assert(not reg::has_wo_field, "Attempt to read write-only register");
            value = bus::template read<value_type>(reg::address::value);
        }

        // evaluate invocables at the beginning
        // it doesn't make much sense to evaluate it at the end because it will have 
        // newly assigned values. this way just literals could be provided in the lambda
        value = detail::get_invocable_write_value(value, write_mask_inv, writes_inv);

        // [TODO] study efficiency of bundling together all writes
        // compile time
        value = detail::get_write_value(value, write_mask_ct, writes_ct);
        // runtime
        value = detail::get_write_value(value, write_mask_rt, writes_rt);

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
requires detail::register_constraints<Op, Ops...>
auto apply(Op op, Ops ...ops) -> return_reads_t<decltype(filter::tuple_filter<is_register_read>(std::make_tuple(op, ops...)))> {
    // 1. evaluate reads if any (may make sense to sort)
    // 2. evaluate invocable writes (doesn't make sense to sort. the point of sorting
    //    is to potentially optimize bus utilization. read operations interleaved with
    //    writes will not make it possible. on the other hand, there's not enough
    //    weight on the side of keeping temporal values of the arguments until issuing
    //    all of the writes at once. That also will make writes handling more complex.)
    // 3. evaluate writes (may make sense to sort)

    // write whole reg
    // checks attempts to write RO fields

    auto operations = std::make_tuple(op, ops...);

    // compile-time writes
    auto writes_ct = filter::tuple_filter<is_register_assignment_ct>(operations);
    // runtime writes
    auto writes_rt = filter::tuple_filter<is_register_assignment_rt>(operations);
    auto writes_inv = filter::tuple_filter<is_register_assignment_invocable>(operations);
    // reads
    auto reads = filter::tuple_filter<is_register_read>(operations);

    constexpr bool has_writes_ct = std::tuple_size_v<decltype(writes_ct)> > 0; 
    constexpr bool has_writes_rt = std::tuple_size_v<decltype(writes_rt)> > 0; 
    constexpr bool has_writes_inv = std::tuple_size_v<decltype(writes_inv)> > 0; 

    // first, make all reads for old values
    //   cluster adjucent reads into separate tuples
    //   call read_bundle to each tuple
    // if there's a write and read for the same register old read
    //   value will be returned

    auto evaluate_reads = []<typename ...Rs>(std::tuple<Rs...>) /* -> ... */ {
        constexpr bool wo_read_attempt = (Rs::type::has_wo_field or ...);
        static_assert(not wo_read_attempt, "Attemp to read write-only register");

        return std::make_tuple(
            Rs::type::bus::template read<typename Rs::type::value_type> (
                Rs::type::address::value)
        ...);
    };
 
    if constexpr (has_writes_inv) {
        detail::evaluate_invocable_assignments(writes_inv);
    }
    
    // third, cluster adjacent writes into separate tuples
    //   call write bundled for each tuple

    // writes are peformed separately. the intent is to get jem
    //   out of compile-time accessible writes, and then let
    //   the compiler to optimize the runtime writes

    if constexpr (has_writes_ct) {
        []<typename ...Ws>(std::tuple<Ws...>) -> void {
            constexpr bool ro_write_attempt = (Ws::type::has_ro_field or ...);
            static_assert(not ro_write_attempt, "Attemp to write read-only register");

            (Ws::type::bus::write(Ws::value, Ws::type::address::value),...);
        }(writes_ct);
    }

    if constexpr (has_writes_rt) {
        []<typename ...Ws>(std::tuple<Ws...> ws) -> void {
            constexpr bool ro_write_attempt = (Ws::type::has_ro_field or ...);
            static_assert(not ro_write_attempt, "Attemp to write read-only register");

            (Ws::type::bus::write(std::get<Ws>(ws).value, Ws::type::address::value),...);
        }(writes_rt);
    }

    return evaluate_reads(reads);
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

struct my_reg1 : ros::reg<my_reg, uint32_t, 0x3000_addr, mmio_bus> {
    ros::field<my_reg, 31_msb, 0_lsb, ros::access_type::RW> field0;
} r1;


int main() {
    uint32_t t = 17;

    // multi-field write/read syntax
    auto [f0, f1] = apply(r0.field0 = 0x0'f_f,
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

    // simple op
    apply(r0.self = 0xf00);
    apply(r0.self = 0xf0'0_r);
    // multi-write
    apply(r0.self = 0xf00_r,
          r1.self = 0xf0'1_r);
    // simple read
    auto [r0_val] = apply(r0.self.read());
    // write and read
    auto [r1_val0] = apply(r0.self = 0xf00_r,
          r1.self.read());
    auto [r1_val1] = apply(r0.self = 0xf00_r,
          r1.self = 0xf01_r,
          r1.self.read()); // receives old value of r1 (before write)
    apply(r0.self = t,
          r1.self = 0xbeef);


    // rmw
    apply(r0.self([](auto old_r0) {
        return old_r0 | 0x3;
    }));

    apply(
        r0.self([](auto old_r0, auto r1) {
            return old_r0 & r1 & 0xf00000;
        }, 
        r0.self, r1.self)
    );


    return 0;
}
