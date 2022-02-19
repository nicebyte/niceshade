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

#include "libniceshade/target.h"

namespace niceshade {

std::string file_ext_for_target(const target_desc& target) {
  const bool is_mobile = target.platform == target_platform_class::MOBILE;
  switch (target.api) {
  case target_api::GL:
    return std::to_string(target.version_maj * 100 + target.version_min * 10) +
           (is_mobile ? "es" : "") + ".glsl";
  case target_api::METAL:
    return std::to_string(target.version_maj * 10 + target.version_min) + (is_mobile ? "ios" : "") +
           ".msl";
  case target_api::VULKAN: return "spv";
  }
  return "unknown";
}

}  // namespace niceshade