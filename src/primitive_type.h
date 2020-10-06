// Copyright 2020 the Toolman project authors. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#ifndef TOOLMAN_PRIMITIVE_TYPE_H_
#define TOOLMAN_PRIMITIVE_TYPE_H_

#include <string>
#include <utility>

#include "src/type.h"

namespace toolman {
class PrimitiveType final : public Type {
 public:
    // Enumeration of toolman primitive types
    enum class TypeKind : char {
        Bool,
        I32,
        U32,
        I64,
        U64,
        Float,
        String,
    };

    PrimitiveType(const std::string &name,
                  TypeKind type_kind,
                  unsigned int line_no,
                  unsigned int column_no)
            : Type(name, line_no, column_no), type_kind_(type_kind) {}

    PrimitiveType(std::string &&name,
                  TypeKind type_kind,
                  unsigned int line_no,
                  unsigned int column_no)
            : Type(std::move(name), line_no, column_no), type_kind_(type_kind) {}

    [[nodiscard]] bool is_primitive() const override { return true; }

    [[nodiscard]] bool is_bool() const { return type_kind_ == TypeKind::Bool; }

    [[nodiscard]] bool is_i32() const { return type_kind_ == TypeKind::I32; }

    [[nodiscard]] bool is_u32() const { return type_kind_ == TypeKind::U32; }

    [[nodiscard]] bool is_i64() const { return type_kind_ == TypeKind::I64; }

    [[nodiscard]] bool is_u64() const { return type_kind_ == TypeKind::U64; }

    [[nodiscard]] bool is_float() const
        { return type_kind_ == TypeKind::Float; }

    [[nodiscard]] bool is_string() const
        { return type_kind_ == TypeKind::String; }

    bool operator==(const PrimitiveType &rhs) const {
        return type_kind_ == rhs.type_kind_;
    }

 private:
    TypeKind type_kind_;
};
}  // namespace toolman
#endif  // TOOLMAN_PRIMITIVE_TYPE_H_
