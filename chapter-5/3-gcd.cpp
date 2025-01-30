#include <iostream>

template <typename I>
I gcd (I a, I b) {
    if (b == 0) return a;
    else        return gcd(b, a % b);
}

template <int A, int B>
struct gcd_meta {
    static int const value = gcd_meta<B, A % B>::value;
};

template <int A>
struct gcd_meta<A, 0> {
    static int const value = A;
};


int main() {

    std::cout <<  gcd(10, 2) << std::endl;
    std::cout <<  gcd_meta<10, 2>::value << std::endl;

    return 0;
}