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

namespace niceshade {

/**
 * A _technique_ is a particular configuration of graphics pipeline stages.
 */
struct technique_desc {
  /**
   * Specifies details about a pipeline stage entry point.
   */
  struct entry_point {
    pipeline_stage stage; /**< The pipeline stage type. */
    std::string    name;  /**< The name of the entry point. */
  };

  std::string      name;    /**< The name of the technique.*/
  define_container defines; /**< Additional definitions to be added to the preprocessor while
                                 compiling this technique. */
  std::vector<entry_point> entry_points; /**< Entry point specifications for the technique. */

  /**
   * User-provided metadata.
   * 
   * This is only really useful when the techniques are specified directly inside the HLSL code
   * using the special `//T:` comment. In that case, the additional metadata shall be parsed out
   * and made available to the application, which can then use it for its own purposes (for example,
   * enable or disable depth write for particular techniques based on user metadata values). See the
   * <a href="https://github.com/nicebyte/niceshade#techniques">README</a> for more details on
   * inline technique definition.
   */
  std::vector<std::pair<std::string, std::string>> additional_metadata;  
};

}  // namespace niceshade
