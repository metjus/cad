#include "core/Expression.h"
#include "core/Units.h"

#include <cctype>
#include <cmath>
#include <stdexcept>

namespace hcad {
namespace {

struct ParseError : std::runtime_error { using std::runtime_error::runtime_error; };

class Parser {
public:
  Parser(const std::string& s, const VariableLookup* lookup, std::set<std::string>* ids)
    : s_(s), lookup_(lookup), ids_(ids) {}

  double parse() {
    double v = expr();
    skipWs();
    if (pos_ != s_.size()) throw ParseError("Unexpected '" + std::string(1, s_[pos_]) + "'");
    return v;
  }

private:
  void skipWs() { while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_; }
  bool eat(char c) { skipWs(); if (pos_ < s_.size() && s_[pos_] == c) { ++pos_; return true; } return false; }

  double expr() {
    double v = term();
    for (;;) {
      if (eat('+')) v += term();
      else if (eat('-')) v -= term();
      else return v;
    }
  }
  double term() {
    double v = unary();
    for (;;) {
      if (eat('*')) v *= unary();
      else if (eat('/')) {
        double d = unary();
        if (d == 0.0) throw ParseError("Division by zero");
        v /= d;
      } else return v;
    }
  }
  double unary() {
    if (eat('-')) return -unary();
    if (eat('+')) return unary();
    return power();
  }
  double power() {
    double b = primary();
    if (eat('^')) return std::pow(b, unary());
    return b;
  }
  std::string identifier() {
    size_t start = pos_;
    while (pos_ < s_.size() && (std::isalnum(static_cast<unsigned char>(s_[pos_])) || s_[pos_] == '_')) ++pos_;
    return s_.substr(start, pos_ - start);
  }
  double primary() {
    skipWs();
    if (pos_ >= s_.size()) throw ParseError("Unexpected end of expression");
    char c = s_[pos_];
    if (c == '(') {
      ++pos_;
      double v = expr();
      if (!eat(')')) throw ParseError("Missing ')'");
      return v;
    }
    if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
      size_t used = 0;
      double v = 0;
      try { v = std::stod(s_.substr(pos_), &used); } catch (...) { throw ParseError("Invalid number"); }
      pos_ += used;
      // optional unit suffix directly or after whitespace
      size_t save = pos_;
      skipWs();
      if (pos_ < s_.size() && std::isalpha(static_cast<unsigned char>(s_[pos_]))) {
        std::string unit = identifier();
        if (auto f = lengthUnitFactor(unit)) return v * *f;
        pos_ = save; // not a unit; let the caller report the stray identifier
      } else {
        pos_ = save;
      }
      return v;
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      std::string id = identifier();
      if (ids_) ids_->insert(id);
      if (!lookup_) return 1.0; // identifier collection mode
      auto v = (*lookup_)(id);
      if (!v) throw ParseError("Unknown parameter '" + id + "'");
      return *v;
    }
    throw ParseError("Unexpected '" + std::string(1, c) + "'");
  }

  const std::string& s_;
  const VariableLookup* lookup_;
  std::set<std::string>* ids_;
  size_t pos_ = 0;
};

} // namespace

EvalResult evaluateExpression(const std::string& text, const VariableLookup& lookup) {
  EvalResult r;
  try {
    Parser p(text, &lookup, nullptr);
    double v = p.parse();
    if (!std::isfinite(v)) { r.error = "Result is not a finite number"; return r; }
    r.value = v;
  } catch (const ParseError& e) {
    r.error = e.what();
  } catch (const std::exception& e) {
    r.error = e.what();
  }
  return r;
}

std::set<std::string> referencedIdentifiers(const std::string& text) {
  std::set<std::string> ids;
  try {
    Parser p(text, nullptr, &ids);
    p.parse();
  } catch (...) {
    return {};
  }
  return ids;
}

bool isValidIdentifier(const std::string& name) {
  if (name.empty() || !(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_')) return false;
  for (char c : name)
    if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
  return !lengthUnitFactor(name).has_value();
}

} // namespace hcad
