#include <iostream>
#include <utility>
#include <type_traits>
#include <vector>
#include <string>

template <typename ...Ts>
struct toople {
public:
    // how can I take args and use in other func's?
    toople (Ts ...ts) 
      : pack{new uint8_t[pack_size()]}, size{0}
    {
        (place(ts), ...);
    }
    ~toople() {
        // delete [] pack;
    }

    template <std::size_t idx>
    decltype(auto) get(toople<Ts...> tpl) {
        std::size_t offset = offset_at(idx);

        typename get_type<idx, Ts...>::type elem;
        memcpy(&elem, &tpl.pack[offset], sizeof(decltype(elem)));

        return elem;
    }

private:
    uint8_t* pack;
    std::size_t size;

    
    constexpr std::size_t pack_size() const {
        return (sizeof(Ts) + ...);
    }

    template <typename T>
    void place(T t) {
        memcpy(&pack[size], &t, sizeof(T));
        size += sizeof(T);
        // std::cout << "(" << t << ", " << sizeof(T) << ", " << size << ")" << std::endl;
    }

    constexpr std::size_t offset_at(int idx) const {
        std::size_t offset = 0;

        // braces are evaluated from left to right
        auto dummy [[maybe_unused]] = {(--idx >= 0? offset += sizeof(Ts) : 0)...};
        
        // wrong evaluation order (right to left)
        // [&](...) {} ((--idx >= 0? offset += sizeof(Ts) : 0)...);
        return offset;
    }

    template<std::size_t idx, typename T_, typename ...Ts_>
    struct get_type {
        using type = get_type<idx-1, Ts_...>::type;
    };

    template<typename T_, typename ...Ts_>
    struct get_type<0, T_, Ts_...> {
        using type = T_;
    };
};

int main() {
    // toople<long long unsigned, int, short, double, std::string> t(0, 1, 2, 3.14, "hello, toople!");
    toople<long long unsigned, int, short, double, bool> t(0, 1, 2, 3.14, true);

    std::cout << t.get<0>(t) << std::endl;
    std::cout << t.get<1>(t) << std::endl;
    std::cout << t.get<2>(t) << std::endl;
    std::cout << t.get<3>(t) << std::endl;
    std::cout << t.get<4>(t) << std::endl;

    return 0;
}