// Copyright 2020 the Toolman project authors. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#ifndef TOOLMAN_FIELD_H_
#define TOOLMAN_FIELD_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "src/stmt_info.h"
#include "src/type.h"

namespace toolman {

class Field final : public HasStmtInfo {
 public:
  template <typename S, typename SI>
  Field(S&& name, SI&& stmt_info)
      : name_(name),
        HasStmtInfo(std::forward<SI>(stmt_info)),
        optional_(false) {}

  template <typename S, typename SI>
  Field(S&& name, SI&& stmt_info, std::vector<std::string> comments)
      : name_(name),
        comments_(comments),
        HasStmtInfo(std::forward<SI>(stmt_info)),
        optional_(false) {}

  template <typename S, typename SI>
  Field(S&& name, std::shared_ptr<Type> type, bool optional, SI&& stmt_info,
        std::vector<std::string> comments)
      : type_(std::forward<S>(type)),
        name_(name),
        optional_(optional),
        comments_(comments),
        HasStmtInfo(std::forward<SI>(stmt_info)) {}

  [[nodiscard]] const std::string& get_name() const { return name_; }

  [[nodiscard]] const std::vector<std::string> get_comments() const {
    return comments_;
  }

  [[nodiscard]] const std::shared_ptr<Type>& get_type() const { return type_; }

  [[nodiscard]] bool is_optional() const { return optional_; }

  void set_optional(bool optional) { optional_ = optional; }

  void set_type(std::shared_ptr<Type> type) { type_ = std::move(type); }

  std::shared_ptr<Type> get_type() { return type_; }

 private:
  std::shared_ptr<Type> type_;
  std::string name_;
  std::vector<std::string> comments_;
  bool optional_;
};

}  // namespace toolman

#endif  // TOOLMAN_FIELD_H_
