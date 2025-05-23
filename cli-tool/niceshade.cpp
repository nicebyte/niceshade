/**
 * Copyright (c) 2025 nicegraf contributors
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

#include "libniceshade/niceshade.h"

#include "cli-tool/file-utils.h"
#include "cli-tool/header-file-writer.h"
#include "cli-tool/metadata-file-writer.h"
#include "cli-tool/target-list.h"
#include "spirv_reflect.hpp"

#include <ctype.h>
#include <memory>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

using namespace niceshade;

const char* USAGE = R"RAW(
Usage: niceshade <input file name> [options] -- [dxc options]

A wrapper for Microsoft DirectX Shader Compiler and SPIRV-Cross that compiles
HLSL shaders for multiple different targets.

Options:

  -O <path> - Folder to store output files in. Default is the current working
    directory.
  
  -t <target> - Generate shaders for the given target.  Accepted values are:
      * gl430;
      * gles310, gles300;
      * msl10, msl11, msl12, msl20, msl21;
      * msl10ios, msl11ios, msl12ios, msl20ios, msl21ios;
      * spv 
    If the option is encountered multiple times, shaders for all of the
    mentioned targets will be generated. At least one occurence of this option is
    required.

  -m <version> - HLSL shader model version to use. Valid values are: 6_0, 6_1, 6_2, 6_3,
   6_4, 6_5, 6_6. Default is 6_2.

  -h <path> - Path (relative to the output folder) for the generated
      header file with descriptor binding and set IDs. If not specified, no
      header file will be generated.

  -n <identifier> - Namespace for the generated shader file. If not specified,
     the global namespace is used.
    
  -D <name>=<value> - Add a preprocessor definition `name` with the value `value` to
     techniques.
 
  -p <yes|no> - Wheter to reserve unused bindings in the generated SPIR-V and pipeline template data (default behavior is NO). 

   Everything following the double dash (`--`) is passed as-is to the
   Microsoft DirectX Shader Compiler.

)RAW";

void report_dxc_error(const char* msg, size_t size) {
  fprintf(stderr, "DXC diagnostic message:\n%.*s\n", (unsigned int)size, msg);
}

int main(int argc, const char* argv[]) {
  if (argc <= 1) {  // Display help if invoked with no arguments.
    printf("%s\n", USAGE);
    exit(0);
  }

#pragma region cmdline
  // Process command line options, stopping at double dash.
  // Everything after the double dash will be passed as-is to
  // Microsoft DirectX Shader Compiler.
  const std::string        input_file_path {argv[1]};
  std::string              out_folder        = ".";
  std::string              header_path       = "";
  std::string              header_namespace  = "";
  std::string              shader_model      = "6_2";
  bool                     preserve_bindings = false;
  std::vector<target_desc> targets;
  define_container         global_macro_definitions;
  size_t                   dxc_options_start = argc;

  for (size_t o = 2u; o < (size_t)argc && dxc_options_start >= argc; o += 2u) {
    const std::string option_name {argv[o]};
    if (option_name == "--") {
      dxc_options_start = o + 1u;
      continue;
    }
    if (o + 1u >= (uint32_t)argc) {
      fprintf(stderr, "Expected an option value after %s\n", argv[o]);
      exit(1);
    }
    const std::string option_value {argv[o + 1u]};
    if (option_value == "--") {
      fprintf(stderr, "Expected an option value after %s,\n", argv[o]);
      exit(1);
    }
    if ("-t" == option_name) {  // Target to generate code for.
      const auto* t = std::find_if(
          TARGET_MAP,
          TARGET_MAP + TARGET_COUNT,
          [&option_value](const named_target_info& x) { return option_value == x.name; });
      if (t == TARGET_MAP + TARGET_COUNT) {
        fprintf(stderr, "Unknown target \"%s\"\n", option_value.c_str());
        exit(1);
      }
      targets.push_back((t->target));
    } else if ("-m" == option_name) {
      shader_model = option_value;
      if (shader_model.length() != 3) {
        fprintf(stderr, "Invalid value for shader model: \"%s\"\n", shader_model.c_str());
        exit(1);
      } else {
        const int sm_maj_ver = shader_model[0] - '0';
        const int sm_min_ver = shader_model[2] - '0';
        if (sm_maj_ver != 6 || sm_min_ver < 0 || sm_min_ver > 6) {
          fprintf(stderr, "Unsupported shader model version: \"%s\"\n", shader_model.c_str());
          exit(1);
        }
      }
    } else if ("-O" == option_name) {  // Output folder.
      out_folder = option_value;
    } else if ("-h" == option_name) {
      header_path = option_value;
    } else if ("-n" == option_name) {
      header_namespace = option_value;
    } else if ("-D" == option_name) {
      const size_t pos = option_value.find('=');
      if (pos < option_value.size())
        global_macro_definitions.emplace_back(
            option_value.substr(0, pos),
            option_value.substr(pos + 1));
      else
        global_macro_definitions.emplace_back(option_value, std::string());
    } else if ("-p" == option_name) {
      preserve_bindings = option_value == "yes";
    } else {
      fprintf(stderr, "Unknown option: \"%s\"\n", option_name.c_str());
      exit(1);
    }
  }

  // Build up parameters for the DirectX Shader Compiler.
  std::vector<std::string> dxc_options = {
      "-Zpc"  // always forbid overriding explicit matrix orientation.
  };
  // Add the remaining dxc parameters from the command line.
  for (size_t o = dxc_options_start; o < argc; ++o) dxc_options.emplace_back(argv[o]);
#pragma endregion cmd_line

#pragma region pre_checks
  // Do a sanity check - no point in running with no targets.
  if (targets.empty()) {
    fprintf(
        stderr,
        "No target shader flavors specified!"
        " Use -t to specify a target.\n");
    exit(1);
  }

  // Make sure targets are always processed in the same order, no matter
  // what order they're specified in.
  std::sort(targets.begin(), targets.end(), [](const target_desc& t1, const target_desc& t2) {
    return t1.api < t2.api;
  });
#pragma endregion pre_checks

  const std::string exe_path(argv[0]);
  const std::string exe_dir = exe_path.substr(0, exe_path.find_last_of("/\\"));

  // Load the input file.
  std::string input_source = read_file(input_file_path.c_str());
  input_source.push_back('\n');

  value_or_error<instance> maybe_inst = instance::create(instance::options {
      shader_model,
      span<std::string> {dxc_options.data(), dxc_options.size()},
      exe_dir,
      report_dxc_error,
      preserve_bindings});
  if (maybe_inst.is_error()) {
    fprintf(stderr, "%s", maybe_inst.error_message().c_str());
    exit(1);
  }
  instance& inst = maybe_inst.get();

  auto maybe_results = inst.parse_techniques_and_compile(
      const_span<std::byte> {(std::byte*)input_source.data(), input_source.size()},
      input_file_path.c_str(),
      const_span<target_desc> {targets.data(), targets.size()},
      global_macro_definitions);
  if (maybe_results.is_error()) {
    fprintf(stderr, "%s", maybe_results.error_message().c_str());
    exit(1);
  }
  const descs_and_compiled_techniques& results         = maybe_results.get();
  const auto&                          technique_descs = std::get<parsed_technique_descs>(results);
  const auto&                          compiled_techs  = std::get<compiled_techniques>(results);

#pragma region gen_output
  // Generate output.

  // Attempt to open header file for writing.
  const bool         generate_header = !header_path.empty();
  header_file_writer header_writer(out_folder, header_path, header_namespace);
  if (generate_header && !header_writer.is_open()) {
    fprintf(stderr, "Failed to open output file %s\n", header_writer.path());
    exit(1);
  }

  for (const technique_desc& tech : technique_descs) {
    const size_t              tech_idx      = &tech - technique_descs.data();
    const compiled_technique& compiled_tech = compiled_techs[tech_idx];
    const pipeline_layout&    res_layout    = compiled_tech.layout;

    std::string                            out_file_path = out_folder + PATH_SEPARATOR + tech.name;
    std::optional<std::array<uint32_t, 3>> maybe_threadgroup_size;

    for (const targeted_output& target_out : compiled_tech.targeted_outputs) {
      std::string native_binding_map_str;
      for (const compiled_stage& out_stage : target_out.stages) {
        const std::string ep_extension = [](pipeline_stage s) {
          switch (s) {
          case pipeline_stage::vertex: return ".vs.";
          case pipeline_stage::fragment: return ".ps.";
          case pipeline_stage::compute: return ".cs.";
          }
          return "";
        }(out_stage.stage);
        const std::string full_out_file_path =
            out_file_path + ep_extension + file_ext_for_target(target_out.target);
        FILE* out_file = fopen(full_out_file_path.c_str(), "wb");
        if (out_file == nullptr) {
          fprintf(stderr, "Failed to open output file %s\n", out_file_path.c_str());
          exit(1);
        }
        fwrite(out_stage.result.data().begin(), 1u, out_stage.result.data().size(), out_file);
        if (target_out.target.api != target_api::VULKAN) {
          if (native_binding_map_str.empty()) {
            std::ostringstream os;
            os << "/**NGF_NATIVE_BINDING_MAP\n";
            for (const auto& set_id_and_layout : compiled_tech.layout) {
              for (const auto& binding_id_and_descriptor : set_id_and_layout.second) {
                os << "(" << set_id_and_layout.first << " " << binding_id_and_descriptor.first
                   << ") : " << binding_id_and_descriptor.second.native_binding << "\n";
              }
            }
            os << "(-1 -1) : -1\n**/\n";
            native_binding_map_str = os.str();
          }
          fwrite(native_binding_map_str.data(), 1u, native_binding_map_str.size(), out_file);
        }
        if (out_stage.stage == pipeline_stage::compute) {
          if (target_out.target.api == target_api::METAL) {
            if (!out_stage.threadgroup_size) {
              fprintf(stderr, "failed to find threadgroup size for compute shader");
              exit(1);
            }
            fprintf(
                out_file,
                "/**NGF_THREADGROUP_SIZE %d %d %d */\n",
                out_stage.threadgroup_size.value()[0],
                out_stage.threadgroup_size.value()[1],
                out_stage.threadgroup_size.value()[2]);
          }
          maybe_threadgroup_size = out_stage.threadgroup_size;
        }
        fclose(out_file);
      }
    }

    // Write out the .pipeline file for the current technique.
    std::string          metadata_file_path = out_folder + PATH_SEPARATOR + tech.name + ".pipeline";
    metadata_file_writer metadata_file(metadata_file_path.c_str());
    header_writer.begin_technique(tech.name);

    // Write out the entrypoints section.
    metadata_file.start_new_record();
    metadata_file.write_field((uint32_t)tech.entry_points.size());
    for (const technique_desc::entry_point& ep : tech.entry_points) {
      metadata_file.write_field((uint32_t)ep.stage);
      metadata_file.write_raw_bytes(ep.name.c_str(), ep.name.length() + 1u);
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
    auto serialize_separate_to_combined_map =
        [&metadata_file](const separate_to_combined_map& map) {
          metadata_file.start_new_record();
          metadata_file.write_field((uint32_t)map.size());
          for (const auto& entry : map) {
            const set_and_binding&    sb                      = entry.first;
            const std::set<uint32_t>& combined_image_samplers = entry.second;
            metadata_file.write_field(sb.set);
            metadata_file.write_field(sb.binding);
            metadata_file.write_field((uint32_t)combined_image_samplers.size());
            for (const auto& c : combined_image_samplers) { metadata_file.write_field(c); }
          }
        };
    serialize_separate_to_combined_map(compiled_tech.image_map);
    serialize_separate_to_combined_map(compiled_tech.sampler_map);

    // Write out user metadata record.
    metadata_file.start_new_record();
    metadata_file.write_field((uint32_t)tech.additional_metadata.size());
    for (const auto& nameval : tech.additional_metadata) {
      metadata_file.write_raw_bytes(nameval.first.c_str(), nameval.first.size() + 1u);
      metadata_file.write_raw_bytes(nameval.second.c_str(), nameval.second.size() + 1u);
    }

    // Write out threadgroup size for compute shader
    metadata_file.start_new_record();
    if (maybe_threadgroup_size) {
      metadata_file.write_field(maybe_threadgroup_size.value()[0]);
      metadata_file.write_field(maybe_threadgroup_size.value()[1]);
      metadata_file.write_field(maybe_threadgroup_size.value()[2]);
    } else {
      metadata_file.write_field(0u);
      metadata_file.write_field(0u);
      metadata_file.write_field(0u);
    }

    metadata_file.finalize();
  }
#pragma endregion gen_output
  return 0;
}
