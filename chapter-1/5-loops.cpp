#include <iostream>
#include <cmath>
#include <iomanip>

const double epsilon = .0e-12;

int main() {
    double aside = 0, bside = 1, middle;

    auto f = [] (double x) -> double {
        return std::sin(5 * x) + std::cos(x);
    };

    while (bside - aside > epsilon) {
        middle = (bside - aside) / 2;
        
        if (std::signbit( f(aside) ) == std::signbit( f(middle) )) {
            bside = middle;
        } else if (std::signbit( f(middle) ) != std::signbit( f(bside) )) {
            aside = middle;
        } else {
            aside = middle;    
        }

        std::cout << std::signbit(f(aside)) << " " << std::signbit(f(middle)) << " " << std::signbit(f(bside)) << std::endl;
    }

    std::cout << "The answer is " << std::setprecision(11) << f(middle) << "." << std::endl;

    return 0;
}