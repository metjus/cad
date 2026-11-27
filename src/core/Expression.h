#pragma once
#include <functional>
#include <optional>
#include <set>
#include <string>

namespace hcad {

// Result of evaluating a parameter expression. Never throws.
struct EvalResult {
  std::optional<double> value; // in millimetres / degrees / plain number
  std::string error;           // empty on success
  bool ok() const { return value.has_value(); }
};

using VariableLookup = std::function<std::optional<double>(const std::string&)>;

// Evaluates expressions such as "Width * 0.5", "10 mm + 0.5 in", "(A+B)/2".
// Grammar:  expr := term (('+'|'-') term)*
//           term := unary (('*'|'/') unary)*
//           unary := '-' unary | power
//           power := primary ('^' unary)?
//           primary := number [unit] | identifier | '(' expr ')'
// Unit suffixes (mm, cm, m, in) multiply the literal into millimetres.
EvalResult evaluateExpression(const std::string& text, const VariableLookup& lookup);

// Collects identifiers referenced by an expression (used for dependency
// tracking and cycle detection). Returns an empty set on parse errors.
std::set<std::string> referencedIdentifiers(const std::string& text);

bool isValidIdentifier(const std::string& name);

} // namespace hcad
