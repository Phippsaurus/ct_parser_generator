#if !defined(PARSER_HPP)
#define PARSER_HPP

#include <array>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace parser {

template <bool Cond, typename If, typename Else> struct if_t {};

template <typename If, typename Else> struct if_t<true, If, Else> {
  using type = If;
};

template <typename If, typename Else> struct if_t<false, If, Else> {
  using type = Else;
};

template <typename... Ts> struct set {
  constexpr static std::size_t num_elements = sizeof...(Ts);
};

template <size_t Idx, typename T, typename... Ts,
          typename std::enable_if<Idx == 0, int>::type = 0>
constexpr auto get_element(set<T, Ts...>) noexcept -> T;

template <size_t Idx, typename T, typename... Ts,
          typename std::enable_if<0 < Idx, int>::type = 0>
constexpr auto get_element(set<T, Ts...>) noexcept
    -> decltype(get_element<Idx - 1>(set<Ts...>()));

template <typename Lhs, typename Seen, typename... Rhs> struct bullet_rule {};

template <typename Lhs, typename... Seen, typename... Rhs>
constexpr auto lhs(bullet_rule<Lhs, set<Seen...>, Rhs...>) noexcept -> Lhs;

template <typename Lhs, typename... Seen, typename... Rhs>
constexpr size_t sizeof_rhs(bullet_rule<Lhs, set<Seen...>, Rhs...>) noexcept {
  return sizeof...(Rhs);
}

template <typename Lhs, typename... Rhs>
constexpr auto make_rule(Lhs, Rhs...) -> bullet_rule<Lhs, set<>, Rhs...>;

template <typename... T, typename Element>
constexpr bool contains(set<T...>, Element) noexcept {
  return (std::is_same<Element, T>::value || ... || false);
}

template <typename... T> constexpr bool empty(set<T...>) noexcept {
  return sizeof...(T) == 0;
}

template <typename Element, typename... T> struct contains_t {
  constexpr static bool value =
      (std::is_same<Element, T>::value || ... || false);
};

template <
    typename Element, typename... Ts,
    typename std::enable_if<contains_t<Element, Ts...>::value, int>::type = 0>
constexpr auto add(set<Ts...>, Element const &) noexcept -> set<Ts...>;

template <
    typename Element, typename... Ts,
    typename std::enable_if<!contains_t<Element, Ts...>::value, int>::type = 0>
constexpr auto add(set<Ts...>, Element const &) noexcept -> set<Ts..., Element>;

template <
    typename Element, typename... Ts,
    typename std::enable_if<contains_t<Element, Ts...>::value, int>::type = 0>
constexpr auto prepend(Element const &, set<Ts...>) noexcept -> set<Ts...>;

template <
    typename Element, typename... Ts,
    typename std::enable_if<!contains_t<Element, Ts...>::value, int>::type = 0>
constexpr auto prepend(Element const &, set<Ts...>) noexcept
    -> set<Element, Ts...>;

template <typename Element>
constexpr auto remove(set<>, Element const &element) noexcept -> set<>;

template <
    typename Element, typename T, typename... Ts,
    typename std::enable_if<!std::is_same<Element, T>::value, int>::type = 0>
constexpr auto remove(set<T, Ts...>, Element const &element) noexcept
    -> decltype(prepend(std::declval<T>(), remove(set<Ts...>(), element)));

template <
    typename Element, typename T, typename... Ts,
    typename std::enable_if<std::is_same<Element, T>::value, int>::type = 0>
constexpr auto remove(set<T, Ts...>, Element const &) noexcept -> set<Ts...>;

template <typename Element, typename... Ts> struct add_t {
  using type = typename if_t<contains_t<Element, Ts...>::value, set<Ts...>,
                             set<Ts..., Element>>::type;
};

template <typename... Ts>
constexpr auto join(set<Ts...>, set<>) noexcept -> set<Ts...> {
  return {};
}

template <typename... Ts, typename U, typename... Us>
constexpr auto join(set<Ts...>, set<U, Us...>) noexcept
    -> decltype(join(std::declval<typename add_t<U, Ts...>::type>(),
                     set<Us...>())) {
  return {};
}

template <typename Lhs, typename... Seen, typename First, typename... Rhs>
constexpr auto first(bullet_rule<Lhs, set<Seen...>, First, Rhs...>) noexcept
    -> First;

template <typename Nonterminal>
constexpr auto with_lhs(Nonterminal const &nonterminal, set<>) noexcept
    -> set<>;

template <typename Nonterminal, typename Lhs, typename... Seen, typename... Rhs,
          typename... Rules,
          typename std::enable_if<std::is_same<Nonterminal, Lhs>::value,
                                  int>::type = 0>
constexpr auto
with_lhs(Nonterminal const &nonterminal,
         set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>) noexcept
    -> decltype(prepend(bullet_rule<Lhs, set<Seen...>, Rhs...>(),
                        with_lhs(nonterminal, set<Rules...>())));

template <typename Nonterminal, typename Lhs, typename... Seen, typename... Rhs,
          typename... Rules,
          typename std::enable_if<!std::is_same<Nonterminal, Lhs>::value,
                                  int>::type = 0>
constexpr auto
with_lhs(Nonterminal const &nonterminal,
         set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>) noexcept
    -> decltype(with_lhs(nonterminal, set<Rules...>()));

template <typename Lhs, typename... Seen, typename FirstRhs, typename... Rhs,
          typename... Rules, typename... Nonterminals,
          typename std::enable_if<contains_t<FirstRhs, Nonterminals...>::value,
                                  int>::type = 0>
constexpr auto expand(bullet_rule<Lhs, set<Seen...>, FirstRhs, Rhs...>,
                      set<Rules...> rules,
                      set<Nonterminals...> nonterminals) noexcept
    -> decltype(with_lhs(std::declval<FirstRhs>(), rules));

template <typename Lhs, typename... Seen, typename FirstRhs, typename... Rhs,
          typename... Rules, typename... Nonterminals,
          typename std::enable_if<!contains_t<FirstRhs, Nonterminals...>::value,
                                  int>::type = 0>
constexpr auto
expand(bullet_rule<Lhs, set<Seen...>, FirstRhs, Rhs...> bullet_rule,
       set<Rules...> rules, set<Nonterminals...> nonterminals) noexcept
    -> set<>;

template <typename... AllRules, typename... Nonterminals>
constexpr auto closures(set<>, set<AllRules...>, set<Nonterminals...>) -> set<>;

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules,
          typename... AllRules, typename... Nonterminals,
          typename std::enable_if<sizeof...(Rhs) == 0, int>::type = 0>
constexpr auto closures(set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>,
                        set<AllRules...> all_rules,
                        set<Nonterminals...> nonterminals) noexcept
    -> decltype(prepend(bullet_rule<Lhs, set<Seen...>>(),
                        closures(set<Rules...>(), all_rules, nonterminals)));

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules,
          typename... AllRules, typename... Nonterminals,
          typename std::enable_if<(sizeof...(Seen) == 0 && sizeof...(Rhs) > 0),
                                  int>::type = 0>
constexpr auto closures(set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>,
                        set<AllRules...> all_rules,
                        set<Nonterminals...> nonterminals) noexcept
    -> decltype(join(
        prepend(bullet_rule<Lhs, set<Seen...>, Rhs...>(),
                closures(expand(bullet_rule<Lhs, set<Seen...>, Rhs...>(),
                                all_rules, nonterminals),
                         all_rules, remove(nonterminals, std::declval<Lhs>()))),
        closures(set<Rules...>(), all_rules, nonterminals)));

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules,
          typename... AllRules, typename... Nonterminals,
          typename std::enable_if<(sizeof...(Seen) > 0 && sizeof...(Rhs) > 0),
                                  int>::type = 0>
constexpr auto closures(set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>,
                        set<AllRules...> all_rules,
                        set<Nonterminals...> nonterminals) noexcept
    -> decltype(
        join(prepend(bullet_rule<Lhs, set<Seen...>, Rhs...>(),
                     closures(expand(bullet_rule<Lhs, set<Seen...>, Rhs...>(),
                                     all_rules, nonterminals),
                              all_rules, nonterminals)),
             closures(set<Rules...>(), all_rules, nonterminals)));

template <typename Nonterminal, typename... Rules, typename... Nonterminals,
          typename std::enable_if<
              contains_t<Nonterminal, Nonterminals...>::value, int>::type = 1>
constexpr auto closure(Nonterminal const &nonterminal, set<Rules...> rules,
                       set<Nonterminals...> nonterminals) noexcept
    -> decltype(closures(with_lhs(nonterminal, set<Rules...>()), rules,
                         remove(nonterminals, nonterminal)));

template <typename Nonterminal, typename... Rules, typename... Nonterminals,
          typename std::enable_if<
              !contains_t<Nonterminal, Nonterminals...>::value, int>::type = 1>
constexpr auto closure(Nonterminal const &nonterminal, set<Rules...>,
                       set<Nonterminals...>) noexcept -> set<>;

template <typename Symbol>
constexpr auto go_to(Symbol const &symbol, set<>) -> set<>;

template <typename Symbol, typename Lhs, typename... Seen, typename... Rules>
constexpr auto go_to(Symbol const &symbol,
                     set<bullet_rule<Lhs, set<Seen...>>, Rules...>) noexcept
    -> decltype(go_to(symbol, set<Rules...>()));

template <typename Symbol, typename Lhs, typename... Seen, typename FirstRhs,
          typename... Rhs, typename... Rules,
          typename std::enable_if<!std::is_same<FirstRhs, Symbol>::value,
                                  int>::type = 0>
constexpr auto
go_to(Symbol const &symbol,
      set<bullet_rule<Lhs, set<Seen...>, FirstRhs, Rhs...>, Rules...>) noexcept
    -> decltype(go_to(symbol, set<Rules...>()));

template <typename Symbol, typename Lhs, typename... Seen, typename FirstRhs,
          typename... Rhs, typename... Rules,
          typename std::enable_if<std::is_same<FirstRhs, Symbol>::value,
                                  int>::type = 0>
constexpr auto
go_to(Symbol const &symbol,
      set<bullet_rule<Lhs, set<Seen...>, FirstRhs, Rhs...>, Rules...>)
    -> decltype(prepend(bullet_rule<Lhs, set<Seen..., FirstRhs>, Rhs...>(),
                        go_to(symbol, set<Rules...>())));

template <typename... Nonterminals, typename... Rules, typename... AllRules>
constexpr auto go_tos(set<>, set<Nonterminals...> nonterminals,
                      set<Rules...> rules, set<AllRules...>) noexcept -> set<>;

template <
    typename Symbol, typename... Symbols, typename... Nonterminals,
    typename... Rules, typename... AllRules,
    typename std::enable_if<
        std::is_same<decltype(go_to(std::declval<Symbol>(), set<Rules...>())),
                     set<>>::value,
        int>::type = 0>
constexpr auto go_tos(set<Symbol, Symbols...>,
                      set<Nonterminals...> nonterminals, set<Rules...> state,
                      set<AllRules...> all_rules) noexcept
    -> decltype(go_tos(set<Symbols...>(), nonterminals, state, all_rules));

template <
    typename Symbol, typename... Symbols, typename... Nonterminals,
    typename... Rules, typename... AllRules,
    typename std::enable_if<
        !std::is_same<decltype(go_to(std::declval<Symbol>(), set<Rules...>())),
                      set<>>::value,
        int>::type = 0>
constexpr auto go_tos(set<Symbol, Symbols...>,
                      set<Nonterminals...> nonterminals, set<Rules...> state,
                      set<AllRules...> all_rules) noexcept
    -> decltype(prepend(
        closures(go_to(std::declval<Symbol>(), state), all_rules, nonterminals),
        go_tos(set<Symbols...>(), nonterminals, state, all_rules)));

template <typename... OldStates, typename... Rules, typename... Symbols,
          typename... Nonterminals>
constexpr auto add_states(set<OldStates...>, set<>, set<Rules...>,
                          set<Symbols...>, set<Nonterminals...>) noexcept
    -> set<OldStates...>;

template <typename... OldStates, typename NewState, typename... NewStates,
          typename... Rules, typename... Symbols, typename... Nonterminals,
          typename std::enable_if<contains_t<NewState, OldStates...>::value,
                                  int>::type = 0>
constexpr auto add_states(set<OldStates...> old_states,
                          set<NewState, NewStates...> new_states,
                          set<Rules...> rules, set<Symbols...> symbols,
                          set<Nonterminals...> nonterminals) noexcept
    -> decltype(add_states(old_states, set<NewStates...>(), rules, symbols,
                           nonterminals));

template <typename... OldStates, typename NewState, typename... NewStates,
          typename... Rules, typename... Symbols, typename... Nonterminals,
          typename std::enable_if<!contains_t<NewState, OldStates...>::value,
                                  int>::type = 0>
constexpr auto add_states(set<OldStates...> old_states,
                          set<NewState, NewStates...> new_states,
                          set<Rules...> rules, set<Symbols...> symbols,
                          set<Nonterminals...> nonterminals) noexcept
    -> decltype(add_states(add_state(std::declval<NewState>(), old_states,
                                     rules, symbols, nonterminals),
                           set<NewStates...>(), rules, symbols, nonterminals));

template <typename... Rules, typename... States, typename... AllRules,
          typename... Symbols, typename... Nonterminals>
constexpr auto add_state(set<Rules...> state, set<States...> states,
                         set<AllRules...> rules, set<Symbols...> symbols,
                         set<Nonterminals...> nonterminals) noexcept
    -> decltype(add_states(add(states, state),
                           go_tos(symbols, nonterminals, state, rules), rules,
                           symbols, nonterminals));

template <typename Start, typename... Rules, typename... Nonterminals,
          typename... Terminals>
constexpr auto make_states(Start const &start_symbol, set<Rules...> rules,
                           set<Nonterminals...> nonterminals,
                           set<Terminals...> terminals) noexcept
    -> decltype(add_state(closure(start_symbol, rules, nonterminals), set<>(),
                          rules, join(nonterminals, terminals), nonterminals));

enum class action_type {
  Unreachable,
  Goto,
  Shift,
  Reduce,
  Accept,
};

struct action {
  action_type type = action_type::Unreachable;
  size_t idx = 0;
  size_t pop_nr = 0;
  size_t produce_fn = 0;
  friend std::ostream &operator<<(std::ostream &stream, action const &a) {
    switch (a.type) {
    case action_type::Unreachable:
      stream << "-       ";
      break;
    case action_type::Goto:
      stream << "goto   " << a.idx;
      break;
    case action_type::Shift:
      stream << "shift  " << a.idx;
      break;
    case action_type::Reduce:
      stream << "reduce " << a.idx;
      break;
    case action_type::Accept:
      stream << "accept  ";
      break;
    }
    return stream;
  }
};

template <size_t Idx, typename FindSymbol, typename Symbol, typename... Symbols,
          typename std::enable_if<
              contains_t<FindSymbol, Symbol, Symbols...>::value, int>::type = 0>
constexpr size_t idx_of() noexcept {
  if constexpr (std::is_same<FindSymbol, Symbol>::value) {
    return Idx;
  } else {
    return idx_of<Idx + 1, FindSymbol, Symbols...>();
  }
}

template <typename FindSymbol, typename Symbol, typename... Symbols,
          typename std::enable_if<
              contains_t<FindSymbol, Symbol, Symbols...>::value, int>::type = 0>
constexpr size_t idx_of(FindSymbol const &,
                        set<Symbol, Symbols...> const &) noexcept {
  if constexpr (std::is_same<FindSymbol, Symbol>::value) {
    return 0;
  } else {
    return idx_of<1, FindSymbol, Symbols...>();
  }
}

template <typename FindSymbol, typename Symbol, typename... Symbols,
          typename std::enable_if<
              contains_t<FindSymbol, Symbol, Symbols...>::value, int>::type = 0>
constexpr size_t idx_of(set<Symbol, Symbols...> const &) noexcept {
  if constexpr (std::is_same<FindSymbol, Symbol>::value) {
    return 0;
  } else {
    return idx_of<1, FindSymbol, Symbols...>();
  }
}

template <typename... Symbols> struct transition_table_row {
  std::array<action, sizeof...(Symbols)> actions;

  friend std::ostream &operator<<(std::ostream &stream,
                                  transition_table_row const &row) {
    for (auto &a : row.actions) {
      stream << a << ' ';
    }
    return stream;
  }
};

template <typename Symbol, typename... Symbols>
[[nodiscard]] constexpr action &
get(transition_table_row<Symbols...> &row) noexcept {
  return row.actions[idx_of<0, Symbol, Symbols...>()];
}

template <typename Symbol, typename... Symbols>
[[nodiscard]] constexpr action &get(transition_table_row<Symbols...> &row,
                                    Symbol, set<Symbols...>) noexcept {
  return row.actions[idx_of<0, Symbol, Symbols...>()];
}

template <typename... Terminals, typename... Nonterminals>
constexpr auto make_row(set<Terminals...>, set<Nonterminals...>) noexcept
    -> transition_table_row<Terminals..., Nonterminals...>;

std::ostream &operator<<(std::ostream &stream, std::vector<size_t> const &vec) {
  for (auto i : vec) {
    stream << i << ", ";
  }
  return stream;
}

template <typename Lhs, typename... Rhs> struct rule {};

constexpr auto to_bullet_rules(set<>) noexcept -> set<>;

template <typename Lhs, typename... Rhs, typename... Rules>
constexpr auto to_bullet_rules(set<rule<Lhs, Rhs...>, Rules...>) noexcept
    -> decltype(prepend(bullet_rule<Lhs, set<>, Rhs...>(),
                        to_bullet_rules(set<Rules...>())));

template <typename... Ts>
constexpr auto to_tuple(set<Ts...>) noexcept -> std::tuple<Ts...>;

template <typename... Ts>
constexpr auto to_variant(set<Ts...>) noexcept -> std::variant<Ts...>;

template <typename Prefix, typename... Prefixes, typename Rule,
          typename... Rules>
constexpr bool is_prefix(set<Prefix, Prefixes...>,
                         set<Rule, Rules...>) noexcept {
  if constexpr (std::is_same<Prefix, Rule>::value) {
    if constexpr (sizeof...(Prefixes) == 0) {
      return true;
    } else if constexpr (sizeof...(Rules) > 0) {
      return is_prefix(set<Prefixes...>(), set<Rules...>());
    } else {
      return false;
    }
  } else {
    return false;
  }
}

template <size_t Idx, typename FindState, typename State, typename... States>
constexpr size_t idx_from_prefix() noexcept {
  if constexpr (is_prefix(FindState(), State())) {
    return Idx;
  } else {
    return idx_from_prefix<Idx + 1, FindState, States...>();
  }
}

template <typename... State, typename... Nonterminals, typename... States,
          typename Symbol>
constexpr action init_nonterminal(set<State...> state, set<Nonterminals...>,
                                  set<States...>, set<Symbol>) noexcept {
  if constexpr (!std::is_same<decltype(go_to(std::declval<Symbol>(), state)),
                              set<>>::value) {
    return action{
        action_type::Goto,
        idx_from_prefix<0, decltype(go_to(std::declval<Symbol>(), state)),
                        States...>()};

  } else {
    return {action_type::Unreachable};
  }
}

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules>
constexpr bool contains_reduce(
    set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>) noexcept {
  if constexpr (sizeof...(Rhs) == 0) {
    return true;
  } else {
    return contains_reduce(set<Rules...>());
  }
}

constexpr bool contains_reduce(set<>) noexcept { return false; }

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules>
constexpr size_t
num_reduces(set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>) noexcept {
  if constexpr (sizeof...(Rhs) == 0) {
    return 1 + num_reduces(set<Rules...>());
  } else {
    return num_reduces(set<Rules...>());
  }
}

constexpr size_t num_reduces(set<>) noexcept { return 0; }

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules,
          typename... AllRules,
          typename std::enable_if<sizeof...(Rhs) == 0, int>::type = 0>
constexpr size_t
reduce_rule(set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>,
            set<AllRules...>) noexcept {
  return idx_of<0, bullet_rule<Lhs, set<>, Seen...>, AllRules...>();
}

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules,
          typename... AllRules,
          typename std::enable_if<(sizeof...(Rhs) > 0), int>::type = 0>
constexpr size_t
reduce_rule(set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>,
            set<AllRules...> rules) noexcept {
  return reduce_rule(set<Rules...>(), rules);
}

template <size_t AcceptIdx, typename... State, typename... AllSymbols,
          typename... States, typename... Rules>
constexpr action init_reduce(set<State...> state, set<AllSymbols...>,
                             set<Rules...> rules) noexcept {
  constexpr size_t idx = reduce_rule(state, rules);
  if constexpr (idx == AcceptIdx) {
    return {action_type::Accept, 0, 0, idx};
  } else {
    decltype(get_element<idx>(rules)) produce_rule;
    constexpr size_t produce_idx =
        idx_of<0, decltype(lhs(produce_rule)), AllSymbols...>();
    return action{action_type::Reduce, produce_idx, sizeof_rhs(produce_rule),
                  idx};
  }
}

template <typename... State, typename... States, typename Symbol>
constexpr action init_shift(set<State...> state, set<States...>,
                            set<Symbol>) noexcept {
  if constexpr (!std::is_same<decltype(go_to(std::declval<Symbol>(), state)),
                              set<>>::value) {
    return action{
        action_type::Shift,
        idx_from_prefix<0, decltype(go_to(std::declval<Symbol>(), state)),
                        States...>()};
  } else {
    return {action_type::Unreachable};
  }
}

template <typename... State, typename... Terminals>
constexpr bool contains_shift(set<State...> state, set<Terminals...>) noexcept {
  return ((!std::is_same<decltype(go_to(std::declval<Terminals>(), state)),
                         set<>>::value) ||
          ...);
}

template <
    size_t AcceptIdx, typename... State, typename... AllSymbols,
    typename... States, typename... Rules, typename Symbol,
    typename std::enable_if<contains_reduce(set<State...>()), int>::type = 0>
constexpr action init_terminal(set<State...> state, set<AllSymbols...> symbols,
                               set<States...>, set<Rules...> rules,
                               set<Symbol>) noexcept {
  return init_reduce<AcceptIdx>(state, symbols, rules);
}

template <
    size_t AcceptIdx, typename... State, typename... AllSymbols,
    typename... States, typename... Rules, typename Symbol,
    typename std::enable_if<!contains_reduce(set<State...>()), int>::type = 0>
constexpr action init_terminal(set<State...> state, set<AllSymbols...>,
                               set<States...> states, set<Rules...>,
                               set<Symbol> symbol) noexcept {
  return init_shift(state, states, symbol);
}

template <size_t AcceptIdx, typename... State, typename... AllSymbols,
          typename... Nonterminals, typename... States, typename... Rules,
          typename Symbol,
          typename std::enable_if<contains_t<Symbol, Nonterminals...>::value,
                                  int>::type = 0>
constexpr action init_action(set<State...> state, set<AllSymbols...>,
                             set<Nonterminals...> nonterminals,
                             set<States...> states, set<Rules...>,
                             set<Symbol> symbol) noexcept {
  return init_nonterminal(state, nonterminals, states, symbol);
}

template <size_t AcceptIdx, typename... State, typename... AllSymbols,
          typename... Nonterminals, typename... States, typename... Rules,
          typename Symbol,
          typename std::enable_if<!contains_t<Symbol, Nonterminals...>::value,
                                  int>::type = 0>
constexpr action init_action(set<State...> state, set<AllSymbols...> symbols,
                             set<Nonterminals...>, set<States...> states,
                             set<Rules...> rules, set<Symbol> symbol) noexcept {
  return init_terminal<AcceptIdx>(state, symbols, states, rules, symbol);
}

template <size_t AcceptIdx, typename... State, typename... AllSymbols,
          typename... Nonterminals, typename... States, typename... Rules>
constexpr transition_table_row<AllSymbols...>
init_row(set<State...> state, set<AllSymbols...> symbols,
         set<Nonterminals...> nonterminals, set<States...> states,
         set<Rules...> rules) noexcept {
  return {init_action<AcceptIdx>(state, symbols, nonterminals, states, rules,
                                 set<AllSymbols>())...};
}

template <typename Start, typename... Rules, typename... Nonterminals,
          typename... Terminals, typename... AllStates>
constexpr auto init_rows(set<Start>, set<Rules...> rules, set<Nonterminals...>,
                         set<Terminals...>, set<AllStates...>) noexcept
    -> std::array<transition_table_row<Terminals..., Nonterminals...>,
                  set<AllStates...>::num_elements> {
  return {init_row<idx_of<decltype(
      get_element<0>(with_lhs(std::declval<Start>(), rules)))>(rules)>(
      AllStates(), set<Terminals..., Nonterminals...>(), set<Nonterminals...>(),
      set<AllStates...>(), rules)...};
}

template <typename... States>
constexpr bool reduce_reduce_conflict(set<States...>) noexcept {
  return ((num_reduces(States()) > 1) || ...);
}

template <typename... States, typename... Terminals>
constexpr bool shift_reduce_conflict(set<States...>,
                                     set<Terminals...> terminals) noexcept {
  return ((contains_reduce(States()) && contains_shift(States(), terminals)) ||
          ...);
}

template <typename... States, typename... Terminals>
constexpr bool has_conflict(set<States...> states, set<Terminals...> terminals) noexcept {
  return reduce_reduce_conflict(states) || shift_reduce_conflict(states, terminals);
}

struct symbol {
  virtual ~symbol() {}
};

template <typename... Symbols> constexpr bool all_symbols() {
  return (std::is_base_of<symbol, Symbols>::value && ...);
}

template <typename... Symbols> constexpr bool all_symbols(set<Symbols...>) {
  return (std::is_base_of<symbol, Symbols>::value && ...);
}

template <typename Lhs, typename... Rhs>
constexpr void eval_nonterminal(std::vector<symbol *> &symbols) {
  auto args_iter = symbols.end() - sizeof...(Rhs);
  Lhs *nonterminal =
      new Lhs{(std::move(*dynamic_cast<Rhs *>(*args_iter++)))...};
  symbols.erase(symbols.end() - sizeof...(Rhs), symbols.end());
  symbols.push_back(nonterminal);
}

template <typename Symbols, typename Lhs, typename... Rhs>
constexpr void eval_nonterminal(std::vector<Symbols> &symbols) {
  auto args_iter = symbols.end() - sizeof...(Rhs);
  Lhs nonterminal{(std::move(std::get<Rhs>(*args_iter++)))...};
  symbols.erase(symbols.end() - sizeof...(Rhs), symbols.end());
  symbols.emplace_back(std::move(nonterminal));
}

template <typename... Symbols,
          typename std::enable_if<all_symbols<Symbols...>(), int>::type = 0>
constexpr auto eval_nonterminal_fn() -> void (*)(std::vector<symbol *> &);

template <typename... Symbols,
          typename std::enable_if<!all_symbols<Symbols...>(), int>::type = 0>
constexpr auto eval_nonterminal_fn()
    -> void (*)(std::vector<std::variant<Symbols...>> &);

template <typename... Symbols>
using eval_fn = decltype(eval_nonterminal_fn<Symbols...>());

template <typename Lhs, typename... Rhs, typename... Symbols,
          typename std::enable_if<all_symbols<Symbols...>(), int>::type = 0>
constexpr eval_fn<Symbols...> init_eval_fn(rule<Lhs, Rhs...>,
                                           set<Symbols...>) noexcept {
  return &eval_nonterminal<Lhs, Rhs...>;
}

template <typename Lhs, typename... Rhs, typename... Symbols,
          typename std::enable_if<!all_symbols<Symbols...>(), int>::type = 0>
constexpr eval_fn<Symbols...> init_eval_fn(rule<Lhs, Rhs...>,
                                           set<Symbols...>) noexcept {
  return &eval_nonterminal<std::variant<Symbols...>, Lhs, Rhs...>;
}

template <typename... Symbols>
constexpr auto make_eval_fn(set<Symbols...>) -> eval_fn<Symbols...>;

template <typename... Rules, typename... Symbols>
constexpr std::array<eval_fn<Symbols...>, sizeof...(Rules)>
init_eval_fns(set<Rules...>, set<Symbols...> symbols) noexcept {
  return {init_eval_fn(Rules(), symbols)...};
}

template <typename... Symbols,
          typename std::enable_if<all_symbols<Symbols...>(), int>::type = 0>
constexpr auto value_stack(set<Symbols...>) -> std::vector<symbol *>;

template <typename... Symbols,
          typename std::enable_if<!all_symbols<Symbols...>(), int>::type = 0>
constexpr auto value_stack(set<Symbols...>)
    -> std::vector<std::variant<Symbols...>>;

template <typename Start, typename Rules, typename Nonterminals,
          typename Terminals>
struct transition_table {
  using rules = decltype(to_bullet_rules(Rules()));
  using states = decltype(
      make_states(std::declval<Start>(), rules(), Nonterminals(), Terminals()));
  using symbols = decltype(join(Terminals(), Nonterminals()));

  const std::array<decltype(make_row(Terminals(), Nonterminals())),
                   states::num_elements>
      rows = init_rows(set<Start>(), rules(), Nonterminals(), Terminals(),
                       states());

  const std::array<decltype(make_eval_fn(symbols())), rules::num_elements>
      eval_functions =
          init_eval_fns(Rules(), join(Terminals(), Nonterminals()));

  std::vector<size_t> stack{0};
  decltype(value_stack(symbols())) values{0};

  template <typename Token> bool read_token(Token &&token) {
    size_t action_idx = idx_of(token, Terminals());
    while (true) {
      action const &act = rows[stack.back()].actions[action_idx];
      switch (act.type) {
      case action_type::Shift:
        stack.push_back(act.idx);
        if constexpr (all_symbols(symbols())) {
          values.emplace_back(new Token(std::move(token)));
        } else {
          values.emplace_back(std::move(token));
        }
        return false;
      case action_type::Reduce: {
        (eval_functions[act.produce_fn])(values);
        stack.erase(stack.end() - act.pop_nr, stack.end());
        action_idx = act.idx;
        continue;
      }
      case action_type::Goto: {
        stack.push_back(act.idx);
        action_idx = idx_of(token, Terminals());
        continue;
      }
      case action_type::Accept:
        return true;
      default:
        throw std::runtime_error("Invalid input token");
      }
    }
  }

  Start const &get_parse_result() {
    if (rows[stack.back()].actions[0].type == action_type::Accept) {
      (eval_functions[rows[stack.back()].actions[0].produce_fn])(values);
      if constexpr (all_symbols(symbols())) {
        return *dynamic_cast<Start *>(values.back());
      } else {
        return std::get<Start>(values.back());
      }
    } else {
      throw std::runtime_error{"Parse result not available yet"};
    }
  }

  friend std::ostream &operator<<(std::ostream &stream,
                                  transition_table const &table) {
    for (auto &row : table.rows) {
      stream << row << '\n';
    }
    stream << '\n';
    return stream;
  }
};

} // namespace parser

#endif
