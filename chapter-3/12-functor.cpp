#include <iostream>


template <size_t N, typename F, typename T>
struct derivative {

    using prev_der = derivative<N-1, F, T>;

public:
    derivative(const F& f, const T& h)
      : pd{f, h}, h{h}
    {}

    T operator() (const T& x) {
        return (pd(x+h) - pd(x)) / h;
    }

private:
    const T& h;
    prev_der pd;
};


template <typename F, typename T>
struct derivative<1, F, T> {
public:
    derivative(const F& f, const T& h)
      : f{f}, h{h}
    {}

    T operator() (const T& x) {
        return (f(x+h) - f(x)) / h;
    }

private:
    const T& h;
    const F& f;
};

template <size_t N, typename F, typename T>
derivative<N, F, T>
derive(const F& f, const T& h) {
    return derivative<N, F, T>{f, h};
}

double square(double x) {
    return x*x;
}

double cube(double x) {
    return x*x*x;
}


int main() {
    auto d2 = derive<2>(square, 0.00001);
    auto d3 = derive<2>(cube, 0.00001);

    std::cout << d2(2.0) << std::endl;
    std::cout << d3(2.0) << std::endl;

    return 0;
}