#include <vector>
#include <iostream>

struct ascending {
    bool operator() (const double & lhs, const double & rhs) const {
        return lhs < rhs;
    }
};

struct discending {
    bool operator() (const double & lhs, const double & rhs) const {
        return lhs > rhs;
    }
};

template <typename T>
void print(std::vector<T> const & v) {
    std::cout << "{ ";
    for (auto val : v) {
        std::cout << val << " ";
    }
    std::cout << " }" << std::endl;
}

int main() {
    std::vector<double> v = {-9.3, -7.4, -3.8, -0.4, 1.3, 3.9, 5.4, 8.2};
    ascending asc;

    std::sort(v.begin(), v.end(), asc);
    print(v);

    discending desc;
    std::sort(v.begin(), v.end(), desc);
    print(v);

    std::sort(v.begin(), v.end(), 
        [](auto rhs, auto lhs) -> bool { 
            return rhs < lhs; 
        });
    print(v);

    return 0;
}