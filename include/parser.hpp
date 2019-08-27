#if !defined(PARSER_HPP)
#define PARSER_HPP

#include <array>
#include <iostream>
#include <type_traits>
#include <utility>
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

// template <typename T, typename... Ts> void print_pack(std::ostream &stream) {
//   stream << T();
//   if constexpr (sizeof...(Ts) > 0) {
//     stream << ' ';
//     print_pack<Ts...>(stream);
//   }
// }

// void print_pack(std::ostream &, set<>) {}

// template <typename... Ts,
//           typename std::enable_if<sizeof...(Ts) == 0, int>::type = 0>
// void print_pack(std::ostream &) {}

// template <typename T, typename... Ts>
// void print_pack(std::ostream &stream, set<T, Ts...>) {
//   stream << T();
//   if constexpr (sizeof...(Ts) > 0) {
//     stream << ' ';
//     print_pack<Ts...>(stream);
//   }
// }

template <typename Lhs, typename Seen, typename... Rhs> struct bullet_rule {
  // friend std::ostream &operator<<(std::ostream &stream, bullet_rule const &)
  // {
  //   stream << Lhs() << " => ";
  //   print_pack(stream, Seen());
  //   stream << " • ";
  //   print_pack<Rhs...>(stream);
  //   stream << '\n';
  //   return stream;
  // }
};

template <typename Lhs, typename... Seen, typename... Rhs>
constexpr auto lhs(bullet_rule<Lhs, set<Seen...>, Rhs...>) -> Lhs;

template <typename Lhs, typename... Seen, typename... Rhs>
constexpr size_t sizeof_rhs(bullet_rule<Lhs, set<Seen...>, Rhs...>) {
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
constexpr auto add(set<Ts...>, Element) -> set<Ts...>;

template <
    typename Element, typename... Ts,
    typename std::enable_if<!contains_t<Element, Ts...>::value, int>::type = 0>
constexpr auto add(set<Ts...>, Element) -> set<Ts..., Element>;

template <
    typename Element, typename... Ts,
    typename std::enable_if<contains_t<Element, Ts...>::value, int>::type = 0>
constexpr auto prepend(Element, set<Ts...>) -> set<Ts...>;

template <
    typename Element, typename... Ts,
    typename std::enable_if<!contains_t<Element, Ts...>::value, int>::type = 0>
constexpr auto prepend(Element, set<Ts...>) -> set<Element, Ts...>;

template <typename Element>
constexpr auto remove(set<>, Element element) -> set<>;

template <
    typename Element, typename T, typename... Ts,
    typename std::enable_if<!std::is_same<Element, T>::value, int>::type = 0>
constexpr auto remove(set<T, Ts...>, Element element)
    -> decltype(prepend(std::declval<T>(), remove(set<Ts...>(), element)));

template <
    typename Element, typename T, typename... Ts,
    typename std::enable_if<std::is_same<Element, T>::value, int>::type = 0>
constexpr auto remove(set<T, Ts...>, Element) -> set<Ts...>;

template <typename Element, typename... Ts> struct add_t {
  using type = typename if_t<contains_t<Element, Ts...>::value, set<Ts...>,
                             set<Ts..., Element>>::type;
};

template <typename... Ts> constexpr auto join(set<Ts...>, set<>) -> set<Ts...>;

template <typename... Ts, typename U, typename... Us>
constexpr auto join(set<Ts...>, set<U, Us...>)
    -> decltype(join(std::declval<typename add_t<U, Ts...>::type>(),
                     set<Us...>()));

template <typename Lhs, typename... Seen, typename First, typename... Rhs>
constexpr auto first(bullet_rule<Lhs, set<Seen...>, First, Rhs...>) -> First;

template <typename Nonterminal>
constexpr auto with_lhs(Nonterminal nonterminal, set<>) -> set<>;

template <typename Nonterminal, typename Lhs, typename... Seen, typename... Rhs,
          typename... Rules,
          typename std::enable_if<std::is_same<Nonterminal, Lhs>::value,
                                  int>::type = 0>
constexpr auto with_lhs(Nonterminal nonterminal,
                        set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>)
    -> decltype(prepend(bullet_rule<Lhs, set<Seen...>, Rhs...>(),
                        with_lhs(nonterminal, set<Rules...>())));

template <typename Nonterminal, typename Lhs, typename... Seen, typename... Rhs,
          typename... Rules,
          typename std::enable_if<!std::is_same<Nonterminal, Lhs>::value,
                                  int>::type = 0>
constexpr auto with_lhs(Nonterminal nonterminal,
                        set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>)
    -> decltype(with_lhs(nonterminal, set<Rules...>()));

template <typename Lhs, typename... Seen, typename FirstRhs, typename... Rhs,
          typename... Rules, typename... Nonterminals,
          typename std::enable_if<contains_t<FirstRhs, Nonterminals...>::value,
                                  int>::type = 0>
constexpr auto expand(bullet_rule<Lhs, set<Seen...>, FirstRhs, Rhs...>,
                      set<Rules...> rules, set<Nonterminals...> nonterminals)
    -> decltype(with_lhs(std::declval<FirstRhs>(), rules));

template <typename Lhs, typename... Seen, typename FirstRhs, typename... Rhs,
          typename... Rules, typename... Nonterminals,
          typename std::enable_if<!contains_t<FirstRhs, Nonterminals...>::value,
                                  int>::type = 0>
constexpr auto
expand(bullet_rule<Lhs, set<Seen...>, FirstRhs, Rhs...> bullet_rule,
       set<Rules...> rules, set<Nonterminals...> nonterminals) -> set<>;

template <typename... AllRules, typename... Nonterminals>
constexpr auto closures(set<>, set<AllRules...>, set<Nonterminals...>) -> set<>;

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules,
          typename... AllRules, typename... Nonterminals,
          typename std::enable_if<sizeof...(Rhs) == 0, int>::type = 0>
constexpr auto closures(set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>,
                        set<AllRules...> all_rules,
                        set<Nonterminals...> nonterminals)
    -> decltype(prepend(bullet_rule<Lhs, set<Seen...>>(),
                        closures(set<Rules...>(), all_rules, nonterminals)));

template <typename Lhs, typename... Seen, typename... Rhs, typename... Rules,
          typename... AllRules, typename... Nonterminals,
          typename std::enable_if<(sizeof...(Seen) == 0 && sizeof...(Rhs) > 0),
                                  int>::type = 0>
constexpr auto closures(set<bullet_rule<Lhs, set<Seen...>, Rhs...>, Rules...>,
                        set<AllRules...> all_rules,
                        set<Nonterminals...> nonterminals)
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
                        set<Nonterminals...> nonterminals)
    -> decltype(
        join(prepend(bullet_rule<Lhs, set<Seen...>, Rhs...>(),
                     closures(expand(bullet_rule<Lhs, set<Seen...>, Rhs...>(),
                                     all_rules, nonterminals),
                              all_rules, nonterminals)),
             closures(set<Rules...>(), all_rules, nonterminals)));

template <typename Nonterminal, typename... Rules, typename... Nonterminals,
          typename std::enable_if<
              contains_t<Nonterminal, Nonterminals...>::value, int>::type = 1>
constexpr auto closure(Nonterminal nonterminal, set<Rules...> rules,
                       set<Nonterminals...> nonterminals)
    -> decltype(closures(with_lhs(nonterminal, set<Rules...>()), rules,
                         remove(nonterminals, nonterminal)));

template <typename Nonterminal, typename... Rules, typename... Nonterminals,
          typename std::enable_if<
              !contains_t<Nonterminal, Nonterminals...>::value, int>::type = 1>
constexpr auto closure(Nonterminal nonterminal, set<Rules...>,
                       set<Nonterminals...>) -> set<>;

template <typename Symbol> constexpr auto go_to(Symbol symbol, set<>) -> set<>;

template <typename Symbol, typename Lhs, typename... Seen, typename... Rules>
constexpr auto go_to(Symbol symbol,
                     set<bullet_rule<Lhs, set<Seen...>>, Rules...>)
    -> decltype(go_to(symbol, set<Rules...>()));

template <typename Symbol, typename Lhs, typename... Seen, typename FirstRhs,
          typename... Rhs, typename... Rules,
          typename std::enable_if<!std::is_same<FirstRhs, Symbol>::value,
                                  int>::type = 0>
constexpr auto
go_to(Symbol symbol,
      set<bullet_rule<Lhs, set<Seen...>, FirstRhs, Rhs...>, Rules...>)
    -> decltype(go_to(symbol, set<Rules...>()));

template <typename Symbol, typename Lhs, typename... Seen, typename FirstRhs,
          typename... Rhs, typename... Rules,
          typename std::enable_if<std::is_same<FirstRhs, Symbol>::value,
                                  int>::type = 0>
constexpr auto
go_to(Symbol symbol,
      set<bullet_rule<Lhs, set<Seen...>, FirstRhs, Rhs...>, Rules...>)
    -> decltype(prepend(bullet_rule<Lhs, set<Seen..., FirstRhs>, Rhs...>(),
                        go_to(symbol, set<Rules...>())));

template <typename... Nonterminals, typename... Rules, typename... AllRules>
constexpr auto go_tos(set<>, set<Nonterminals...> all_symbols,
                      set<Rules...> rules, set<AllRules...>) -> set<>;

template <
    typename Symbol, typename... Symbols, typename... Nonterminals,
    typename... Rules, typename... AllRules,
    typename std::enable_if<
        std::is_same<decltype(go_to(std::declval<Symbol>(), set<Rules...>())),
                     set<>>::value,
        int>::type = 0>
constexpr auto go_tos(set<Symbol, Symbols...>,
                      set<Nonterminals...> nonterminals, set<Rules...> state,
                      set<AllRules...> all_rules)
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
                      set<AllRules...> all_rules)
    -> decltype(prepend(
        closures(go_to(std::declval<Symbol>(), state), all_rules, nonterminals),
        go_tos(set<Symbols...>(), nonterminals, state, all_rules)));

template <typename... OldStates, typename... Rules, typename... Symbols,
          typename... Nonterminals>
constexpr auto add_states(set<OldStates...>, set<>, set<Rules...>,
                          set<Symbols...>, set<Nonterminals...>)
    -> set<OldStates...>;

template <typename... OldStates, typename NewState, typename... NewStates,
          typename... Rules, typename... Symbols, typename... Nonterminals,
          typename std::enable_if<contains_t<NewState, OldStates...>::value,
                                  int>::type = 0>
constexpr auto add_states(set<OldStates...> old_states,
                          set<NewState, NewStates...> new_states,
                          set<Rules...> rules, set<Symbols...> symbols,
                          set<Nonterminals...> nonterminals)
    -> decltype(add_states(old_states, set<NewStates...>(), rules, symbols,
                           nonterminals));

template <typename... OldStates, typename NewState, typename... NewStates,
          typename... Rules, typename... Symbols, typename... Nonterminals,
          typename std::enable_if<!contains_t<NewState, OldStates...>::value,
                                  int>::type = 0>
constexpr auto add_states(set<OldStates...> old_states,
                          set<NewState, NewStates...> new_states,
                          set<Rules...> rules, set<Symbols...> symbols,
                          set<Nonterminals...> nonterminals)
    -> decltype(add_states(add_state(std::declval<NewState>(), old_states,
                                     rules, symbols, nonterminals),
                           set<NewStates...>(), rules, symbols, nonterminals));

template <typename... Rules, typename... States, typename... AllRules,
          typename... Symbols, typename... Nonterminals>
constexpr auto add_state(set<Rules...> state, set<States...> states,
                         set<AllRules...> rules, set<Symbols...> symbols,
                         set<Nonterminals...> nonterminals)
    -> decltype(add_states(add(states, state),
                           go_tos(symbols, nonterminals, state, rules), rules,
                           symbols, nonterminals));

template <typename Start, typename... Rules, typename... Nonterminals,
          typename... Terminals>
constexpr auto make_states(Start start_symbol, set<Rules...> rules,
                           set<Nonterminals...> nonterminals,
                           set<Terminals...> terminals)
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
constexpr size_t idx_of(FindSymbol const &, set<Symbol, Symbols...> const &) noexcept {
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
constexpr auto make_row(set<Terminals...>, set<Nonterminals...>)
    -> transition_table_row<Terminals..., Nonterminals...>;

template <typename... Rules, typename... States, typename Symbol,
          typename... Symbols>
constexpr auto make_row(set<Rules...> state, set<States...> states,
                        set<Symbol, Symbols...>)
    -> decltype(prepend(go_to(std::declval<Symbol>(), state),
                        make_row(state, states, set<Symbols...>())));

/*
template <typename Rule, typename... Rules>
void print_state(std::ostream &stream) {
  stream << Rule();
  if constexpr (sizeof...(Rules) > 0) {
    print_state<Rules...>(stream);
  }
}

template <typename Rule, typename... Rules>
void print_state(std::ostream &stream, set<Rule, Rules...>) {
  stream << Rule();
  if constexpr (sizeof...(Rules) > 0) {
    print_state<Rules...>(stream);
  }
        case '5':
}

        case '5':
template <size_t Idx, typename State, typename... States>
void print_states(std::ostream &stream) {
  stream << "State " << Idx << ":\n";
  print_state(stream, State());
  if constexpr (sizeof...(States) > 0) {
    print_states<Idx + 1, States...>(stream);
  }
}

template <size_t Idx, typename State, typename... States>
void print_states(std::ostream &stream, set<State, States...>) {
  stream << "State " << Idx << ":\n";
  print_state(stream, State());
  if constexpr (sizeof...(States) > 0) {
    print_states<Idx + 1, States...>(stream);
  }
}

template <size_t Idx, typename Rule, typename... Rules>
void print_rules(std::ostream &stream) {
  stream << "Rule " << Idx << ": " << Rule() << '\n';
  if constexpr (sizeof...(Rules) > 0) {
    print_rules<Idx + 1, Rules...>(stream);
  }
}

template <size_t Idx, typename Rule, typename... Rules>
void print_rules(std::ostream &stream, set<Rule, Rules...>) {
  stream << "Rule " << Idx << ": " << Rule() << '\n';
  if constexpr (sizeof...(Rules) > 0) {
    print_rules<Idx + 1, Rules...>(stream);
  }
}
*/

std::ostream &operator<<(std::ostream &stream, std::vector<size_t> const &vec) {
  for (auto i : vec) {
    stream << i << ", ";
  }
  return stream;
}

template <typename Lhs, typename... Rhs> struct rule {};

constexpr auto to_bullet_rules(set<>) -> set<>;

template <typename Lhs, typename... Rhs, typename... Rules>
constexpr auto to_bullet_rules(set<rule<Lhs, Rhs...>, Rules...>)
    -> decltype(prepend(bullet_rule<Lhs, set<>, Rhs...>(),
                        to_bullet_rules(set<Rules...>())));

template <typename Start, typename Rules, typename Nonterminals,
          typename Terminals>
struct transition_table {
  using rules = decltype(to_bullet_rules(Rules()));
  using states = decltype(
      make_states(std::declval<Start>(), rules(), Nonterminals(), Terminals()));
  using symbols = decltype(join(Terminals(), Nonterminals()));
  std::array<decltype(make_row(Terminals(), Nonterminals())),
             states::num_elements>
      rows;
  std::vector<size_t> stack{0};

  template <typename Token> bool read_token(Token const &token) {
    std::cout << "Read " << token << '\n';
    size_t action_idx = idx_of(token, Terminals());
    while (true) {
      auto &act = rows[stack.back()].actions[action_idx];
      std::cout << "Action idx: " << action_idx << ", Stack: " << stack << '\n';
      switch (act.type) {
      case action_type::Shift:
        std::cout << "State " << stack.back() << ", Shift " << act.idx << '\n';
        stack.push_back(act.idx);
        return false;
      case action_type::Reduce: {
        std::cout << "State " << stack.back() << ", Reduce " << act.pop_nr
                  << " states\n";
        stack.erase(stack.end() - act.pop_nr, stack.end());
        action_idx = act.idx;
        continue;
      }
      case action_type::Goto: {
        std::cout << "State " << stack.back() << ", Goto " << act.idx << "\n";
        stack.push_back(act.idx);
        action_idx = idx_of(token, Terminals());
        continue;
      }
      case action_type::Accept:
        std::cout << "Accept\n";
        return true;
      default:
        throw std::runtime_error{"Invalid input token"};
      }
    }
  }

  friend std::ostream &operator<<(std::ostream &stream,
                                  transition_table const &table) {
    for (auto &row : table.rows) {
      stream << row << '\n';
    }
    stream << '\n';
    // print_rules<0>(stream, Rules());
    // print_states<0>(stream, states());
    return stream;
  }
};

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

template <typename... AllSymbols, typename... State, typename Nonterminal,
          typename... Nonterminals, typename... States>
constexpr void
init_row_gotos(transition_table_row<AllSymbols...> &row, set<State...> state,
               set<Nonterminal, Nonterminals...>, set<States...>) {
  if constexpr (!std::is_same<decltype(
                                  go_to(std::declval<Nonterminal>(), state)),
                              set<>>::value) {
    get<Nonterminal>(row) = action{
        action_type::Goto,
        idx_from_prefix<0, decltype(go_to(std::declval<Nonterminal>(), state)),
                        States...>()};
  }
  if constexpr (sizeof...(Nonterminals) > 0) {
    init_row_gotos(row, state, set<Nonterminals...>(), set<States...>());
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

template <
    size_t AcceptIdx, typename... AllSymbols, typename... State,
    typename... Terminals, typename... Rules,
    typename std::enable_if<contains_reduce(set<State...>()), int>::type = 0>
constexpr void init_row_reduces(transition_table_row<AllSymbols...> &row,
                                set<State...> state, set<Terminals...>,
                                set<Rules...> rules) {
  constexpr size_t idx = reduce_rule(state, rules);
  if constexpr (idx == AcceptIdx) {
    for (size_t terminal_idx = 0; terminal_idx < sizeof...(Terminals);
         ++terminal_idx) {
      row.actions[terminal_idx] = action{action_type::Accept};
    }
  } else {
    decltype(get_element<idx>(rules)) produce_rule;
    for (size_t terminal_idx = 0; terminal_idx < sizeof...(Terminals);
         ++terminal_idx) {
      constexpr size_t produce_idx =
          idx_of<0, decltype(lhs(produce_rule)), AllSymbols...>();
      row.actions[terminal_idx] =
          action{action_type::Reduce, produce_idx, sizeof_rhs(produce_rule)};
    }
  }
}

template <
    size_t AcceptIdx, typename... AllSymbols, typename... State,
    typename... Terminals, typename... Rules,
    typename std::enable_if<!contains_reduce(set<State...>()), int>::type = 0>
constexpr void init_row_reduces(transition_table_row<AllSymbols...> &,
                                set<State...>, set<Terminals...>,
                                set<Rules...>) {}

template <typename... AllSymbols, typename... State, typename Terminal,
          typename... Terminals, typename... States>
constexpr void init_row_shifts(transition_table_row<AllSymbols...> &row,
                               set<State...> state, set<Terminal, Terminals...>,
                               set<States...>) {
  if constexpr (!std::is_same<decltype(go_to(std::declval<Terminal>(), state)),
                              set<>>::value) {
    get<Terminal>(row) = action{
        action_type::Shift,
        idx_from_prefix<0, decltype(go_to(std::declval<Terminal>(), state)),
                        States...>()};
  }
  if constexpr (sizeof...(Terminals) > 0) {
    init_row_shifts(row, state, set<Terminals...>(), set<States...>());
  }
}

template <typename Start, typename... Rules, typename... Nonterminals,
          typename... Terminals, typename State, typename... States,
          typename... AllStates>
constexpr void
init_rows(transition_table<Start, set<Rules...>, set<Nonterminals...>,
                           set<Terminals...>> &table,
          set<State, States...>, set<AllStates...> states, size_t row_idx) {
  init_row_gotos(table.rows[row_idx], State(), set<Nonterminals...>(), states);
  init_row_shifts(table.rows[row_idx], State(), set<Terminals...>(), states);
  typename transition_table<Start, set<Rules...>, set<Nonterminals...>,
                            set<Terminals...>>::rules rules;
  constexpr auto start_idx =
      idx_of<decltype(get_element<0>(
                    with_lhs(std::declval<Start>(), rules)))>(rules);
  init_row_reduces<start_idx>(table.rows[row_idx], State(), set<Terminals...>(),
                              rules);
  if constexpr (sizeof...(States) > 0) {
    init_rows(table, set<States...>(), states, row_idx + 1);
  }
}

template <typename Start, typename... Rules, typename... Nonterminals,
          typename... Terminals>
constexpr auto make_table(Start, set<Rules...>, set<Nonterminals...>,
                          set<Terminals...>)
    -> transition_table<Start, set<Rules...>, set<Nonterminals...>,
                        set<Terminals...>> {
  transition_table<Start, set<Rules...>, set<Nonterminals...>,
                   set<Terminals...>>
      table;
  typename transition_table<Start, set<Rules...>, set<Nonterminals...>,
                            set<Terminals...>>::states states;
  init_rows(table, states, states, 0);
  return table;
}

} // namespace parser

#endif
