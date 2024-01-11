#include <iostream>
#include <tuple>
#include <vector>
#include <list>


// template <typename Range0, typename Range1, typename ...RangeN>
template <typename T, typename ...Range>
class zip_iterator {
    // let's assume for now ranges are of the same type
    // using riter = decltype( std::begin( std::declval<Range0>() ) );
    // using riter = decltype( std::begin( std::declval<Range>() ) );
    using self = zip_iterator<Range...>;
    // using rtuple = std::tuple<riter, riter, decltype( std::begin( std::declval<RangeN>() ) )...>;
    using rtuple = std::tuple<Range...>;
public:
    // explicit zip_iterator(riter it0, riter it1, riter ...itN)
    //   : its{it0, it1, itN,...} {}

    explicit zip_iterator(Range ...its)
      : its{its...} {}


    std::tuple<T> operator* () const noexcept {
        return {std::get<0>(its)[0]};
    }
    bool operator!= (self other) const noexcept {
        return its != other.its;
    }
    self& operator++ () noexcept {
        std::apply(
            [](auto&&... it) {
                ((++it), ...);
            }, 
            its);
        return *this; 
    }

private:
    rtuple its;
};


template <typename ...Range>
class zip_view {
    using iter = zip_iterator<Range...>;
    using trs = std::tuple<Range...>;
public:
    zip_view (Range & ...rs)
      : rs{rs...} {}

    iter begin() const noexcept {
        return iter{begin(std::get<0>(rs))};
    }

    iter end() const noexcept {
        return iter{end(std::get<0>(rs))};
    }

private:
    trs rs;
};


int main() {
    std::vector<int> v = {1, 2, 3};

    // zip_iterator<int, std::vector<int>> zip(v);
    // std::cout << std::get<0>(*zip) << std::endl;

    // std::tuple<int, int> t = {1, 1};
    
    // std::cout << std::get<0>(t) << std::endl;

    return 0;
}