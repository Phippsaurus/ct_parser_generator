#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "parser.hpp"
using namespace parser;

struct id {
  int value = 0;
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

struct times {
  friend std::ostream &operator<<(std::ostream &stream, times const &) {
    stream << '*';
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
  T(lparen, E &&e, rparen);
  T(id &&i) : value(i.value) {}
  friend std::ostream &operator<<(std::ostream &stream, T const &) {
    stream << 'T';
    return stream;
  }
};

struct E {
  int value = 0;
  E(T &&p) : value(p.value) {}
  E(E &&e, plus, T &&p) : value(e.value + p.value) {}
  E(E &&e, times, T &&p) : value(e.value * p.value) {}
  friend std::ostream &operator<<(std::ostream &stream, E const &) {
    stream << 'E';
    return stream;
  }
};

T::T(lparen, E &&e, rparen) : value(e.value) {}

struct S {
  int value = 0;
  S(E &&e, end) : value(e.value) {}
  friend std::ostream &operator<<(std::ostream &stream, S const &) {
    stream << 'S';
    return stream;
  }
};

using rule1 = rule<S, E, end>;
using rule2 = rule<E, E, plus, T>;
using rule4 = rule<T, id>;
using rule5 = rule<E, E, times, T>;
using rule6 = rule<T, lparen, E, rparen>;
using rule7 = rule<E, T>;

using rules = set<rule1, rule2, rule4, rule5, rule6, rule7>;
using terminals = set<id, lparen, rparen, plus, times, end>;
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
        case '*':
          table.read_token(times());
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
  if (auto result = scan.parse("3 * 7"sv); result) {
    std::cout << "Parsing successful: " << *result << '\n';
  } else {
    std::cout << "Parsing failed\n";
  }

  return 0;
}
