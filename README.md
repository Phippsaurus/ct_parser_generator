# Compile Time LR(0)-Parser

## Description

A project to teach myself how parsers work in detail, and how to use the full
power of _C++_ constant evaluation. The transition table is generated through
template magic at compile time, so there is no extra generator necessary.

### Trivial symbol types

The [expression parser](./src/expression_parser_main.cpp) contains an example
implementation that uses the parser for the classic example of a language of
parenthesized addition. All symbols are trivially constructible and can be
collected in a variant which provides the best performance.

### Complex symbol types

The [json parser](./src/json_parser_main.cpp) represents the different JSON
value types through a common base class, which is necessary to represent
recursive types.  The use of `std::unique_ptr` prohibits collecting the symbols
into one `std::variant`, so indirection through a common base class is
required. This is less performant, but more flexible.  If all symbol types
inherit from `symbol`, this parser variant is constructed.

# Building

To build the project in debug configuration.

```sh
git clone https://github.com/Phippsaurus/ct_parser_generator.git
cd ct_parser_generator
mkdir -p build/debug
(cd build/debug &&
  cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=yes ../..)
make -C build/debug
```

Consecutive builds only require

```sh
make -C build/debug
```

## TODOs

- [ ] Implement full IELR to parse more complex grammars
- [X] Add `JSON` parser example
- [X] Switch between `std::variant` and `symbol *` base class pointer,
  depending on which is applicable / more performant
