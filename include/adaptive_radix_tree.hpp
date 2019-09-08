#if !defined(ADAPTIVE_RADIX_TREE_H)
#define ADAPTIVE_RADIX_TREE_H

#include <array>
#include <bitset>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

namespace parser {

constexpr uint8_t empty_key = std::numeric_limits<uint8_t>::max();
constexpr size_t empty_slot = std::numeric_limits<size_t>::max();

struct node_256 {
  constexpr static size_t num_slots = 256;
  std::array<size_t, num_slots> follow_idxs;
  constexpr void insert_next(char c, size_t follow_idx) {
    follow_idxs[c] = follow_idx;
  }
};

struct node_48 {
  constexpr static size_t num_keys = 256;
  constexpr static size_t num_slots = 48;
  uint8_t filled = 0;
  std::array<uint8_t, num_keys> keys;
  std::array<size_t, num_slots> follow_idxs;
  constexpr void insert_next(char c, size_t follow_idx) noexcept {
    keys[c] = static_cast<uint8_t>(filled);
    follow_idxs[filled++] = follow_idx;
  }
};

struct node_16 {
  constexpr static size_t num_slots = 16;
  uint8_t filled = 0;
  std::array<char, num_slots> keys;
  std::array<size_t, num_slots> follow_idxs;
  constexpr void insert_next(char c, size_t follow_idx) noexcept {
    keys[filled] = c;
    follow_idxs[filled++] = follow_idx;
  }
};

struct node_4 {
  constexpr static size_t num_slots = 4;
  uint8_t filled = 0;
  std::array<char, num_slots> keys;
  std::array<size_t, num_slots> follow_idxs;
  constexpr void insert_next(char c, size_t follow_idx) noexcept {
    keys[filled] = c;
    follow_idxs[filled++] = follow_idx;
  }
};

struct NodesCount {
  size_t node4s = 0, node16s = 0, node48s = 0, node256s = 0;
  constexpr NodesCount &operator+=(NodesCount const &other) noexcept {
    node4s += other.node4s;
    node16s += other.node16s;
    node48s += other.node48s;
    node256s += other.node256s;
    return *this;
  }
};
constexpr size_t common_prefix_length(std::string_view const &a,
                                      std::string_view const &b,
                                      size_t max_common_prefix) noexcept {
  size_t common_prefix = 0;
  for (auto it_a = a.begin(), it_b = b.begin();
       it_a != a.end() && it_b != b.end() && common_prefix < max_common_prefix;
       ++it_a, ++it_b, ++common_prefix) {
    if (*it_a != *it_b) {
      return common_prefix;
    }
  }
  return a.length();
}

template <typename InputIt>
constexpr size_t common_prefix_length(InputIt first, InputIt last) noexcept {
  if (first == last) {
    return 0;
  }
  std::string_view const &ref_string = *first;
  size_t common_prefix = ref_string.length();
  while (++first != last && common_prefix > 0) {
    common_prefix = common_prefix_length(ref_string, *first, common_prefix);
  }
  return common_prefix;
}

template <typename InputIt>
constexpr NodesCount count_required_nodes(InputIt first,
                                          InputIt last) noexcept {
  size_t prefix_len = common_prefix_length(first, last);
  for (auto str = first; str != last; ++str) {
    *str = str->substr(prefix_len);
  }
  while (first != last && first->empty()) {
    ++first;
  }
  NodesCount count;
  if (first == last) {
    ++count.node4s;
    return count;
  }
  size_t children = 1;
  auto next = first;
  while (++next != last) {
    if ((*first)[0] != (*next)[0]) {
      for (auto str = first; str != next; ++str) {
        *str = str->substr(1);
      }
      count += count_required_nodes(first, next);
      ++children;
      first = next;
    }
  }

  for (auto str = first; str != next; ++str) {
    *str = str->substr(1);
  }
  count += count_required_nodes(first, next);

  if (children <= 4) {
    ++count.node4s;
  } else if (children <= 16) {
    ++count.node16s;
  } else if (children <= 48) {
    ++count.node48s;
  } else {
    ++count.node256s;
  }

  return count;
}

template <typename It>
constexpr void quicksort(It first, It last) noexcept {
  if (std::distance(first, last) < 2) {
    return;
  }
  It pivot = first;
  auto lower = first;
  auto upper = last;
  bool done = false;
  while (!done) {
    do {
      if (++lower == upper) {
        done = true;
        break;
      }
    } while (*lower <= *pivot);
    while (!done && *--upper > *pivot) {
      if (lower == upper) {
        done = true;
        break;
      }
    }
    if (!done) {
      auto tmp = *lower;
      *lower = *upper;
      *upper = tmp;
    }
  }
  if (lower == last || *pivot < *lower) {
    --lower;
  }
  auto tmp = *lower;
  *lower = *pivot;
  *pivot = tmp;
  quicksort(first, lower);
  if (upper != last) {
    quicksort(++upper, last);
  }
}

template <size_t NumStrings>
constexpr NodesCount count_required_nodes(
    std::array<std::string_view, NumStrings> const &strings) noexcept {
  std::array strings_copy(strings);
  quicksort(strings_copy.begin(), strings_copy.end());
  return count_required_nodes(strings_copy.begin(), strings_copy.end());
}

template <NodesCount Count> class adaptive_radix_tree {
  constexpr static size_t Node4s = Count.node4s;
  constexpr static size_t Node16s = Count.node16s;
  constexpr static size_t Node48s = Count.node48s;
  constexpr static size_t Node256s = Count.node256s;
  std::array<node_4, Node4s> node_4s{};
  std::array<node_16, Node16s> node_16s{};
  std::array<node_48, Node48s> node_48s{};
  std::array<node_256, Node256s> node_256s{};
  std::array<std::string_view, Node4s + Node16s + Node48s + Node256s>
      prefixes{};
  size_t root_idx = std::numeric_limits<size_t>::max();

public:
  constexpr adaptive_radix_tree() noexcept = default;

  template <size_t NumStrings>
  constexpr adaptive_radix_tree(
      std::array<std::string_view, NumStrings> const &strings) noexcept {
    make_nodes<NumStrings>(strings);
  }

  void print_node4_debug(size_t idx) {
    auto &n = node_4s[idx];
    for (size_t c = 0; c < n.filled; ++c) {
      std::cout << "|<f" << c << "> '" << n.keys[c] << "' ";
    }
    std::cout << "\"];\n";
    for (size_t c = 0; c < n.filled; ++c) {
      print_node_debug(n.follow_idxs[c]);
      std::cout << "\"node" << idx << "\":f" << c << " -> \"node"
                << n.follow_idxs[c] << "\"\n";
    }
  }

  void print_node16_debug(size_t idx) {
    auto &n = node_16s[idx - Node4s];
    for (size_t c = 0; c < n.filled; ++c) {
      std::cout << "|<f" << c << "> '" << n.keys[c] << "' ";
    }
    std::cout << "\"];\n";
    for (size_t c = 0; c < n.filled; ++c) {
      print_node_debug(n.follow_idxs[c]);
      std::cout << "\"node" << idx << "\":f" << c << " -> \"node"
                << n.follow_idxs[c] << "\"\n";
    }
  }

  void print_node48_debug(size_t idx) {
    auto &n = node_48s[idx - Node4s - Node16s];
    size_t c = 0;
    for (auto key : n.keys) {
      if (key != empty_key) {
        std::cout << "|<f" << c++ << "> '" << key << "' ";
      }
    }
    std::cout << "\"];\n";
    for (size_t c = 0; c < n.filled; ++c) {
      print_node_debug(n.follow_idxs[c]);
      std::cout << "\"node" << idx << "\":f" << c << " -> \"node"
                << n.follow_idxs[c] << "\"\n";
    }
  }

  void print_node256_debug(size_t idx) {
    auto &n = node_256s[idx - Node4s - Node16s - Node48s];
    char c = 0;
    for (auto slot : n.follow_idxs) {
      if (slot != empty_slot) {
        std::cout << "|<f" << static_cast<size_t>(c) << "> '" << c << "' ";
      }
      c++;
    }
    std::cout << "\"];\n";
    c = 0;
    for (auto slot : n.follow_idxs) {
      if (slot != empty_slot) {
        print_node_debug(slot);
        std::cout << "\"node" << idx << "\":f" << static_cast<size_t>(c)
                  << " -> \"node" << slot << "\"\n";
      }
      c++;
    }
  }

  void print_node_debug(size_t idx) {
    std::cout << "node" << idx << R"([label = "\")" << prefixes[idx] << R"(\" )";
    if (idx < Node4s) {
      print_node4_debug(idx);
    } else if (idx < Node4s + Node16s) {
      print_node16_debug(idx);
    } else if (idx < Node4s + Node16s + Node48s) {
      print_node48_debug(idx);
    } else if (idx < Node4s + Node16s + Node48s + Node256s) {
      print_node256_debug(idx);
    } else {
      std::cout << "\"];\n";
    }
  }

  void print_debug() {
    std::cout << "digraph g {\nnode [shape = record, height = .1];\n";
    print_node_debug(root_idx);
    std::cout << "}\n";
  };

private:
  template <typename InputIt>
  constexpr size_t make_nodes(InputIt first, InputIt last,
                              NodesCount &count) noexcept {
    size_t prefix_len = common_prefix_length(first, last);
    auto prefix = first->substr(0, prefix_len);
    for (auto str = first; str != last; ++str) {
      *str = str->substr(prefix_len);
    }
    while (first != last && first->empty()) {
      ++first;
    }
    if (first == last) {
      prefixes[count.node4s] = prefix;
      return count.node4s++;
    }
    size_t children = 1;
    auto begin = first;
    auto next = first;
    while (++next != last) {
      if ((*begin)[0] != (*next)[0]) {
        ++children;
        begin = next;
      }
    }

    if (children <= 4) {
      size_t idx = count.node4s++;
      prefixes[idx] = prefix;
      make_node(first, last, node_4s[idx], count);
      return idx;
    } else if (children <= 16) {
      size_t idx = count.node16s++;
      prefixes[Count.node4s + idx] = prefix;
      make_node(first, last, node_16s[idx], count);
      return Count.node4s + idx;
    } else if (children <= 48) {
      size_t idx = count.node48s++;
      prefixes[Count.node4s + Count.node4s + idx] = prefix;
      make_node(first, last, node_48s[idx], count);
      return Count.node4s + Count.node16s + idx;
    } else {
      size_t idx = count.node256s++;
      prefixes[Count.node4s + Count.node4s + Count.node4s + idx] = prefix;
      make_node(first, last, node_256s[idx], count);
      return Count.node4s + Count.node16s + Count.node48s + idx;
    }
  }

  template <typename InputIt, typename NodeType>
  constexpr void make_node(InputIt first, InputIt last, NodeType &node,
                           NodesCount &count) {
    auto next = first;
    while (++next != last) {
      if ((*first)[0] != (*next)[0]) {
        char c = (*first)[0];
        for (auto str = first; str != next; ++str) {
          *str = str->substr(1);
        }
        node.insert_next(c, make_nodes(first, next, count));
        first = next;
      }
    }

    char c = (*first)[0];
    for (auto str = first; str != next; ++str) {
      *str = str->substr(1);
    }
    node.insert_next(c, make_nodes(first, next, count));
  }

  template <size_t NumStrings>
  constexpr void
  make_nodes(std::array<std::string_view, NumStrings> const &strings) noexcept {
    NodesCount count;
    std::array strings_copy(strings);
    quicksort(strings_copy.begin(), strings_copy.end());
    root_idx = make_nodes(strings_copy.begin(), strings_copy.end(), count);
  }
};

} // namespace parser

#endif
