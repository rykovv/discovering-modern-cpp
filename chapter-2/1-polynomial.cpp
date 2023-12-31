#include <vector>
#include <initializer_list>
#include <iostream>

struct polynomial_degree_not_equal_coefs_size {};

class Polynomial {
public:
    // if I want a polynomial coefficients, I'll get them from initializer list
    Polynomial(long long unsigned degree, const std::vector<double> coefs)
    :   degree{degree},
        coefs{coefs}
    {
        if (degree != coefs.size()-1) {
            throw polynomial_degree_not_equal_coefs_size{};
        }
    };
    ~Polynomial() {
        degree = 0;
        coefs.clear();
    }

    friend std::ostream& operator<< (std::ostream& os, Polynomial& p);
private:
    long long unsigned degree;
    std::vector<double> coefs;
};

std::ostream& operator<< (std::ostream& os, Polynomial& p) {
    os << "Polynomial degree(" << p.degree << "), coefs(";
    for (long long unsigned i = 0; i < p.degree; i++) {
        os << p.coefs[i] << ", ";
    }
    return os << p.coefs.back() << ")" << std::endl;
}

int main() {
    Polynomial p = {4, {1,2,3,4,5}};
    std::cout << p;
    
    return 0;
}