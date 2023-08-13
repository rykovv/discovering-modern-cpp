#include <iostream>
#include <cmath>
#include <iomanip>
#include <fstream>

const double epsilon = 1e-11; //0.001;

int main() {
    double aside, bside, result;

    std::ifstream ifs;
    ifs.open("io1-out.txt");

    auto lam = [] (double a, double b) -> double {
        double middle = a + (b - a) / 2;

        auto f = [] (double x) -> double {
            return std::sin(5 * x) + std::cos(x);
        };

        while (b - a > epsilon) {
            if (std::signbit( f(a) ) != std::signbit( f(middle) )) {
                b = middle;
                middle = a + (b - a) / 2;
            } else if (std::signbit( f(middle) ) != std::signbit( f(b) )) {
                a = middle;
                middle += (b - a) / 2;
            }
        }

        return middle;
    };

    while(ifs >> aside >> bside >> result) {
        std::cout << "aside = " << aside << ", bside = " << bside << ", result = " << result << std::endl;

        if (result - lam(aside, bside) < epsilon) {
            std::cout << "The result is correct." << std::endl;
        } else {
            std::cout << "The result is incorrect." << std::endl;
        }
    }

    ifs.close();

    return 0;
}