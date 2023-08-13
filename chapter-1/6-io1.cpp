#include <iostream>
#include <cmath>
#include <iomanip>
#include <fstream>

const double epsilon = 1e-11; //0.001;

int main() {
    double aside = 0, bside = 1;

    std::cout << "Write input parameters:" << std::endl;
    std::cout << "aside = "; std::cin >> aside;
    std::cout << "bside = "; std::cin >> bside;

    std::ofstream ofs;
    ofs.open("io1-out.txt", std::ios_base::app);
    ofs << std::setprecision(12) << aside << " " << bside << " ";
    
    double middle = aside + (bside - aside) / 2;

    auto f = [] (double x) -> double {
        return std::sin(5 * x) + std::cos(x);
    };

    while (bside - aside > epsilon) {
        if (std::signbit( f(aside) ) != std::signbit( f(middle) )) {
            bside = middle;
            middle = aside + (bside - aside) / 2;
        } else if (std::signbit( f(middle) ) != std::signbit( f(bside) )) {
            aside = middle;
            middle += (bside - aside) / 2;
        }
    }

    ofs << middle << std::endl;
    ofs.close();

    return 0;
}