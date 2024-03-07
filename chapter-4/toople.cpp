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
// ASK: how to unify it with get_
template <typename T, std::size_t idx>
struct get_index_ {
    template <typename ...Ts>
    decltype(auto) operator() (toople<Ts...> const& t) {
        if constexpr (std::is_same_v<T, decltype(t.t)>) {
            return get_index_<T, idx-1>{}(t.ts);
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

// template <typename T, std::size_t idx = 0, typename ...Ts>
// decltype(auto) get(toople<Ts...> const& t) {
//     return get_index_<T, idx>{}(t);
// }

// different types, don't care idx
template <typename T0, typename T1, std::size_t idx>
struct get_up_ {
    template <typename ...Ts>
    decltype(auto) operator() (T1 const& prev, toople<Ts...> const& t) {
        return get_up_<T0, decltype(t.t), idx>{}(t.t, t.ts);
    }
};

// same types, non-zero idx
template <typename T, std::size_t idx>
struct get_up_<T, T, idx> {
    // general case
    template <typename ...Ts>
    decltype(auto) operator() (T const& prev, toople<Ts...> const& t) {
        return get_up_<T, decltype(t.t), idx-1>{}(t.t, t.ts);
    }
    // overload for a tuple with one elem
    decltype(auto) operator() (T const& prev, toople<T> const& t) {
        return get_up_<T, decltype(t.t), idx-1>{}(t.t, t);
    }
};

// same types, zero idx
template <typename T>
struct get_up_<T, T, 0> {
    template <typename ...Ts>
    decltype(auto) operator() (T const& prev, toople<Ts...> const& t) {
        return prev;
    }
};

template <typename T, std::size_t idx = 0, typename ...Ts>
decltype(auto) get(toople<Ts...> const& t) {
    return get_up_<T, decltype(t.t), idx>{}(t.t, t);
}

template <typename T, std::size_t idx>
struct get_idx_by_type {
    template <typename ...Ts>
    constexpr std::size_t operator() (toople<Ts...> const& t) {
        if constexpr (std::is_same_v<T, decltype(t.t)>) {
            return 1 + get_idx_by_type<T, idx-1>{}(t.ts);
        } else {
            return 1 + get_idx_by_type<T, idx>{}(t.ts);
        }
    }

    template <typename R>
    constexpr std::size_t operator() (toople<R> const& t) {
        static_assert(idx == 0, "Toople index out of bounds.");
        return 0;
    }
};

template <typename T>
struct get_idx_by_type<T, 0> {
    template <typename ...Ts>
    constexpr std::size_t operator() (toople<Ts...> const& t) {
        if constexpr (std::is_same_v<T, decltype(t.t)>) {
            return 0;
        } else {
            return 1 + get_idx_by_type<T, 0>{}(t.ts);
        }
    }

    template <typename R>
    constexpr std::size_t operator() (toople<R> const& t) {
        if constexpr (std::is_same_v<T, R>) {
            return 0;
        } else {
            static_assert(std::is_same_v<T, R>, "Toople index out of bounds.");
        }
    }
};

template <typename T, std::size_t idx = 0, typename ...Ts>
constexpr decltype(auto) get_idx(toople<Ts...> const& t) {
    return get_idx_by_type<T, idx>{}(t);
}

template <typename ...Ts>
toople<Ts...> make_toople(Ts ...ts) {
    return toople<Ts...>{ts...};
}

template <typename T, std::size_t idx = 0, typename ...Ts>
constexpr decltype(auto) get_i(const toople<Ts...> t) {
    return get_<get_idx_by_type<T, idx>{}(t)>{}(t);
}


template <typename Toople, typename ...Tooples>
struct toople_cat_ {
    decltype(Toople::T) t;
    toople_cat_<decltype(Toople::ts), Tooples...> tooples;
};

template <typename Toople>
struct toople_cat_<Toople> {
    typename Toople::T t;
    toople_cat_<decltype(Toople::ts)> toople;
};

template <typename T>
struct toople_cat_<toople<T>> {
    T t;
};

template <typename Toople, typename ...Tooples>
constexpr decltype(auto) toople_cat (const Toople t, const Tooples ...ts) {
    return toople_cat_<Toople, Tooples...>{t, ts...};
}

template <typename ...Ts, typename ...Tooples>
constexpr decltype(auto) toople_cat1 (const toople<Ts...> t, const Tooples ...ts) {
    return toople<decltype(t.t), Ts...>{t.t, t.ts, ts...};
}

int main() {
    toople<long long unsigned, short, int, double, std::string, std::string> t{0, 1, 2, 3.14, "hello, toople!", "Extra string"};
    // auto t = make_toople(0, 1, 2, 3.14, "hello, toople!", "Extra string");

    std::cout << get<0>(t) << std::endl;
    std::cout << get<1>(t) << std::endl;
    std::cout << get<2>(t) << std::endl;
    std::cout << get<3>(t) << std::endl;
    std::cout << get<4>(t) << std::endl;

    std::cout << get<std::string>(t) << std::endl; // hello, toople!
    std::cout << get<std::string, 1>(t) << std::endl; // Extra string

    std::cout << get_idx<long long unsigned>(t) << std::endl;
    // std::cout << get_idx_by_type<int>{}(t) << std::endl;
    // std::cout << get_idx_by_type<short>{}(t) << std::endl;
    // std::cout << get_idx_by_type<double>{}(t) << std::endl;
    std::cout << get_idx<std::string>(t) << std::endl;
    std::cout << get_idx<std::string, 1>(t) << std::endl;
    // std::cout << get_idx<float>(t) << std::endl;

    std::cout << get_i<std::string>(t) << std::endl; // hello, toople!
    std::cout << get_i<std::string, 1>(t) << std::endl; // Extra string

    toople<std::string, std::string> t0 {"toople 0 1", "toople 0 2"};
    toople<std::string, std::string> t1 {"toople 1 1", "toople 1 2"};

    auto tt = toople_cat(t0, t1);

    return 0;
}