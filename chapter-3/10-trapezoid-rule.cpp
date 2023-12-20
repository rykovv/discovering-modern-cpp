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


template <unsigned N, typename F, typename T>
class trapezoid_sum_reg {
public:

    trapezoid_sum_reg(const F& f, const T& a, const T& h)
      : f{f}, a{a}, h{h} 
    {}

    T operator() () const {
        T sum = 0.0;
        for (unsigned j = 1; j <= N; j++) {
            sum += f(a + j*h);
        }
        return sum;
    }

private:
    T a, h;
    const F& f;
};

template <unsigned N, typename F, typename T>
T trapezoid_reg(const F& f, const T& a, const T& b) {
    T h = (b - a) / N;
    return h*(f(a) + f(b))/2 + h*trapezoid_sum_reg<N-1, F, T>{f, a, h}();
}

template <typename F, typename T>
class finite_difference {
public:

    finite_difference(const F& f, const T& h)
      : f{f}, h{h} 
    {}

    T operator() (const T& x) const {
        return ( f(x+h) - f(x) ) / h;
    }

private:
    const F& f;
    T h;
};

template <typename F, typename T>
auto finite_difference_proxy(const F& f, const T& h) {
    finite_difference<F, T> fd{f, h};
    return fd;
}

// referring std and passing it in as a pointer is undefined behavior
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

double mysin(double x) {
    return std::sin(x);
}


int main() {
    auto i1 = trapezoid<50>(exp3f, 0.0, 4.0);
    std::cout << "trapezoid<50>(exp3f, 0.0, 4.0) = " << i1 << std::endl;
    i1 = trapezoid_reg<50>(exp3f, 0.0, 4.0);
    std::cout << "trapezoid_reg<50>(exp3f, 0.0, 4.0) = " << i1 << std::endl;

    exp3t exp3ts;
    auto i2 = trapezoid<50>(exp3ts, 0.0, 4.0);
    std::cout << "trapezoid<50>(exp3ts, 0.0, 4.0) = " << i2 << std::endl;
    i2 = trapezoid_reg<50>(exp3ts, 0.0, 4.0);
    std::cout << "trapezoid_reg<50>(exp3ts, 0.0, 4.0) = " << i2 << std::endl;

    auto i3 = trapezoid<50>(mysincos, 0.0, 4.0);
    std::cout << "trapezoid<50>(mysincos, 0.0, 4.0) = " << i3 << std::endl;
    i3 = trapezoid_reg<50>(mysincos, 0.0, 4.0);
    std::cout << "trapezoid_reg<50>(mysincos, 0.0, 4.0) = " << i3 << std::endl;

    // how do I pass in a pointer to the right overloaded function?
    // wrap it into a proxy function that will select the right 
    //   overload by arg type
    auto i4 = trapezoid<50>(mysin, 0.0, 2.0);
    std::cout << "trapezoid<50>(mysin, 0.0, 2.0) = " << i4 << std::endl;

    // integrating the finite difference
    double p = 0.5;     // point
    double h = 0.0001;
    // check function value at .5
    std::cout << "mysin(0.5) = " << mysin(0.5) << std::endl;
    // get the slope at that point
    // h of the finite difference is the delta 
    auto findif = finite_difference_proxy(mysin, h);
    std::cout << "findif(mysin, " << h << ")(" << p-h << ") = " << findif(p-h) << std::endl;
    // integrate the derivative function at the previous point
    auto i5 = trapezoid<50>(findif, p-h, p);
    std::cout << "trapezoid<50>(findif, " << p-h << ", " << p << ") = " << i5 << std::endl;

    // this gives the function value back, as h works as an approximation of the derivation step
    auto i6 = trapezoid<50>(mysin, p-h, p);
    std::cout << i6/h << std::endl;


    return 0;
}