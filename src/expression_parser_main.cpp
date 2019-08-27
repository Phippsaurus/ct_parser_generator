#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "parser.hpp"
using namespace parser;

struct id {
  int value = 0;
  id() = default;
  id(int value) : value(value) {}
  friend std::ostream &operator<<(std::ostream &stream, id const &i) {
    stream << i.value;
    return stream;
  }
};

struct lparen {
  friend std::ostream &operator<<(std::ostream &stream, lparen const &) {
    stream << '(';
    return stream;
  }
};

struct rparen {
  friend std::ostream &operator<<(std::ostream &stream, rparen const &) {
    stream << ')';
    return stream;
  }
};

struct plus {
  friend std::ostream &operator<<(std::ostream &stream, plus const &) {
    stream << '+';
    return stream;
  }
};

struct end {
  friend std::ostream &operator<<(std::ostream &stream, end const &) {
    stream << '$';
    return stream;
  }
};

struct E;

struct T {
  int value = 0;
  T() = default;
  T(lparen const &, E const & e, rparen const &);
  T(id const & i) : value(i.value) {}
  friend std::ostream &operator<<(std::ostream &stream, T const &) {
    stream << 'T';
    return stream;
  }
};

struct E {
  int value = 0;
  E() = default;
  E(T const & t) : value(t.value) {}
  E(E const & e, plus const &, T const & t) : value(e.value + t.value) {}
  friend std::ostream &operator<<(std::ostream &stream, E const &) {
    stream << 'E';
    return stream;
  }
};

T::T(lparen const &, E const & e, rparen const &) : value(e.value) {}

struct S {
  int value = 0;
  S() = default;
  S(E const & e, end const &) : value(e.value) {}
  friend std::ostream &operator<<(std::ostream &stream, S const &) {
    stream << 'S';
    return stream;
  }
};

using rule1 = rule<S, E, end>;
using rule2 = rule<E, E, plus, T>;
using rule3 = rule<E, T>;
using rule4 = rule<T, id>;
using rule5 = rule<T, lparen, E, rparen>;

using rules = set<rule1, rule2, rule3, rule4, rule5>;
using terminals = set<id, lparen, rparen, plus, end>;
using nonterminals = set<S, E, T>;

class scanner {
  transition_table<S, rules, nonterminals, terminals> table;

public:
  std::optional<int> parse(std::string_view input) {
    while (!input.empty()) {
      try {
        switch (input[0]) {
        case ' ':
        case '\t':
        case '\f':
        case '\v':
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          table.read_token(id(input[0] - '0'));
          break;
        case '+':
          table.read_token(plus());
          break;
        case '(':
          table.read_token(lparen());
          break;
        case ')':
          table.read_token(rparen());
          break;
        default:
          return {};
        }
      } catch (std::exception const &e) {
        std::cout << e.what() << '\n';
        return {};
      }
      input = input.substr(1);
    }
    try {
      table.read_token(end());
    } catch (std::exception const &e) {
      std::cout << e.what() << '\n';
      return {};
    }
    return {table.get_parse_result().value};
  }
};

int main() {
  transition_table<S, rules, nonterminals, terminals> table;
  std::cout << '\n' << table << '\n';

  scanner scan;
  using namespace std::literals::string_view_literals;
  if (auto result = scan.parse("1 + (3 + 2) + (9) + 4"sv); result) {
    std::cout << "Parsing successful: " << *result << '\n';
  } else {
    std::cout << "Parsing failed\n";
  }

  return 0;
}
