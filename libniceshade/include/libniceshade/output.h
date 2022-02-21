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
#include "libniceshade/technique.h"
#include "libniceshade/separate-to-combined-map.h"

#include <tuple>
#include <variant>
#include <vector>

/**
 * @file
 * @brief
 */

namespace niceshade {

/**
 * Container for generated output data.
 */
class compilation_result {
  friend class compilation;

public:
  compilation_result()                     = default;
  compilation_result(compilation_result&&) = default;
  compilation_result& operator=(compilation_result&&) = default;

  /**
   * A span of memory containing raw output bytes (GLSL, Metal Shading Language, or SPIR-V...).
   */
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

/**
 * Compilation result for a specific pipeline stage.
 */
struct compiled_stage {
  compiled_stage()                 = default;
  compiled_stage(compiled_stage&&) = default;
  compiled_stage&    operator=(compiled_stage&&) = default;

  /**
   * The pipeline stage for which the output was generated.
   */
  compilation_result result;

  /**
   * The generated output.
   */
  pipeline_stage     stage;
};

/**
 * A container for target-specific output.
 */
struct targeted_output {
  /**
   * The description of the target for which the output was generated.
   */
  target_desc                 target;

  /**
   * A vector of generated target-specific shaders. Each element corresponds to a single pipeline stage.
   */
  std::vector<compiled_stage> stages;
};

/**
 * A container with the shader code generated for a requested target.
 */
struct compiled_technique {
  compiled_technique()                     = default;
  compiled_technique(compiled_technique&&) = default;
  compiled_technique&          operator=(compiled_technique&&) = default;

  /** The name of the technique for which the shaders were generated. */
  std::string                  name;

  /** A vector of per-target output. Each element corresponds to a single target. */
  std::vector<targeted_output> targeted_outputs;

  /**
   * The pipeline layout, containing information about all resources used by all pipeline stages.
   */
  pipeline_layout              layout;

  /**
   * For targets that do not have a separation between images and samplers at the shader level (i.e. OpenGL),
   * this contains a mapping from a separate image ID to an auto-generated combined image+sampler ID.
   */
  separate_to_combined_map     image_map;

  /**
   * For targets that do not have a separation between images and samplers at the shader level (i.e. OpenGL),
   * this contains a mapping from a separate sampler ID to an auto-generated combined image+sampler ID.
   */
  separate_to_combined_map     sampler_map;
};

/** A vector of \ref compiled_technique objects. */
using compiled_techniques           = std::vector<compiled_technique>;

/** 
 * A sequence of \ref technique_desc objects parsed out of inline technique definitions in the
 * input HLSL code.
 */
using parsed_technique_descs        = std::vector<technique_desc>;

/**
 * A tuple where the first element is a sequence of parsed technique descriptions and the second
 * element is a sequence of corresponding \ref compiled_technique objects.
 */
using descs_and_compiled_techniques = std::tuple<parsed_technique_descs, compiled_techniques>;

}  // namespace niceshade