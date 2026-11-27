#include "document/Document.h"
#include "kernel/MeshBridge.h"
#include "mesh/MeshAnalysis.h"
#include "mesh/MeshIO.h"
#include <gtest/gtest.h>

using namespace hcad;
static std::string data(const char* f) { return std::string(HCAD_TEST_DATA) + "/" + f; }

TEST(MeshImport, AsciiStlCubeIsWatertight) {
  auto r = mesh::importMesh(data("cube_ascii.stl"));
  ASSERT_TRUE(r.ok()) << r.error;
  EXPECT_EQ(r.format, "STL (ASCII)");
  EXPECT_EQ(r.mesh.triangles.size(), 12u);
  EXPECT_EQ(r.mesh.vertices.size(), 8u); // welded from 36
  auto rep = mesh::analyzeMesh(r.mesh);
  EXPECT_TRUE(rep.closed);
  EXPECT_FALSE(rep.normalsInverted);
  EXPECT_EQ(rep.componentCount, 1u);
  EXPECT_NEAR(rep.signedVolume, 1000.0, 1e-9);
  EXPECT_NEAR(rep.surfaceArea, 600.0, 1e-9);
}

TEST(MeshImport, BinaryStlWithMisleadingSolidHeader) {
  auto r = mesh::importMesh(data("cube_binary.stl"));
  ASSERT_TRUE(r.ok()) << r.error;
  EXPECT_EQ(r.format, "STL (binary)");
  EXPECT_TRUE(mesh::analyzeMesh(r.mesh).closed);
}

TEST(MeshImport, ObjQuadsAreTriangulatedAndScaled) {
  auto r = mesh::importMesh(data("cube_quads.obj"), 25.4); // file in inches
  ASSERT_TRUE(r.ok()) << r.error;
  EXPECT_EQ(r.sourcePolygons, 6u);
  EXPECT_EQ(r.mesh.triangles.size(), 12u);
  auto rep = mesh::analyzeMesh(r.mesh);
  EXPECT_NEAR(rep.bboxMax[0] - rep.bboxMin[0], 254.0, 1e-9);
  EXPECT_TRUE(rep.closed);
}

TEST(MeshAnalysis, DetectsHole) {
  auto r = mesh::importMesh(data("open_box.obj"));
  ASSERT_TRUE(r.ok());
  auto rep = mesh::analyzeMesh(r.mesh);
  EXPECT_FALSE(rep.closed);
  EXPECT_EQ(rep.boundaryEdges, 4u);
  EXPECT_EQ(rep.boundaryLoops, 1u);
}

TEST(MeshAnalysis, DetectsNonManifoldEdge) {
  auto r = mesh::importMesh(data("non_manifold.obj"));
  ASSERT_TRUE(r.ok());
  auto rep = mesh::analyzeMesh(r.mesh);
  EXPECT_EQ(rep.nonManifoldEdges, 1u);
  EXPECT_FALSE(rep.closed);
}

TEST(MeshAnalysis, DetectsInvertedNormalsAndDegenerates) {
  auto r = mesh::importMesh(data("cube_ascii.stl"));
  for (auto& t : r.mesh.triangles) std::swap(t[1], t[2]);
  auto rep = mesh::analyzeMesh(r.mesh);
  EXPECT_TRUE(rep.normalsInverted);
  r.mesh.triangles.push_back({0, 0, 1});
  r.mesh.triangles.push_back(r.mesh.triangles.front());
  rep = mesh::analyzeMesh(r.mesh);
  EXPECT_EQ(rep.degenerateTriangles, 1u);
  EXPECT_EQ(rep.duplicateTriangles, 1u);
}

TEST(MeshImport, ErrorsAreReported) {
  EXPECT_FALSE(mesh::importMesh(data("does_not_exist.stl")).ok());
  EXPECT_FALSE(mesh::importMesh(data("cube.3ds")).ok());
}

TEST(MeshReference, StaysSeparateFromCadFeatures) {
  doc::Document d;
  EXPECT_TRUE(d.importMesh(data("cube_ascii.stl")).empty());
  EXPECT_EQ(d.meshes().size(), 1u);
  EXPECT_TRUE(d.features().empty()); // importing never creates CAD features
  auto tri = kernel::toPolyTriangulation(d.meshes()[0].mesh);
  EXPECT_EQ(tri->NbTriangles(), 12);
  EXPECT_EQ(tri->NbNodes(), 8);
}
