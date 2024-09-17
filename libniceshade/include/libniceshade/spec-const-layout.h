#pragma once

#include "error.h"

#include <stdint.h>
#include <map>
#include <string>

namespace niceshade {

struct spec_const {
  uint32_t id;
  uint32_t type_id;  // TODO: use enum
};

using spec_const_layout = std::map<std::string, spec_const>;

class spec_const_layout_builder {
private:
  std::map<std::string, spec_const> layout_;

public:
  error add_spec_const(const std::string_view name, spec_const constant) {
    auto insert_result = layout_.insert(std::make_pair(name, constant));
    return (!insert_result.second && (insert_result.first->second.id != constant.id ||
                                      insert_result.first->second.type_id != constant.type_id))
               ? error {"Spec constant redefinition: ", name}
               : error {};
  }

  spec_const_layout build() { return std::move(layout_); }

};

}  // namespace niceshade