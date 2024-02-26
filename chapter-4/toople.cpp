#include <iostream>
#include <utility>
#include <type_traits>
#include <vector>
#include <string>
#include <algorithm>
#include <type_traits>


namespace attempt0 {
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
            offset_rec<idx, Ts...> offset;
            
            typename get_type<idx, Ts...>::type elem;
            memcpy(&elem, &tpl.pack[offset()], sizeof(decltype(elem)));
            // std::copy(&tpl.pack[offset()], &tpl.pack[offset() + sizeof(decltype(elem))], &elem);

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

        template <std::size_t idx, typename T_, typename ...Ts_>
        struct offset_rec {
            using next_offset_rec = offset_rec<idx-1, Ts_...>;
            
            std::size_t operator() () const {
                return sizeof(T_) + next();
            }
            
        private:
            next_offset_rec next;
        };

        template <typename T_, typename ...Ts_>
        struct offset_rec<0, T_, Ts_...> {
            std::size_t operator() () const {
                return 0;
            }
        };

        template<std::size_t idx, typename T_, typename ...Ts_>
        struct get_type {
            using type = get_type<idx-1, Ts_...>::type;
        };

        template<typename T_, typename ...Ts_>
        struct get_type<0, T_, Ts_...> {
            using type = T_;
        };
    };
};


template <typename T, typename ...Ts>
struct toople {
    T t;
    toople<Ts...> ts;
};

template <typename T>
struct toople<T> {
    T t;
};

template <std::size_t idx>
struct get_ {
    template <typename ...Ts>
    decltype(auto) operator() (toople<Ts...> const& t) {
       return get_<idx-1>{}(t.ts);
    }
};

template <>
struct get_<0> {
    template <typename ...Ts>
    decltype(auto) operator() (toople<Ts...> const& t) {
       return t.t;
    }
};

template <std::size_t idx, typename ...Ts>
decltype(auto) get(toople<Ts...> const& t) {
    return get_<idx>{}(t);
}

template <typename T>
struct get_no_index_ {
    template <typename ...Ts>
    decltype(auto) operator() (toople<Ts...> const& t) {
        if constexpr (std::is_same_v<T, decltype(t.t)>) {
            return t.t;
        } else {
            return get_no_index_<T>{}(t.ts);
        }
    }
};

template <typename T, typename ...Ts>
decltype(auto) get_no_index(toople<Ts...> const& t) {
    return get_no_index_<T>{}(t);
}

// versitile, but not declarative
// ASK: how do I make it declarative?
template <typename T, std::size_t idx>
struct get_index_ {
    template <typename ...Ts>
    decltype(auto) operator() (toople<Ts...> const& t) {
        if constexpr (std::is_same_v<T, decltype(t.t)>) {
            if constexpr (idx == 0) {
                return t.t;
            } else {
                return get_index_<T, idx-1>{}(t.ts);
            }
        } else {
            return get_index_<T, idx>{}(t.ts);
        }
    }
};

template <typename T>
struct get_index_<T, 0> {
    template <typename ...Ts>
    decltype(auto) operator() (toople<Ts...> const& t) {
        if constexpr (std::is_same_v<T, decltype(t.t)>) {
            return t.t;
        } else {
            return get_index_<T, 0>{}(t.ts);
        }
    }
};

template <typename T, std::size_t idx = 0, typename ...Ts>
decltype(auto) get(toople<Ts...> const& t) {
    return get_index_<T, idx>{}(t);
}

int main() {
    toople<long long unsigned, int, short, double, std::string, std::string> t{0, 1, 2, 3.14, "hello, toople!", "Extra string"};

    std::cout << get<0>(t) << std::endl;
    std::cout << get<1>(t) << std::endl;
    std::cout << get<2>(t) << std::endl;
    std::cout << get<3>(t) << std::endl;
    std::cout << get<4>(t) << std::endl;

    std::cout << get<std::string>(t) << std::endl; // hello, toople!
    std::cout << get<std::string, 1>(t) << std::endl; // Extra string


    return 0;
}