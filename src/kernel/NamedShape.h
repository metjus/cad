#pragma once
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <optional>
#include <string>
#include <utility>
#include <vector>

class BRepBuilderAPI_MakeShape;

namespace hcad::kernel {

// A B-Rep shape plus persistent, semantic names for its faces.
//
// This is the core of the topological-naming strategy (see
// docs/ARCHITECTURE.md, "Topological reference stability"):
//  * Faces receive names from the feature that creates them (generator role,
//    e.g. "Box1.Top"), never from their index inside the shape.
//  * Names are propagated through each modelling operation using the OCCT
//    operation history (Modified / Generated / IsDeleted).
//  * Edges and vertices are not named directly. They are referenced by the
//    set of named faces that meet at them ("Edge(Box1.Front|Box1.Top)"), so
//    they survive any upstream change that keeps the adjacent faces.
class NamedShape {
public:
  NamedShape() = default;
  explicit NamedShape(TopoDS_Shape s) : shape_(std::move(s)) {}

  const TopoDS_Shape& shape() const { return shape_; }
  bool isNull() const { return shape_.IsNull(); }

  void nameFace(const std::string& name, const TopoDS_Face& face);
  const std::vector<std::pair<std::string, TopoDS_Face>>& namedFaces() const { return faces_; }

  // Persistent reference string for a face, edge or vertex of this shape.
  std::optional<std::string> nameOf(const TopoDS_Shape& sub) const;

  // Resolves a reference produced by nameOf() against this shape.
  std::optional<TopoDS_Shape> resolve(const std::string& reference) const;

  size_t unnamedFaceCount() const;

  // Builds the named result of an OCCT operation whose input was `input`.
  // `generatorTag` prefixes names of faces generated from input edges.
  static NamedShape propagate(const NamedShape& input, BRepBuilderAPI_MakeShape& op,
                              const std::string& generatorTag);

private:
  std::optional<std::string> faceName(const TopoDS_Shape& face) const;
  std::vector<std::string> adjacentFaceNames(const TopoDS_Shape& sub) const;

  TopoDS_Shape shape_;
  std::vector<std::pair<std::string, TopoDS_Face>> faces_;
};

} // namespace hcad::kernel
