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

#include "libniceshade/span.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

/**
 * @file
 * @brief
 */

/**
 * Main namespace containing niceshade types and routines.
 */
namespace niceshade {

/**
 * Enumerates the supported types of programmable GPU pipeline stages.
 */
enum class pipeline_stage {
  vertex,   /**< Corresponds to the vertex stage. */
  fragment, /**< Corresponds to the fragment stage. */
  compute   /**< Corresponds to the compute stage. */
};

/**
 * Stores a sequence of preprocessor definitions.
 *
 * Each preprocessor definition is stored as a pair of strings, where the first element represents
 * the name of the definition, and the second element represents the value.
 */
using define_container = std::vector<std::pair<std::string, std::string>>;

/**
 * A blob of SPIR-V code.
 */
using spirv_blob = std::vector<uint32_t>;

/**
 * A span of memory containing HLSL code to be processed by niceshade.
 */
using input_blob = const_span<std::byte>;

}  // namespace niceshade
