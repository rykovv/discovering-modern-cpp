#include <iostream>
#include <sstream>

template<typename T>
std::string to_string(T const& t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}

int main() {
    float pi = 3.1415;
    int ten = 10;
    std::string s = "string";

    std::cout << to_string(pi) << ", " << to_string(ten) << ", " << to_string(s) <<std::endl;

    return 0;
}