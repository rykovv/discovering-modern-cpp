#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>
#include <type_traits>

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
struct is_digit {
    static constexpr bool value = Char - '0' > 0 && Char - '0' < 9;
};

template <char Char>
constexpr bool is_digit_v = ros::detail::is_digit<Char>::value;

template <char ...Chars>
concept IntegerDigits = (ros::detail::is_digit_v<Chars> && ...);

template <typename T, char... Chars>
requires IntegerDigits<Chars...>
[[nodiscard]] static constexpr auto to_unsigned_const() -> T {
    // FIXME: handle or fail at compile-time for invalid strings
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
}

enum class AccessType {
    RO,
    RW
};

template <typename Reg, unsigned Msb, unsigned Lsb, AccessType AT>
requires FieldSelectable<typename Reg::value_type, Msb, Lsb>
struct field {
    using value_type = typename Reg::value_type;
    using reg = Reg;
    static constexpr unsigned msb = Msb;
    static constexpr unsigned lsb = Lsb;

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
        static_assert(rhs_type::msb - rhs_type::lsb <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
        value = rhs.value;
        return *this;
    }

    constexpr auto operator= (auto && rhs) -> field & {
        static_assert(access_type != AccessType::RO, "cannot write read-only field");
        using rhs_type = std::remove_reference_t<decltype(rhs)>;
        static_assert(rhs_type::msb - rhs_type::lsb <= msb - lsb, "larger field cannot be safely assigned to a narrower one");
        value = rhs.value;
        return *this;
    }

    static constexpr value_type update (value_type old_value, value_type new_value) {
        return (old_value & ~mask) | (new_value & mask);
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
    
    // std::cout << "<new value>_f = " << new_value << std::endl;

    return std::integral_constant<T, new_value>{};
}

template<typename ...Fields>
void apply(Fields ...fields);
 
template<typename Field, typename ...Fields>
requires(is_field_v<Field> && (is_field_v<Fields> && ...)) &&
        (std::is_same_v<typename Field::reg, typename Fields::reg> && ...)
void apply(Field field, Fields ...fields) {
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
}

template<>
void apply() {};

}
}

template <typename T, typename Reg, unsigned Msb, unsigned Lsb, ros::AccessType AT>
requires (std::unsigned_integral<T> && 
          std::is_convertible_v<typename Reg::value_type, T>) &&
          (std::numeric_limits<T>::digits >= Msb - Lsb) // unsafe assignement
constexpr T& operator<= (T & lhs, const ros::field<Reg, Msb, Lsb, AT> & rhs) {
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
    apply(r0.field0 = 15_f,
          r0.field1 = 12_f);

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
    uint8_t v; 
    v <= r0.field0;
    // return v;
    return v;
}
