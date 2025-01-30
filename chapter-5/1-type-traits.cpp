#include <type_traits>

template <typename T>
struct remove_ref {
    using type = T;
};

template <typename T>
struct remove_ref<T&> {
    using type = T;
};

template <typename T>
struct add_ref {
    using type = T&;
};

template <typename...>
struct my_vector;

template <typename Value>
struct my_vector<Value> {
    using type_value = Value;
};

template <typename E1, typename E2>
struct my_vector<E1, E2> {
    using type_e1 = E1;
    using type_e2 = E2;
};

template <typename...>
struct is_vector {
    static constexpr bool value = false;
};

template <typename Value>
struct is_vector<my_vector<Value>> {
    static constexpr bool value = true;
};

template <typename E1, typename E2>
struct is_vector<my_vector<E1, E2>> {
    static constexpr bool value = true;
};


int main() {

    static_assert(std::is_same_v<remove_ref<int&>::type, int>);
    static_assert(std::is_same_v<add_ref<int>::type, int&>);
    static_assert(is_vector<my_vector<int>>::value);
    static_assert(is_vector<my_vector<float, float>>::value);

    return 0;
}