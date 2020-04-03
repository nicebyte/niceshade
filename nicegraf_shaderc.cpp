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
#include "spirv_glsl.hpp"
#include "spirv_msl.hpp"
#include "spirv_reflect.hpp"

#include <ctype.h>
#include <memory>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

const char *USAGE = R"RAW(
Usage: ngf_shaderc <input file name> [options]

Compiles HLSL shaders for multiple different targets.

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
    mentioned targets will be generated.

  -o <level> - Set SPIR-V optimization level. `1` will apply the same optimizations as
   `spirv-opt -O`. `0` will turn off all optimizations. The default value is `0`. SPIR-V
   optimizations have an effect on the output for non-SPIR-V targets. Enabling them will
   result in less readable output that doesn't match the input HLSL as closely. Using
   code generated from optimized SPIR-V may result in performance wins on some platforms,
   but, as always, measure.

  -m <version> - HLSL shader model version to use. Valid values are: 6_0, 6_1, 6_2, 6_3,
   6_4, 6_5, 6_6. Default is 6_0.

  -h <path> - Path (relative to the output folder) for the generated
      header file with descriptor binding and set IDs. If not specified, no
      header file will be generated.

  -n <identifier> - Namespace for the generated shader file. If not specified,
     global namespace is used.

  -D <name>=<value> - Add a preprocessor definition `name` with the value `value` to
     techniques.
)RAW";

int main(int argc, const char *argv[]) {
  if (argc <= 1) { // Display help if invoked with no arguments.
    printf("%s\n", USAGE);
    exit(0);
  }

  // Process command line arguments.
  const std::string input_file_path { argv[1] };
  std::string out_folder = ".";
  std::string header_path = "";
  std::string header_namespace = "";
  std::string shader_model = "6_0";
  std::vector<const target_info*> targets;
  define_container global_macro_definitions;
  bool enable_spv_opt = false;

  for (uint32_t o = 2u; o < (uint32_t)argc; o += 2u) {
    const std::string option_name { argv[o] };
    if (o + 1u >= (uint32_t)argc) {
      fprintf(stderr, "Expected an option value after %s\n", argv[o]);
      exit(1);
    }
    const std::string option_value { argv[o + 1u] };
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
    } else if ("-o" == option_name) {
      if (option_value == "0") enable_spv_opt = false;
      else if (option_value == "1") enable_spv_opt = true;
      else {
        fprintf(stderr, "Unsupported SPIR-V optimization level \"%s\"\n",
                option_value.c_str());
        exit(1);
      }
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
  // Obtain SPIR-V.
  dxc_wrapper dxcompiler(shader_model, enable_spv_opt, exe_dir);
  std::vector<dxc_wrapper::result> spv_results;
  for (const technique &tech : techniques) {
    for (const technique::entry_point ep : tech.entry_points) {
      // Produce SPIR-V.
      spv_results.emplace_back(dxcompiler.compile_hlsl2spv(
          input_source.c_str(),
          input_source.size(),
          input_file_path.c_str(),
          ep,
          tech.defines));
      const dxc_wrapper::result &result = spv_results.back();
      if (result.HasDiagMessage()) {
        fprintf(stderr, "%s", result.diag_message.c_str());
      }
      if (!result.HasData()) {
        exit(1);
      }
    }
  }

  // Attempt to open header file.
  const bool generate_header = !header_path.empty();
  header_file_writer header_writer(out_folder, header_path, header_namespace);
  if (!header_path.empty() && !header_writer.is_open()) {
    fprintf(stderr, "Failed to open output file %s\n", header_writer.path());
    exit(1);
  }

  // Generate output.
  bool generate_pipeline_metadata = true;
  for (const target_info *target_info : targets) {
    uint32_t spv_idx = 0u;
    for (const technique &tech : techniques) {
      pipeline_layout res_layout;
      separate_to_combined_map images_to_cis, samplers_to_cis;
      for (const technique::entry_point ep : tech.entry_points) {
        std::vector<uint32_t> &spv_result =
            spv_results[spv_idx++].spirv_result;
        std::string out;
        std::string out_file_path =
            out_folder + PATH_SEPARATOR + tech.name + 
            (ep.kind == shader_kind::vertex ? ".vs." : ".ps.")
            + target_info->file_ext;

        // Create an instance of SPIRV-Cross compiler.
        std::unique_ptr<spirv_cross::Compiler> spv_cross_compiler;
        switch(target_info->api) {
        case target_api::GL: {
          auto gl_compiler = std::make_unique<spirv_cross::CompilerGLSL>(
             spv_result.data(), spv_result.size());
          spirv_cross::CompilerGLSL::Options opts;
          opts.version = target_info->version_maj * 100u + target_info->version_min * 10u;
          opts.separate_shader_objects = true;
          opts.es = (target_info->platform == target_platform_class::MOBILE);
          gl_compiler->set_common_options(opts);
          gl_compiler->build_dummy_sampler_for_combined_images();
          gl_compiler->build_combined_image_samplers();
          spv_cross_compiler = std::move(gl_compiler);
          break;
        }
        case target_api::VULKAN: {
          spv_cross_compiler =
              std::make_unique<spirv_cross::CompilerReflection>(spv_result.data(),
                                                                spv_result.size());
          break;
        }
        case target_api::METAL: {
          auto msl_compiler = std::make_unique<spirv_cross::CompilerMSL>(spv_result.data(),
                                                                      spv_result.size());
          spirv_cross::CompilerMSL::Options opts;
          opts.set_msl_version(target_info->version_maj, target_info->version_min);
          const bool ios = target_info->platform == target_platform_class::MOBILE;
          opts.platform = ios ? spirv_cross::CompilerMSL::Options::iOS
                              : spirv_cross::CompilerMSL::Options::macOS;
          opts.enable_decoration_binding = true;
          msl_compiler->set_msl_options(opts);
          spv_cross_compiler = std::move(msl_compiler);
          break;
        }
        default: assert(false);
        }

        spirv_cross::ShaderResources resources =
            spv_cross_compiler->get_shader_resources();
        const spirv_cross::SmallVector<spirv_cross::CombinedImageSampler> &cis =
            spv_cross_compiler->get_combined_image_samplers();
        for (uint32_t cis_idx = 0u; cis_idx < cis.size(); ++cis_idx) {
          const spirv_cross::CombinedImageSampler &remap = cis[cis_idx];
          spv_cross_compiler->set_name(
              remap.combined_id,
              spv_cross_compiler->get_name(remap.image_id) + "_" +
              spv_cross_compiler->get_name(remap.sampler_id));
          spv_cross_compiler->set_decoration(remap.combined_id,
                                             spv::DecorationBinding,
                                             cis_idx);
          spv_cross_compiler->set_decoration(remap.combined_id,
                                             spv::DecorationDescriptorSet,
                                             AUTOGEN_CIS_DESCRIPTOR_SET);
        }
        const bool do_remapping = target_info->api == target_api::GL
                                  || target_info->api == target_api::METAL;
        if (do_remapping || generate_pipeline_metadata) {
          for (const spirv_cross::CombinedImageSampler &cis:
                   spv_cross_compiler->get_combined_image_samplers()) {
            images_to_cis.add_resource(cis.image_id, cis.combined_id,
                                       *spv_cross_compiler);
            samplers_to_cis.add_resource(cis.sampler_id, cis.combined_id,
                                         *spv_cross_compiler);
          }
          const stage_mask_bit smb =
              ep.kind == shader_kind::vertex
                           ? STAGE_MASK_VERTEX
                           : STAGE_MASK_FRAGMENT;
          auto process_resources =
            [smb, do_remapping, &spv_cross_compiler, &res_layout](
              const spirv_cross::SmallVector<spirv_cross::Resource> &resources,
              descriptor_type dtype) {
              res_layout.process_resources(resources, dtype, smb,
                                           do_remapping, *spv_cross_compiler);
            };
          process_resources(resources.uniform_buffers,
                            descriptor_type::UNIFORM_BUFFER);
          process_resources(resources.storage_buffers,
                            descriptor_type::STORAGE_BUFFER);
          process_resources(resources.separate_samplers,
                            descriptor_type::SAMPLER);
          process_resources(resources.separate_images,
                            descriptor_type::TEXTURE);
        }
        FILE *out_file = fopen(out_file_path.c_str(), "wb");
        if (out_file == nullptr) {
          fprintf(stderr, "Failed to open output file %s\n",
                  out_file_path.c_str());
          exit(1);
        }
        if (target_info->api != target_api::VULKAN) {
          out = spv_cross_compiler->compile();
          fwrite(&out[0], sizeof(uint8_t), out.length(), out_file);
        } else {
          fwrite(spv_result.data(), sizeof(uint32_t),
                 spv_result.size(), out_file);
        }
        fclose(out_file);
      }

      // Write out the .pipeline file for the current technique.
      if (generate_pipeline_metadata) {
        header_writer.begin_technique(tech.name);
        std::string metadata_file_path =
            out_folder + PATH_SEPARATOR + tech.name + ".pipeline";
        pipeline_metadata_file metadata_file(metadata_file_path.c_str());

        // Write out the pipeline layout record.
        metadata_file.start_new_record();
        metadata_file.write_field(res_layout.set_count());
        for (uint32_t set = 0u; set < res_layout.set_count(); ++set) {
          const descriptor_set_layout &ds = res_layout.set(set);
          metadata_file.write_field((uint32_t)ds.size());
          for (const auto &d : ds) {
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
        for (const auto &nameval : tech.additional_metadata) {
          metadata_file.write_raw_bytes(nameval.first.c_str(),
                                        nameval.first.size() + 1u);
          metadata_file.write_raw_bytes(nameval.second.c_str(),
                                        nameval.second.size() + 1u);
        }
        metadata_file.finalize();
      }
    }
    generate_pipeline_metadata = false;
  }

  return 0;
}
