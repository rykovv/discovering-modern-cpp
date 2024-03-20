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

struct make_toople_ {
    template <typename T, typename ...Ts>
    toople<T, Ts...> operator() (T t, Ts ...ts) {
        make_toople_ mt;
        return toople<T, Ts...>{t, mt(ts...)};
    }
    template <typename T>
    toople<T> operator() (T t) {
        return toople<T>{t};
    }
};

template <typename ...Ts>
toople<Ts...> make_toople(Ts ...ts) {
    make_toople_ mt;
    return mt(ts...);
}

template <typename T, std::size_t idx = 0, typename ...Ts>
constexpr decltype(auto) get_i(const toople<Ts...> t) {
    return get_<get_idx_by_type<T, idx>{}(t)>{}(t);
}


// template <typename Toople, typename ...Tooples>
// struct toople_cat_ {
//     decltype(Toople::T) t;
//     toople_cat_<decltype(Toople::ts), Tooples...> tooples;
// };

// template <typename Toople>
// struct toople_cat_<Toople> {
//     typename Toople::T t;
//     toople_cat_<decltype(Toople::ts)> toople;
// };

// template <typename T>
// struct toople_cat_<toople<T>> {
//     T t;
// };

// template <typename Toople, typename ...Tooples>
// constexpr decltype(auto) toople_cat (const Toople t, const Tooples ...ts) {
//     return toople_cat_<Toople, Tooples...>{t, ts...};
// }

// template <typename ...Ts, typename ...Tooples>
// constexpr decltype(auto) toople_cat1 (const toople<Ts...> t, const Tooples ...ts) {
//     return toople<decltype(t.t), Ts...>{t.t, t.ts, ts...};
// }

struct toople_printer {
public:
    template <typename ...Ts>
    void operator()(toople<Ts...> t) {
        std::cout << "{ " << t.t << " ";
        printer(t.ts);
    }
private:
    template <typename ...Ts>
    void printer(toople<Ts...> t) {
        std::cout << t.t << " ";
        printer(t.ts);
    }
    template <typename T>
    void printer(toople<T> t) {
        std::cout << t.t << " }" << std::endl;
    }
};

template <typename ...Rs>
struct toople_cat {
    template <typename T, typename ...Ts>
    decltype(auto) operator() (toople<T, Ts...> t0, toople<Rs...> t1) {
        toople_cat<Rs...> tcat;
        return toople<T, Ts..., Rs...>{t0.t, tcat(t0.ts, t1)};
    }

    template <typename T>
    decltype(auto) operator() (toople<T> t0, toople<Rs...> t1) {
        return toople<T, Rs...>{t0.t, t1};
    }
};

int main() {
    toople<long long unsigned, short, int, double, std::string, std::string> t{0, 1, 2, 3.14, "hello, toople!", "Extra string"};

    // get by index
    std::cout << get<0>(t) << std::endl;
    std::cout << get<1>(t) << std::endl;
    std::cout << get<2>(t) << std::endl;
    std::cout << get<3>(t) << std::endl;
    std::cout << get<4>(t) << std::endl;

    // get by type and type order index
    std::cout << get<std::string>(t) << std::endl; // hello, toople!
    std::cout << get<std::string, 1>(t) << std::endl; // Extra string

    // get index by type
    std::cout << get_idx<long long unsigned>(t) << std::endl;
    // std::cout << get_idx_by_type<int>{}(t) << std::endl;
    // std::cout << get_idx_by_type<short>{}(t) << std::endl;
    // std::cout << get_idx_by_type<double>{}(t) << std::endl;
    std::cout << get_idx<std::string>(t) << std::endl;
    std::cout << get_idx<std::string, 1>(t) << std::endl;
    // std::cout << get_idx<float>(t) << std::endl;

    // get by type using get by index
    std::cout << get_i<std::string>(t) << std::endl; // hello, toople!
    std::cout << get_i<std::string, 1>(t) << std::endl; // Extra string

    // toople cat
    toople<std::string, std::string> t0 {"toople 0 1", "toople 0 2"};
    toople<std::string, std::string> t1 {"toople 1 1", "toople 1 2"};

    toople_cat<std::string, std::string> tc;
    auto tt = tc(t0, t1);
    toople_printer printer;
    printer(tt);

    // make toople
    auto mkt = make_toople(0, 1, 2, 3.14, "hello, toople!", "Extra string");
    printer(mkt);

    return 0;
}