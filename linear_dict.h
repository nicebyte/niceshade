/**
Copyright © 2018 nicegraf contributors
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once

#include <algorithm>
#include <utility>
#include <vector>

// A dictionary that stores entries in a linear container.
template <class K, class V>
class linear_dict {
  using container_type = std::vector<std::pair<K, V>>;
public:
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

  iterator begin() { return data_.begin(); }
  iterator end() { return data_.end(); }
  const_iterator begin() const { return data_.begin(); }
  const_iterator end() const { return data_.end(); }
  const_iterator cbegin() const { return data_.cbegin(); }
  const_iterator cend() const { return data_.cend(); }
  iterator find(const K &k) {
    return std::find_if(
        begin(), end(),
        [&k](const std::pair<K, V> &p) { return k == p.first; });
  }
  const_iterator find(const K &k) const {
    return std::find_if(
        cbegin(), cend(),
        [&k](const std::pair<K, V> &p) { return k == p.first; });
  }

  V& operator[](const K &k) {
    auto it = find(k);
    if (it == end()) {
      it = data_.insert(data_.end(), std::make_pair(k, V()));
    }
    return it->second;
  }

  size_t size() const { return data_.size(); }

private:
  std::vector<std::pair<K, V>> data_;
};
