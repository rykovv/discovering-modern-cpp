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

template <typename T>
concept MaskTrait = requires {
    T::mask;
};

template <typename ...Fields>
struct rmw_register {};

template <typename Field>
requires MaskTrait<Field>
struct rmw_register<Field> {
    Field field;
};

template <typename Field, typename ...Fields>
requires MaskTrait<Field>
struct rmw_register<Field, Fields...> {
    Field field;
    rmw_register<Fields...> rest;
};

int main() {
    // using bar = rmw<uint8_t, 4, 2>;
    // using bar = rmw<uint32_t, 13, 7>;
    // using bar = rmw<uint32_t, 12, 8>;
    using bar = rmw<uint32_t, 31, 31>;
    auto v = bar::update(0xFFFFFFFF, 0xF0BBF0F0);
    std::cout << std::hex << bar::mask << std::endl;
    std::cout << std::hex << v << std::endl;
    return v;
}
