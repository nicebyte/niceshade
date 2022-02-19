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

#include "libniceshade/pipeline-layout.h"

#include <sstream>
#include <stdio.h>
#include <stdlib.h>

namespace niceshade {

std::string pipeline_layout::native_binding_map_string() const {
  std::ostringstream os;
  os << "/**NGF_NATIVE_BINDING_MAP\n";
  for (const auto& set_id_and_layout : sets_) {
    for (const auto& binding_id_and_descriptor : set_id_and_layout.second.layout) {
      os << "(" << set_id_and_layout.first << " " << binding_id_and_descriptor.first << ") : "
         << binding_id_and_descriptor.second.native_binding << "\n";
    }
  }
  os << "(-1 -1) : -1\n**/";
  return os.str();
}

const descriptor_set_layout& pipeline_layout::set(uint32_t set_id) const {
  static const descriptor_set_layout empty_layout{};
  auto                               it = sets_.find(set_id);
  if (it != sets_.cend()) {
    return it->second.layout;
  } else {
    return empty_layout;
  }
}


}  // namespace niceshade
