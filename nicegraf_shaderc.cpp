/**
 * Copyright (c) 2020 nicegraf contributors
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

#define _CRT_SECURE_NO_WARNINGS

#include "dxc_wrapper.h"
#include "file_utils.h"
#include "header_file_writer.h"
#include "linear_dict.h"
#include "pipeline_layout.h"
#include "pipeline_metadata_file.h"
#include "separate_to_combined_map.h"
#include "shader_defines.h"
#include "target.h"
#include "technique_parser.h"
#include "spirv_reflect.hpp"
#include "compilation.h"

#include <ctype.h>
#include <memory>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

const char *USAGE = R"RAW(
Usage: ngf_shaderc <input file name> [options] -- [dxc options]

A wrapper for Microsoft DirectX Shader Compiler and SPIRV-Cross that compiles
HLSL shaders for multiple different targets.

Options:

  -O <path> - Folder to store output files in. Default is the current working
    directory.
  
  -t <target> - Generate shaders for the given target.  Accepted values are:
      * gl430;
      * gles310, gles300;
      * msl10, msl11, msl12, msl20;
      * msl10ios, msl11ios, msl12ios, msl20ios;
      * spv 
    If the option is encountered multiple times, shaders for all of the
    mentioned targets will be generated. At least one occurence of this option is
    required.

  -m <version> - HLSL shader model version to use. Valid values are: 6_0, 6_1, 6_2, 6_3,
   6_4, 6_5, 6_6. Default is 6_0.

  -h <path> - Path (relative to the output folder) for the generated
      header file with descriptor binding and set IDs. If not specified, no
      header file will be generated.

  -n <identifier> - Namespace for the generated shader file. If not specified,
     global namespace is used.
    
  -D <name>=<value> - Add a preprocessor definition `name` with the value `value` to
     techniques.

   Everything following the double dash (`--`) is passed as-is to the
   Microsoft DirectX Shader Compiler.

)RAW";

int main(int argc, const char *argv[]) {
  if (argc <= 1) { // Display help if invoked with no arguments.
    printf("%s\n", USAGE);
    exit(0);
  }

#pragma region cmdline
  // Process command line options, stopping at double dash.
  // Everything after the double dash will be passed as-is to
  // Microsoft DirectX Shader Compiler.
  const std::string input_file_path { argv[1] };
  std::string out_folder = ".";
  std::string header_path = "";
  std::string header_namespace = "";
  std::string shader_model = "6_2";
  std::vector<const target_info*> targets;
  define_container global_macro_definitions;
  size_t dxc_options_start = argc;

  for (size_t o = 2u;
       o < (size_t)argc && dxc_options_start >= argc;
       o += 2u) {
    const std::string option_name { argv[o] };
    if (option_name == "--") {
      dxc_options_start = o + 1u;
      continue;
    }
    if (o + 1u >= (uint32_t)argc) {
      fprintf(stderr, "Expected an option value after %s\n", argv[o]);
      exit(1);
    }
    const std::string option_value { argv[o + 1u] };
    if (option_value == "--") {
      fprintf(stderr, "Expected an option value after %s,\n", argv[o]);
      exit(1);
    }
    if ("-t" == option_name) { // Target to generate code for.
      const auto *t = std::find_if(TARGET_MAP, TARGET_MAP + TARGET_COUNT,
                                   [&option_value](const named_target_info &x) {
                                     return option_value == x.name;
                                   });
      if (t == TARGET_MAP + TARGET_COUNT) {
        fprintf(stderr, "Unknown target \"%s\"\n", option_value.c_str());
        exit(1);
      }
      targets.push_back(&(t->target));
    } else if ("-m" == option_name) {
      shader_model = option_value;
      if (shader_model.length() != 3) {
          fprintf(stderr, "Invalid value for shader model: \"%s\"\n", shader_model.c_str());
          exit(1);
      } else {
        const int sm_maj_ver = shader_model[0] - '0';
        const int sm_min_ver = shader_model[2] - '0';
        if (sm_maj_ver != 6 ||
            sm_min_ver < 0 ||
            sm_min_ver > 6) {
            fprintf(stderr, "Unsupported shader model version: \"%s\"\n", shader_model.c_str());
            exit(1);
        }
      }
    } else if ("-O" == option_name) { // Output folder.
      out_folder = option_value;
    } else if ("-h" == option_name) {
      header_path = option_value;
    } else if ("-n" == option_name) {
      header_namespace = option_value;
    } else if ("-D" == option_name) {
        const size_t pos = option_value.find('=');
        if (pos < option_value.size())
          global_macro_definitions.emplace_back(option_value.substr(0, pos), option_value.substr(pos+1));
        else
          global_macro_definitions.emplace_back(option_value, std::string());
    } else {
      fprintf(stderr, "Unknown option: \"%s\"\n", option_name.c_str());
      exit(1);
    }
  }

  // Build up parameters for the DirectX Shader Compiler.
  std::vector<std::string> dxc_options = {
    "-spirv",  // always enable spir-v codegen
    "-Zpc"     // always forbid overriding explicit matrix orientation.
  };
  bool enable_spv_opt = false;
  // Add the remaining dxc parameters from the command line.
  for (size_t o = dxc_options_start; o < argc; ++o) {
    dxc_options.emplace_back(argv[o]);
    if (dxc_options.back() == "-O3") {
      enable_spv_opt = true;
    }
  }
  if (!enable_spv_opt) {
    // Always enable SPIR-V optimization passes unless the user explicitly
    // turned them off.
    dxc_options.emplace_back("-O0");
  }
#pragma endregion cmd_line

#pragma region pre_checks
  // Do a sanity check - no point in running with no targets.
  if (targets.empty()) {
    fprintf(stderr, "No target shader flavors specified!"
                    " Use -t to specify a target.\n");
    exit(1);
  }

  // Make sure targets are always processed in the same order, no matter
  // what order they're specified in.
  std::sort(targets.begin(), targets.end(), [](const target_info *t1,
                                               const target_info *t2) { 
                                              return t1->api < t2->api;
                                            });
#pragma endregion pre_checks

#pragma region load_input
  // Load the input file.
  std::string input_source = read_file(input_file_path.c_str());
  input_source.push_back('\n');

  // Look for and parse technique directives in the code.
  std::vector<technique> techniques;
  parse_techniques(input_source, techniques, global_macro_definitions);
  if (techniques.size() == 0u) {
    fprintf(stderr, "Input file does not appear to define any techniques. "
                    "Define techniques with a special comment (`//T:').\n");
    exit(1);
  }
  const std::string exe_path(argv[0]);
  const std::string exe_dir = exe_path.substr(0, exe_path.find_last_of("/\\"));
#pragma endregion load_input

#pragma region gen_spv
  // Obtain SPIR-V.
  dxc_wrapper dxcompiler(shader_model, dxc_options, exe_dir);
  for (technique &tech : techniques) {
    for (technique::entry_point &ep : tech.entry_points) {
      // Produce SPIR-V.
      dxc_wrapper::result result = dxcompiler.compile_hlsl2spv(
          input_source.c_str(),
          input_source.size(),
          input_file_path.c_str(),
          ep,
          tech.defines);
      if (result.HasDiagMessage()) {
        fprintf(stderr, "%s", result.diag_message.c_str());
      }
      if (!result.HasData()) {
        exit(1);
      }
      ep.spirv_code = std::move(result.spirv_code);
    }
  }
#pragma endregion gen_spv

 #pragma region gen_output
  // Generate output.

  // Attempt to open header file for writing.
  const bool generate_header = !header_path.empty();
  header_file_writer header_writer(out_folder, header_path, header_namespace);
  if (generate_header && !header_writer.is_open()) {
    fprintf(stderr, "Failed to open output file %s\n", header_writer.path());
    exit(1);
  }

  for (const technique& tech : techniques) {
    pipeline_layout res_layout;
    separate_to_combined_map images_to_cis, samplers_to_cis;
    std::vector<compilation> compilations;
    
    for (const technique::entry_point& ep : tech.entry_points) {
      const std::vector<uint32_t>& spv_code = ep.spirv_code;
      for (const target_info* target_info : targets) {
        compilations.emplace_back(ep.kind, spv_code, *target_info);
        compilations.back().add_cis_to_map(images_to_cis, samplers_to_cis);
        compilations.back().add_resources_to_pipeline_layout(res_layout);
      }
    }

    for (compilation &c : compilations) {
      std::string out_file_path = out_folder + PATH_SEPARATOR + tech.name;
      c.run(out_file_path.c_str());
      // TODO: remap bindings here
    }

    // Write out the .pipeline file for the current technique.
    std::string metadata_file_path =
      out_folder + PATH_SEPARATOR + tech.name + ".pipeline";
    pipeline_metadata_file metadata_file(metadata_file_path.c_str());
    header_writer.begin_technique(tech.name);

    // Write out the entrypoints section.
    metadata_file.start_new_record();
    metadata_file.write_field(tech.entry_points.size());
    for (const technique::entry_point& ep : tech.entry_points) {
      metadata_file.write_field((uint32_t)ep.kind);
      metadata_file.write_raw_bytes(ep.name.c_str(),
        ep.name.length() + 1u);
    }

    // Write out the pipeline layout record.
    metadata_file.start_new_record();
    metadata_file.write_field(res_layout.set_count());
    for (uint32_t set = 0u; set < res_layout.set_count(); ++set) {
      const descriptor_set_layout& ds = res_layout.set(set);
      metadata_file.write_field((uint32_t)ds.size());
      for (const auto& d : ds) {
        metadata_file.write_field(d.second.slot);
        metadata_file.write_field((uint32_t)d.second.type);
        metadata_file.write_field(d.second.stage_mask);
        header_writer.write_descriptor(d.second, set);
      }
    }
    header_writer.end_technique();

    // Write out separate-to-combined map records.
    metadata_file.start_new_record();
    images_to_cis.serialize(metadata_file);
    metadata_file.start_new_record();
    samplers_to_cis.serialize(metadata_file);

    // Write out user metadata record.
    metadata_file.start_new_record();
    metadata_file.write_field((uint32_t)tech.additional_metadata.size());
    for (const auto& nameval : tech.additional_metadata) {
      metadata_file.write_raw_bytes(nameval.first.c_str(),
        nameval.first.size() + 1u);
      metadata_file.write_raw_bytes(nameval.second.c_str(),
        nameval.second.size() + 1u);
    }
    metadata_file.finalize();
  }
#pragma endregion gen_output
  return 0;
}
