#include <iostream>
#include <memory>

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

class stack_bool_proxy {
public:
    stack_bool_proxy(unsigned char& byte, int bpos)
      : byte{byte}, mask{static_cast<unsigned char>(1 << bpos)}
    {}

    operator bool() const {
        return byte & mask;
    }

    stack_bool_proxy& operator= (bool b) {
        if (b) {
            byte |= mask;
        } else {
            byte &= ~mask;
        }
        return (*this);
    }
    unsigned char& byte;
private:
    
    unsigned char  mask;
};

struct empty_bool_stack{};

template<>
class stack<bool> {
public:
    stack()
      : _size{0}, _data{new unsigned char[(_max_size + 7) / 8]}
    {}

    bool top() {
        if (_size == 0) {
            throw empty_bool_stack{};
        }

        return stack_bool_proxy(_data[_size / 8], _size % 8);
    }

    void push(bool elem) {
        if (_size < _max_size) {
            stack_bool_proxy bprox (_data[_size / 8], _size % 8);
            bprox = elem;
            _size++;
        } else {
            throw stack_overflow {};
        }
    }

    void print() {
        std::cout << "[";
        if (_size > 0) {
            std::cout << bool(stack_bool_proxy(_data[0], 0));
        }
        if (_size > 1) {
            for (size_t i = 1; i < _size; i++) {
                std::cout << ", " << bool(stack_bool_proxy(_data[i / 8], i % 8));
            }
        }
        std::cout << "]" << std::endl;
    }

private:
    size_t                              _size = 0;
    size_t                              _max_size = 23;
    std::unique_ptr<unsigned char []>   _data;
};

int main () {
    stack<bool> s;
    s.push(true);
    s.push(false);
    s.push(true);
    s.push(false);
    s.push(true);
    s.push(false);
    s.push(true);
    s.push(false);
    s.push(true);
    s.push(false);

    s.print();

    return 0;
}