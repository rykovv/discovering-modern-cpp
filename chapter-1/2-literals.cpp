#include <iostream>

using std::cout;

int main() {
    const auto c1{4000000000u};
    const auto c2{4000000000000lu};
    const auto c3{400000000000000000llu};

    cout << c1 << ", " <<  c2 << ", " <<  c3 << std::endl;

    return 0;
}