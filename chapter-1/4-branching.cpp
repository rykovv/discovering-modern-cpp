#include <iostream>

using std::cin;
using std::cout;

int main() {
    double first, second, third, middle;
    cout << "Introduce 3 numbers." << std::endl;
    cout << "1st number: ";
    cin >> first;
    cout << "2nd number: ";
    cin >> second;
    cout << "3rd number: ";
    cin >> third;

    middle = first > second && first < third ? first : second > first && second < third ? second : third;
    cout << "The middle number is " << middle << "." << std::endl;

    return 0;
}