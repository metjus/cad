#pragma once
#include "document/Feature.h"
#include "document/ParameterTable.h"
#include "mesh/MeshAnalysis.h"
#include "mesh/TriMesh.h"

#include <memory>
#include <string>
#include <vector>

namespace hcad::doc {

// Imported polygon data kept as a non-destructive reference. It is never
// presented as, or silently converted into, CAD geometry.
struct MeshReference {
  std::string id;
  std::string sourcePath;
  std::string format;
  mesh::TriMesh mesh;
  mesh::MeshReport report;
  bool visible = true;
};

struct RecomputeReport {
  int computed = 0;
  int failed = 0;
  double totalMs = 0;
  std::vector<std::string> messages;
};

class Document {
public:
  ParameterTable& parameters() { return params_; }
  const ParameterTable& parameters() const { return params_; }

  // Takes ownership; returns a raw pointer for convenience.
  template <class T> T* addFeature(std::unique_ptr<T> f) {
    T* raw = f.get();
    features_.push_back(std::move(f));
    return raw;
  }
  std::string nextId(const std::string& prefix) const;

  const std::vector<std::unique_ptr<Feature>>& features() const { return features_; }
  Feature* feature(const std::string& id);
  const Feature* feature(const std::string& id) const;

  // Features after this index are not computed (history rollback marker).
  // -1 = no rollback.
  void setRollbackIndex(int index) { rollback_ = index; }
  int rollbackIndex() const { return rollback_; }

  // Re-evaluates parameters and recomputes the history in order.
  RecomputeReport recompute();

  // Features whose result is not consumed by a later feature: the visible
  // bodies/surfaces of the document.
  std::vector<const Feature*> tipFeatures() const;

  std::vector<MeshReference>& meshes() { return meshes_; }
  const std::vector<MeshReference>& meshes() const { return meshes_; }
  // Returns error text, empty on success.
  std::string importMesh(const std::string& path, double unitToMm = 1.0);

private:
  ParameterTable params_;
  std::vector<std::unique_ptr<Feature>> features_;
  std::vector<MeshReference> meshes_;
  int rollback_ = -1;
};

// Creates the Phase 0 demo document: named parameters, a box, a chamfer on a
// persistently referenced edge, and a NURBS patch with a control-point edit.
void buildDemoDocument(Document& doc);

} // namespace hcad::doc
