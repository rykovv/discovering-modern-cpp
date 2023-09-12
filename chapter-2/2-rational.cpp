#include <utility>
#include <iostream>

// exceptions
struct rantional_cannot_have_negative_denominator{};
struct numerator_and_denominator_are_not_coprimes{};

class Rational {
public:
    Rational(int p, int q)
    :   p{p}, 
        q{q} 
    {
        if (q < 0) {
            throw rantional_cannot_have_negative_denominator{};
        } else if ( !is_coprime(p, q) ) {
            throw numerator_and_denominator_are_not_coprimes{};
        }
    };
    // for implicit conversion
    Rational(int p): Rational{p, 1} {};

    Rational operator+ (const Rational& r2) {
        return normalize({p + r2.p, q + r2.q});
    }
    Rational& operator+= (const Rational& rhs) {
        p += rhs.p;
        q += rhs.q;
        return normalize_by_ref(*this);
    }
    Rational operator- (const Rational& r2) {
        return q == r2.q? normalize({p - r2.p, q}) : normalize({p*r2.q - r2.p*q, q * r2.q});
    }
    Rational& operator-= (const Rational& rhs) {
        if (q == rhs.q) {
            p =- rhs.p;
        } else {
            p = p*rhs.q - rhs.p*q;
            q *= rhs.q;
        }
        return normalize_by_ref(*this); 
    }

    Rational operator* (const Rational& r2) {
        return normalize({p * r2.p, q * r2.q});
    }
    Rational& operator*= (const Rational& rhs) {
        p *= rhs.p;
        q *= rhs.q;
        return normalize_by_ref(*this);
    }

    Rational operator/ (const Rational& r2) {
        return normalize(Rational{p * r2.q, q * r2.p});
    }
    Rational& operator/= (const Rational& rhs) {
        p *= rhs.q;
        q *= rhs.p;
        return normalize_by_ref(*this);
    }

    friend std::ostream& operator<< (std::ostream& os, Rational& r);
private:
    int p, q;

    // greatest common divisor 
    int gcd(int a, int b) {
        // old Euclidean algorithm
        if (a == b) { // a numbers is not coprime to itself
            return -1;
        }
        if (a < b) {
            std::swap(a, b);
        }
        while (b > 0) {
            // std::cout << "a, b = " << a << ", " << b << std::endl;
            a %= b;
            std::swap(a, b);
        }
        return a;
    }
    bool is_coprime(int a, int b) {
        int c = gcd(a, b);
        // std::cout << "gcd(" << a << ", " << b << ") = " << c << std::endl;
        return c == 1;
    }
    Rational normalize(Rational r) {
        int gg = gcd(r.p, r.q);
        return Rational{r.p / gg, r.q / gg};
    }
    Rational& normalize_by_ref(Rational& r) {
        int gg = gcd(r.p, r.q);
        r.p = r.p / gg;
        r.q = r.q / gg;

        return r;
    }
};

std::ostream& operator<< (std::ostream& os, Rational& r) {
    return os << "Rational " << r.p << "/" << r.q;
}

int main() {
    // Rational r{2, 3}; // ok
    // Rational r{1071, 462}; // ok
    // Rational r{2, 4}; // ok
    // Rational r{1, 1}; // ok
    // Rational r0{5, 13}; // ok
    
    // Rational r1 = 5; // ok
    // Rational r2 = 1; // ok

    Rational r3{1, 5}, r4{2, 3};
    Rational r5 = r3 * r4; // ok
    std::cout << r3 << " * " << r4 << " = " << r5 << std::endl;
    
    Rational r6 = r3 / 2; // ok
    std::cout << r3 << " / Rational 2/1 = " << r6 << std::endl;
    
    Rational r7 = r3 + r4; // ok
    std::cout << r3 << " + " << r4 << " = " << r7 << std::endl;

    Rational r8 = r4 - r3; // ok
    std::cout << r3 << " - " << r4 << " = " << r8 << std::endl;

    // r3 += r4; // ok
    // std::cout << r3 << " += " << r4 << " = " << r3 << std::endl;

    // r4 -= r3; // ok
    // std::cout << r4 << " -= " << r3 << " = " << r4 << std::endl;

    // r3 *= r4; // ok
    // std::cout << r3 << " *= " << r4 << " = " << r3 << std::endl;

    r3 /= r4; // ok
    std::cout << r3 << " /= " << r4 << " = " << r3 << std::endl;
    

    return 0;
}