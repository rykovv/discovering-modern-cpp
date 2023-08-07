
#include <iostream>

using std::cout;

int main() {
    // compiles well
    const unsigned c1{4000000000};
    // narrowing conversion errors
    // const unsigned c2{40000000000};
    // const int c2{-4000000000};
    // const float c2{1234567890123456789012345678901234567890.0};

    cout << c1 << /* ", " <<  c2 << */ std::endl;

    return 0;
}