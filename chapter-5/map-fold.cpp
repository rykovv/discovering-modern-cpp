#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>


template <typename T>
struct list {
    list(std::initializer_list<T> it)
      : v{it}
    {}

    std::vector<T> v;
};

template <typename T>
void print(list<T> l){
    std::cout << "{ ";
    std::copy(l.v.begin(), l.v.end(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << "}" << std::endl;
}

template <typename T>
void print(std::vector<T> v){
    std::cout << "{ ";
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << "}" << std::endl;
}

// template <typename R, typename U>
// list<U> map(list<R> l, std::function<U(R)> f, int i) {
//     if ( l.v.size() == i ) {
//         return {f(l.v[0])};
//     } else {
//         return { f(l.v[0]), map(l, f, i+1) };
//     }
// }

//------------

// template <typename Iter, typename R, typename U>
// std::vector<U> map_aux(Iter b, Iter e, std::function<U(R)> f) {
//     if ( b == e ) {
//         return {};
//     } else {
//         return map_aux(++b, e, f).push_back(f(*b));
//     }
// }

// template <typename R, typename U>
// std::vector<U> map(std::vector<R> v, std::function<U(R)> f) {
//     if ( v.empty() ) {
//         return std::vector<U>();
//     } else {
//         return map_aux(v.begin(), v.end(), f);
//     }
// }

//-----------

// 1. Create a pure interface
template<typename T>
std::vector<T> empty() {
    return std::vector<T>();
}

template<typename T>
std::vector<T> add_to_front(T v, std::vector<T> vs) {
    vs.emplace(vs.begin(), v);
    return vs;
}

template<typename T>
bool is_empty(std::vector<T> vs) {
    return vs.empty();
}

template<typename T>
T front(std::vector<T> vs) {
    return vs.at(0);
}

template<typename T>
std::vector<T> rest(std::vector<T> vs) {
    if (vs.empty()) {
        return {};
    } else {
        std::vector<T> nvs(vs);
        nvs.erase(nvs.begin());
        // why this is not working?
        // std::copy(++vs.begin(), vs.end(), nvs.begin());
        return nvs;
    }
}

template <typename R, typename U>
std::vector<U> map(std::vector<R> v, std::function<U(R)> f) {
    if ( is_empty(v) ) {
        return empty<U>();
    } else {
        return add_to_front( f(front(v)), map(rest(v), f) );
    }
}

template <typename T>
T fold(std::vector<T> v, std::function<T(T,T)> f, T init) {
    if ( is_empty(v) ) {
        return init;
    } else {
        return f(front(v), fold(rest(v), f, init));
    }
}

double square (int n) {
    return double(n*n);
}

std::string to_string(double d) {
    return std::to_string(d);
}

int add (int a, int b) {
    return a + b;
}

int mult (int a, int b) {
    return a * b;
}

template <typename T>
T sum(std::vector<T> v) {
    return fold<int>(v, add, 0);
}

// reverse?

int main(void) {
    std::vector<int> v = {1,2,3,4,5};
    std::vector<double> vd = map<int, double>(v, square);
    // std::vector<std::string> vds = map<double, std::string>(vd, to_string);

    print(v);
    print(vd);
    // print(vds);

    std::cout << "sum = " << sum(v) << std::endl;
    std::cout << "fold mult = " << fold<int>(v, mult, 1) << std::endl;


    return 0;
}