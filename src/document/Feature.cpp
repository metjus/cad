#include "document/Feature.h"

namespace hcad::doc {

const char* toString(FeatureStatus s) {
  switch (s) {
  case FeatureStatus::NotComputed: return "not computed";
  case FeatureStatus::Ok: return "ok";
  case FeatureStatus::Error: return "error";
  case FeatureStatus::Suppressed: return "suppressed";
  case FeatureStatus::RolledBack: return "rolled back";
  }
  return "?";
}

FeatureParam* Feature::param(const std::string& key) {
  for (auto& p : params_) if (p.key == key) return &p;
  return nullptr;
}
const FeatureParam* Feature::param(const std::string& key) const {
  for (const auto& p : params_) if (p.key == key) return &p;
  return nullptr;
}
double Feature::value(const std::string& key) const {
  const auto* p = param(key);
  return p ? p->value : 0.0;
}

FeatureParam& Feature::addParam(std::string key, std::string label, ParamKind kind, std::string expr) {
  params_.push_back({std::move(key), std::move(label), kind, std::move(expr), 0.0, {}});
  return params_.back();
}

const FeatureResult* Feature::inputResult(const FeatureContext& ctx) const {
  if (inputId_.empty()) return nullptr;
  const Feature* f = ctx.findFeature(inputId_);
  return f ? &f->result : nullptr;
}

} // namespace hcad::doc
