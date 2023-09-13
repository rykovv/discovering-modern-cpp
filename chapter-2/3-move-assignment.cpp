#include <vector>
#include <initializer_list>
#include <iostream>
#include <assert.h>

struct polynomial_degree_not_equal_coefs_size {};

class Polynomial {
public:
    Polynomial () : degree{0} {};
    // if I want a polynomial coefficients, I'll get them from initializer list
    Polynomial (long long unsigned degree, const std::initializer_list<double> coefs)
    :   degree{degree},
        coefs{coefs}
    {
        if (degree != coefs.size()-1) {
            throw polynomial_degree_not_equal_coefs_size{};
        }
    };
    // default copy ctor
    Polynomial (const Polynomial& p) = default;
    // move ctor
    Polynomial (Polynomial&& p) noexcept
    :   degree{p.degree},
        coefs{p.coefs} 
    {
        p.degree = 0;
        p.coefs.clear();
        std::cout << "move ctor used." << std::endl;
    }
    // move assignement
    Polynomial& operator= (Polynomial&& p) noexcept {
        // check default state
        assert (degree == 0 && coefs.size() == 0);
        std::swap(degree, p.degree);
        std::swap(coefs, p.coefs);
        std::cout << "move assigned." << std::endl;

        return *this;
    }
    // if use instead of explicit, never used
    // Polynomial& operator= (Polynomial&& p) noexcept = default;
    
    ~Polynomial() {
        degree = 0;
        coefs.clear();
    }
    
    friend std::ostream& operator<< (std::ostream& os, Polynomial& p);
private:
    long long unsigned degree = 0;
    std::vector<double> coefs;
};

std::ostream& operator<< (std::ostream& os, Polynomial& p) {
    os << "Polynomial degree(" << p.degree << "), coefs(";
    for (long long unsigned i = 0; i < p.degree; i++) {
        os << p.coefs[i] << ", ";
    }
    return os << p.coefs.back() << ")" << std::endl;
}

Polynomial f(double c2, double c1, double c0) {
    return {2, {c0, c1, c2}};
}

int main() {
    // regular ctor
    Polynomial p {2, {1,1,1}};
    std::cout << p;
    
    // copy ctor (object inferred) ?
    // delete default copy ctor works too
    // delele dtor doesn't work neither
    Polynomial p1 = {2, {2,2,2}};
    std::cout << p1;

    // move ctor - ok
    Polynomial p2 {std::move(p1)};
    std::cout << p2;

    // not ok
    // should move assignment, but it's a copy ctor?
    // delete default copy ctor works too
    // delele dtor doesn't work neither
    Polynomial p3 = f(3,3,3);
    std::cout << p3;

    // move assigned! - ok
    Polynomial p4;
    p4 = f(4,4,4);
    std::cout << p4;

    // not ok
    // from xvalue
    // should be move assignment, but it's move ctor again
    // same if delele dtor
    Polynomial p5 = std::move(f(5,5,5));
    std::cout << p5;

    // move assigned! - ok
    Polynomial p6;
    p6 = std::move(f(6,6,6));
    std::cout << p6;

    return 0;
}