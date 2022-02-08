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
#include "libniceshade/technique-parser.h"
#include "libniceshade/pipeline-layout.h"

#include <vector>
#include <tuple>

namespace libniceshade {

struct compiled_stage {
    std::vector<char> blob;
    pipeline_stage stage;
};

struct compiled_technique {
    std::vector<compiled_stage> stages;
    pipeline_layout layout;
};

using compiled_techniques = std::vector<compiled_technique>;
using parsed_technique_descs = std::vector<technique_desc>;
using descs_and_compiled_techniques = std::tuple<parsed_technique_descs, compiled_techniques>;
}