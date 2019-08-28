#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "parser.hpp"
using namespace parser;

struct json_value {
  virtual ~json_value() = 0;
};

json_value::~json_value() {}

struct json_null : json_value {
  ~json_null() override {}
};

struct json_string : json_value {
  std::string value;
  json_string(std::string &&value) : value(std::move(value)) {}
  ~json_string() {}
};

struct json_number : json_value {
  double value;
  json_number(double value) : value(value) {}
  ~json_number() {}
};

struct true_ {};
struct false_ {};
struct json_bool : json_value {
  bool value;
  json_bool(true_) : value(true) {}
  json_bool(false_) : value(false) {}
  ~json_bool() override {}
};

struct colon {};
struct comma {};

struct end {};

struct V {
  V(json_null) {}
  V(json_bool &&) {}
  V(json_number &&) {}
  V(json_string &&) {}
};

struct S {
  S(V &&, end) {}
};

using rule1 = rule<S, V, end>;
using rule2 = rule<V, json_null>;
using rule3 = rule<V, json_bool>;
using rule4 = rule<V, json_number>;
using rule5 = rule<V, json_string>;
using rule6 = rule<json_bool, true_>;
using rule7 = rule<json_bool, false_>;
using rule8 = rule<json_string, std::string>;
using rules = set<rule1, rule2, rule3, rule4, rule5, rule6, rule7, rule8>;
using terminals = set<json_null, true_, false_, json_number, std::string, end>;
using nonterminals = set<S, V, json_bool, json_string>;

int main() {
  transition_table<S, rules, nonterminals, terminals> table;
  std::cout << '\n' << table << '\n';

  return 0;
}
