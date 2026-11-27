#include "document/Features.h"
#include "kernel/Primitives.h"

#include <cmath>
#include <cstdio>

namespace hcad::doc {

// ---------------------------------------------------------------- Box
BoxFeature::BoxFeature(std::string id, std::string w, std::string d, std::string h)
  : Feature(std::move(id), "Box") {
  addParam("Width", "Width (X)", ParamKind::Length, std::move(w));
  addParam("Depth", "Depth (Y)", ParamKind::Length, std::move(d));
  addParam("Height", "Height (Z)", ParamKind::Length, std::move(h));
}

std::string BoxFeature::summary() const {
  char b[128];
  std::snprintf(b, sizeof b, "Box %.2f x %.2f x %.2f mm", value("Width"), value("Depth"), value("Height"));
  return b;
}

std::string BoxFeature::execute(const FeatureContext&) {
  auto r = kernel::makeBox(value("Width"), value("Depth"), value("Height"), id());
  if (!r.ok()) return r.error;
  result.body = r.shape;
  return {};
}

// ---------------------------------------------------------------- Chamfer / Fillet
EdgeBlendFeature::EdgeBlendFeature(std::string id, Kind k, std::string input, std::vector<std::string> refs, std::string size)
  : Feature(std::move(id), k == Kind::Chamfer ? "Chamfer" : "Fillet"), edgeRefs(std::move(refs)), kind(k) {
  setInputId(std::move(input));
  addParam(k == Kind::Chamfer ? "Distance" : "Radius", k == Kind::Chamfer ? "Distance" : "Radius", ParamKind::Length, std::move(size));
}

std::string EdgeBlendFeature::summary() const {
  char b[128];
  std::snprintf(b, sizeof b, "%s %.2f mm on %zu edge(s)", type().c_str(), params_.front().value, edgeRefs.size());
  return b;
}

std::string EdgeBlendFeature::execute(const FeatureContext& ctx) {
  const FeatureResult* in = inputResult(ctx);
  if (!in || in->body.isNull()) return "Input body '" + inputId() + "' is not available";
  auto r = kind == Kind::Chamfer ? kernel::chamferEdges(in->body, edgeRefs, params_.front().value, id())
                                 : kernel::filletEdges(in->body, edgeRefs, params_.front().value, id());
  if (!r.ok()) return r.error;
  result.body = r.shape;
  return {};
}

// ---------------------------------------------------------------- NURBS surface
NurbsSurfaceFeature::NurbsSurfaceFeature(std::string id, std::string su, std::string sv, int cu, int cv, int degree,
                                         std::string ox, std::string oy)
  : Feature(std::move(id), "NURBS Surface") {
  addParam("OriginX", "Origin X", ParamKind::Length, std::move(ox));
  addParam("OriginY", "Origin Y", ParamKind::Length, std::move(oy));
  addParam("SizeU", "Size U", ParamKind::Length, std::move(su));
  addParam("SizeV", "Size V", ParamKind::Length, std::move(sv));
  addParam("CountU", "Control points U", ParamKind::Count, std::to_string(cu));
  addParam("CountV", "Control points V", ParamKind::Count, std::to_string(cv));
  addParam("Degree", "Degree", ParamKind::Count, std::to_string(degree));
}

std::string NurbsSurfaceFeature::summary() const {
  char b[128];
  std::snprintf(b, sizeof b, "NURBS %dx%d poles, degree %d", int(value("CountU")), int(value("CountV")), int(value("Degree")));
  return b;
}

std::string NurbsSurfaceFeature::execute(const FeatureContext&) {
  int cu = static_cast<int>(std::lround(value("CountU")));
  int cv = static_cast<int>(std::lround(value("CountV")));
  int deg = static_cast<int>(std::lround(value("Degree")));
  if (cu < 2 || cv < 2 || cu > 64 || cv > 64) return "Control point counts must be between 2 and 64";
  if (deg < 1 || deg > 9) return "Degree must be between 1 and 9";
  auto def = kernel::NurbsSurfaceDef::planarGrid(cu, cv, value("SizeU"), value("SizeV"), 0.0, deg);
  for (auto& p : def.poles) p.SetCoord(p.X() + value("OriginX"), p.Y() + value("OriginY"), p.Z());
  auto r = kernel::buildNurbsSurface(def, id());
  if (!r.ok()) return r.error;
  result.nurbs = def;
  result.body = r.face;
  return {};
}

// ---------------------------------------------------------------- Control point edit
ControlPointEditFeature::ControlPointEditFeature(std::string id, std::string input, int u_, int v_,
                                                 std::string dx, std::string dy, std::string dz, std::string w)
  : Feature(std::move(id), "Move Control Point"), u(u_), v(v_) {
  setInputId(std::move(input));
  addParam("DX", "Offset X", ParamKind::Length, std::move(dx));
  addParam("DY", "Offset Y", ParamKind::Length, std::move(dy));
  addParam("DZ", "Offset Z", ParamKind::Length, std::move(dz));
  addParam("Weight", "Weight", ParamKind::Scalar, std::move(w));
}

std::string ControlPointEditFeature::summary() const {
  char b[160];
  std::snprintf(b, sizeof b, "Move CP[%d,%d] by (%.2f, %.2f, %.2f) mm", u, v, value("DX"), value("DY"), value("DZ"));
  return b;
}

std::string ControlPointEditFeature::execute(const FeatureContext& ctx) {
  const FeatureResult* in = inputResult(ctx);
  if (!in || !in->nurbs) return "Input '" + inputId() + "' is not a NURBS surface";
  kernel::NurbsSurfaceDef def = *in->nurbs;
  if (u < 0 || v < 0 || u >= def.countU || v >= def.countV)
    return "Control point [" + std::to_string(u) + "," + std::to_string(v) + "] no longer exists in the input surface";
  gp_Pnt& p = def.pole(u, v);
  p.SetCoord(p.X() + value("DX"), p.Y() + value("DY"), p.Z() + value("DZ"));
  double w = value("Weight");
  if (std::abs(w - 1.0) > 1e-12 || !def.weights.empty()) {
    if (def.weights.empty()) def.weights.assign(def.poles.size(), 1.0);
    def.weights[static_cast<size_t>(u * def.countV + v)] = w;
  }
  // Keep the face name of the source surface so references stay stable.
  std::string tag = inputId();
  if (!in->body.namedFaces().empty()) {
    const std::string& n = in->body.namedFaces().front().first;
    tag = n.substr(0, n.rfind(".Surface"));
  }
  auto r = kernel::buildNurbsSurface(def, tag);
  if (!r.ok()) return r.error;
  result.nurbs = def;
  result.body = r.face;
  return {};
}

} // namespace hcad::doc
