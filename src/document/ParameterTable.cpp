#include "document/ParameterTable.h"
#include "core/Expression.h"

#include <functional>
#include <set>

namespace hcad::doc {

std::string ParameterTable::add(const std::string& name, const std::string& expr, const std::string& comment) {
  if (!isValidIdentifier(name)) return "'" + name + "' is not a valid parameter name";
  if (find(name)) return "Parameter '" + name + "' already exists";
  params_.push_back({name, expr, comment});
  return {};
}

bool ParameterTable::setExpression(const std::string& name, const std::string& expr) {
  for (auto& p : params_) if (p.name == name) { p.expression = expr; return true; }
  return false;
}

bool ParameterTable::remove(const std::string& name) {
  for (auto it = params_.begin(); it != params_.end(); ++it)
    if (it->name == name) { params_.erase(it); return true; }
  return false;
}

const Parameter* ParameterTable::find(const std::string& name) const {
  for (const auto& p : params_) if (p.name == name) return &p;
  return nullptr;
}

void ParameterTable::evaluate() {
  values_.clear();
  errors_.clear();
  std::set<std::string> inProgress;

  std::function<std::optional<double>(const std::string&)> eval = [&](const std::string& n) -> std::optional<double> {
    if (auto it = values_.find(n); it != values_.end()) return it->second;
    if (errors_.count(n)) return std::nullopt;
    const Parameter* p = find(n);
    if (!p) return std::nullopt;
    if (!inProgress.insert(n).second) {
      errors_[n] = "Circular reference involving '" + n + "'";
      return std::nullopt;
    }
    auto r = evaluateExpression(p->expression, [&](const std::string& id) { return eval(id); });
    inProgress.erase(n);
    if (!r.ok()) {
      if (!errors_.count(n)) errors_[n] = r.error;
      return std::nullopt;
    }
    values_[n] = *r.value;
    return r.value;
  };
  for (const auto& p : params_) eval(p.name);
}

std::optional<double> ParameterTable::value(const std::string& name) const {
  if (auto it = values_.find(name); it != values_.end()) return it->second;
  return std::nullopt;
}

std::string ParameterTable::error(const std::string& name) const {
  if (auto it = errors_.find(name); it != errors_.end()) return it->second;
  return {};
}

} // namespace hcad::doc
