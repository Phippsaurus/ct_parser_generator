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

struct json_null : json_value, symbol {
  ~json_null() override {}
};

struct string : symbol {
  std::string value;
  string(std::string_view value) : value(value) {}
  ~string() override {}
};

struct json_string : json_value, symbol {
  std::string value;
  json_string(string &&value) : value(std::move(value.value)) {}
  json_string(json_string &&other) = default;
  ~json_string() override {}
};

struct json_number : json_value, symbol {
  double value;
  json_number(double value) : value(value) {}
  ~json_number() override {}
};

struct true_ : symbol {};
struct false_ : symbol {};
struct json_bool : json_value, symbol {
  bool value;
  json_bool(true_) : value(true) {}
  json_bool(false_) : value(false) {}
  json_bool(bool value) : value(value) {}
  ~json_bool() override {}
};

struct lbrace : symbol {};
struct rbrace : symbol {};

struct lbracket : symbol {};
struct rbracket : symbol {};

struct colon : symbol {};
struct comma : symbol {};

struct end : symbol {};

struct V;
struct json_member : symbol {
  std::string name;
  std::unique_ptr<json_value> value;
  json_member(string &&name, colon, V &&value);
  json_member(json_member &&) = default;
  ~json_member() override {}
};

struct M : symbol {
  std::vector<json_member> members;
  M(json_member &&member) { members.emplace_back(std::move(member)); }
  M(M &&m, comma, json_member &&member) : members(std::move(m.members)) {
    members.emplace_back(std::move(member));
  }
  ~M() override {}
};

struct json_object : json_value, symbol {
  std::vector<json_member> members;
  json_object(lbrace, M &&m, rbrace) : members(std::move(m.members)) {}
  json_object(lbrace, rbrace) {}
  json_object(json_object &&) = default;
  ~json_object() override {}
};

struct L : symbol {
  std::vector<std::unique_ptr<json_value>> values;
  L(L &&l, comma, V &&v);
  L(V &&v);
  ~L() override {}
};

struct json_list : json_value, symbol {
  std::vector<std::unique_ptr<json_value>> values;
  json_list(lbracket, L &&l, rbracket) : values(std::move(l.values)) {}
  json_list(lbracket, rbracket) {}
  json_list(json_list &&) = default;
  ~json_list() override {}
};

struct V : symbol {
  std::unique_ptr<json_value> value;
  V(json_null &&) : value(new json_null) {}
  V(json_bool &&value) : value(new json_bool(value.value)) {}
  V(json_number &&value) : value(new json_number(value.value)) {}
  V(json_string &&value) : value(new json_string(std::move(value))) {}
  V(json_object &&value) : value(new json_object(std::move(value))) {}
  V(json_list &&value) : value(new json_list(std::move(value))) {}
  V(V &&other) = default;
  ~V() override {}
};

json_member::json_member(string &&name, colon, V &&value)
    : name(std::move(name.value)), value(std::move(value.value)) {}

L::L(L &&l, comma, V &&v) : values(std::move(l.values)) {
  values.emplace_back(std::move(v.value));
}

L::L(V &&v) { values.emplace_back(std::move(v.value)); }

struct S : symbol {
  S(json_object &&, end) {}
};

using rule1 = rule<S, json_object, end>;
using rule2 = rule<V, json_null>;
using rule3 = rule<V, json_bool>;
using rule4 = rule<V, json_number>;
using rule5 = rule<V, json_string>;
using rule6 = rule<json_bool, true_>;
using rule7 = rule<json_bool, false_>;
using rule8 = rule<json_string, string>;
using rule9 = rule<json_member, string, colon, V>;
using rule10 = rule<M, M, comma, json_member>;
using rule11 = rule<M, json_member>;
using rule12 = rule<json_object, lbrace, M, rbrace>;
using rule13 = rule<V, json_object>;
using rule14 = rule<V, json_list>;
using rule15 = rule<L, L, comma, V>;
using rule16 = rule<L, V>;
using rule17 = rule<json_list, lbracket, L, rbracket>;
using rule18 = rule<json_list, lbracket, rbracket>;
using rule19 = rule<json_object, lbrace, rbrace>;
using rules =
    set<rule1, rule2, rule3, rule4, rule5, rule6, rule7, rule8, rule9, rule10,
        rule11, rule12, rule13, rule14, rule15, rule16, rule17>;
using terminals = set<json_null, true_, false_, json_number, string, end, colon,
                      comma, lbrace, rbrace, lbracket, rbracket>;
using nonterminals = set<S, V, json_bool, json_string, json_member, M,
                         json_object, L, json_list>;

class scanner {
  transition_table<S, rules, nonterminals, terminals> table;

public:
  std::optional<int> parse(std::string_view input) {
    while (!input.empty()) {
      try {
        if (input.find("null") == 0) {
          table.read_token(json_null());
          input = input.substr(4);
        } else if (input[0] == '"') {
          if (auto end_idx = input.find('"', 1);
              end_idx != std::string_view::npos) {
            table.read_token(string(input.substr(1, end_idx - 1)));
            input = input.substr(end_idx + 1);
          } else {
            throw std::runtime_error("Unterminated string literal");
          }
        } else if (input.find("true") == 0) {
          table.read_token(true_());
          input = input.substr(4);
        } else if (input.find("false") == 0) {
          table.read_token(false_());
          input = input.substr(5);
        } else {
          switch (input[0]) {
          case ' ':
          case '\t':
          case '\n':
          case '\r':
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
            table.read_token(json_number(input[0] - '0'));
            break;
          case ':':
            table.read_token(colon());
            break;
          case ',':
            table.read_token(comma());
            break;
          case '{':
            table.read_token(lbrace());
            break;
          case '}':
            table.read_token(rbrace());
            break;
          case '[':
            table.read_token(lbracket());
            break;
          case ']':
            table.read_token(rbracket());
            break;
          default:
            return {};
          }
          input = input.substr(1);
        }
      } catch (std::exception const &e) {
        std::cout << e.what() << '\n';
        return {};
      }
    }
    try {
      table.read_token(end());
    } catch (std::exception const &e) {
      std::cout << e.what() << '\n';
      return {};
    }
    return {1};
  }
};

int main() {
  transition_table<S, rules, nonterminals, terminals> table;
  std::cout << '\n' << table << '\n';

  scanner s;
  using namespace std::literals::string_view_literals;
  auto input =
      R"({
 "true": true,
 "null": null,
 "number": 8,
 "object": {
   "field": false,
   "int": 2
 },
 "list": [{"string": "text"}, 4, 1, null]
})";
  std::cout << (s.parse(input).has_value() ? "Success\n" : "Fail\n");
  return 0;
}
