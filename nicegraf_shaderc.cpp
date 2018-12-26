/**
Copyright © 2018 nicegraf contributors
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
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

#define _CRT_SECURE_NO_WARNINGS

#include "linear_dict.h"
#include "metadata_parser.h"
#include "pipeline_layout.h"
#include "pipeline_metadata_file.h"
#include "separate_to_combined_map.h"
#include "target.h"
#include "shaderc/shaderc.hpp"
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

// Platform-specific stuff.
#if defined(_WIN32) || defined(_WIN64)
  #include <io.h>
  #define PATH_SEPARATOR  "\\"
  #if defined(_WIN64)
  #define filelen(f) _filelengthi64(_fileno(f))
  #elif defined(_WIN32)
  #define filelen(f) _filelength(_fileno(f))
  #endif
#else
  #define PATH_SEPARATOR "/"
  #include <sys/stat.h>
  size_t filelen(FILE *f) {
    struct stat statbuf;
    fstat(fileno(f), &statbuf);
    return statbuf.st_size;
  }
#endif

const char *USAGE = R"RAW(
Usage: ngf_shaderc <input file name> [options]

Compiles HLSL shaders for multiple different targets.

Options:

  -O <path> - Folder to store output files in. Default is the current working
    directory.
  
  -D <name>=<value> - add a preprocessor macro definition `name' with the value
      `value' to all shaders generated from the given input file.

  -t <target> - Generate shaders for the given target.  Accepted values are:
      * gl430;
      * gles310, gles300;
      * msl10, msl11, msl12, msl20;
      * msl10ios, msl11ios, msl12ios, msl20ios;
      * spv 
    If the option is encountered multiple times, shaders for all of the
    mentioned targets will be generated.
)RAW";

// Stores a sequence of preprocessor definitions.
using define_container = std::vector<std::pair<std::string, std::string>>;

// Adds the preprocessor definitions from the given container to the
// shaderc compile options.
void add_defines_from_container(shaderc::CompileOptions &options,
                                const define_container &container) {
  for (const auto &name_value_pair : container) {
    options.AddMacroDefinition(name_value_pair.first, name_value_pair.second);
  }
}

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

// Technique description.
struct technique {
  struct entry_point {
    shaderc_shader_kind kind;
    std::string name;
  };
  std::string name;
  define_container defines;
  std::vector<entry_point> entry_points;
  std::vector<std::pair<std::string, std::string>> additional_metadata;
};

// Create an instance of SPIRV-Cross compiler for a given target.
std::unique_ptr<spirv_cross::Compiler> create_cross_compiler(
    const uint32_t *spv_data, uint32_t spv_data_size, const target_info &ti) {
  switch(ti.api) {
  case target_api::GL: {
    auto spv_cross = std::make_unique<spirv_cross::CompilerGLSL>(
       spv_data, spv_data_size);
    spirv_cross::CompilerGLSL::Options opts;
    opts.version = ti.version_maj * 100u + ti.version_min * 10u;
    opts.separate_shader_objects = true;
    opts.es = (ti.platform == target_platform_class::MOBILE);
    spv_cross->set_common_options(opts);
    return spv_cross;
    break;
  }
  case target_api::VULKAN: {
    auto spv_cross =
        std::make_unique<spirv_cross::CompilerReflection>(spv_data,
                                                          spv_data_size);
    return spv_cross;
    break;
  }
  case target_api::METAL: {
    auto spv_cross = std::make_unique<spirv_cross::CompilerMSL>(spv_data,
                                                                spv_data_size);
    spirv_cross::CompilerMSL::Options opts;
    opts.set_msl_version(ti.version_maj, ti.version_min);
    const bool ios = ti.platform == target_platform_class::MOBILE;
    opts.platform = ios ? spirv_cross::CompilerMSL::Options::iOS
                        : spirv_cross::CompilerMSL::Options::macOS;
    spv_cross->set_msl_options(opts);
    return spv_cross;
    break;
  }
  default: assert(false);
  }
  return nullptr;
}

#define IS_IDENT(c) (isalnum(c) || c == '_')
#define IS_TAB_SPACE(c) (c == ' '  || c == '\t')

// Reports a technique preprocessor error and exits.
void report_technique_parser_error(uint32_t line_num,
                                   const char *format, ...) {
  va_list varargs;
  va_start(varargs, format);
  fprintf(stderr, "line %d: ", line_num);
  vfprintf(stderr, format, varargs);
  fprintf(stderr, "\n");
  exit(1);
}

// Reads the contents of a file into an std::string.
std::string read_file(const char *path) {
  FILE *input_file = fopen(path, "rb");
  if (input_file == nullptr) {
    fprintf(stderr, "Failed to open file %s\n", path);
    exit(1);
  }
  size_t len = filelen(input_file);
  std::string contents;
  contents.reserve(len + 1u);
  contents.resize(len);
  size_t read_bytes = fread(&contents[0], 1u, len, input_file);
  if (read_bytes != len) {
    fprintf(stderr, "Failed to read file %s\n", path);
    exit(1);
  }
  fclose(input_file);
  return contents;
}

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
    data->content =read_file(file_name);
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

int main(int argc, const char *argv[]) {
  if (argc <= 1) { // Display help if invoked with no arguments.
    printf("%s\n", USAGE);
    exit(0);
  }

  // Process command line arguments.
  const std::string input_file_path { argv[1] }; // input file name.
  std::string out_folder = ".";
  define_container global_defines;
  std::vector<target_info> targets;
  for (uint32_t o = 2u; o < (uint32_t)argc; o += 2u) { // process options.
    const std::string option_name { argv[o] };
    if (o + 1u >= (uint32_t)argc) {
      fprintf(stderr, "Expected an option value after %s\n", argv[o]);
      exit(1);
    }
    const std::string option_value { argv[o + 1u] };
    if ("-D" == option_name) { // Additional #define
      size_t eq_idx = option_value.find_first_of('=');
      const std::string name = option_value.substr(0, eq_idx);
      const std::string value =
          eq_idx != std::string::npos && eq_idx < option_value.size() - 1u
              ? option_value.substr(eq_idx + 1u) : "";
      global_defines.emplace_back(name, value);
    } else if ("-t" == option_name) { // Target to generate code for.
      const auto *t = std::find_if(TARGET_MAP, TARGET_MAP + TARGET_COUNT,
                                   [&option_value](const auto &x) {
                                     return option_value == x.name;
                                   });
      if (t == TARGET_MAP + TARGET_COUNT) {
        fprintf(stderr, "Unknown target \"%s\"\n", option_value.c_str());
        exit(1);
      }
      targets.push_back(t->target);
    } else if ("-O" == option_name) { // Output folder.
      out_folder = option_value;
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

  // Load the input file.
  std::string input_source = read_file(input_file_path.c_str());
  input_source.push_back('\n');

  // Look for and parse technique directives in the code.
  uint32_t last_four_chars = 0u;
  uint32_t line_num = 1u;
  const uint32_t technique_prefix = 0x2f2f543a; // `//T:'
  technique_parser_state state = technique_parser_state::LOOKING_FOR_PREFIX;
  std::string parameter_name, entry_point_name, nameval_name,
              nameval_value;
  std::vector<technique> techniques;
  bool have_vertex_stage = false;
  for (uint32_t c_idx = 0u; c_idx < input_source.size(); ++c_idx) {
    char c = input_source[c_idx];
    // Collapse windows line endings into '\n'.
    if (c == '\r' && (c_idx == input_source.size() - 1u ||
                      input_source[c_idx + 1u] != '\n')) {
      fprintf(stderr, "Stray carriage return in input on line %d\n", line_num);
      exit(1);
    } else if (c == '\r') {
      continue;
    }
    if (!isspace(c)) {
      last_four_chars <<= 8u;
      last_four_chars |= (uint32_t)c;
    }
    switch(state) {
    case technique_parser_state::LOOKING_FOR_PREFIX:
      if (last_four_chars == technique_prefix) {
        state = technique_parser_state::LOOKING_FOR_NAME;
        techniques.emplace_back();
        have_vertex_stage = false;
      }
      break;
    case technique_parser_state::LOOKING_FOR_NAME:
      if (IS_IDENT(c)) {
        state = technique_parser_state::PARSING_NAME;
        techniques.back().name.push_back(c);
      } else if (!IS_TAB_SPACE(c)) {
        report_technique_parser_error(
            line_num, "unexpected character [%c] in technique name", c);
      }
      break;
    case  technique_parser_state::PARSING_NAME:
      if (IS_IDENT(c)) {
        techniques.back().name.push_back(c);
      } else if (IS_TAB_SPACE(c)) {
        state = technique_parser_state::LOOKING_FOR_PARAMETER_NAME;
      } else {
        report_technique_parser_error(
            line_num, "unexpected character [%c] in technique name", c);
      }
      break;
    case technique_parser_state::LOOKING_FOR_PARAMETER_NAME:
      if (IS_IDENT(c)) {
        state = technique_parser_state::PARSING_PARAMETER_NAME;
        parameter_name.clear();
        parameter_name.push_back(c);
      } else if (c == '\n') {
        state = technique_parser_state::FINALIZING_TECHNIQUE;
      } else if (!IS_TAB_SPACE(c)) {
        report_technique_parser_error(
            line_num, "unexpected character [%c] in technique param name", c);
      }
      break;
    case technique_parser_state::PARSING_PARAMETER_NAME:
      if (IS_IDENT(c)) {
        parameter_name.push_back(c);
      } else if (c == ':') {
        if (parameter_name == "define" || parameter_name == "meta") {
          state = technique_parser_state::PARSING_NAMEVAL_NAME;
          nameval_name.clear();
        } else if (parameter_name == "vs" || parameter_name == "ps") {
          state = technique_parser_state::PARSING_ENTRYPOINT_NAME;
          entry_point_name.clear();
        } else {
          report_technique_parser_error(line_num, "unknown parameter [%s]", 
                                        parameter_name.c_str());
        }
      } else {
        report_technique_parser_error(
            line_num, "unexpected character [%c] in technique param name", c);
      }
      break;
    case technique_parser_state::PARSING_ENTRYPOINT_NAME:
      if (IS_IDENT(c)) {
        entry_point_name.push_back(c);
      } else if (IS_TAB_SPACE(c) || c == '\n') {
        technique::entry_point ep {
          parameter_name == "vs"
              ? shaderc_vertex_shader
              : shaderc_fragment_shader,
          entry_point_name
        };
        for (const auto &prev_ep : techniques.back().entry_points) {
          if (prev_ep.kind == ep.kind) {
            report_technique_parser_error(line_num,
                                          "duplicate entry point %s:%s",
                                          parameter_name.c_str(),
                                          ep.name.c_str());
          }
        }
        techniques.back().entry_points.emplace_back(ep);
        have_vertex_stage |= (parameter_name == "vs");
        state =
            c != '\n'
            ? technique_parser_state::LOOKING_FOR_PARAMETER_NAME
            : technique_parser_state::FINALIZING_TECHNIQUE;
      } else {
        report_technique_parser_error(
            line_num, "unexpected character [%c] in entry point name", c);
      }
      break;
    case technique_parser_state::PARSING_NAMEVAL_NAME:
      if (IS_IDENT(c)) {
        nameval_name.push_back(c);
      } else if (c == '=') {
        state = technique_parser_state::PARSING_NAMEVAL_VALUE;
        nameval_value.clear();
      } else {
        report_technique_parser_error(
            line_num, "unexpected character [%c] in definition name", c);
      }
      break;
    case technique_parser_state::PARSING_NAMEVAL_VALUE:
      if(!IS_TAB_SPACE(c) && c != '\n') {
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
        report_technique_parser_error(
            line_num, "technique needs to define at least a vertex stage");
      }
      state = technique_parser_state::LOOKING_FOR_PREFIX;
      break;
    }
    if (c == '\n') ++line_num;
  }

  // Obtain SPIR-V.
  std::vector<shaderc::SpvCompilationResult> spv_results;
  shaderc::Compiler compiler;
  for (const technique &tech : techniques) {
    for (const technique::entry_point ep : tech.entry_points) {
      // Set compile options.
      shaderc::CompileOptions shaderc_opts;
      add_defines_from_container(shaderc_opts, global_defines);
      add_defines_from_container(shaderc_opts, tech.defines);
      shaderc_opts.SetAutoBindUniforms(true);
      shaderc_opts.SetAutoMapLocations(true);
      shaderc_opts.SetSourceLanguage(shaderc_source_language_hlsl);
      shaderc_opts.SetIncluder(std::make_unique<includer>());
      shaderc_opts.SetWarningsAsErrors();
      // Produce SPIR-V.
      spv_results.emplace_back(
        compiler.CompileGlslToSpv(input_source,
                                  ep.kind,
                                  input_file_path.c_str(),
                                  ep.name.c_str(),
                                  shaderc_opts));
      if (spv_results.back().GetNumErrors() > 0u) {
        fprintf(stderr, "%s", spv_results.back().GetErrorMessage().c_str());
        exit(1);
      } 
    }
  }

  // Generate output.
  bool generate_pipeline_metadata = true;
  for (const target_info &t : targets) {
    uint32_t spv_idx = 0u;
    for (const technique &tech : techniques) {
      pipeline_layout res_layout;
      separate_to_combined_map images_to_cis, samplers_to_cis;
      for (const technique::entry_point ep : tech.entry_points) {
        
        const shaderc::SpvCompilationResult &spv_result =
            spv_results[spv_idx++];
        std::string out;
        std::string out_file_path =
            out_folder + PATH_SEPARATOR + tech.name + 
            (ep.kind == shaderc_vertex_shader ? ".vs." : ".ps.")
            + t.file_ext;
        std::unique_ptr<spirv_cross::Compiler> compiler =
            create_cross_compiler(
                spv_result.cbegin(),
                (uint32_t)(spv_result.cend() - spv_result.cbegin()), t);
        compiler->build_combined_image_samplers();
        const std::vector<spirv_cross::CombinedImageSampler> &cis =
            compiler->get_combined_image_samplers();
        const uint32_t cis_binding_offset =
            compiler->get_shader_resources().sampled_images.size();
        for (uint32_t cis_idx = 0u; cis_idx < cis.size(); ++cis_idx) {
          const spirv_cross::CombinedImageSampler &remap = cis[cis_idx];
          compiler->set_name(remap.combined_id,
                              compiler->get_name(remap.image_id) + "_" +
                              compiler->get_name(remap.sampler_id));
          compiler->set_decoration(remap.combined_id, spv::DecorationBinding,
                                    cis_binding_offset + cis_idx);
        }
        if (generate_pipeline_metadata) {
          spirv_cross::ShaderResources resources =
              compiler->get_shader_resources();
          stage_mask_bit smb =
              ep.kind == shaderc_vertex_shader
                           ? STAGE_MASK_VERTEX
                           : STAGE_MASK_FRAGMENT;
          res_layout.add_resources(resources.uniform_buffers, *compiler,
                                   descriptor_type::UNIFORM_BUFFER, smb);
          res_layout.add_resources(resources.storage_buffers, *compiler,
                                   descriptor_type::STORAGE_BUFFER, smb);
          res_layout.add_resources(resources.sampled_images, *compiler,
                                   descriptor_type::TEXTURE_AND_SAMPLER, smb);
          res_layout.add_resources(resources.separate_samplers, *compiler,
                                   descriptor_type::SAMPLER, smb);
          res_layout.add_resources(resources.separate_images, *compiler,
                                   descriptor_type::TEXTURE, smb);
          for (const spirv_cross::CombinedImageSampler &cis:
                   compiler->get_combined_image_samplers()) {
            images_to_cis.add_resource(cis.image_id, cis.combined_id,
                                       *compiler);
            samplers_to_cis.add_resource(cis.sampler_id, cis.combined_id,
                                         *compiler);
          }
        }
        FILE *out_file = fopen(out_file_path.c_str(), "wb");
        if (out_file == nullptr) {
          fprintf(stderr, "Failed to open output file %s\n",
                  out_file_path.c_str());
          exit(1);
        }
        if (t.api != target_api::VULKAN) {
          out = compiler->compile();
          fwrite(&out[0], sizeof(uint8_t), out.length(), out_file);
        } else {
          fwrite(spv_result.cbegin(), sizeof(uint32_t),
                 spv_result.cend() - spv_result.cbegin(), out_file);
        }
        fclose(out_file);
      }

      // Write out the .pipeline file for the current technique.
      if (generate_pipeline_metadata) {
        std::string metadata_file_path =
            out_folder + PATH_SEPARATOR + tech.name + ".pipeline";
        pipeline_metadata_file metadata_file(metadata_file_path.c_str());

        // Write out the pipeline layout record.
        metadata_file.start_new_record();
        metadata_file.write_field(res_layout.set_count());
        for (uint32_t set = 0u; set < res_layout.set_count(); ++set) {
          const auto &ds = res_layout.set(set);
          metadata_file.write_field((uint32_t)ds.size());
          for (const auto &d : ds) {
            metadata_file.write_field(d.second.slot);
            metadata_file.write_field((uint32_t)d.second.type);
            metadata_file.write_field(d.second.stage_mask);
          }
        }

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