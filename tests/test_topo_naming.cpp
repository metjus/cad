// Topological reference stability: downstream features must keep pointing at
// the same *semantic* edge/face after upstream parameters change.
#include "document/Document.h"
#include "document/Features.h"
#include "kernel/Primitives.h"
#include "kernel/ShapeInfo.h"

#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <gtest/gtest.h>

using namespace hcad;

static gp_Pnt centroid(const TopoDS_Shape& s) {
  GProp_GProps p;
  if (s.ShapeType() == TopAbs_FACE) BRepGProp::SurfaceProperties(s, p);
  else BRepGProp::LinearProperties(s, p);
  return p.CentreOfMass();
}

TEST(TopoNaming, BoxFacesHaveSemanticNames) {
  auto r = kernel::makeBox(60, 40, 30, "B");
  ASSERT_TRUE(r.ok());
  auto top = r.shape.resolve("B.Top");
  ASSERT_TRUE(top);
  gp_Pnt c = centroid(*top);
  EXPECT_NEAR(c.Z(), 30.0, 1e-9);
  EXPECT_NEAR(centroid(*r.shape.resolve("B.Front")).X(), 60.0, 1e-9);
  EXPECT_NEAR(centroid(*r.shape.resolve("B.Right")).Y(), 40.0, 1e-9);
  EXPECT_EQ(r.shape.unnamedFaceCount(), 0u);
}

TEST(TopoNaming, EveryEdgeAndVertexOfABoxHasAUniqueName) {
  auto r = kernel::makeBox(10, 20, 30, "B");
  for (auto type : {TopAbs_EDGE, TopAbs_VERTEX}) {
    TopTools_IndexedMapOfShape m;
    TopExp::MapShapes(r.shape.shape(), type, m);
    std::set<std::string> names;
    for (int i = 1; i <= m.Extent(); ++i) {
      auto n = r.shape.nameOf(m(i));
      ASSERT_TRUE(n);
      names.insert(*n);
      auto back = r.shape.resolve(*n);
      ASSERT_TRUE(back);
      EXPECT_TRUE(back->IsSame(m(i)));
    }
    EXPECT_EQ(names.size(), static_cast<size_t>(m.Extent()));
  }
}

TEST(TopoNaming, NamesPropagateThroughChamfer) {
  auto box = kernel::makeBox(60, 40, 30, "B");
  auto ch = kernel::chamferEdges(box.shape, {"Edge(B.Front|B.Top)"}, 5, "C");
  ASSERT_TRUE(ch.ok()) << ch.error;
  EXPECT_EQ(ch.shape.unnamedFaceCount(), 0u);
  auto chamferFace = ch.shape.resolve("C.Edge(B.Front|B.Top)");
  ASSERT_TRUE(chamferFace);
  // Chamfer face lies along the original top/front edge: X near 60, Z near 30.
  gp_Pnt c = centroid(*chamferFace);
  EXPECT_NEAR(c.X(), 57.5, 1e-6);
  EXPECT_NEAR(c.Z(), 27.5, 1e-6);
  // Edges between the chamfer and modified faces are referencable too.
  EXPECT_TRUE(ch.shape.resolve("Edge(B.Top|C.Edge(B.Front|B.Top))"));
}

// The key regression: edit upstream (box size) and check the chamfer still
// sits on the *same semantic edge*, even though OCCT rebuilds all topology.
TEST(TopoNaming, ChamferFollowsEdgeAfterUpstreamResize) {
  doc::Document d;
  d.parameters().add("W", "60");
  d.addFeature(std::make_unique<doc::BoxFeature>("Box1", "W", "40", "30"));
  d.addFeature(std::make_unique<doc::EdgeBlendFeature>("Chamfer1", doc::EdgeBlendFeature::Kind::Chamfer, "Box1",
                                                       std::vector<std::string>{"Edge(Box1.Front|Box1.Top)"}, "5"));
  for (double w : {60.0, 120.0, 25.0, 300.0}) {
    d.parameters().setExpression("W", std::to_string(w));
    auto rep = d.recompute();
    ASSERT_EQ(rep.failed, 0) << "W=" << w;
    const auto& body = d.feature("Chamfer1")->result.body;
    auto f = body.resolve("Chamfer1.Edge(Box1.Front|Box1.Top)");
    ASSERT_TRUE(f) << "W=" << w;
    gp_Pnt c = centroid(*f);
    EXPECT_NEAR(c.X(), w - 2.5, 1e-6) << "W=" << w;
    EXPECT_NEAR(c.Y(), 20.0, 1e-6);
    EXPECT_NEAR(c.Z(), 27.5, 1e-6);
    EXPECT_NEAR(kernel::shapeStats(body.shape()).volume, w * 40 * 30 - 0.5 * 25 * 40, 1e-6);
  }
}

TEST(TopoNaming, ReferenceThatNoLongerExistsFailsLoudly) {
  auto box = kernel::makeBox(60, 40, 30, "B");
  auto ch = kernel::chamferEdges(box.shape, {"Edge(B.Front|B.Bogus)"}, 5, "C");
  EXPECT_FALSE(ch.ok());
  EXPECT_NE(ch.error.find("could not be resolved"), std::string::npos);
}

TEST(TopoNaming, OversizedChamferReportsError) {
  auto box = kernel::makeBox(10, 10, 10, "B");
  auto ch = kernel::chamferEdges(box.shape, {"Edge(B.Front|B.Top)"}, 50, "C");
  EXPECT_FALSE(ch.ok());
  EXPECT_FALSE(ch.error.empty());
}
