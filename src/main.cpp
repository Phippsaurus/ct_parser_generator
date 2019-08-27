#include <iostream>
#include <string>
#include <vector>

#include "parser.hpp"
using namespace parser;

struct S {
  friend std::ostream &operator<<(std::ostream &stream, S const &) {
    stream << 'S';
    return stream;
  }
};

struct E {
  friend std::ostream &operator<<(std::ostream &stream, E const &) {
    stream << 'E';
    return stream;
  }
};

struct end {
  friend std::ostream &operator<<(std::ostream &stream, end const &) {
    stream << '$';
    return stream;
  }
};

struct plus {
  friend std::ostream &operator<<(std::ostream &stream, plus const &) {
    stream << '+';
    return stream;
  }
};

struct T {
  friend std::ostream &operator<<(std::ostream &stream, T const &) {
    stream << 'T';
    return stream;
  }
};

struct id {
  int i = 0;
  id(int i) : i(i) {}
  friend std::ostream &operator<<(std::ostream &stream, id const &i) {
    stream << i.i;
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

using rule1 = rule<S, E, end>;
using rule2 = rule<E, E, plus, T>;
using rule3 = rule<E, T>;
using rule4 = rule<T, id>;
using rule5 = rule<T, lparen, E, rparen>;

using rules = set<rule1, rule2, rule3, rule4, rule5>;
using terminals = set<id, lparen, rparen, plus, end>;
using nonterminals = set<S, E, T>;

class scanner {
  transition_table<S, rules, nonterminals, terminals> table =
      make_table(S(), rules(), nonterminals(), terminals());

public:
  bool parse(std::string_view input) {
    while (!input.empty()) {
      try {
        switch (input[0]) {
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
          return false;
        }
      } catch (...) {
        return false;
      }
      input = input.substr(1);
    }
    try {
      table.read_token(end());
    } catch (...) {
      return false;
    }
    return true;
  }
};

int main() {
  auto table = make_table(S(), rules(), nonterminals(), terminals());
  std::cout << '\n' << table << '\n';

  scanner scan;
  using namespace std::literals::string_view_literals;
  bool success = scan.parse("3+(1)+4+5+(5+2)+1"sv);

  std::cout << "Success: " << (success? "true": "false") << '\n';
  return 0;
}
