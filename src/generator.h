// Copyright 2020 the Toolman project authors. All rights reserved.
// Use of this source code is governed by a MIT license that can be
// found in the LICENSE file.

#ifndef TOOLMAN_GENERATOR_H_
#define TOOLMAN_GENERATOR_H_

#include <memory>

#include "src/custom_type.h"
#include "src/document.h"

#define INDENTATION "    "
#define NL "\n"

namespace toolman {

class Generator {
 public:
  virtual void before_generate_document(std::ostream& ostream,
                                        const Document* document) const {}
  virtual void after_generate_document(std::ostream& ostream,
                                       const Document* document) const {}
  virtual void before_generate_struct(std::ostream& ostream,
                                      const Document* document) const {}
  virtual void after_generate_struct(std::ostream& ostream,
                                     const Document* document) const {}
  virtual void before_generate_enum(std::ostream& ostream,
                                    const Document* document) const {}
  virtual void after_generate_enum(std::ostream& ostream,
                                   const Document* document) const {}

  virtual void generate_struct(
      std::ostream& ostream,
      const std::shared_ptr<StructType>& struct_type) const = 0;
  virtual void generate_enum(
      std::ostream& ostream,
      const std::shared_ptr<EnumType>& enum_type) const = 0;

  void generate(std::ostream& ostream,
                const std::unique_ptr<Document>& document) const {
    before_generate_document(ostream, document.get());
    before_generate_struct(ostream, document.get());
    for (const auto& struct_type : document->get_struct_types()) {
      generate_struct(ostream, struct_type);
    }

    after_generate_struct(ostream, document.get());
    before_generate_enum(ostream, document.get());
    for (const auto& enum_type : document->get_enum_types()) {
      generate_enum(ostream, enum_type);
    }
    after_generate_enum(ostream, document.get());
    after_generate_document(ostream, document.get());
    ostream << std::flush;
  }
};

/**
 * Transforms a camel case string to an equivalent one separated by underscores
 * e.g. aMultiWord -> a_multi_word
 *      someName   -> some_name
 *      CamelCase  -> camel_case
 *      name       -> name
 *      Name       -> name
 */
std::string underscore(std::string in) {
  in[0] = tolower(in[0]);
  for (size_t i = 1; i < in.size(); ++i) {
    if (isupper(in[i])) {
      in[i] = tolower(in[i]);
      in.insert(i, "_");
    }
  }
  return in;
}

/**
 * Transforms a string with words separated by underscores to a camel case
 * equivalent
 * e.g.
 * a_multi_word -> aMultiWord
 * some_name    -> someName
 * name         -> name
 */
std::string camelcase(const std::string& in) {
  std::ostringstream out;
  bool underscore = false;

  for (char c : in) {
    if (c == '_') {
      underscore = true;
      continue;
    }
    if (underscore) {
      out << (char)toupper(c);
      underscore = false;
      continue;
    }
    out << c;
  }

  return out.str();
}

/**
 * Capitalization helpers
 */
std::string capitalize(std::string in) {
  in[0] = toupper(in[0]);
  return in;
}

std::string decapitalize(std::string in) {
  in[0] = tolower(in[0]);
  return in;
}

}  // namespace toolman

#endif  // TOOLMAN_GENERATOR_H_
