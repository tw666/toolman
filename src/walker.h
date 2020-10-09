// Copyright 2020 the Toolman project authors. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#ifndef TOOLMAN_WALKER_H_
#define TOOLMAN_WALKER_H_

#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "ToolmanParserBaseListener.h"
#include "src/custom_type.h"
#include "src/document.h"
#include "src/error.h"
#include "src/field.h"
#include "src/list_type.h"
#include "src/literal.h"
#include "src/map_type.h"
#include "src/scope.h"

namespace toolman {

template <typename NODE, typename FILE>
StmtInfo get_stmt_info(NODE* node, FILE&& file) {
  auto id_start_token = node->getStart();
  return StmtInfo(
      {id_start_token->getLine(), node->getStop()->getLine()},
      {id_start_token->getStartIndex(), id_start_token->getStopIndex()},
      std::forward<FILE>(file));
}

// Declare phase
class DeclPhaseWalker final : public ToolmanParserBaseListener {
 public:
  explicit DeclPhaseWalker(std::shared_ptr<std::string> file)
      : type_scope_(std::make_shared<Scope>(Scope())), file_(std::move(file)) {}
  void enterStructDecl(ToolmanParser::StructDeclContext* node) override {
    decl_type<ToolmanParser::StructDeclContext, StructType>(node);
  }

  void enterEnumDecl(ToolmanParser::EnumDeclContext* node) override {
    decl_type<ToolmanParser::EnumDeclContext, EnumType>(node);
  }

  [[nodiscard]] const std::shared_ptr<Scope>& get_type_scope() const {
    return type_scope_;
  }

  [[nodiscard]] const std::vector<Error>& get_errors() const { return errors_; }

 private:
  std::shared_ptr<Scope> type_scope_;
  std::vector<Error> errors_;
  std::shared_ptr<std::string> file_;

  template <typename NODE, typename DECL_TYPE>
  void decl_type(NODE* node) {
    StmtInfo stmt_info = get_stmt_info(node->identifierName(), file_);
    if (auto search =
            type_scope_->lookup_type(node->identifierName()->getText());
        search.has_value()) {
      errors_.emplace_back(DuplicateDeclError(search.value(), stmt_info));
      return;
    } else {
      type_scope_->declare(std::make_shared<DECL_TYPE>(
          DECL_TYPE(node->identifierName()->getText(), stmt_info,
                    node->Pub() != nullptr)));
    }
  }
};

class FieldTypeBuilder {
 public:
  enum class TypeLocation : char { Top, ListElement, MapKey, MapValue };

  void set_type_location(TypeLocation type_location) {
    current_type_location_ = type_location;
  }

  void start_type(const std::shared_ptr<Type>& type) {
    if (!type_stack_.empty()) {
      if (type_stack_.top()->is_list()) {
        if (TypeLocation::ListElement == current_type_location_) {
          auto list_type =
              std::dynamic_pointer_cast<ListType>(type_stack_.top());
          list_type->set_elem_type(type);
        }

      } else if (type_stack_.top()->is_map()) {
        auto map_type = std::dynamic_pointer_cast<MapType>(type_stack_.top());

        if (TypeLocation::MapKey == current_type_location_) {
          // The key of the map must be a primitive type.
          if (!type->is_primitive()) {
            throw MapKeyTypeMustBePrimitiveError(type);
          } else {
            map_type->set_key_type(
                std::dynamic_pointer_cast<PrimitiveType>(type));
          }
        } else if (TypeLocation::MapValue == current_type_location_) {
          map_type->set_value_type(
              std::dynamic_pointer_cast<PrimitiveType>(type));
        }
      }
    }

    if (type->is_map() || type->is_list()) {
      type_stack_.push(type);
    } else {
      current_single_type_ = type;
    }
  }

  // If return value is not null-pointer
  // that means returned is current filed type
  std::shared_ptr<Type> end_map_or_list_type() {
    auto top = type_stack_.top();
    type_stack_.pop();
    if (type_stack_.empty()) {
      return top;
    }
    return std::shared_ptr<Type>(nullptr);
  }

  // Other types besides map and list.
  // If return value is not null-pointer
  // that means returned is current filed type
  std::shared_ptr<Type> end_single_type() {
    if (type_stack_.empty()) {
      return current_single_type_;
    }
    return std::shared_ptr<Type>(nullptr);
  }

 private:
  std::stack<std::shared_ptr<Type>> type_stack_;
  std::shared_ptr<Type> current_single_type_;
  TypeLocation current_type_location_ = TypeLocation::Top;
};

template <typename FIELD>
class StructTypeBuilder {
 public:
  StructTypeBuilder() : current_field_(std::nullopt) {}

  void start_field(FIELD field) {
    current_field_ = std::optional<FIELD>{field};
  }

  void set_current_field_type(std::shared_ptr<Type> type) {
    if (current_field_.has_value()) {
      current_field_.value().set_type(std::move(type));
    }
  }

  void set_current_filed_literal(std::unique_ptr<Literal> literal) {
    if (current_field_.has_value()) {
      current_field_.value().set_literal(literal);
    }
  }

  void end_field() { current_struct_type_->append_field(current_field_); }

  void start_struct_type(std::shared_ptr<StructType> struct_type) {
    current_struct_type_ = std::move(struct_type);
  }

  [[nodiscard]] std::shared_ptr<StructType> end_struct_type() const {
    return current_struct_type_;
  }

  std::shared_ptr<Type> get_current_field_type() {
    if (current_field_.has_value()) {
      return current_field_.value().get_type();
    }
    return std::shared_ptr<Type>(nullptr);
  }

 private:
  std::optional<FIELD> current_field_;
  std::shared_ptr<StructType> current_struct_type_;
};

class LiteralBuilder {
 public:
  enum class LiteralLocation : char { Top, ListElement, MapKey, MapValue };
  void start_literal(std::unique_ptr<Literal> literal) {
    if (!literal_stack_.empty()) {
      const auto& top = literal_stack_.top();
      if (top->is_list()) {
        if (LiteralLocation::ListElement == current_literal_location_) {
          auto list_literal = dynamic_cast<ListLiteral*>(top.get());
          list_literal->insert(literal);
        }

      } else if (top->is_map()) {
        if (LiteralLocation::MapKey == current_literal_location_) {
          // The map key must be a primitive type.
          if (!literal->is_primitive()) {
            // todo
            //                          throw
            //                          MapKeyTypeMustBePrimitiveError(type);
          } else {
            // In this branch the the `literal` must be `PrimitiveLiteral`.
            // the dynamic_cast Won't fail. so `literal.release()` may not leak
            // memory.
            current_map_key_literal_ = std::unique_ptr<PrimitiveLiteral>(
                dynamic_cast<PrimitiveLiteral*>(literal.release()));
            return;
          }
        } else if (LiteralLocation::MapValue == current_literal_location_) {
          if (current_map_key_literal_) {
            auto map_literal = dynamic_cast<MapLiteral*>(top.get());
            map_literal->insert(
                std::make_pair(std::unique_ptr<PrimitiveLiteral>(
                                   current_map_key_literal_.release()),
                               std::unique_ptr<Literal>(literal.release())));
          }
        }
      }
    }
    if (literal->is_map() || literal->is_list()) {
      literal_stack_.push(std::unique_ptr<Literal>(literal.release()));
    } else {
      current_single_literal_ = std::unique_ptr<Literal>(literal.release());
    }
  }

  void set_current_literal_location(LiteralLocation literal_location) {
    current_literal_location_ = literal_location;
  }

  // If return value is not null-pointer
  // that means returned is current filed init literal
  std::unique_ptr<Literal> end_map_or_list_literal() {
    auto top = std::unique_ptr<Literal>(literal_stack_.top().release());
    literal_stack_.pop();
    if (literal_stack_.empty()) {
      return top;
    }
    return std::unique_ptr<Literal>(nullptr);
  }

  // Other types besides map and list.
  // If return value is not null-pointer
  // that means returned is current filed init literal
  std::unique_ptr<Literal> end_single_literal() {
    if (literal_stack_.empty()) {
      return std::unique_ptr<Literal>(current_single_literal_.release());
    }
    return std::unique_ptr<Literal>(nullptr);
  }

  void set_current_literal_type(std::shared_ptr<Type> current_literal_type) {
    current_literal_type_ = current_literal_type;
  }

 private:
  std::stack<std::unique_ptr<Literal>> literal_stack_;
  std::stack<std::shared_ptr<Type>> type_stack_;
  std::unique_ptr<PrimitiveLiteral> current_map_key_literal_;
  std::unique_ptr<Literal> current_single_literal_;
  LiteralLocation current_literal_location_ = LiteralLocation::Top;
  std::shared_ptr<Type> current_literal_type_;
};

class RefPhaseWalker final : public ToolmanParserBaseListener {
 public:
  RefPhaseWalker(std::shared_ptr<Scope> type_scope,
                 std::shared_ptr<std::string> file)
      : type_scope_(std::move(type_scope)),
        file_(std::move(file)),
        struct_builder_() {}
  std::unique_ptr<Document> get_document() {
    return std::unique_ptr<Document>(document_.release());
  }

  [[nodiscard]] const std::vector<Error>& get_errors() const { return errors_; }

  void enterDocument(ToolmanParser::DocumentContext* node) override {
    document_ = std::make_unique<Document>();
  }

  void enterStructDecl(ToolmanParser::StructDeclContext* node) override {
    auto type_name = node->identifierName()->getText();
    auto search_opt =
        type_scope_->lookup_type(node->identifierName()->getText());
    if (!search_opt.has_value()) {
      // Logically, this situation will not happen
      throw std::runtime_error("The type name`" + type_name + "` not found.");
    }
    auto search = std::dynamic_pointer_cast<StructType>(search_opt.value());
    if (!search) {
      // Logically, this situation will not happen
      throw std::runtime_error("The type name`" + type_name + "` is " +
                               search->to_string());
    }
    struct_builder_.start_struct_type(search);
  }

  void enterStructField(ToolmanParser::StructFieldContext* node) override {
    struct_builder_.start_field(
        Field(node->identifierName()->getText(), get_stmt_info(node, file_)));
  }

  void enterFieldType(ToolmanParser::FieldTypeContext* node) override {
    field_type_builder_.set_type_location(FieldTypeBuilder::TypeLocation::Top);
  }

  void enterListType(ToolmanParser::ListTypeContext* node) override {
    field_type_builder_.start_type(
        std::make_shared<ListType>(ListType(get_stmt_info(node, file_))));
  }

  void exitListType(ToolmanParser::ListTypeContext*) override {
    if (auto type = field_type_builder_.end_map_or_list_type(); type) {
      struct_builder_.set_current_field_type(type);
    }
  }

  void enterListElementType(ToolmanParser::ListElementTypeContext*) override {
    field_type_builder_.set_type_location(
        FieldTypeBuilder::TypeLocation::ListElement);
  }

  void enterMapType(ToolmanParser::MapTypeContext* node) override {
    try {
      field_type_builder_.start_type(
          std::make_shared<MapType>(MapType(get_stmt_info(node, file_))));
    } catch (MapKeyTypeMustBePrimitiveError& e) {
      errors_.emplace_back(e);
    }
  }

  void exitMapType(ToolmanParser::MapTypeContext*) override {
    if (auto type = field_type_builder_.end_map_or_list_type(); type) {
      struct_builder_.set_current_field_type(type);
    }
  }

  void enterMapKeyType(ToolmanParser::MapKeyTypeContext*) override {
    field_type_builder_.set_type_location(
        FieldTypeBuilder::TypeLocation::MapKey);
  }
  void enterMapValueType(ToolmanParser::MapValueTypeContext*) override {
    field_type_builder_.set_type_location(
        FieldTypeBuilder::TypeLocation::MapKey);
  }

  void enterPrimitiveType(ToolmanParser::PrimitiveTypeContext* node) override {
    PrimitiveType::TypeKind type_kind;
    if (node->Bool() != nullptr) {
      type_kind = PrimitiveType::TypeKind::Bool;
    } else if (node->I32() != nullptr) {
      type_kind = PrimitiveType::TypeKind::I32;
    } else if (node->U32() != nullptr) {
      type_kind = PrimitiveType::TypeKind::U32;
    } else if (node->I64() != nullptr) {
      type_kind = PrimitiveType::TypeKind::I64;
    } else if (node->U64() != nullptr) {
      type_kind = PrimitiveType::TypeKind::U64;
    } else if (node->Float() != nullptr) {
      type_kind = PrimitiveType::TypeKind::Float;
    } else if (node->String() != nullptr) {
      type_kind = PrimitiveType::TypeKind::String;
    } else {
      type_kind = PrimitiveType::TypeKind::Any;
    }
    field_type_builder_.start_type(std::make_shared<PrimitiveType>(
        PrimitiveType(type_kind, get_stmt_info(node, file_))));
  }

  void exitPrimitiveType(ToolmanParser::PrimitiveTypeContext*) override {
    if (auto type = field_type_builder_.end_single_type(); type) {
      struct_builder_.set_current_field_type(type);
    }
  }

  void enterCustomTypeName(
      ToolmanParser::CustomTypeNameContext* node) override {
    auto custom_type =
        type_scope_->lookup_type(node->identifierName()->getText());
    if (!custom_type.has_value()) {
      errors_.emplace_back(CustomTypeNotFoundError(
          node->identifierName()->getText(), get_stmt_info(node, file_)));
      return;
    }
    field_type_builder_.start_type(custom_type.value());
  }

  void exitCustomTypeName(ToolmanParser::CustomTypeNameContext*) override {
    if (auto type = field_type_builder_.end_single_type(); type) {
      struct_builder_.set_current_field_type(type);
    }
  }

  void enterStructFieldInit(ToolmanParser::StructFieldInitContext*) override {
    literal_builder_.set_current_literal_location(
        LiteralBuilder::LiteralLocation::Top);
  }

  void enterStructFieldInitListLiteral(
      ToolmanParser::StructFieldInitListLiteralContext* node) override {
    auto current_type = struct_builder_.get_current_type();
    if (current_type && current_type->is_list()) {
      auto literal_pointer =
          new ListLiteral(std::dynamic_pointer_cast<ListType>(current_type),
                          get_stmt_info(node, file_));
      literal_builder_.start_literal(
          std::unique_ptr<Literal>(dynamic_cast<Literal*>(literal_pointer)));
    } else {
      // todo abnormal situation
    }
  }
  void exitStructFieldInitListLiteral(
      ToolmanParser::StructFieldInitListLiteralContext*) override {
    if (auto literal = literal_builder_.end_map_or_list_literal(); literal) {
      struct_builder_.set_current_filed_literal(
          std::unique_ptr<Literal>(literal.release()));
    }
  }

  void enterStructFieldInitMapLiteral(
      ToolmanParser::StructFieldInitListLiteralContext*) override {
    auto current_type = struct_builder_.get_current_type();
    if (current_type && current_type->is_map()) {
      auto literal_pointer =
          new MapLiteral(std::dynamic_pointer_cast<MapType>(current_type),
                         get_stmt_info(node, file_));
      literal_builder_.start_literal(
          std::unique_ptr<Literal>(dynamic_cast<Literal*>(literal_pointer)));
    } else {
      // todo abnormal situation
    }
  }

  void exitStructFieldInitMapLiteral(
      ToolmanParser::StructFieldInitMapLiteralContext*) override {
    if (auto literal = literal_builder_.end_map_or_list_literal(); literal) {
      struct_builder_.set_current_filed_literal(
          std::unique_ptr<Literal>(literal.release()));
    }
  }

  void enterStructFieldInitPrimitiveLiteral(
      ToolmanParser::StructFieldInitPrimitiveLiteralContext* node) override {
    auto current_field_type = struct_builder_.get_current_field_type();

    literal_builder_.start_literal();
  }

 private:
  std::unique_ptr<Document> document_;
  StructTypeBuilder<Field> struct_builder_;
  FieldTypeBuilder field_type_builder_;
  LiteralBuilder literal_builder_;
  std::shared_ptr<Scope> type_scope_;
  std::shared_ptr<std::string> file_;
  std::vector<Error> errors_;
};

}  // namespace toolman

#endif  // TOOLMAN_WALKER_H_
