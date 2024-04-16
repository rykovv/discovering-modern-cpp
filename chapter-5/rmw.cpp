#include <concepts>
#include <limits>
#include <cstdint>
#include <iostream>

template <typename T, unsigned msb, unsigned lsb>
concept FieldSelectable = requires {
    requires std::integral<T>;
} && (
    msb <= std::numeric_limits<T>::digits-1 &&
    lsb <= std::numeric_limits<T>::digits-1
) && (
    msb >= lsb
);

template <typename T, unsigned msb, unsigned lsb>
struct rmw {
    static constexpr T update (T old_value, T new_value)
        requires FieldSelectable<T, msb, lsb>
    {
        T mask = ((1 << msb) - 1) & ~((1 << lsb) - 1);
        return (old_value & ~mask) | (new_value & mask);
    }
};

int main() {
    using bar = rmw<uint32_t, 24, 18>;
    // using bar = rmw<uint32_t, 13, 7>;
    auto v = bar::update(0xFFFFFFFF, 0xF0BBF0F0);
    std::cout << std::hex << v << std::endl;
    return v;
}
