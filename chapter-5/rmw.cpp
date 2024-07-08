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
constexpr bool is_decimal_digit_v = Char - '0' >= 0 && Char - '0' <= 9;

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
template <typename Field>
struct field_assignment;
template <typename Field, typename Field::value_type val>
struct field_assignment_safe;
template <typename Field>
struct field_assignment_safe_runtime;
template <typename Field>
struct field_assignment_unsafe;
template <typename Field>
struct field_read;

template <typename Field>
struct unsafe_operations_handler {
    constexpr auto operator= (auto const& rhs) -> field_assignment_unsafe<Field> {
        static_assert(Field::access_type != AccessType::RO, "cannot write read-only field");
        // narrowing conversion from int to unsigned int
        return ros::detail::field_assignment_unsafe<Field>(rhs);
    }

    constexpr auto operator= (auto && rhs) -> field_assignment_unsafe<Field> {
        static_assert(Field::access_type != AccessType::RO, "cannot write read-only field");
        // narrowing conversion from int to unsigned int
        return field_assignment_unsafe<Field>(rhs);
    }
};
}

namespace error {
    
    template <typename Field, typename T = typename Field::value_type>
    using ErrorHandler = typename Field::value_type(*)(T);

    template <typename Field, typename T = typename Field::value_type>
    constexpr ErrorHandler<Field> ignoreHandler = [](T v) -> T {
        std::cout << "ignore handler with " << v << std::endl;
        return T{0};
    };
    template <typename Field, typename T = typename Field::value_type>
    constexpr ErrorHandler<Field> clampHandler = [](T v) -> T {
        std::cout << "clamp handler with " << v << std::endl;
        return T{((1 << Field::length::value) - 1)};
    };
    template <typename Field>
    constexpr ErrorHandler<Field> handle = clampHandler<Field>;
}

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
    constexpr auto operator= (std::integral_constant<U, val>) -> ros::detail::field_assignment_safe<field, val> {
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
              std::numeric_limits<T>::digits >= msb - lsb) //&&
    // requires {requires std::unsigned_integral<T>;}
    constexpr auto operator= (T const& rhs) -> ros::detail::field_assignment_safe_runtime<field> {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");

        value_type value;
        if (rhs <= mask >> lsb) {
            value = rhs;
        } else {
            value = ros::error::handle<field>(rhs);
        }

        return ros::detail::field_assignment_safe_runtime<field>{value};
    }

    template <typename T>
    requires (std::is_convertible_v<T, value_type> &&
              std::numeric_limits<T>::digits >= msb - lsb) &&
    requires {requires std::unsigned_integral<T>;} // issues warning if uncommented
    constexpr auto operator= (T && rhs) -> ros::detail::field_assignment_safe_runtime<field> {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        static_assert(std::numeric_limits<value_type>::digits >= std::numeric_limits<T>::digits, "Unsafe assignment. Assigned value type is too wide.");

        value_type value;
        if (rhs <= mask >> lsb) {
            value = rhs;
        } else {
            value = ros::error::handle<field>(rhs);
        }
        // ask Mike about narrowing conversion from int to unsigned int warning
        return ros::detail::field_assignment_safe_runtime<field>{value};
        // return ros::detail::field_assignment_safe_runtime<field>(opt_rhs);
    }

    static constexpr value_type update (value_type old_value, value_type new_value) {
        return (old_value & ~mask) | (new_value & mask);
    }

    static constexpr value_type to_reg (value_type reg_value, value_type value) {
        // [TODO] check correctness
        // static_assert(value <= (mask >> lsb), "larger field cannot be safely assigned to a narrower one");
        return (reg_value & ~mask) | (value << lsb) & mask;
    }

    static constexpr value_type to_field (value_type value) {
        return (value & mask) >> lsb;
    }

    constexpr auto read() const -> ros::detail::field_read<field> {
        return ros::detail::field_read<field>{};
    }

    ros::detail::unsafe_operations_handler<field> unsafe;
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

template <typename Field>
struct field_read {
    using type = Field;
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

// how does this work???

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

template <typename Tuple, unsigned ...Idx>
constexpr auto get_write_mask_helper (Tuple const& tup, std::integer_sequence<unsigned, Idx...>) {
    return (std::tuple_element_t<Idx, Tuple>::type::mask | ...);
};

template <typename T>
constexpr T get_write_mask (std::tuple<> const& tup) {
    return 0;
}

template <typename T, typename... Ts>
constexpr T get_write_mask (std::tuple<Ts...> const& tup) {
    return get_write_mask_helper(tup, std::make_integer_sequence<unsigned, sizeof...(Ts)>{});
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
constexpr auto filtered_index_sequence_helper(const Tuple& tuple, std::index_sequence<Is...>) {
    return index_sequence_concat_t<
        conditional_index_sequence_t<
            Predicate<std::tuple_element_t<Is, Tuple>>::value,
            Is
        >...
    >{};
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
constexpr auto tuple_filter(const Tuple& tuple) {
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
template <typename T>
T get_safe_runtime_value(T const& value) {
    return value;
}

template <typename T, typename Tuple, std::size_t... Idx>
constexpr T get_write_value_helper(T value, Tuple tup, std::index_sequence<Idx...>) {
    return (std::tuple_element_t<Idx, Tuple>::type::to_reg(value, std::get<Idx>(tup).value) | ...);
}

template <typename T, typename... Ts>
constexpr T get_write_value(T value, std::tuple<Ts...> const& tup) {
    return get_write_value_helper<T>(value, tup, std::make_index_sequence<sizeof...(Ts)>{});
}

template <typename T>
constexpr T get_write_value(T value, std::tuple<> const& tup) {
    return value;
}

template <typename T>
T get_unsafe_runtime_value(T const& value) {
    return value;
}

template <typename T>
T get_unsafe_runtime_value(T const& value, auto write, auto ...writes) {
    return decltype(write)::type::to_reg(value, write.value) | get_unsafe_runtime_value<T>(value, writes...);
}
}

// namespace ros {

template<typename ...Fields>
void apply(Fields ...fields);

// template<>
// void apply() {};

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

    value_type value{};

    constexpr bool return_reads = std::disjunction_v<std::is_same<Op, ros::detail::field_read<typename Op::type>>, std::is_same<Ops, ros::detail::field_read<typename Ops::type>>...>;

    // compile-time writes
    auto safe_writes = filter::tuple_filter<is_field_assignment_safe>(std::make_tuple(op, ops...));
    // runtime writes
    auto safe_runtime_writes = filter::tuple_filter<is_field_assignment_safe_runtime>(std::make_tuple(op, ops...));
    auto unsafe_writes = filter::tuple_filter<is_field_assignment_unsafe>(std::make_tuple(op, ops...));

    auto runtime_writes = std::tuple_cat(safe_runtime_writes, unsafe_writes);

    constexpr value_type comptime_write_mask = detail::get_write_mask<value_type>(safe_writes);
    constexpr value_type runtime_write_mask = detail::get_write_mask<value_type>(runtime_writes);
    constexpr value_type write_mask = comptime_write_mask | runtime_write_mask;

    constexpr value_type rmw_mask = detail::get_rmw_mask(Reg{});

    if constexpr (write_mask != 0) {
        constexpr bool is_partial_write = (rmw_mask & write_mask != rmw_mask);

        if constexpr (is_partial_write) {
            value = bus::template read<value_type>(Reg::address::value);
        }

        // [TODO] study efficiency of bundling together all writes
        // compile time
        value = detail::get_write_value<value_type>(value, safe_writes);
        // runtime
        value = detail::get_write_value<value_type>(value, runtime_writes);

        bus::write(value, Reg::address::value);
    } else /* if (return_reads) */ {
        // implicit because if there're no writes, the only possible op is read
        value = bus::template read<value_type>(Reg::address::value);
    }

    auto get_reads = [&value]<typename ...Ts>(std::tuple<Ts...> reads) /* -> ... */ {
        return std::make_tuple(Ts::type::to_field(value)...);
    };

    return get_reads(filter::tuple_filter<is_field_read>(std::make_tuple(op, ops...)));
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
template <typename ...Ts, unsigned ...Idx>
constexpr void print_tuple_helper(std::tuple<Ts...> const& t, std::integer_sequence<unsigned, Idx...> iseq) {
    ((std::cout << std::get<Idx>(t).value << ", "),...);
}

template <typename ...Ts>
constexpr void print_tuple(std::tuple<Ts...> const& t) {
    print_tuple_helper(t, std::make_integer_sequence<unsigned, sizeof...(Ts)>{});
}


struct mmio_bus : ros::bus {
    template <typename T, typename Addr>
    static constexpr T read(Addr address) {
        std::cout << "mmio read called on addr " << std::hex << address << std::endl;
        return T{0x0};
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
};


int main() {

    my_reg r0;

    // multi-field write syntax
    // apply(r0.field0 = 0xf_f,
    //       r0.field1 = 12_f);

    // multi-field read syntax
    // auto [f2, f3] = apply(r0.field2.read(),
    //                       r0.field3.read());
    uint32_t t = 17;
    // multi-field write/read syntax
    auto [f0, f1] = apply(r0.field0 = 0xf_f,
                          r0.field1.unsafe = 13,
                          r0.field2 = t,
                          r0.field0.read(),
                          r0.field2.read());

    // apply(r0.field0 = 13,
    //       r0.field1 = 13,
    //       r0.field2.unsafe = 2);


    // std::cout << std::hex << f0 << std::endl;
    // std::cout << std::hex << f1 << std::endl;

    // unsafe writes
    // uint64_t foo = 0x10; // ignored
    // apply(r0.field0.unsafe = 0x1,
    //       r0.field1.unsafe = foo);

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

    // auto t = ros::reflect::to_tuple(r0);
    // print_tuple(t);

    // single-field read syntax
    uint8_t v;
    v <= r0.field3;
    
    // return v;
    return v;
}
