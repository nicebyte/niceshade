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
  #pragma comment(lib, "ws2_32.lib")
  #include <winsock2.h>
  #include <io.h>
  #define PATH_SEPARATOR  "\\"
  #if defined(_WIN64)
  #define filelen(f) _filelengthi64(_fileno(f))
  #elif defined(_WIN32)
  #define filelen(f) _filelength(_fileno(f))
  #endif
#else
  #include <arpa/inet.h>
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

// Target API class.
enum class target_api {
  GL, METAL, VULKAN
};

// Device type that a target API runs on.
enum class target_platform_class {
  DONTCARE,
  DESKTOP,
  MOBILE
};

// Information about a compilation target.
struct target_info {
  target_api api; // API class.
  const char *file_ext; // Extension for the generated files.
  uint32_t version_maj; // Major version number.
  uint32_t version_min; // Minor version number.
  target_platform_class platform; // Deviice types that the target API runs on.
};

// Map of string identifiers to a target-specific information.
static const struct { const char *name;
                      target_info target; } TARGET_MAP[] = {
  {
    "gl430",
    {
      target_api::GL,
      "430.glsl",
      4u, 3u,
      target_platform_class::DESKTOP
    }
  },
  {
    "gles300",
    {
      target_api::GL,
      "300es.glsl",
      3u, 0u,
      target_platform_class::MOBILE
    }
  },
  {
    "gles310",
    {
      target_api::GL,
      "310es.glsl",
      3u, 1u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl10",
    {
      target_api::METAL,
      "10.msl",
      1u, 0u,
      target_platform_class::DESKTOP
    }
  },
  {
    "msl11",
    {
      target_api::METAL,
      "11.msl",
      1u, 1u,
      target_platform_class::DESKTOP
    }
  },
  {
    "msl12",
    {
      target_api::METAL,
      "12.msl",
      1u, 2u,
      target_platform_class::DESKTOP
    }
  },
  {
    "msl20",
    {
      target_api::METAL,
      "20.msl",
      2u, 0u,
      target_platform_class::DESKTOP
    }
  },
  {
    "msl10ios",
    {
      target_api::METAL,
      "10ios.msl",
      1u, 0u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl11ios",
    {
      target_api::METAL,
      "11ios.msl",
      1u, 1u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl12ios",
    {
      target_api::METAL,
      "12ios.msl",
      1u, 2u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl20ios",
    {
      target_api::METAL,
      "20ios.msl",
      2u, 0u,
      target_platform_class::MOBILE
    }
  },
  {
    "spv",
    {
      target_api::VULKAN,
      "spv",
      0u, 0u,
      target_platform_class::DONTCARE
    }
  }
};
constexpr uint32_t TARGET_COUNT = sizeof(TARGET_MAP)/sizeof(TARGET_MAP[0]);

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

// Indicates the type of resource accessed by a programmable shader stage.
enum class descriptor_type {
  UNIFORM_BUFFER = 0x00,
  STORAGE_BUFFER = 0x01,
  LOADSTORE_IMAGE = 0x02,
  TEXTURE = 0x03,
  SAMPLER = 0x04,
  TEXTURE_AND_SAMPLER = 0x05,
  INVALID = 0x06
};

// Indicates which programmable shader stage a descriptor is visible from.
enum stage_mask_bit {
  STAGE_MASK_VERTEX = 0x01,
  STAGE_MASK_FRAGMENT = 0x10
};

// Descriptor data.
struct descriptor {
  uint32_t slot; // A descriptor's binding within its set.
  descriptor_type type = descriptor_type::INVALID; // Type of resorce accessed.
  uint32_t stage_mask = 0u; // Which stages the descriptor is used from.
  std::string name; // The name used to refer to it in the source code.
};

// A dictionary that stores entries in a linear container.
template <class K, class V>
class linear_dict {
  using container_type = std::vector<std::pair<K, V>>;
public:
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

  iterator begin() { return data_.begin(); }
  iterator end() { return data_.end(); }
  const_iterator begin() const { return data_.begin(); }
  const_iterator end() const { return data_.end(); }
  const_iterator cbegin() const { return data_.cbegin(); }
  const_iterator cend() const { return data_.cend(); }
  iterator find(const K &k) {
    return std::find_if(
        begin(), end(),
        [&k](const std::pair<K, V> &p) { return k == p.first; });
  }
  const_iterator find(const K &k) const {
    return std::find_if(
        cbegin(), cend(),
        [&k](const std::pair<K, V> &p) { return k == p.first; });
  }

  V& operator[](const K &k) {
    auto it = find(k);
    if (it == end()) {
      it = data_.insert(data_.end(), std::make_pair(k, V()));
    }
    return it->second;
  }

  size_t size() const { return data_.size(); }

private:
  std::vector<std::pair<K, V>> data_;
};

using descriptor_set_layout = linear_dict<uint32_t, descriptor>;

// Stores information about shader resources accessed by a technique.
class resource_layout {
public:
  void add_resources(const std::vector<spirv_cross::Resource> &resources,
                     const spirv_cross::Compiler &refl,
                     descriptor_type resource_type,
                     stage_mask_bit smb) {
    for (const auto &r : resources) {
      uint32_t set_id =
          refl.get_decoration(r.id, spv::DecorationDescriptorSet);
	    uint32_t binding_id = refl.get_decoration(r.id, spv::DecorationBinding);
      max_set_ = max_set_ < set_id ? set_id : max_set_;
      descriptor_set &set = sets_[set_id];
      set.slot = set_id;
      descriptor &desc = set.layout[binding_id];
      if (desc.type != descriptor_type::INVALID &&
          desc.type != resource_type) {
        fprintf(stderr, "Attempt to assign a descriptor of different type to "
                        "slot %d in set %d which is already occupied by "
                        "\"%s\"\n", binding_id, set_id, desc.name.c_str());
        exit(1);
      }
      if (desc.type != descriptor_type::INVALID &&
          r.name != desc.name) {
        fprintf(stderr, "Assigning different names "
                        "(\"%s\" and \"%s\")  to descriptor at slot %d in set "
                        "%d.\n", desc.name.c_str(), r.name.c_str(),
                        binding_id, set_id);
        exit(1);
      }
      desc.slot = binding_id;
      desc.type = resource_type;
      desc.stage_mask |= smb;
      desc.name = r.name;
      nres_++;
    }
  }

  uint32_t set_count() const { return max_set_ + 1; }
  uint32_t res_count() const { return nres_; }

  const descriptor_set_layout& resource_layout::set(uint32_t set_id) const {
    static const descriptor_set_layout empty_layout;
    auto it = sets_.find(set_id);
    if (it != sets_.cend()) {
      return it->second.layout;
    } else {
      return empty_layout;
    }
  }

private:
  struct descriptor_set {
    uint32_t slot = 0u;
    descriptor_set_layout layout;
  };
  linear_dict<uint32_t, descriptor_set> sets_;
  uint32_t max_set_ = 0u;
  uint32_t nres_ = 0u;
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
    spv_cross->build_combined_image_samplers();
    const std::vector<spirv_cross::CombinedImageSampler> &cis =
        spv_cross->get_combined_image_samplers();
    const uint32_t cis_binding_offset =
        spv_cross->get_shader_resources().sampled_images.size();
    for (uint32_t cis_idx = 0u; cis_idx < cis.size(); ++cis_idx) {
      const spirv_cross::CombinedImageSampler &remap = cis[cis_idx];
      spv_cross->set_name(remap.combined_id,
                          spv_cross->get_name(remap.image_id) + "_" +
                          spv_cross->get_name(remap.sampler_id));
      spv_cross->set_decoration(remap.combined_id, spv::DecorationBinding,
                                cis_binding_offset + cis_idx);
    }
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

// Writes a 32-bit integer in network byte order to the given file.
void write_network_word(uint32_t word, FILE *f) {
  uint32_t nbo = htonl(word);
  fwrite(&nbo, sizeof(uint32_t), 1u, f);
}

// A mapping from separate image or sampler resources to combined image/sampler
// resources autogenerated by SPIRV-Cross. Needed to emulate separate images
// and samplers on platforms that do not support them.
class separate_to_combined_map {
public:
  void add_resource(uint32_t separate_id,
                    uint32_t combined_id,
                    const spirv_cross::Compiler &compiler) {
    uint32_t set_id = compiler.get_decoration(separate_id,
                                              spv::DecorationDescriptorSet);
    uint32_t binding_id = compiler.get_decoration(separate_id,
                                                  spv::DecorationBinding);
    uint32_t combined_binding_id =
        compiler.get_decoration(combined_id, spv::DecorationBinding);
    map_[set_and_binding{set_id, binding_id}][combined_binding_id] = true;
  }

  void serialize(FILE *metadata_file) const {
    write_network_word((uint32_t)map_.size(), metadata_file);
    for (const auto &entry : map_) {
      const set_and_binding &sb = entry.first;
      const linear_dict<uint32_t, bool> &combined_image_samplers =
          entry.second;
      write_network_word(sb.set, metadata_file);
      write_network_word(sb.binding, metadata_file);
      write_network_word((uint32_t)combined_image_samplers.size(),
                          metadata_file);
      for (const auto &c : combined_image_samplers) {
        write_network_word(c.first, metadata_file);
      }
    }
  }

private:
  struct set_and_binding {
    uint32_t set;
    uint32_t binding;
    bool operator==(const set_and_binding &rhs) const {
      return set == rhs.set && binding == rhs.binding;
    }
  };
  linear_dict<set_and_binding, linear_dict<uint32_t, bool>> map_;
};

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
      resource_layout res_layout;
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
        FILE *metadata_file = fopen(metadata_file_path.c_str(), "wb");
        if (metadata_file == nullptr) {
          fprintf(stderr, "Error opening output file %s\n",
                  metadata_file_path.c_str());
          exit(1);
        }
        write_network_word(0xdeadbeef, metadata_file);
        write_network_word(res_layout.set_count(), metadata_file);
        for (uint32_t set = 0u; set < res_layout.set_count(); ++set) {
          const auto &ds = res_layout.set(set);
          write_network_word(set, metadata_file);
          write_network_word((uint32_t)ds.size(), metadata_file);
          for (const auto &d : ds) {
            write_network_word(d.second.slot, metadata_file);
            write_network_word((uint32_t)d.second.type, metadata_file);
            write_network_word(d.second.stage_mask, metadata_file);
          }
        }
        images_to_cis.serialize(metadata_file);
        samplers_to_cis.serialize(metadata_file);
        write_network_word((uint32_t)tech.additional_metadata.size(),
                            metadata_file);
        for (const auto &nameval : tech.additional_metadata) {
          fwrite(nameval.first.c_str(), 1u, nameval.first.size() + 1u,
                 metadata_file);
          fwrite(nameval.second.c_str(), 1u, nameval.second.size() + 1u,
                 metadata_file);
        }
      }
    }
    generate_pipeline_metadata = false;
  }
  return 0;
}