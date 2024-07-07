#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

template <class P, class T>
concept Predicate = requires(P p, T t) { requires std::is_same_v<decltype(p(t)), bool>; };

template <class T>
concept RandomAccessIterator = requires(T t, typename T::difference_type n) {
    { t.begin() } -> std::convertible_to<typename T::iterator>;
    { t.end() } -> std::convertible_to<typename T::iterator>;
    { t.begin()[n] } -> std::convertible_to<decltype(*(t.begin() + n))>;
};

template <class T>
concept Container = requires(T t, size_t n) { requires !std::is_same_v<decltype(t[n]), void>; };

template <class T>
concept String = requires {
    requires std::is_convertible_v<T, typename std::string> ||
                 std::is_convertible_v<T, std::string_view>;
};

template <class T>
concept Optional = requires { requires std::is_same_v<T, std::optional<typename T::value_type>>; };

template <class T>
concept Pair = requires(T t) {
    { t.first } -> std::convertible_to<typename T::first_type>;
    { t.second } -> std::convertible_to<typename T::second_type>;
    requires String<typename T::first_type>;
};

template <class T>
concept Range = requires { requires std::ranges::range<T> && !String<T>; };

template <class T>
concept RangeWithPairs = requires { requires Range<T> && Pair<std::ranges::range_value_t<T>>; };

template <class T>
concept RangeWithoutPairs = requires { requires Range<T> && !Pair<std::ranges::range_value_t<T>>; };

namespace {

template <class T>
struct JsonObject : std::false_type {};

template <class T>
    requires std::is_same_v<T, bool> || std::is_integral_v<T> || std::is_floating_point_v<T>
struct JsonObject<T> : std::true_type {};

template <String T>
struct JsonObject<T> : std::true_type {};

template <RangeWithPairs T>
struct JsonObject<T> : JsonObject<typename std::ranges::range_value_t<T>::second_type> {};

template <RangeWithoutPairs T>
struct JsonObject<T> : JsonObject<std::ranges::range_value_t<T>> {};

template <Optional T>
struct JsonObject<T> : JsonObject<typename T::value_type> {};

template <class... Args>
    requires(JsonObject<Args>::value && ...)
struct JsonObject<std::variant<Args...>> : std::true_type {};

}  // namespace

template <class T>
concept Indexable = RandomAccessIterator<T> || Container<T>;

template <class T>
concept SerializableToJson = JsonObject<T>::value;