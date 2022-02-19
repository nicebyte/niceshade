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

#include "libniceshade/error.h"
#include "libniceshade/output.h"
#include "libniceshade/input.h"
#include "libniceshade/span.h"
#include "libniceshade/target.h"

#include <string>
#include <vector>
#include <stdint.h>

namespace niceshade {

class dxc_wrapper;

class instance {
public:
    struct options {
        std::string shader_model = "6_2";
        span<std::string> dxc_params;
        std::string dxc_lib_folder = "./";
    };

    static value_or_error<instance> create(const options& opts);
    instance() = default;
    ~instance();
    instance(const instance&) = delete;
    instance& operator=(const instance&) = delete;
    instance(instance&& other) { *this = std::move(other); }
    instance& operator=(instance&& other) {
      dxc_       = other.dxc_;
      other.dxc_ = nullptr;
      return *this;
    }

    value_or_error<compiled_techniques> compile(const_span<compiler_input> compiler_inputs, const_span<target_desc> targets);
    value_or_error<descs_and_compiled_techniques> parse_techniques_and_compile(
        input_blob              in_blob,
        const char*             file_name,
        const_span<target_desc> targets,
        const define_container& global_defines);

  private:
    dxc_wrapper* dxc_;
};

}