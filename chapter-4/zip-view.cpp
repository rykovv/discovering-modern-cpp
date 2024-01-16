#include <iostream>
#include <tuple>
#include <vector>
#include <list>


template <typename ...Iters>
class zip_iterator {
    // let's assume for now ranges are of the same type
    // using riter = decltype( std::begin( std::declval<Range0>() ) );
    // using riter = decltype( std::begin( std::declval<Range>() ) );
    using self = zip_iterator<Iters...>;
    // using rtuple = std::tuple<riter, riter, decltype( std::begin( std::declval<RangeN>() ) )...>;
    using rtuple = std::tuple<Iters...>;
    using zip_type = std::tuple<typename Iters::value_type...>;
    // using type = decltype(std::begin(*std::declval(Iter0)));
public:
    // explicit zip_iterator(riter it0, riter it1, riter ...itN)
    //   : its{it0, it1, itN,...} {}

    explicit zip_iterator(Iters ...its)
      : its{its...} {}


    zip_type operator* () const noexcept {
        return _slice(std::make_index_sequence<sizeof...(Iters)>{});
    }
    template <std::size_t ...IterSize>
    zip_type _slice(std::index_sequence<IterSize...>) const {
        return std::make_tuple<typename Iters::value_type...>(std::move(*std::get<IterSize>(its))...);
        // return (*std::get<IterSize>(its),...);
    }

    bool operator!= (const self other) const noexcept {
        return _tuple_neq(other.its, std::make_index_sequence<sizeof...(Iters)>{});
    }
    template<typename TupleT, std::size_t ...IterSize>
    bool _tuple_neq(const TupleT& otherIts, std::index_sequence<IterSize...>) const {
        return ((std::get<IterSize>(its) != std::get<IterSize>(otherIts)) && ...);
    }

    self& operator++ () noexcept {
        _tuple_inc(std::make_index_sequence<sizeof...(Iters)>{});
        return *this; 
    }
    template<std::size_t ...IterSize>
    void _tuple_inc(std::index_sequence<IterSize...>) {
        (++std::get<IterSize>(its), ...);
    }

private:
    rtuple its;
};


template <typename ...Range>
class zip_view {
    using iterator = zip_iterator<typename Range::iterator...>;
    using iters = std::tuple<typename Range::iterator...>;
    using trs = std::tuple<Range...>;
public:
    zip_view (Range & ...rs)
      : rs{rs...} {}

    iterator begin() noexcept {
        return get_begins(std::make_index_sequence<sizeof...(Range)>{});
        // return std::make_tuple<typename Range::iterator...>(get_begins(std::make_index_sequence<sizeof...(Range)>{}));
        // return iter{std::for_each(rs.begin(), rs.end(), begin)};
        // return iter{begin(std::get<0>(rs))};
    }

    iterator end() noexcept {
        return get_ends(std::make_index_sequence<sizeof...(Range)>{});
        // return std::make_tuple<typename Range::iterator...>(get_ends(std::make_index_sequence<sizeof...(Range)>{}));
        // return iter{std::for_each(rs.begin(), rs.end(), end)};
        // return iter{end(std::get<0>(rs))};
    }


private:
    trs rs;

    template<std::size_t ...RangeSize>
    iterator get_begins(std::index_sequence<RangeSize...>) {
        // return std::make_tuple<typename Range::iterator...>(std::get<RangeSize>(rs).begin()...);
        return iterator{std::get<RangeSize>(rs).begin()...};
    }

    template<std::size_t ...RangeSize>
    iterator get_ends(std::index_sequence<RangeSize...>) {
        return iterator{std::get<RangeSize>(rs).end()...};
        // return zip_iterator{ std::get<RangeSize>(rs).end()... };
    }
};

namespace my_views {
    struct zip_t {
        template<typename ...Ranges>
        zip_view<Ranges...> operator() (Ranges&& ...ranges) {
            return zip_view<Ranges...>(std::forward(ranges)...);
        }
    };
    static constexpr zip_t zip;
}

template<typename TupleT, std::size_t ...IterSize>
void printTuple (const TupleT& t, std::index_sequence<IterSize...>) {
    std::cout << "{ ";
    auto dummy = {(std::cout << std::get<IterSize>(t) << ", ", 0)...};
    std::cout << " }" << std::endl;
};

int main() {
    std::vector<int> v0 = {1, 4, 7};
    std::vector<int> v1 = {2, 5, 8};
    std::vector<int> v2 = {3, 6, 9};
    
    // using viter = std::vector<int>::iterator;

    // for (auto it = zip_iterator<viter, viter>(v0.begin(), v1.begin()), 
    //     e = zip_iterator<viter, viter>(v0.end(), v1.end()); it != e; ++it) 
    // {
    //     auto const tupSize = std::tuple_size<decltype(*it)>::value;
    //     printTuple(*it, std::make_index_sequence<tupSize>{});
    // }

    // zip_view<std::vector<int>, std::vector<int>, std::vector<int>> zip(v0, v1, v2);
    for (auto [s0, s1, s2] : my_views::zip(v0, v1, v2)) {
        std::cout << "{ " << s0 << ", " << s1 << ", " << s2 << " }" << std::endl;
    }

    return 0;
}