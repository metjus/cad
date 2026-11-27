#include "kernel/NamedShape.h"

#include <BRepBuilderAPI_MakeShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>

#include <algorithm>

namespace hcad::kernel {

void NamedShape::nameFace(const std::string& name, const TopoDS_Face& face) {
  faces_.emplace_back(name, face);
}

std::optional<std::string> NamedShape::faceName(const TopoDS_Shape& face) const {
  for (const auto& [n, f] : faces_)
    if (f.IsSame(face)) return n;
  return std::nullopt;
}

std::vector<std::string> NamedShape::adjacentFaceNames(const TopoDS_Shape& sub) const {
  std::vector<std::string> names;
  TopTools_IndexedDataMapOfShapeListOfShape anc;
  TopExp::MapShapesAndAncestors(shape_, sub.ShapeType(), TopAbs_FACE, anc);
  const int idx = anc.FindIndex(sub);
  if (idx == 0) return names;
  for (TopTools_ListOfShape::Iterator it(anc.FindFromIndex(idx)); it.More(); it.Next()) {
    auto n = faceName(it.Value());
    names.push_back(n ? *n : std::string("?"));
  }
  std::sort(names.begin(), names.end());
  names.erase(std::unique(names.begin(), names.end()), names.end());
  return names;
}

namespace {
std::string joinNames(const char* kind, const std::vector<std::string>& names) {
  std::string s = kind;
  s += '(';
  for (size_t i = 0; i < names.size(); ++i) { if (i) s += '|'; s += names[i]; }
  s += ')';
  return s;
}
} // namespace

std::optional<std::string> NamedShape::nameOf(const TopoDS_Shape& sub) const {
  if (sub.IsNull()) return std::nullopt;
  switch (sub.ShapeType()) {
  case TopAbs_FACE:
    return faceName(sub);
  case TopAbs_EDGE:
  case TopAbs_VERTEX: {
    auto names = adjacentFaceNames(sub);
    if (names.empty() || std::find(names.begin(), names.end(), "?") != names.end()) return std::nullopt;
    if (sub.ShapeType() == TopAbs_EDGE && names.size() < 2) return std::nullopt; // seam/boundary: not unique
    return joinNames(sub.ShapeType() == TopAbs_EDGE ? "Edge" : "Vertex", names);
  }
  default:
    return std::nullopt;
  }
}

std::optional<TopoDS_Shape> NamedShape::resolve(const std::string& ref) const {
  // Face names are matched first: generated faces may contain parentheses
  // themselves (e.g. "Chamfer1.Edge(Box1.Front|Box1.Top)").
  for (const auto& [n, f] : faces_) if (n == ref) return TopoDS_Shape(f);
  auto open = ref.find('(');
  if (open == std::string::npos) return std::nullopt;
  const std::string kind = ref.substr(0, open);
  const TopAbs_ShapeEnum type = kind == "Edge" ? TopAbs_EDGE : kind == "Vertex" ? TopAbs_VERTEX : TopAbs_SHAPE;
  if (type == TopAbs_SHAPE) return std::nullopt;

  std::optional<TopoDS_Shape> found;
  TopTools_IndexedMapOfShape subs;
  TopExp::MapShapes(shape_, type, subs);
  for (int i = 1; i <= subs.Extent(); ++i) {
    auto n = nameOf(subs(i));
    if (n && *n == ref) {
      if (found) return std::nullopt; // ambiguous reference: refuse rather than guess
      found = subs(i);
    }
  }
  return found;
}

size_t NamedShape::unnamedFaceCount() const {
  size_t n = 0;
  TopTools_IndexedMapOfShape fs;
  TopExp::MapShapes(shape_, TopAbs_FACE, fs);
  for (int i = 1; i <= fs.Extent(); ++i) if (!faceName(fs(i))) ++n;
  return n;
}

NamedShape NamedShape::propagate(const NamedShape& in, BRepBuilderAPI_MakeShape& op, const std::string& tag) {
  NamedShape out(op.Shape());
  TopTools_IndexedMapOfShape resultFaces;
  TopExp::MapShapes(out.shape_, TopAbs_FACE, resultFaces);

  for (const auto& [name, face] : in.faces_) {
    if (op.IsDeleted(face)) continue;
    const TopTools_ListOfShape& mod = op.Modified(face);
    if (mod.IsEmpty()) {
      if (resultFaces.Contains(face)) out.nameFace(name, face);
      continue;
    }
    int k = 0;
    for (TopTools_ListOfShape::Iterator it(mod); it.More(); it.Next(), ++k) {
      if (it.Value().ShapeType() != TopAbs_FACE) continue;
      // A face split into several pieces keeps its name plus a piece index.
      out.nameFace(mod.Extent() == 1 ? name : name + "#" + std::to_string(k + 1), TopoDS::Face(it.Value()));
    }
  }

  // Faces generated from edges or vertices of the input (fillets, chamfers)
  // are named after the generating feature and the persistent edge name.
  TopTools_IndexedMapOfShape inEdges;
  TopExp::MapShapes(in.shape_, TopAbs_EDGE, inEdges);
  for (int i = 1; i <= inEdges.Extent(); ++i) {
    const TopTools_ListOfShape& gen = op.Generated(inEdges(i));
    if (gen.IsEmpty()) continue;
    auto edgeName = in.nameOf(inEdges(i));
    int k = 0;
    for (TopTools_ListOfShape::Iterator it(gen); it.More(); it.Next(), ++k) {
      if (it.Value().ShapeType() != TopAbs_FACE) continue;
      std::string n = tag + "." + (edgeName ? *edgeName : "Edge?");
      if (gen.Extent() > 1) n += "#" + std::to_string(k + 1);
      out.nameFace(n, TopoDS::Face(it.Value()));
    }
  }
  return out;
}

} // namespace hcad::kernel
