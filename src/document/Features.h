#pragma once
#include "document/Feature.h"

namespace hcad::doc {

class BoxFeature : public Feature {
public:
  BoxFeature(std::string id, std::string w, std::string d, std::string h);
  std::string summary() const override;
  std::string execute(const FeatureContext& ctx) override;
};

// Chamfer or fillet on edges referenced by persistent names.
class EdgeBlendFeature : public Feature {
public:
  enum class Kind { Chamfer, Fillet };
  EdgeBlendFeature(std::string id, Kind kind, std::string inputId, std::vector<std::string> edgeRefs, std::string size);
  std::string summary() const override;
  std::string execute(const FeatureContext& ctx) override;
  std::vector<std::string> edgeRefs;
  Kind kind;
};

// Base NURBS patch: planar control grid of CountU x CountV poles.
class NurbsSurfaceFeature : public Feature {
public:
  NurbsSurfaceFeature(std::string id, std::string sizeU, std::string sizeV, int countU, int countV, int degree,
                      std::string originX = "0", std::string originY = "0");
  std::string summary() const override;
  std::string execute(const FeatureContext& ctx) override;
};

// Editable control-point move recorded in history (never baked into a mesh).
class ControlPointEditFeature : public Feature {
public:
  ControlPointEditFeature(std::string id, std::string inputId, int u, int v,
                          std::string dx, std::string dy, std::string dz, std::string weight = "1");
  std::string summary() const override;
  std::string execute(const FeatureContext& ctx) override;
  int u, v;
};

} // namespace hcad::doc
