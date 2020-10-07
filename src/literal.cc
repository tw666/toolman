// Copyright 2020 the Toolman project authors. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "src/literal.h"
#include "src/error.h"

namespace toolman {

template <typename KL, typename VL>
void MapLiteral<KL, VL>::insert(std::pair<KL, VL> kv_pair) {
  if (type_->get_key_type() != kv_pair.first.get_type()) {
    throw LiteralElementTypeMismatchError(
        "map key mismatched types. expected `" +
            type_->get_key_type()->to_string() + "`, found `" +
            kv_pair.first.get_type()->to_string() + "`",
        kv_pair.first.get_stmt_info());
  }

  if (type_->get_value_type() != kv_pair.second.get_type()) {
    throw LiteralElementTypeMismatchError(
        "map value mismatched types. expected `" +
            type_->get_value_type()->to_string() + "`, found `" +
            kv_pair.second.get_type()->to_string() + "`",
        kv_pair.second.get_stmt_info());
  }
  value_.emplace(kv_pair);
}

template <typename VL>
void ListLiteral<VL>::insert(VL&& value) {
  if (type_->get_elem_type() != value.get_value()) {
    throw LiteralElementTypeMismatchError(
        "list mismatched types. expected `" +
            type_->get_elem_type()->to_string() + "`, found `" +
            value.get_type()->to_string() + "`",
        value.get_stmt_info());
  }
  value_.emplace(std::forward<VL>(value));
}

}  // namespace toolman
