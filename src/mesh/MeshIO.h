#pragma once
#include "mesh/TriMesh.h"
#include <string>

namespace hcad::mesh {

struct ImportResult {
  TriMesh mesh;
  std::string error;            // empty on success
  std::string format;           // "STL (binary)", "STL (ASCII)", "OBJ"
  size_t sourcePolygons = 0;    // polygons before triangulation (OBJ)
  size_t weldedVertices = 0;    // vertices merged at import
  bool ok() const { return error.empty(); }
};

// Reads STL (ASCII or binary) or OBJ by file extension. Polygons are fan
// triangulated. Coincident vertices are welded (STL stores none shared).
// `unitToMm` scales the file units into millimetres (files carry no units).
ImportResult importMesh(const std::string& path, double unitToMm = 1.0);

ImportResult importStl(const std::string& path);
ImportResult importObj(const std::string& path);

} // namespace hcad::mesh
