#pragma once
#include "kernel/NamedShape.h"
#include "kernel/Nurbs.h"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace hcad::doc {

enum class ParamKind { Length, Angle, Count, Scalar };

// One editable input of a feature. The expression is authoritative; `value`
// is the last evaluated result (mm for lengths, degrees for angles).
struct FeatureParam {
  std::string key;
  std::string label;
  ParamKind kind = ParamKind::Length;
  std::string expression;
  double value = 0;
  std::string error;
};

enum class FeatureStatus { NotComputed, Ok, Error, Suppressed, RolledBack };
const char* toString(FeatureStatus s);

// Output of a feature. A feature produces either a body (solid/sheet) or a
// NURBS surface (which also carries its face).
struct FeatureResult {
  kernel::NamedShape body;
  std::optional<kernel::NurbsSurfaceDef> nurbs;
  void clear() { body = kernel::NamedShape(); nurbs.reset(); }
};

class Feature;
struct FeatureContext {
  std::function<const Feature*(const std::string&)> findFeature;
};

class Feature {
public:
  Feature(std::string id, std::string type) : id_(std::move(id)), type_(std::move(type)) {}
  virtual ~Feature() = default;

  const std::string& id() const { return id_; }
  const std::string& type() const { return type_; }
  virtual std::string summary() const { return type_; }

  std::vector<FeatureParam>& params() { return params_; }
  const std::vector<FeatureParam>& params() const { return params_; }
  FeatureParam* param(const std::string& key);
  const FeatureParam* param(const std::string& key) const;
  double value(const std::string& key) const;

  // Id of the upstream feature whose result this one modifies (empty = root).
  const std::string& inputId() const { return inputId_; }
  void setInputId(std::string id) { inputId_ = std::move(id); }

  bool suppressed = false;
  FeatureStatus status = FeatureStatus::NotComputed;
  std::string error;
  FeatureResult result;
  double lastComputeMs = 0;

  // Computes `result` from evaluated params and the input feature's result.
  // Returns an error message or empty string.
  virtual std::string execute(const FeatureContext& ctx) = 0;

protected:
  FeatureParam& addParam(std::string key, std::string label, ParamKind kind, std::string expr);
  const FeatureResult* inputResult(const FeatureContext& ctx) const;

  std::vector<FeatureParam> params_;

private:
  std::string id_;
  std::string type_;
  std::string inputId_;
};

} // namespace hcad::doc
