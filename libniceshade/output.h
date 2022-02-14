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

#include "libniceshade/common-types.h"
#include "libniceshade/pipeline-layout.h"
#include "libniceshade/span.h"
#include "libniceshade/target.h"
#include "libniceshade/technique-parser.h"
#include "libniceshade/separate-to-combined-map.h"

#include <tuple>
#include <variant>
#include <vector>

namespace libniceshade {

class compilation_result {
  friend class compilation;

public:
  compilation_result()                     = default;
  compilation_result(compilation_result&&) = default;
  compilation_result& operator=(compilation_result&&) = default;

  const_span<std::byte> data() const {
    if (std::holds_alternative<spirv_blob>(result_)) {
      const spirv_blob& blob = std::get<spirv_blob>(result_);
      return const_span<std::byte> {reinterpret_cast<const std::byte*>(blob.data()), blob.size()};
    } else {
      const std::string& str = std::get<std::string>(result_);
      return const_span<std::byte> {reinterpret_cast<const std::byte*>(str.data()), str.size()};
    }
  }

private:
  explicit compilation_result(const spirv_blob& blob) { result_.emplace<spirv_blob>(blob); }
  explicit compilation_result(std::string&& str) { result_.emplace<std::string>(std::move(str)); }
  std::variant<spirv_blob, std::string> result_;
};

struct compiled_stage {
  compiled_stage()                 = default;
  compiled_stage(compiled_stage&&) = default;
  compiled_stage&    operator=(compiled_stage&&) = default;
  compilation_result result;
  pipeline_stage     stage;
};

struct targeted_output {
  target_desc                 target;
  std::vector<compiled_stage> stages;
};

struct compiled_technique {
  compiled_technique()                     = default;
  compiled_technique(compiled_technique&&) = default;
  compiled_technique&          operator=(compiled_technique&&) = default;
  std::string                  name;
  std::vector<targeted_output> targeted_outputs;
  pipeline_layout              layout;
  separate_to_combined_map     image_map;
  separate_to_combined_map     sampler_map;
  target_desc                  target;
};

using compiled_techniques           = std::vector<compiled_technique>;
using parsed_technique_descs        = std::vector<technique_desc>;
using descs_and_compiled_techniques = std::tuple<parsed_technique_descs, compiled_techniques>;
}  // namespace libniceshade