#include <iostream>

struct A {
    static constexpr int value = 1;
};

struct B {
    static constexpr int value = 2;
};

struct C {
    static constexpr int value = 3;
};

template <typename ...Ts>
struct typelist {};

template <typename ...Ts>
constexpr int sum(typelist<Ts...>) {
    return (Ts::value + ...);
}

int main () {
    using l = typelist<A, B, C>;
    std::cout << sum(l{}) << std::endl;
}
