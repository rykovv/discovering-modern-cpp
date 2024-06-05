#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>
#include <type_traits>

struct ml {
    unsigned long long int v;
};

template <unsigned v, unsigned m>
struct rmw_assignment {
    static_assert(v <= m, "assigned value greater than allowed");
};

struct rmw_read {

};

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

template <typename Reg, typename T, unsigned msb, unsigned lsb>
requires FieldSelectable<T, msb, lsb>
struct rmw {
    using value_type = T;

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

    static constexpr value_type update (value_type old_value, value_type new_value) {
        return (old_value & ~mask) | (new_value & mask);
    }

    consteval rmw& operator= (value_type const new_value) {
        // static_assert(new_value <= (mask >> lsb), "assigned value greater than allowed");
        constexpr value_type max = mask >> lsb;
        rmw_assignment<new_value, max> rmwa;
        return *this;
    }
};

template <typename ...Ts>
concept MaskTrait = requires {
    (Ts::mask,...);
};

template <typename T, typename ...Ts>
concept OfSameValueType = (std::same_as<typename T::value_type, typename Ts::value_type> && ...);

template <typename T, typename ...Ts>
concept RmwAble = MaskTrait<T, Ts...> && OfSameValueType<T, Ts...>;

template <typename Field0, typename ...Fields>
requires RmwAble<Field0, Fields...>
struct rmw_fields {
    using value_type = typename Field0::value_type;
    static constexpr value_type layout = (Field0::mask | ... | Fields::mask);
};

template <typename DerivedReg>
struct rmw_reg {
    using f = typename DerivedReg::Fields;
    using value_type = typename DerivedReg::Fields::value_type;
    static constexpr value_type layout = DerivedReg::Fields::layout;

    // static constexpr bool needs_read (value_type new_value) {
    //     return !(((F0::mask & new_value) > 0) && (((Fs::mask & new_value) > 0) && ...));
    // }

    static constexpr value_type update (value_type old_value, value_type new_value) {
        return (old_value & ~layout) | (new_value & layout);
    }
};

template<typename ...Fields>
void apply(Fields ...fields) {

}

template<>
void apply() {};


// template<size_t V>
// struct rmw_integral_constant : std::integral_constant<size_t, V> {};

consteval auto operator""_c(unsigned long long int v) {
    return v;
}

// // delayed declaration
// template <typename T = void>
// struct my_reg : rmw_reg<my_reg<T>> {
//     using field0 = rmw<my_reg, uint32_t, 4, 0>;
//     using field1 = rmw<my_reg, uint32_t, 12, 8>;
//     using field2 = rmw<my_reg, uint32_t, 20, 16>;
//     using field3 = rmw<my_reg, uint32_t, 31, 28>;

//     using Fields = rmw_fields<field0, field1, field2, field3>;
//     // using Reg = rmw_reg<Fields>;
// };

struct my_reg {
    using value_type = uint32_t;
    
    rmw<my_reg, value_type, 4, 0>   field0;
    rmw<my_reg, value_type, 12, 8>  field1;
    rmw<my_reg, value_type, 20, 16> field2;
    rmw<my_reg, value_type, 31, 28> field3;
};

int main() {
    // using bar = rmw<uint8_t, 4, 2>;
    // using bar = rmw<uint32_t, 13, 7>;
    // using bar = rmw<uint32_t, 12, 8>;
    // using bar = rmw<uint32_t, 31, 31>;
    // auto v = bar::update(0xFFFFFFFF, 0xF0BBF0F0);
    // std::cout << std::hex << bar::mask << std::endl;
    // std::cout << std::hex << v << std::endl;
    my_reg r0;
    // my_reg::field0::write(23);
    apply(r0.field0 = 23_c,
          r0.field1 = 42_c);
    // std::cout << std::hex << reg0::layout << std::endl;

    // reg0::field0::write(23);
    // auto r = reg0::field0::read();
    // reg0::write(42);
    // combined_write(reg0::field0::write(23), reg0::field1::write(42));

    // std::cout << std::hex << reg0::needs_read(0xFFFF) << std::endl;
    // std::cout << std::hex << reg0::needs_read(0xFFFFFFFF) << std::endl;

    // return v;
}
