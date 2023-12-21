#include <memory>
#include <iostream>


// from cppref
// Constructs an object of type T and wraps it in a std::unique_ptr.
// Two overloads to implement
//   1. Constructs a non-array type T
//   2. Constructs an array of the given dynamic size. The array elements are value-initialized
//   3. Binded arrays

template <typename T>
std::unique_ptr<T>
make_unique(T&& t) {
    return std::unique_ptr<T>(new T(std::forward<T>(t)));
}

template <typename T, typename... Args>
std::unique_ptr<T>
make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// cannot differentiate between this and the previous one
// ask Mike how to specialize in a generic way
//   1. specializing make_unique for specific types

// template <typename T>
// std::unique_ptr<T>
// make_unique(size_t size) {
//     std::unique_ptr<T[]> up (new T[size]);
//     return up;
// }


template <typename T>
struct myVector {
    myVector(T t1, T t2, T t3)
      : t1{t1}, t2{t2}, t3{t3}
    {}

    void print() {
        std::cout << t1 << ", " << t2 << ", " << t3 << std::endl; 
    }
private:
    T t1, t2, t3;
};


int main() {
    int forty_two = 42;
    std::unique_ptr<int> unique42 = make_unique<int>(forty_two);
    std::cout << *unique42 << std::endl;

    std::unique_ptr<myVector<double> > u2 = make_unique<myVector<double> >(1.0, 2.0, 3.0);
    (*u2).print();

    // std::unique_ptr<int[]> a = make_unique<int[]>(10);

    return 0;
}