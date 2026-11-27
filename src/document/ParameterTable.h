#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hcad::doc {

// User-visible named parameters ("Width = 100 mm", "Height = Width * 0.5").
struct Parameter {
  std::string name;
  std::string expression;
  std::string comment;
};

class ParameterTable {
public:
  // Returns an error message if the name is invalid or already used.
  std::string add(const std::string& name, const std::string& expression, const std::string& comment = {});
  bool setExpression(const std::string& name, const std::string& expression);
  bool remove(const std::string& name);
  const std::vector<Parameter>& parameters() const { return params_; }
  const Parameter* find(const std::string& name) const;

  // Evaluates all parameters in dependency order. Detects unknown names and
  // cycles. Results stay available through value()/error().
  void evaluate();
  std::optional<double> value(const std::string& name) const;
  std::string error(const std::string& name) const;
  bool hasErrors() const { return !errors_.empty(); }

private:
  std::vector<Parameter> params_;
  std::map<std::string, double> values_;
  std::map<std::string, std::string> errors_;
};

} // namespace hcad::doc
