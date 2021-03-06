// Copyright 2020 the Toolman project authors. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#ifndef TOOLMAN_GENERATOR_H_
#define TOOLMAN_GENERATOR_H_

#include <memory>
#include <sstream>

#include "src/custom_type.h"
#include "src/document.h"

#define INDENT_1 "    "
#define INDENT INDENT_1
#define INDENT_2 INDENT_1 INDENT_1
#define INDENT_3 INDENT_2 INDENT_1
#define INDENT_4 INDENT_3 INDENT_1

#define NL "\n"
#define NL1 NL
#define NL2 NL1 NL1
#define NL3 NL2 NL1
#define NL4 NL3 NL1

namespace toolman::generator {

enum class TargetLanguage : char { GOLANG, TYPESCRIPT, JAVA };

TargetLanguage target_language_from_string(std::string target);

void generate(std::unique_ptr<Document> document, TargetLanguage targetLanguage,
              std::ostream& ostream);

class Generator {
 public:
  virtual ~Generator() = default;
  void generate(std::ostream& ostream,
                const std::unique_ptr<Document>& document);

 protected:
  virtual void before_generate_document(std::ostream& ostream,
                                        const Document* document) {}
  virtual void after_generate_document(std::ostream& ostream,
                                       const Document* document) {}
  virtual void before_generate_struct(std::ostream& ostream,
                                      const Document* document) {}
  virtual void after_generate_struct(std::ostream& ostream,
                                     const Document* document) {}
  virtual void before_generate_enum(std::ostream& ostream,
                                    const Document* document) {}
  virtual void after_generate_enum(std::ostream& ostream,
                                   const Document* document) {}

  [[nodiscard]] virtual std::string single_line_comment(
      std::string code) const = 0;

  virtual void generate_struct(
      std::ostream& ostream,
      const std::shared_ptr<StructType>& struct_type) = 0;
  virtual void generate_enum(std::ostream& ostream,
                             const std::shared_ptr<EnumType>& enum_type) = 0;
};

/**
 * Transforms a camel case string to an equivalent one separated by underscores
 * e.g. aMultiWord -> a_multi_word
 *      someName   -> some_name
 *      CamelCase  -> camel_case
 *      name       -> name
 *      Name       -> name
 */
std::string underscore(std::string in);

/**
 * Transforms a string with words separated by underscores to a camel case
 * equivalent
 * e.g.
 * a_multi_word -> aMultiWord
 * some_name    -> someName
 * name         -> name
 */
std::string camelcase(const std::string& in);

/**
 * Capitalization helpers
 */
std::string capitalize(std::string in);

std::string decapitalize(std::string in);

}  // namespace toolman::generator

#endif  // TOOLMAN_GENERATOR_H_
