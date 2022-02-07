/**
 * Copyright (c) 2021 nicegraf contributors
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

#include "libniceshade/technique-parser.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

namespace libniceshade {

// States of the technique parser.
enum class technique_parser_state {
  LOOKING_FOR_PREFIX,
  LOOKING_FOR_NAME,
  PARSING_NAME,
  LOOKING_FOR_PARAMETER_NAME,
  PARSING_PARAMETER_NAME,
  PARSING_ENTRYPOINT_NAME,
  PARSING_NAMEVAL_NAME,
  PARSING_NAMEVAL_VALUE,
  FINALIZING_TECHNIQUE
};

namespace {
bool is_ident(char c) { return (isalnum(c) || c == '_'); }
constexpr bool is_tab_space(char c) { return (c == ' '  || c == '\t'); }
}

value_or_error<std::vector<technique_desc>>
parse_techniques(const std::string &input_source, const define_container &default_defines) {
  uint32_t last_four_chars = 0u;
  uint32_t line_num = 1u;
  const uint32_t technique_prefix = 0x2f2f543a; // `//T:'
  technique_parser_state state = technique_parser_state::LOOKING_FOR_PREFIX;
  std::string parameter_name, entry_point_name, nameval_name,
              nameval_value;
  bool have_vertex_stage = false;
  std::vector<technique_desc> techniques;
  for (uint32_t c_idx = 0u; c_idx < input_source.size(); ++c_idx) {
    char c = input_source[c_idx];
    // Collapse windows line endings into '\n'.
    if (c == '\r' && (c_idx == input_source.size() - 1u ||
                      input_source[c_idx + 1u] != '\n')) {
      NICESHADE_RETURN_ERROR("Stray carriage return in input on line %d\n", line_num);
    } else if (c == '\r') {
      continue;
    }
    if (!is_tab_space(c)) {
      last_four_chars <<= 8u;
      last_four_chars |= (uint32_t)c;
    }
    switch(state) {
    case technique_parser_state::LOOKING_FOR_PREFIX:
      if (last_four_chars == technique_prefix) {
        state = technique_parser_state::LOOKING_FOR_NAME;
        techniques.emplace_back();
        technique_desc &new_tech = techniques.back();
        new_tech.defines.insert(new_tech.defines.end(),
                                default_defines.begin(),
                                default_defines.end());
        have_vertex_stage = false;
      }
      break;
    case technique_parser_state::LOOKING_FOR_NAME:
      if (is_ident(c)) {
        state = technique_parser_state::PARSING_NAME;
        techniques.back().name.push_back(c);
      } else if (!is_tab_space(c)) {
        NICESHADE_RETURN_ERROR("unexpected character ", c, " in technique name ", "on line ", line_num);
      }
      break;
    case  technique_parser_state::PARSING_NAME:
      if (is_ident(c) || c == '-') {
        techniques.back().name.push_back(c);
      } else if (is_tab_space(c)) {
        state = technique_parser_state::LOOKING_FOR_PARAMETER_NAME;
      } else {
        NICESHADE_RETURN_ERROR("unexpected character ", c, " in technique name ", "on line ", line_num);
      }
      break;
    case technique_parser_state::LOOKING_FOR_PARAMETER_NAME:
      if (is_ident(c)) {
        state = technique_parser_state::PARSING_PARAMETER_NAME;
        parameter_name.clear();
        parameter_name.push_back(c);
      } else if (c == '\n') {
        state = technique_parser_state::FINALIZING_TECHNIQUE;
      } else if (!is_tab_space(c)) {
        NICESHADE_RETURN_ERROR(
            "unexpected character ",
            c,
            " in technique param name ",
            "on line ",
            line_num);
      }
      break;
    case technique_parser_state::PARSING_PARAMETER_NAME:
      if (is_ident(c)) {
        parameter_name.push_back(c);
      } else if (c == ':') {
        if (parameter_name == "define" || parameter_name == "meta") {
          state = technique_parser_state::PARSING_NAMEVAL_NAME;
          nameval_name.clear();
        } else if (parameter_name == "vs" || parameter_name == "ps") {
          state = technique_parser_state::PARSING_ENTRYPOINT_NAME;
          entry_point_name.clear();
        } else {
          NICESHADE_RETURN_ERROR("unknown parameter ", parameter_name.c_str(), " on line ", line_num);
        }
      } else {
        NICESHADE_RETURN_ERROR(
            "unexpected character ",
            c,
            " in technique param name ",
            "on line ",
            line_num);
      }
      break;
    case technique_parser_state::PARSING_ENTRYPOINT_NAME:
      if (is_ident(c)) {
        entry_point_name.push_back(c);
      } else if (is_tab_space(c) || c == '\n') {
        if (entry_point_name.empty()) {
          NICESHADE_RETURN_ERROR("entry point name cannot be empty on line ", line_num);
        }
        technique_desc::entry_point ep {
          parameter_name == "vs"
              ? pipeline_stage::vertex
              : pipeline_stage::fragment,
          entry_point_name
        };
        for (const auto &prev_ep : techniques.back().entry_points) {
          if (prev_ep.stage == ep.stage) {
            NICESHADE_RETURN_ERROR(
                "duplicate entry point ",
                parameter_name.c_str(),
                ":",
                ep.name.c_str(),
                " on line ",
                line_num);
          }
        }
        techniques.back().entry_points.emplace_back(ep);
        have_vertex_stage |= (parameter_name == "vs");
        state =
            c != '\n'
            ? technique_parser_state::LOOKING_FOR_PARAMETER_NAME
            : technique_parser_state::FINALIZING_TECHNIQUE;
      } else {
        NICESHADE_RETURN_ERROR(
            "unexpected character ",
            c,
            " in entry point name on line ",
            line_num);
      }
      break;
    case technique_parser_state::PARSING_NAMEVAL_NAME:
      if (is_ident(c)) {
        nameval_name.push_back(c);
      } else if (c == '=') {
        state = technique_parser_state::PARSING_NAMEVAL_VALUE;
        nameval_value.clear();
      } else {
        NICESHADE_RETURN_ERROR(
            "unexpected character ",
            c,
            " in definition name on line ",
            line_num);
      }
      break;
    case technique_parser_state::PARSING_NAMEVAL_VALUE:
      if(!is_tab_space(c) && c != '\n') {
        nameval_value.push_back(c);
      } else {
        if (parameter_name == "define") {
          techniques.back().defines.emplace_back(nameval_name, nameval_value);
        } else if (parameter_name == "meta") {
        techniques.back().additional_metadata.emplace_back(nameval_name,
                                                           nameval_value);
        } else {
          assert(false);
        }
        state = c != '\n'
          ? technique_parser_state::LOOKING_FOR_PARAMETER_NAME
          : technique_parser_state::FINALIZING_TECHNIQUE;
      }
      break;
    case technique_parser_state::FINALIZING_TECHNIQUE:
      if (!have_vertex_stage) {
        NICESHADE_RETURN_ERROR("technique needs to define at least a vertex stage on line ", line_num);
      }
      state = technique_parser_state::LOOKING_FOR_PREFIX;
      break;
    }
    if (c == '\n') ++line_num;
  }
  return techniques;
}

}  // namespace libniceshade