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

class odd_range {
public:
    using iterator = odd_iterator;

    odd_range(int begin, int end)
      : vbegin{begin}, vend{end}
    {}

    iterator begin() const {
        return iterator{vbegin};
    }
    iterator end() const {
        return iterator{vend};
    }

private:
    int vbegin;
    int vend;
};

int main () {
    for (int i : odd_range(7, 27))
        std::cout << i << "\n";

    return 0;
}