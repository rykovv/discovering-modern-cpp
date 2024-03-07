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

template <typename R, typename U>
list<U> map(list<R> l, std::function<U(R)> f, int i) {
    if ( l.v.size() == i ) {
        return {f(l.v[0])};
    } else {
        return { f(l.v[0]), map(l, f, i+1) };
    }
}

double square (int n) {
    return n*n;
}


int main(void) {
    list<int> l = {1,2,3,4,5};
    list<double> ld = map<int, double>(l, square, 0);
    print(ld);

    return 0;
}