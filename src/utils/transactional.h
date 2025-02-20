#ifndef BDK_TRANSACTIONAL_H
#define BDK_TRANSACTIONAL_H

#include <memory>
#include <tuple>

namespace transactional {

template<typename T, std::invocable<T&> Revert>
class BasicTransactional {
public:
  BasicTransactional(T& target, Revert revert)
    : target_(&target), revert_(std::move(revert)) {}

  ~BasicTransactional() { revert(); }

  BasicTransactional(const BasicTransactional&) = delete;

  BasicTransactional(BasicTransactional&& other) noexcept
    : target_(other.target_), revert_(std::move(other.revert_)) {
    other.target_ = nullptr;
  }

  BasicTransactional& operator=(const BasicTransactional&) = delete;

  BasicTransactional& operator=(BasicTransactional&& other) noexcept {
    using std::swap;
    swap(target_, other.target_);
    swap(revert_, other.revert_);
    return *this;
  }

  void commit() { target_ = nullptr; }

  void revert() {
    if (target_ == nullptr)
      return;

    std::invoke(revert_, *target_);
    target_ = nullptr;
  }

private:
  T *target_;
  Revert revert_;
};

class AnyTransactional {
public:
  template<typename T>
  explicit AnyTransactional(T transactional) : impl_(std::make_unique<Derived<T>>(std::move(transactional))) {}

  void commit() { impl_->commit(); }

  void revert() { impl_->revert(); }

private:
  struct Base {
    virtual ~Base() = default;
    virtual void commit() = 0;
    virtual void revert() = 0;
  };

  template<typename T>
  struct Derived : Base {
    explicit Derived(T impl) : impl_(std::move(impl)) {}
    void commit() override { impl_.commit(); }
    void revert() override { impl_.revert(); }
    T impl_;
  };

  std::unique_ptr<Base> impl_;
};

template<typename... Ts>
class Group {
public:
  constexpr Group(Ts... transactions) : transactions_(std::forward<Ts>(transactions)...) {}

  constexpr ~Group() { revert(); }

  constexpr void commit() {
    std::invoke([this] <size_t... Is> (std::index_sequence<Is...>) {
      (void) std::initializer_list<int>({(std::get<sizeof...(Ts) - Is - 1>(transactions_).commit(), 0)...});
    }, std::make_index_sequence<sizeof...(Ts)>{});
  }

  constexpr void revert() {
    std::invoke([this] <size_t... Is> (std::index_sequence<Is...>) {
      (void) std::initializer_list<int>({(std::get<sizeof...(Ts) - Is - 1>(transactions_).revert(), 0)...});
    }, std::make_index_sequence<sizeof...(Ts)>{});
  }

private:
  std::tuple<Ts...> transactions_;
};

template<typename T>
concept Checkpointable = requires (T t) { t.checkpoint(); };

auto checkpoint(auto& any) {
  return BasicTransactional(any, [any] (auto& ref) mutable { ref = std::move(any); });
}

auto checkpoint(Checkpointable auto& checkpointable) {
  return checkpointable.checkpoint();
}

auto copy(auto& target) {
  return BasicTransactional(target, [target] (auto& ref) mutable { ref = std::move(target); });
}

auto emplace(auto& container, auto&&... args) {
  auto [iterator, inserted] = container.emplace(std::forward<decltype(args)>(args)...);

  return std::make_tuple(BasicTransactional(container, [key = iterator->first, inserted] (auto& container) {
    if (inserted) {
      container.erase(key);
    }
  }), inserted);
}

auto emplaceOrAssign(auto& container, const auto& key, auto&&... args) {
  auto [iterator, inserted] = container.try_emplace(key, std::forward<decltype(args)>(args)...);

  using Mapped = std::remove_cvref_t<decltype(iterator->second)>;
  std::optional<Mapped> previousValue;

  if (!inserted) {
    previousValue = std::move(iterator->second);
    iterator->second = Mapped(std::forward<decltype(args)>(args)...);
  }

  return BasicTransactional(container, [key = iterator->first, mapped = std::move(previousValue)] (auto& container) {
    if (mapped.has_value()) {
      container.at(key) = std::move(mapped).value();
    } else {
      container.erase(key);
    }
  });
}

auto emplaceBack(auto& container, auto&&... args) {
  container.emplace_back(std::forward<decltype(args)>(args)...);
  return BasicTransactional(container, [] (auto& container) { container.pop_back(); });
}

} // namespace transactional

#endif // BDK_TRANSACTIONAL_H
