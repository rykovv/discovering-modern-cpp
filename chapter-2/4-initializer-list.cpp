#include <vector>
#include <initializer_list>
#include <iostream>


struct polynomial_degree_not_equal_coefs_size {};

class Polynomial {
public:
    Polynomial () = default;
    // if I want a polynomial coefficients, I'll get them from initializer list
    Polynomial (long long unsigned degree, const std::vector<double> coefs)
    :   degree{degree},
        coefs{coefs}
    {
        if (degree != coefs.size()-1) {
            throw polynomial_degree_not_equal_coefs_size{};
        }
    };
    Polynomial (const std::initializer_list<double> coefs)
    :   degree{coefs.size() - 1},
        coefs{coefs}
    {
        std::cout << "initializer list ctor" << std::endl;
    }

    // assignement operator. copy assignment
    Polynomial& operator= (const std::initializer_list<double> new_coefs) {
        degree = new_coefs.size() - 1;
        coefs.assign(new_coefs);

        std::cout << "initializer list assignment" << std::endl;
        
        return *this;
    }

    ~Polynomial () {
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
    // ctor
    Polynomial p2 {2,2,2,2};
    std::cout << p2;

    // ctor
    Polynomial p3 = {3,3,3,3};
    std::cout << p3;

    // assignment
    Polynomial p4;
    p4 = {4,4,4,4};
    std::cout << p4;
    
    return 0;
}