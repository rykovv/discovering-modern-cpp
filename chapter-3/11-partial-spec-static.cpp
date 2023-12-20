#include <complex>
#include <iostream>


template <typename T>
struct abs_functor {
    typedef T result_type;

    T operator()(const T& x) {
        return x < T(0) ? -x : x;
    }
};

template <typename T>
struct abs_functor<std::complex<T> > {
    typedef T result_type;

    T operator()(const std::complex<T>& x) {
        return sqrt(real(x)*real(x) + imag(x)*imag(x));
    }

    static T eval(const std::complex<T>& x) {
        return sqrt(real(x)*real(x) + imag(x)*imag(x));
    }
};

template <typename T>
auto abs(const T& t) -> decltype(abs_functor<T>()(t)) {
    abs_functor<T> func_obj;
    return func_obj(t);
}

template <typename T>
auto sabs(const T& t) -> decltype(abs_functor<T>()(t)) {
    // how do I call eval() without creating a class object?
    return abs_functor<T>().eval(t);
    // return abs_functor<T>()(t); // works the same way
}


int main() {
    std::complex<double> s = {1.0, -2.0};
    double d = -0.1;

    std::cout << abs(s) << std::endl;
    std::cout << abs(d) << std::endl;

    std::cout << sabs(s) << std::endl;
    // std::cout << sabs(d) << std::endl; // static eval not implemented 
    
    return 0;
}