#include <iostream>

struct stack_overflow {};
struct stack_underflow {};

template <typename T>
class stack {
public:
    stack() {
        _arr = new T[_max_size];
    }

    ~stack() {
        delete [] _arr;
    }

    T top() {
        return _size > 0 ? _arr[_size - 1] : nullptr;
    }

    void pop() {
        if (_size > 0) {
            _size--;
        } else {
            throw stack_underflow {};
        }
    }

    void push(T elem) {
        if (_size < _max_size) {
            _arr[_size] = elem;
            _size++;
        } else {
            throw stack_overflow {};
        }
    }

    void clear() {
        _size = 0;
    }

    size_t size() {
        return _size;
    }

    bool full() {
        return _size == _max_size;
    }

    bool empty() {
        return _size == 0;
    }

    void print() {
        std::cout << "[";
        if (_size > 0) {
            std::cout << _arr[0];
        }
        if (_size > 1) {
            for (size_t i = 1; i < _size; i++) {
                std::cout << ", " << _arr[i];
            }
        }
        std::cout << "]" << std::endl;
    }

private:
    size_t _size = 0;
    size_t _max_size = 32;
    T *_arr;
};

int main () {
    stack<std::string> s;
    
    s.print();
    s.push("zero");
    s.push("one");
    s.push("two");
    s.push("three");
    s.push("four");

    s.print();

    s.pop();
    s.print();

    std::cout << s.top();

    return 0;
}