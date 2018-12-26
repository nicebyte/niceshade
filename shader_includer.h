/**
Copyright © 2018 nicegraf contributors
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "file_utils.h"
#include "shaderc/shaderc.hpp"
#include <string>

// Provide file inclusion for shaderc.
class includer: public shaderc::CompileOptions::IncluderInterface {
  struct includer_data {
    std::string file_name;
    std::string content;
  };
public:
  shaderc_include_result* GetInclude(const char *file_name,
                                     shaderc_include_type,
                                     const char *, size_t) override {
    includer_data *data = new includer_data{};
    data->content = read_file(file_name);
    data->file_name = file_name;
    auto result = new shaderc_include_result;
    result->source_name = data->file_name.c_str();
    result->source_name_length = data->file_name.length();
    result->content = data->content.c_str();
    result->content_length = data->content.length();
    result->user_data = data;
    return result;
  }

  void ReleaseInclude(shaderc_include_result *data) override {
    delete static_cast<includer_data*>(data->user_data);
  }
};