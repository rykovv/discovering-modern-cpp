#include <cassert>
#include <iostream>
#include <memory>
#include <initializer_list>
#include <algorithm>


template <typename T>
class MyVectorIterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T*;
    using reference = T&;

public:
    MyVectorIterator(T* ptr = nullptr)
      : _ptr{ptr}
    {}

    MyVectorIterator(const MyVectorIterator<T>& rawIter) = default;
    MyVectorIterator<T>& operator= (const MyVectorIterator<T>& rawIter) = default;
    MyVectorIterator<T>& operator= (T* ptr) {
        _ptr = ptr;
        return (*this);
    }

    ~MyVectorIterator() {}

    operator bool() const {
        return _ptr? true : false;
    }

    bool operator== (const MyVectorIterator<T> rawIter) const {
        return (_ptr == rawIter.getConstPtr());
    }
    bool operator!= (const MyVectorIterator<T> rawIter) const {
        return (_ptr != rawIter.getConstPtr());
    }

    MyVectorIterator<T>& operator+=(const difference_type& inc) {
        _ptr += inc;
        return (*this);
    }
    MyVectorIterator<T>& operator-=(const difference_type& dec) {
        _ptr -= dec;
        return (*this);
    }

    MyVectorIterator<T>& operator++() {
        ++_ptr;
        return (*this);
    }
    MyVectorIterator<T>& operator--() {
        --_ptr;
        return (*this);
    }

    MyVectorIterator<T> operator+ (const difference_type& rhs) {
        return MyVectorIterator(_ptr + rhs);
    }
    MyVectorIterator<T> operator- (const difference_type& rhs) {
        return MyVectorIterator(_ptr - rhs);
    }

    difference_type operator- (const MyVectorIterator<T>& rhs) {
        return std::distance(_ptr, rhs.getPtr());
    }

    T& operator* () {
        return *_ptr;
    }
    const T& operator* () const {
        return *_ptr;
    }
    T* operator-> () {
        return _ptr;
    }


    T* getPtr() const {
        return _ptr;
    }

    const T* getConstPtr() const {
        return _ptr;
    }

private:
    T* _ptr;
};


template <typename T>
class vector {
    using iterator = MyVectorIterator<T>;
    using const_iterator = MyVectorIterator<const T>;

    void check_size(int that_size) const { 
        assert(my_size == that_size);
    }

    void check_index(int i) const {
        assert(i >= 0 && i < my_size);
    }

public:
    explicit vector(int size)
      : my_size(size), data( new T[my_size] )
    {}

    vector()
      : my_size(0), data(0)
    {}
    
    ~vector() = default;

    vector(const vector& that)
      : my_size(that.my_size), data(new T[my_size])
    {
	    std::copy(&that.data[0], &that.data[that.my_size], &data[0]);
    }

    vector& operator=(const vector& that) {
        check_size(that.my_size);
        std::copy(&that.data[0], &that.data[that.my_size], &data[0]);
        return (*this);
    }

    vector(std::initializer_list<T> that)
      : my_size(that.size()), data{new T(that.size())}
    {
        std::copy(that.begin(), that.end(), &data[0]);
    }

    int size() const { 
        return my_size;
    }

    const T& operator[](int i) const {
        check_index(i);
        return data[i];
    }
		     
    T& operator[] (int i) {
        check_index(i);
        return data[i];
    }

    vector operator+(const vector& that) const {
        check_size(that.my_size);
        vector sum(my_size);

        for (int i= 0; i < my_size; ++i) 
            sum[i] = data[i] + that[i];

        return sum;
    }

    iterator begin() {
        return iterator (&data[0]);
    }
    iterator end() {
        return iterator (&data[my_size]);
    }
    const_iterator cbegin() {
        return const_iterator (&data[0]);
    }
    const_iterator cend() {
        return const_iterator (&data[my_size]);
    }
    

private:
    int                   my_size;
    // std::unique_ptr<T[]>  data;
    T*  data;
};

template<typename T>
void print (const vector<T>& v) {
    for (int i = 0; i < v.size(); i++)
        std::cout << v[i] << ' ';
    std::cout << "\n";
};

int main() {
    vector<int> v {2, 1, 6, 10, 11, 3};
    print(v);

    std::sort(v.begin(), v.end());
    print(v);
}