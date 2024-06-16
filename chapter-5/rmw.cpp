#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <tuple>

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

    constexpr field()
        : value{0}
    {}

    template <typename U, U val>
    requires (std::is_convertible_v<U, value_type>)
    constexpr auto operator= (std::integral_constant<U, val>) -> field & {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        static_assert(val <= (mask >> lsb), "assigned value greater than allowed");
        value = val;
        return *this;
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

    constexpr auto read() const {
        // to be substituted with read template expr
        return *this;
    }

    value_type value;
};

template <typename T>
constexpr bool is_field_v = false;

template <typename Reg, unsigned msb, unsigned lsb, AccessType AT>
constexpr bool is_field_v<field<Reg, msb, lsb, AT>> = true;


// template <typename ...Ts>
// concept MaskTrait = requires {
//     (Ts::mask,...);
// };

// template <typename T, typename ...Ts>
// concept OfSameValueType = (std::same_as<typename T::value_type, typename Ts::value_type> && ...);

// template <typename T, typename ...Ts>
// concept Fieldable = MaskTrait<T, Ts...> && OfSameValueType<T, Ts...>;

// template <typename Field0, typename ...Fields>
// requires Fieldable<Field0, Fields...>
// struct rmw_fields {
//     using value_type = typename Field0::value_type;
//     static constexpr value_type layout = (Field0::mask | ... | Fields::mask);
// };

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

template<typename ...Fields>
void apply(Fields ...fields);
 
template<typename Field, typename ...Fields>
requires(is_field_v<Field> && (is_field_v<Fields> && ...)) &&
        (std::is_same_v<typename Field::reg, typename Fields::reg> && ...)
std::tuple<typename Field::value_type, typename Fields::value_type...> apply(Field field, Fields ...fields) {
    // optimization cases
    // 1. No read needed
    //   (a) there's only one RW field (goto)
    //   (b) all of the RW fields are written
    // 2. No more than one write to the same field allowed
    // 3. No more than one read to the same field/reg allowed

    // need to learn how to filter a parameter pack
    // filter based on a RW AccessType and examine size_of...

    // check all RW fields of the register are written
    // and partial masks of the fields and compare with the reg layout?
    //   maybe will need to exlude RO fields from the layout
    // need to learn how to iterate over a struct to create a layout first

    // finally enforcement rules

    // to be filtered out by read
    return std::make_tuple(field.value, fields.value...);
}

template<>
void apply() {};

}
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
    // can call apply from here
    lhs = rhs.value;
    return lhs;
}

struct my_reg : ros::reg<uint32_t, 0x2000> {
    ros::field<my_reg, 4, 0, ros::AccessType::RW>   field0;
    ros::field<my_reg, 12, 8, ros::AccessType::RW>  field1;
    ros::field<my_reg, 20, 16, ros::AccessType::RW> field2;
    ros::field<my_reg, 31, 28, ros::AccessType::RW> field3;
};

using namespace ros::literals;

int main() {
    
    my_reg r0;

    // multi-field write syntax
    apply(r0.field0 = 0xf_f,
          r0.field1 = 12_f);

    // multi-field read syntax
    auto [f2, f3] = apply(r0.field2.read(),
                          r0.field3.read());

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

    // single-field read syntax
    uint8_t v;
    v <= r0.field0;
    
    // return v;
    return v;
}
