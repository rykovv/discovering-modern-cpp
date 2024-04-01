#include <vector>
#include <iostream>
#include <algorithm>

int main () {
    std::vector<int> vi{12, 3, 15, 5, 7, 9};
    
    for (int i = 2; i < 10; ++i) {
        // auto it = find_first_multiple(vi, i);
        auto it = find_if(vi.begin(), vi.end(), 
            [i](int candidate) -> bool { 
                return candidate % i == 0; 
            });
        if (it != vi.end())
            std::cout << "The first multiple of " << i << " is " << *it << std::endl;
        else
            std::cout << "There is no multiple of " << i << std::endl;

}

    return 0;
}