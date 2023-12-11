#include <iostream>

struct new_value_is_not_even {};


class odd_iterator {
public:
    odd_iterator()
      : value{1}
    {}
    odd_iterator(int new_value) {
        _throw_if_odd(new_value);
        value = new_value;
    }
    odd_iterator(const odd_iterator & it) = default;

    odd_iterator & operator++ () {
        value += 2;
        return (*this);
    }
    
    int operator* () const {
        return value;
    }

    bool operator== (const odd_iterator & rhs) const {
        return value == rhs.value;
    }
    bool operator!= (const odd_iterator & rhs) const {
        return value != rhs.value;
    }

    odd_iterator & operator= (int new_value) {
        _throw_if_odd(new_value);
        value = new_value;
        return (*this);
    }

private:
    int value;

    void _throw_if_odd (int new_value) {
        if (new_value % 2 == 0) {
            throw new_value_is_not_even{};
        }
    }
};

int main () {
    odd_iterator oi{};

    std::cout << *oi;

    for (int i = 0; i < 30; i++) {
        ++oi;
        std::cout << ", " << *oi;
    }

    return 0;
}