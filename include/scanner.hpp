#if !defined(SCANNER_HPP)
#define SCANNER_HPP

#include "parser.hpp"
#include <optional>

namespace parser {

template <typename Token> struct parse_result {
  Token token;
  std::string_view remaining;
};

template <std::string_view Prefix, typename Token> class scan_token {

public:
  static std::optional<parse_result<Token>> scan(std::string_view input) {
    if (input.find(Prefix) == 0) {
      return Token(input.substr(Prefix.length()));
    }
  }
};

template <typename Start, typename Rules, typename Nonterminals,
          typename Terminals, typename... Tokens>
class parser {
  transition_table<Start, Rules, Nonterminals, Terminals> table;
  std::string_view input;

  template <typename Token, typename... Others>
  bool tokenize_next() {
    if (auto result = Token::scan(input); result) {
      input = result->remaining;
      table.read_token(std::move(result->token));
      return true;
    } else {
      if constexpr (sizeof...(Others) > 0) {
        return tokenize_next<Others...>(input);
      }
    }
  }

public:
  void tokenize() {
    while (!input.empty() && tokenize_next()) {}
  }
};

} // namespace parser

#endif
