#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>
#include <type_traits>


template <unsigned v, unsigned m>
struct rmw_assignment {
    static_assert(v <= m, "assigned value greater than allowed");
};

struct rmw_read {

};

template <typename T> struct unsafe_cast_ferry {
  private:
    T v;

  public:
    constexpr explicit(true) unsafe_cast_ferry(T new_value)
        : v{new_value} {}

    [[nodiscard]] constexpr auto value() const -> T { return v; }
};

template <typename T, char... Chars>
[[nodiscard]] static constexpr auto to_compile_time_constant() -> T {
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

template <typename Reg, typename T, unsigned msb, unsigned lsb, unsigned ci = 0>
requires FieldSelectable<T, msb, lsb>
struct rmw {
    using value_type = T;
    static constexpr value_type init = ci;

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

    constexpr rmw()
        : value{init}
    {}

    template <typename U>
    requires(std::is_convertible_v<value_type, U>)
    constexpr rmw(unsafe_cast_ferry<U> ferry)
        : value {static_cast<value_type>(ferry.value())} 
    {
        // std::cout << "ferry ctor " << ferry.value() << std::endl;
    }

    constexpr auto operator= (auto const& rhs) -> rmw & {
        using rhs_type = std::remove_reference_t<decltype(rhs)>;
        static_assert(rhs_type::init <= (mask >> lsb), "assigned value greater than allowed");
        constexpr value_type max = mask >> lsb;
        // rmw_assignment<init, max> rmwa;
        value = rhs.value;
        return *this;
    }

    static constexpr value_type update (value_type old_value, value_type new_value) {
        return (old_value & ~mask) | (new_value & mask);
    }

    value_type value;
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

template <char ...Chars>
constexpr auto operator""_f () {
    using T = std::size_t; // platform max
    constexpr T new_value = to_compile_time_constant<T, Chars...>();
    // constexpr value_type max = mask >> lsb;
    // rmw_assignment<new_value, 100> rmwa;
    std::cout << "<new value>_f = " << new_value << std::endl;
    using dummy = void;
    using pass = rmw<dummy, T, std::numeric_limits<T>::digits-1, 0, new_value>;
    return pass(unsafe_cast_ferry{new_value});
    // return new_value;
}

template<typename ...Fields>
void apply(Fields ...fields) {

}

template<>
void apply() {};

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
    apply(r0.field0 = 10_f,
          r0.field1 = 42_f);
    // std::cout << std::hex << reg0::layout << std::endl;

    // reg0::field0::write(23);
    // auto r = reg0::field0::read();
    // reg0::write(42);
    // combined_write(reg0::field0::write(23), reg0::field1::write(42));

    // std::cout << std::hex << reg0::needs_read(0xFFFF) << std::endl;
    // std::cout << std::hex << reg0::needs_read(0xFFFFFFFF) << std::endl;

    // return v;
    return r0.field0.value;
}
