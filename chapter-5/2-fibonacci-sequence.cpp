#include <iostream>

template <int N>
struct fib {
    static constexpr int value = fib<N-1>::value + fib<N-2>::value;
};

template <>
struct fib<0> {
    static constexpr int value = 0;
};

template <>
struct fib<1> {
    static constexpr int value = 1;
};

int main() {

    static_assert(fib<10>::value == 55);

    return 0;
}