#include "document/Document.h"
#include "core/Expression.h"
#include "document/Features.h"
#include "mesh/MeshIO.h"

#include <chrono>
#include <set>

namespace hcad::doc {

std::string Document::nextId(const std::string& prefix) const {
  for (int i = 1;; ++i) {
    std::string id = prefix + std::to_string(i);
    if (!feature(id)) return id;
  }
}

Feature* Document::feature(const std::string& id) {
  for (auto& f : features_) if (f->id() == id) return f.get();
  return nullptr;
}
const Feature* Document::feature(const std::string& id) const {
  for (const auto& f : features_) if (f->id() == id) return f.get();
  return nullptr;
}

RecomputeReport Document::recompute() {
  using clock = std::chrono::steady_clock;
  RecomputeReport rep;
  const auto t0 = clock::now();
  params_.evaluate();

  FeatureContext ctx{[this](const std::string& id) -> const Feature* { return feature(id); }};
  const VariableLookup lookup = [this](const std::string& n) { return params_.value(n); };

  for (size_t i = 0; i < features_.size(); ++i) {
    Feature& f = *features_[i];
    const auto ft0 = clock::now();
    f.result.clear();
    f.error.clear();

    if (rollback_ >= 0 && static_cast<int>(i) > rollback_) { f.status = FeatureStatus::RolledBack; continue; }

    const Feature* input = f.inputId().empty() ? nullptr : feature(f.inputId());
    if (!f.inputId().empty() && (!input || (input->status != FeatureStatus::Ok && input->status != FeatureStatus::Suppressed))) {
      f.status = FeatureStatus::Error;
      f.error = "Depends on '" + f.inputId() + "', which has no valid result";
      ++rep.failed;
      rep.messages.push_back(f.id() + ": " + f.error);
      continue;
    }
    if (f.suppressed) {
      // A suppressed feature passes its input through unchanged.
      if (input) f.result = input->result;
      f.status = FeatureStatus::Suppressed;
      continue;
    }

    std::string paramError;
    for (auto& p : f.params()) {
      auto r = evaluateExpression(p.expression, lookup);
      p.error = r.error;
      if (r.ok()) p.value = *r.value;
      else if (paramError.empty()) paramError = p.label + ": " + r.error;
    }
    std::string err = paramError.empty() ? f.execute(ctx) : paramError;
    f.lastComputeMs = std::chrono::duration<double, std::milli>(clock::now() - ft0).count();
    if (err.empty()) {
      f.status = FeatureStatus::Ok;
      ++rep.computed;
    } else {
      f.status = FeatureStatus::Error;
      f.error = err;
      f.result.clear();
      ++rep.failed;
      rep.messages.push_back(f.id() + ": " + err);
    }
  }
  rep.totalMs = std::chrono::duration<double, std::milli>(clock::now() - t0).count();
  return rep;
}

std::vector<const Feature*> Document::tipFeatures() const {
  std::set<std::string> consumed;
  for (size_t i = 0; i < features_.size(); ++i) {
    const auto& f = features_[i];
    if (f->status == FeatureStatus::RolledBack) continue;
    if (!f->inputId().empty()) consumed.insert(f->inputId());
  }
  std::vector<const Feature*> tips;
  for (const auto& f : features_)
    if (!consumed.count(f->id()) && f->status != FeatureStatus::RolledBack) tips.push_back(f.get());
  return tips;
}

std::string Document::importMesh(const std::string& path, double unitToMm) {
  auto r = mesh::importMesh(path, unitToMm);
  if (!r.ok()) return r.error;
  MeshReference m;
  m.id = "Mesh" + std::to_string(meshes_.size() + 1);
  m.sourcePath = path;
  m.format = r.format;
  m.mesh = std::move(r.mesh);
  m.report = mesh::analyzeMesh(m.mesh);
  meshes_.push_back(std::move(m));
  return {};
}

void buildDemoDocument(Document& doc) {
  auto& p = doc.parameters();
  p.add("Width", "60 mm", "Overall width of the block");
  p.add("Depth", "40 mm");
  p.add("Height", "Width * 0.5", "Driven by Width");
  p.add("Bevel", "4 mm");

  doc.addFeature(std::make_unique<BoxFeature>("Box1", "Width", "Depth", "Height"));
  doc.addFeature(std::make_unique<EdgeBlendFeature>("Chamfer1", EdgeBlendFeature::Kind::Chamfer, "Box1",
                                                    std::vector<std::string>{"Edge(Box1.Front|Box1.Top)"}, "Bevel"));
  doc.addFeature(std::make_unique<EdgeBlendFeature>("Fillet1", EdgeBlendFeature::Kind::Fillet, "Chamfer1",
                                                    std::vector<std::string>{"Edge(Box1.Back|Box1.Right)"}, "3 mm"));

  auto* surf = doc.addFeature(std::make_unique<NurbsSurfaceFeature>("Surface1", "60 mm", "40 mm", 5, 4, 3, "0 mm", "Depth + 25 mm"));
  (void)surf;
  doc.addFeature(std::make_unique<ControlPointEditFeature>("CPEdit1", "Surface1", 2, 1, "0", "0", "15 mm"));
  doc.addFeature(std::make_unique<ControlPointEditFeature>("CPEdit2", "CPEdit1", 2, 2, "0", "0", "15 mm"));
}

} // namespace hcad::doc
