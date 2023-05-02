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

/**
 * @file
 * @brief
 */

#include "impl/pipeline-layout-builder.h"
#include "impl/separate-to-combined-builder.h"
#include "libniceshade/common-types.h"
#include "libniceshade/output.h"
#include "libniceshade/target.h"
#include "spirv_cross.hpp"

#include <array>
#include <optional>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

namespace niceshade {

class compilation {
public:
  static value_or_error<compilation> create(
      pipeline_stage     kind,
      const spirv_blob&  spirv_code,
      const target_desc& target_info) noexcept;

  error add_resources_to_pipeline_layout(pipeline_layout_builder& builder) const noexcept;
  void add_cis_to_map(
      separate_to_combined_builder& image_map,
      separate_to_combined_builder& sampler_map) const noexcept;
  pipeline_stage                     stage() const noexcept { return stage_; }
  value_or_error<compilation_result> run (const pipeline_layout& pipeline_layout) noexcept;
  const target_desc&                 target() const noexcept { return target_info_; }
  std::optional<std::array<uint32_t, 3>> threadgroup_size () const noexcept;

private:
  target_desc                            target_info_;
  pipeline_stage                         stage_;
  std::unique_ptr<spirv_cross::Compiler> spv_cross_compiler_;
  const spirv_blob*                      original_spirv_ = nullptr;
};

}  // namespace niceshade
