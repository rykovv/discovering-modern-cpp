#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>

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

template <typename T, unsigned msb, unsigned lsb>
requires FieldSelectable<T, msb, lsb>
struct rmw {
    using value_type = T;

    static constexpr T mask = []() {
        if (msb != lsb) {
            if (msb == std::numeric_limits<T>::digits-1) {
                return ~((1 << lsb) - 1);
            } else {
                return ((1 << msb) - 1) & ~((1 << lsb) - 1);
            }
        } else {
            return 1 << msb;
        }
    }();

    static constexpr T update (T old_value, T new_value) {
        return (old_value & ~mask) | (new_value & mask);
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
struct rmw_reg;

// Forward declaration
struct derived_proxy;

template <>
struct rmw_reg<derived_proxy> {
    using value_type = typename derived_proxy::Fields::value_type;
    static constexpr value_type layout = derived_proxy::Fields::layout;

    // static constexpr bool needs_read (value_type new_value) {
    //     return !(((F0::mask & new_value) > 0) && (((Fs::mask & new_value) > 0) && ...));
    // }

    static constexpr value_type update (value_type old_value, value_type new_value) {
        return (old_value & ~layout) | (new_value & layout);
    }
};

struct derived_proxy : rmw_reg<derived_proxy> {};

struct my_reg : derived_proxy {
    using field0 = rmw<uint32_t, 4, 0>;
    using field1 = rmw<uint32_t, 12, 8>;
    using field2 = rmw<uint32_t, 20, 16>;
    using field3 = rmw<uint32_t, 31, 28>;

    using Fields = rmw_fields<field0, field1, field2, field3>;
};

int main() {
    // using bar = rmw<uint8_t, 4, 2>;
    // using bar = rmw<uint32_t, 13, 7>;
    // using bar = rmw<uint32_t, 12, 8>;
    using bar = rmw<uint32_t, 31, 31>;
    auto v = bar::update(0xFFFFFFFF, 0xF0BBF0F0);
    std::cout << std::hex << bar::mask << std::endl;
    std::cout << std::hex << v << std::endl;


    using RegFields = rmw_fields<
        rmw<uint32_t, 4, 0>,
        rmw<uint32_t, 12, 8>,
        rmw<uint32_t, 20, 16>,
        rmw<uint32_t, 31, 28>
    >;
    using reg0 = rmw_reg<RegFields>;
    // std::cout << std::hex << reg0::layout << std::endl;

    // reg0::field0::write(23);
    // auto r = reg0::field0::read();

    // std::cout << std::hex << reg0::needs_read(0xFFFF) << std::endl;
    // std::cout << std::hex << reg0::needs_read(0xFFFFFFFF) << std::endl;

    return v;
}
