#include "document/Document.h"
#include "document/Features.h"
#include "kernel/Nurbs.h"
#include "kernel/ShapeInfo.h"

#include <BRepCheck_Analyzer.hxx>
#include <gtest/gtest.h>

using namespace hcad;

TEST(Nurbs, PlanarGridBuildsRealBSplineSurface) {
  auto def = kernel::NurbsSurfaceDef::planarGrid(4, 4, 30, 30);
  auto r = kernel::buildNurbsSurface(def, "S");
  ASSERT_TRUE(r.ok()) << r.error;
  EXPECT_EQ(r.surface->UDegree(), 3);
  EXPECT_EQ(r.surface->NbUPoles(), 4);
  EXPECT_EQ(r.surface->NbVKnots(), 2); // single span: 4 poles, degree 3
  EXPECT_TRUE(BRepCheck_Analyzer(r.face.shape()).IsValid());
  EXPECT_NEAR(kernel::shapeStats(r.face.shape()).area, 900.0, 1e-6);
}

TEST(Nurbs, MultiSpanKnotVector) {
  auto def = kernel::NurbsSurfaceDef::planarGrid(7, 5, 60, 40);
  auto r = kernel::buildNurbsSurface(def, "S");
  ASSERT_TRUE(r.ok());
  EXPECT_EQ(r.surface->NbUKnots(), 5); // 7 - 3 + 1
  EXPECT_EQ(r.surface->NbVKnots(), 3);
}

TEST(Nurbs, MovingAControlPointDeformsSurfaceLocally) {
  auto def = kernel::NurbsSurfaceDef::planarGrid(4, 4, 30, 30);
  def.pole(1, 1).SetZ(10);
  auto r = kernel::buildNurbsSurface(def, "S");
  ASSERT_TRUE(r.ok());
  gp_Pnt mid = r.surface->Value(0.33, 0.33);
  EXPECT_GT(mid.Z(), 1.0);
  gp_Pnt corner = r.surface->Value(1.0, 1.0); // clamped: interpolates corner pole
  EXPECT_NEAR(corner.Z(), 0.0, 1e-12);
  EXPECT_NEAR(corner.X(), 30.0, 1e-12);
}

TEST(Nurbs, WeightsMakeSurfaceRational) {
  auto def = kernel::NurbsSurfaceDef::planarGrid(4, 4, 30, 30);
  def.pole(1, 1).SetZ(10);
  def.weights.assign(def.poles.size(), 1.0);
  auto base = kernel::buildNurbsSurface(def, "S");
  def.weights[1 * 4 + 1] = 4.0;
  auto heavy = kernel::buildNurbsSurface(def, "S");
  ASSERT_TRUE(heavy.ok());
  EXPECT_TRUE(heavy.surface->IsURational());
  EXPECT_GT(heavy.surface->Value(0.33, 0.33).Z(), base.surface->Value(0.33, 0.33).Z());
}

TEST(Nurbs, InvalidDefinitionsAreRejected) {
  auto def = kernel::NurbsSurfaceDef::planarGrid(4, 4, 30, 30);
  def.degreeU = 4; // needs >= 5 poles
  EXPECT_FALSE(kernel::buildNurbsSurface(def, "S").ok());
  def = kernel::NurbsSurfaceDef::planarGrid(4, 4, 30, 30);
  def.weights.assign(def.poles.size(), 1.0);
  def.weights[0] = 0.0;
  EXPECT_FALSE(kernel::buildNurbsSurface(def, "S").ok());
}

TEST(Nurbs, ControlPointEditsAreEditableHistory) {
  doc::Document d;
  d.parameters().add("Lift", "15 mm");
  d.addFeature(std::make_unique<doc::NurbsSurfaceFeature>("Surface1", "60", "40", 5, 4, 3));
  auto* edit = d.addFeature(std::make_unique<doc::ControlPointEditFeature>("CPEdit1", "Surface1", 2, 1, "0", "0", "Lift"));
  ASSERT_EQ(d.recompute().failed, 0);
  ASSERT_TRUE(edit->result.nurbs);
  EXPECT_NEAR(edit->result.nurbs->pole(2, 1).Z(), 15.0, 1e-12);
  // Face name of the source surface is kept through edits.
  EXPECT_TRUE(edit->result.body.resolve("Surface1.Surface"));

  d.parameters().setExpression("Lift", "-8 mm");
  d.recompute();
  EXPECT_NEAR(edit->result.nurbs->pole(2, 1).Z(), -8.0, 1e-12);

  // Upstream change that removes the edited control point fails clearly.
  d.feature("Surface1")->param("CountU")->expression = "2";
  d.feature("Surface1")->param("Degree")->expression = "1";
  d.recompute();
  EXPECT_EQ(edit->status, doc::FeatureStatus::Error);
  EXPECT_NE(edit->error.find("no longer exists"), std::string::npos);
}
