#pragma once
#include "kernel/NamedShape.h"
#include <string>
#include <vector>

namespace hcad::kernel {

struct OpResult {
  NamedShape shape;
  std::string error; // empty on success
  bool ok() const { return error.empty() && !shape.isNull(); }
};

// Axis-aligned box with a corner at the origin. Face names:
// <tag>.Bottom (Z-), Top (Z+), Back (X-), Front (X+), Left (Y-), Right (Y+).
OpResult makeBox(double dx, double dy, double dz, const std::string& tag);

// Equal-distance chamfer on the given edges. Edges are given as persistent
// references and resolved against `input` at execution time.
OpResult chamferEdges(const NamedShape& input, const std::vector<std::string>& edgeRefs,
                      double distance, const std::string& tag);

// Constant-radius fillet, same reference semantics as chamferEdges.
OpResult filletEdges(const NamedShape& input, const std::vector<std::string>& edgeRefs,
                     double radius, const std::string& tag);

} // namespace hcad::kernel
