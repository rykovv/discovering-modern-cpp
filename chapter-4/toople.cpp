#include <iostream>

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
        std::size_t offset = offset_at(idx);
        
        // elem_type elem;
        type_getter<Ts..., idx=='make_index_sequence through the pack'>::type elem;
        // how do I get the type I need from the parameter pack?
        memcpy(&elem, tpl.pack[offset], sizeof(elem_type));

        return elem;
    }
    
    template<typename T, bool Cond = true>
    struct type_getter {
        using type = T;
    };

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

    // std::cout << t.offset_at(3) << std::endl;

    return 0;
}