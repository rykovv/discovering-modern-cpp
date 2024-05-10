#include <iostream>
#include <concepts>
#include <cmath>

namespace activation {
    template <std::floating_point T>
    static constexpr T sigmoid (T const& z) {
        // bad for activation due to vanishing gradient
        // okay for gating functions
        using std::exp;
        return 1/(1+exp(-z));
    }
}

int main () {
    std::cout << "sigmoid(2) = " << activation::sigmoid(2.0) << std::endl;

    return 0;
}
