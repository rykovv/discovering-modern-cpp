#include <iostream>
#include <utility>
#include <type_traits>
#include <vector>

template <typename ...Ts>
struct toople {
public:
    // how can I take args and use in other func's?
    toople (Ts ...ts) 
      : pack{new uint8_t[size()]}
    {
        (place(ts), ...);
    }
    ~toople() {
        delete [] pack;
    }

    std::size_t size() const {
        return (sizeof(Ts) + ...);
    }

    template <std::size_t idx>
    decltype(auto) get(toople<Ts...> tpl) {
        // std::size_t offset = offset_at(idx);
        
        // decltype(auto) elem = getT<idx>(std::make_index_sequence<sizeof...(Ts)>{});

        // elem_type elem;
        // type_getter<Ts..., idx=='make_index_sequence through the pack'>::type elem;
        // how do I get the type I need from the parameter pack?
        // memcpy(&elem, &tpl.pack[offset], sizeof(decltype(elem)));

        // return elem;
        is_right_idx<idx> iri{idx};
        return (getAlt<Ts, decltype(iri), idx>()...);
    }
    
    template<typename T, bool Cond>
    struct type_getter {};

    template<typename T>
    struct type_getter<T, true> {
        using type = T;
    };

    template <typename std::size_t idx>
    struct is_right_idx {
        std::size_t in_idx;
        bool value = idx == in_idx;
    };

    template <typename T, typename is_right_index, std::size_t idx>
    typename type_getter<T, is_right_index<idx>::value>::type
    getAlt() const {
        T elem;
        
        std::size_t offset = idx;
        memcpy(&elem, &tpl.pack[offset], sizeof(T));
        
        return T;
    }

    

    

    // template<std::size_t idx, std::size_t ...Szs>
    // decltype(auto) getT(std::index_sequence<Szs...>) {

    //     ((typename type_getter<Ts, Szs==idx>::type t)..., 0);

    //     // std::vector<> t {type_getter<Ts, idx==Szs>{}...};
    //     // static_assert( std::is_same<decltype(elem), long long unsigned>::value == true );
    //     // static_assert(std::is_same_v<decltype(el), long long unsigned>::value == true);
    //     return 0;
    // }

private:
    uint8_t* pack;

    template <typename T>
    void place(T t) {
        static std::size_t i = 0;
        memcpy(&pack[i], &t, sizeof(T));
        i += sizeof(T);
        // std::cout << i << std::endl;
    }

    std::size_t offset_at(int idx) const {
        std::size_t offset = 0;

        // braces are evaluated from right to left
        auto dummy [[maybe_unused]] = {(--idx >= 0? offset += sizeof(Ts) : 0)...};
        
        // wrong evaluation order (left to right)
        // [&](...) {} ((--idx >= 0? offset += sizeof(Ts) : 0)...);
        return offset;
    }


};

int main() {
    toople<long long unsigned, int, short> t(0, 1, 2);

    std::cout << t.get<1>(t) << std::endl;

    return 0;
}