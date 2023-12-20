#include <iostream>


template<size_t N>
auto derive = [] (auto f, auto h) {
    auto prev = derive<N-1>(f, h);
    return [prev, h] (auto x) { return (prev(x+h) - prev(x)) / h; };
};

template<>
auto derive<0> = [] (auto f, auto h) {
    return [f] (auto x) { return f(x); };
};


template <size_t N>
auto derive2 = [] (auto f, auto h, auto x) {
    auto prev = [&] (auto x2) {
        return derive2<N-1>(f, h, x2);
    };
    return (prev(x+h) - prev(x)) / h;
};

template <>
auto derive2<0> = [] (auto f, auto h, auto x) {
    return f(x);
};


// why it doesn't work? why return type void?

// template<size_t N>
// auto derive3 = [] (auto f, auto h) {
//     if constexpr (N == 0) {
//         return [f] (auto x) { f(x); };
//     } else {
//         auto prev = derive3<N-1>(f, h);
//         return [=] (auto x) { (prev(x+h) - prev(x)) / h; };
//     }
// };


int main() {
    auto d2 = derive<2>([] (double x) { return x*x; }, 0.00001);
    auto d3 = derive<2>([] (double x) { return x*x*x; }, 0.00001);

    std::cout << d2(2.0) << std::endl;
    std::cout << d3(2.0) << std::endl;

    auto d4 = derive2<2>([] (double x) { return x*x; }, 0.00001, 2.0);
    auto d5 = derive2<2>([] (double x) { return x*x*x; }, 0.00001, 2.0);

    std::cout << d4 << std::endl;
    std::cout << d5 << std::endl;

    // auto d6 = derive3<2>([] (double x) { return x*x; }, 0.00001);
    // auto d7 = derive3<2>([] (double x) { return x*x*x; }, 0.00001);

    // std::cout << d6(2.0) << std::endl;
    // std::cout << d7(2.0) << std::endl;

    return 0;
}