#include "mesh/MeshIO.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>

namespace hcad::mesh {
namespace {

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

std::string baseName(const std::string& path) {
  auto p = path.find_last_of("/\\");
  std::string n = p == std::string::npos ? path : path.substr(p + 1);
  auto d = n.find_last_of('.');
  return d == std::string::npos ? n : n.substr(0, d);
}

bool readAll(const std::string& path, std::string& data) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  data = ss.str();
  return true;
}

} // namespace

ImportResult importStl(const std::string& path) {
  ImportResult r;
  std::string data;
  if (!readAll(path, data)) { r.error = "Cannot open file: " + path; return r; }
  r.mesh.name = baseName(path);

  // Binary STL: 80-byte header + uint32 count + 50 bytes per triangle. Some
  // binary files start with "solid", so the size check is authoritative.
  bool binary = false;
  if (data.size() >= 84) {
    uint32_t n = 0;
    std::memcpy(&n, data.data() + 80, 4);
    if (84ull + 50ull * n == data.size()) binary = true;
  }

  if (binary) {
    r.format = "STL (binary)";
    uint32_t n = 0;
    std::memcpy(&n, data.data() + 80, 4);
    r.mesh.vertices.reserve(n * 3);
    r.mesh.triangles.reserve(n);
    const char* p = data.data() + 84;
    for (uint32_t i = 0; i < n; ++i, p += 50) {
      float f[12];
      std::memcpy(f, p, sizeof f);
      uint32_t base = static_cast<uint32_t>(r.mesh.vertices.size());
      for (int k = 0; k < 3; ++k) r.mesh.vertices.push_back({f[3 + 3 * k], f[4 + 3 * k], f[5 + 3 * k]});
      r.mesh.triangles.push_back({base, base + 1, base + 2});
    }
    r.sourcePolygons = n;
  } else {
    r.format = "STL (ASCII)";
    std::istringstream in(data);
    std::string tok;
    std::vector<Vec3> facet;
    if (!(in >> tok) || lower(tok) != "solid") { r.error = "Not a valid STL file (no 'solid' header and size does not match binary layout)"; return r; }
    while (in >> tok) {
      tok = lower(tok);
      if (tok == "vertex") {
        Vec3 v{};
        if (!(in >> v[0] >> v[1] >> v[2])) { r.error = "Malformed vertex line"; return r; }
        facet.push_back(v);
      } else if (tok == "endfacet") {
        if (facet.size() >= 3) {
          uint32_t base = static_cast<uint32_t>(r.mesh.vertices.size());
          for (auto& v : facet) r.mesh.vertices.push_back(v);
          for (size_t k = 1; k + 1 < facet.size(); ++k)
            r.mesh.triangles.push_back({base, base + static_cast<uint32_t>(k), base + static_cast<uint32_t>(k + 1)});
        }
        ++r.sourcePolygons;
        facet.clear();
      }
    }
  }
  if (r.mesh.triangles.empty()) { r.error = "STL contains no triangles"; return r; }
  return r;
}

ImportResult importObj(const std::string& path) {
  ImportResult r;
  r.format = "OBJ";
  std::ifstream f(path);
  if (!f) { r.error = "Cannot open file: " + path; return r; }
  r.mesh.name = baseName(path);
  std::string line;
  size_t lineNo = 0;
  while (std::getline(f, line)) {
    ++lineNo;
    std::istringstream ls(line);
    std::string tag;
    if (!(ls >> tag)) continue;
    if (tag == "v") {
      Vec3 v{};
      if (!(ls >> v[0] >> v[1] >> v[2])) { r.error = "Malformed vertex at line " + std::to_string(lineNo); return r; }
      r.mesh.vertices.push_back(v);
    } else if (tag == "f") {
      std::vector<uint32_t> poly;
      std::string ref;
      while (ls >> ref) {
        long idx = 0;
        try { idx = std::stol(ref.substr(0, ref.find('/'))); } catch (...) {
          r.error = "Malformed face at line " + std::to_string(lineNo); return r;
        }
        long n = static_cast<long>(r.mesh.vertices.size());
        long resolved = idx > 0 ? idx - 1 : n + idx; // negative = relative
        if (resolved < 0 || resolved >= n) { r.error = "Face index out of range at line " + std::to_string(lineNo); return r; }
        poly.push_back(static_cast<uint32_t>(resolved));
      }
      if (poly.size() < 3) continue;
      ++r.sourcePolygons;
      for (size_t k = 1; k + 1 < poly.size(); ++k) r.mesh.triangles.push_back({poly[0], poly[k], poly[k + 1]});
    }
  }
  if (r.mesh.triangles.empty()) { r.error = "OBJ contains no faces"; return r; }
  return r;
}

ImportResult importMesh(const std::string& path, double unitToMm) {
  std::string ext = lower(path.substr(path.find_last_of('.') + 1));
  ImportResult r;
  if (ext == "stl") r = importStl(path);
  else if (ext == "obj") r = importObj(path);
  else { r.error = "Unsupported mesh format: ." + ext + " (supported: .stl, .obj)"; return r; }
  if (!r.ok()) return r;
  if (unitToMm != 1.0) r.mesh.scale(unitToMm);
  r.weldedVertices = weldVertices(r.mesh, 1e-6);
  return r;
}

} // namespace hcad::mesh
