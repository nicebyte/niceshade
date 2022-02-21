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

#include <stdint.h>
#include <string>

namespace niceshade {

/**
 * Enumerates the supported target APIs
 */
enum class target_api { GL, METAL, VULKAN };

/**
 * Enumerates the device type that a target API might run on.
 */
enum class target_platform_class {
  DONTCARE, /**< No device preference (falls back to desktop for GL). */
  DESKTOP, /**< Desktop GL, Metal for macOS. */
  MOBILE   /**< GL ES, Metal for iOS. */
};

// Information about a compilation target.
struct target_desc {
  target_api            api;          /**< Target API. */
  uint32_t              version_maj;  /**< Major version number. */
  uint32_t              version_min;  /**< Minor version number. */
  target_platform_class platform;     /**< Device type that the target API runs on. */

  bool operator==(const target_desc& other) {
    return api == other.api && version_maj == other.version_maj &&
           version_min == other.version_min && platform == other.platform;
  }

  bool operator!=(const target_desc& other) { return !(*this == other); }
};

/**
 * @param target A target description.
 * @return File extension for the given target description.
 */
std::string file_ext_for_target(const target_desc& target);

}  // namespace niceshade
