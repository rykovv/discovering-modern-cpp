#include <iostream>
#include <cmath>
#include <iomanip>

const double epsilon = 1e-11; //0.001;

int main() {
    double aside = 0, bside = 1;
    double middle = aside + (bside - aside) / 2;

    auto f = [] (double x) -> double {
        return std::sin(5 * x) + std::cos(x);
    };

    while (bside - aside > epsilon) {
        // std::cout << "f(" << aside << ") = " << f(aside) << std::setprecision(11) << ", signbit = " << std::signbit(f(aside)) << std::endl;
        // std::cout << "f(" << middle << ") = " << f(middle) << std::setprecision(11) << ", signbit = " << std::signbit(f(middle)) << std::endl;
        // std::cout << "f(" << bside << ") = " << f(bside) << std::setprecision(11) << ", signbit = " << std::signbit(f(bside)) << std::endl << "---" << std::endl;

        if (std::signbit( f(aside) ) != std::signbit( f(middle) )) {
            bside = middle;
            middle = aside + (bside - aside) / 2;
        } else if (std::signbit( f(middle) ) != std::signbit( f(bside) )) {
            aside = middle;
            middle += (bside - aside) / 2;
        }
    }

    std::cout << std::setprecision(11) << "The answer is " << middle << "." << std::endl;

    return 0;
}