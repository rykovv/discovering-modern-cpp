#include <iostream>
#include <sstream>


template<typename T>
std::stringstream & to_tuple_string_rec_helper(std::stringstream & ss, T t) {
    ss << t;
    return ss;
}

template<typename T, typename ...P>
std::stringstream & to_tuple_string_rec_helper(std::stringstream & ss, T t, P ...p) {
    ss << t << ", ";
    return to_tuple_string_rec_helper(ss, p...);
}

template <typename ...T>
std::string to_tuple_string_rec() {
    std::stringstream ss;
    ss << "()";

    return ss.str();
}

template <typename ...T>
std::string to_tuple_string_rec(T const& ...t) {
    std::stringstream ss;
    ss << "(";

    to_tuple_string_rec_helper(ss, t...);
    
    // direct unfolding is also possible, but can't avoid ', ' at the end
    // auto dummy [[maybe_unused]] = {(ss << t << ", ", 0)...};
    
    ss << ")";

    return ss.str();
}

template <typename ...T>
std::string to_tuple_string_dummy(T const& ...t) {
    std::stringstream ss;
    ss << "(";

    // direct unfolding is also possible, but can't avoid ', ' at the end
    auto dummy [[maybe_unused]] = {(ss << t << ", ", 0)...};
    
    ss << ")";

    return ss.str();
}

template <typename ...T>
std::string to_tuple_string_fold() {
    std::stringstream ss;
    
    ss << "()";

    return ss.str();
}

template <typename T>
std::string to_tuple_string_fold(T const& t) {
    std::stringstream ss;
    
    ss << "(" << t << ")";

    return ss.str();
}

template <typename P, typename ...T>
std::string to_tuple_string_fold(P const& p, T const& ...t) {
    std::stringstream ss;
    auto size = sizeof...(T);

    ss << "(" << p;

    if (size > 0) {
        auto dummy [[maybe_unused]] = {( ss << ", " << t , 0)... };
    }
    
    ss << ")";

    return ss.str();
}

int main() {
    int x = 0, y = 1, z = 3;

    // dummy (always has trailing ', ')
    std::cout << to_tuple_string_dummy(x, y, z) << std::endl;
    
    // recursion (needed 2 overloads and 2 helper functions)
    std::cout << to_tuple_string_rec(x, y, z) << std::endl;
    
    // folding (needed 3 overloads)
    std::cout << to_tuple_string_fold(x, y, z) << std::endl;

    // fake initilizer list ???
    
    return 0;
}