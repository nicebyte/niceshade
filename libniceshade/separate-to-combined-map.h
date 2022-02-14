/**
 * Copyright (c) 2022 nicegraf contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

#include "libniceshade/metadata-file-writer.h"
#include "spirv_reflect.hpp"

#include <map>
#include <set>
#include <stdint.h>

namespace libniceshade {

// A mapping from separate image or sampler resources to combined image/sampler
// resources autogenerated by SPIRV-Cross. Needed to emulate separate images
// and samplers on platforms that do not support them.
class separate_to_combined_map {
  friend class separate_to_combined_builder;
public:
  void serialize(metadata_file_writer& metadata_file) const;

private:
  struct set_and_binding {
    uint32_t set;
    uint32_t binding;
    bool     operator<(const set_and_binding& rhs) const {
      return set < rhs.set ? true : binding < rhs.binding;
    }
  };
  std::map<set_and_binding, std::set<uint32_t>> map_;
};

}  // namespace libniceshade