#include "kernel/Primitives.h"
#include "kernel/ShapeInfo.h"

#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS.hxx>

namespace hcad::kernel {

OpResult makeBox(double dx, double dy, double dz, const std::string& tag) {
  OpResult r;
  const double tol = Precision::Confusion();
  if (dx <= tol || dy <= tol || dz <= tol) {
    r.error = "Box dimensions must be greater than the modelling tolerance (" + std::to_string(tol) + " mm)";
    return r;
  }
  try {
    BRepPrimAPI_MakeBox mk(dx, dy, dz);
    mk.Build();
    if (!mk.IsDone()) { r.error = "OCCT failed to build the box"; return r; }
    NamedShape ns(mk.Shape());
    ns.nameFace(tag + ".Bottom", mk.BottomFace());
    ns.nameFace(tag + ".Top", mk.TopFace());
    ns.nameFace(tag + ".Back", mk.BackFace());
    ns.nameFace(tag + ".Front", mk.FrontFace());
    ns.nameFace(tag + ".Left", mk.LeftFace());
    ns.nameFace(tag + ".Right", mk.RightFace());
    r.shape = ns;
  } catch (const Standard_Failure& e) {
    r.error = std::string("Kernel exception: ") + e.GetMessageString();
  }
  return r;
}

template <class MakeOp, class AddFn>
static OpResult edgeOperation(const NamedShape& input, const std::vector<std::string>& refs,
                              double value, const std::string& tag, const char* what, AddFn add) {
  OpResult r;
  if (input.isNull()) { r.error = "No input body"; return r; }
  if (refs.empty()) { r.error = std::string(what) + " needs at least one edge"; return r; }
  if (value <= Precision::Confusion()) { r.error = std::string(what) + " size must be positive"; return r; }
  try {
    MakeOp mk(input.shape());
    for (const auto& ref : refs) {
      auto e = input.resolve(ref);
      if (!e || e->ShapeType() != TopAbs_EDGE) {
        r.error = "Reference '" + ref + "' could not be resolved on the input body";
        return r;
      }
      add(mk, value, TopoDS::Edge(*e));
    }
    mk.Build();
    if (!mk.IsDone()) { r.error = std::string(what) + " failed (size too large for the adjacent faces?)"; return r; }
    r.shape = NamedShape::propagate(input, mk, tag);
    if (!isValidSolid(r.shape.shape())) {
      r.error = std::string(what) + " produced an invalid solid";
      r.shape = NamedShape();
    }
  } catch (const Standard_Failure& e) {
    r.error = std::string(what) + " kernel exception: " + e.GetMessageString();
  }
  return r;
}

OpResult chamferEdges(const NamedShape& input, const std::vector<std::string>& refs, double d, const std::string& tag) {
  return edgeOperation<BRepFilletAPI_MakeChamfer>(input, refs, d, tag, "Chamfer",
    [](BRepFilletAPI_MakeChamfer& mk, double v, const TopoDS_Edge& e) { mk.Add(v, e); });
}

OpResult filletEdges(const NamedShape& input, const std::vector<std::string>& refs, double rad, const std::string& tag) {
  return edgeOperation<BRepFilletAPI_MakeFillet>(input, refs, rad, tag, "Fillet",
    [](BRepFilletAPI_MakeFillet& mk, double v, const TopoDS_Edge& e) { mk.Add(v, e); });
}

} // namespace hcad::kernel
