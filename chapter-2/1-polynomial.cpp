#include <vector>
#include <initializer_list>
#include <iostream>

struct polynomial_degree_not_equal_coefs_size {};

class Polynimial {
public:
    // if I want a polynomial coefficients, I'll get them from initializer list
    Polynimial(long long unsigned degree, const std::initializer_list<double> coefs)
    :   degree{degree},
        coefs{coefs}
    {
        if (degree != coefs.size()) {
            throw polynomial_degree_not_equal_coefs_size{};
        }
    };
    ~Polynimial() {
        degree = 0;
        coefs.clear();
    }
    unsigned get_degree() const {
        return degree;
    }
    const std::vector<double>& get_coefs() const {
        return coefs;
    }
    const double& get_coef(long long unsigned degree) const {
        return coefs[degree];
    }
private:
    long long unsigned degree;
    std::vector<double> coefs;
};

std::ostream& operator<< (std::ostream& os, Polynimial& p) {
    os << "Polynomial degree(" << p.get_degree() << "), coefs(";
    for (long long unsigned i = 0; i < p.get_degree()-1; i++) {
        os << p.get_coef(i) << ", ";
    }
    return os << p.get_coefs().back() << ")" << std::endl;
}

int main() {
    Polynimial p = {5, {1,2,3,4,5}};
    std::cout << p;
    
    return 0;
}