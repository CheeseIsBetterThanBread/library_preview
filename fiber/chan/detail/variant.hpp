#pragma once

#include <type_traits>
#include <optional>
#include <variant>

namespace exe::fiber::detail {

template <typename... Types>
auto ConstructorPtrHelper(size_t index, void* value) {
  return [](std::size_t i, void* v) {
    auto constructor =
        [&]<std::size_t I, std::size_t... Is>(
            this auto&& self,
            std::index_sequence<I, Is...>) -> std::variant<Types...> {
      using Type = std::variant_alternative_t<I, std::variant<Types...>>;
      if constexpr (sizeof...(Is) > 0) {
        if (i == I) {
          auto* actual = static_cast<std::optional<Type>*>(v);
          return std::variant<Types...>(std::in_place_index<I>,
                                        std::move(**actual));
        }
        return self(std::index_sequence<Is...>{});
      } else {
        auto* actual = static_cast<std::optional<Type>*>(v);
        return std::variant<Types...>(std::in_place_index<I>,
                                      std::move(**actual));
      }
    };

    return constructor(std::index_sequence_for<Types...>{});
  }(index, value);
}

template <typename... Types>
struct VariantConstructor {
  static std::variant<Types...> Construct(size_t index, void* value) {
    return ConstructorPtrHelper<Types...>(index, value);
  }
};

}  // namespace exe::fiber::detail

