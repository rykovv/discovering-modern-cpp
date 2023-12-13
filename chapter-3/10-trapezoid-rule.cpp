#include <iostream>
#include <cmath>


template <unsigned N, typename F, typename T>
class trapezoid_sum {

    using prev_sum = trapezoid_sum<N-1, F, T>;

public:

    trapezoid_sum(const F& f, const T& a, const T& h)
      : f{f}, a{a}, h{h}, ps{f, a, h}
    {}

    T operator() () const {
        return ps() + f(a + N*h);
    }

private:
    prev_sum ps;
    T a, h;
    const F& f;
};

template <typename F, typename T>
class trapezoid_sum<1, F, T> {
public:

    trapezoid_sum(const F& f, const T& a, const T& h)
      : f{f}, a{a}, h{h} 
    {}

    T operator() () const {
        return f(a + h);
    }

private:
    T a, h;
    const F& f;
};

// N - approximation factor
// F - function type
// T - type of limits [a, b]
template <unsigned N, typename F, typename T>
T trapezoid(const F& f, const T& a, const T& b) {
    T h = (b - a) / N;
    return h*(f(a) + f(b))/2 + h*trapezoid_sum<N-1, F, T>{f, a, h}();
}


double exp3f(double x) {
    return std::exp(3.0 * x);
}
struct exp3t {
    double operator() (double x) const {
        return std::exp(3.0 * x);
    }
};
double mysincos(double x) {
    return x < 1 ? std::sin(x) : std::cos(x);
}


int main() {
    auto i1 = trapezoid<50>(exp3f, 0.0, 4.0);
    
    exp3t exp3ts;
    auto i2 = trapezoid<50>(exp3ts, 0.0, 4.0);

    std::cout << "trapezoid<50>(exp3f, 0.0, 4.0) = " << i1 << std::endl;
    std::cout << "trapezoid<50>(exp3ts, 0.0, 4.0) = " << i2 << std::endl;

    auto i3 = trapezoid<50>(mysincos, 0.0, 4.0);

    std::cout << "trapezoid<50>(mysincos, 0.0, 4.0) = " << i3 << std::endl;


    // how do I pass in a pointer to the right overloaded function?
    // auto i4 = trapezoid<50>(std::sin, 0.0, 2.0);

    return 0;
}