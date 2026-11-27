#include "document/Document.h"
#include "document/Features.h"
#include "kernel/ShapeInfo.h"
#include <gtest/gtest.h>

using namespace hcad;

TEST(BoxFeature, CreatesValidBRepSolid) {
  doc::Document d;
  d.parameters().add("W", "60 mm");
  d.addFeature(std::make_unique<doc::BoxFeature>("Box1", "W", "40", "W * 0.5"));
  auto rep = d.recompute();
  ASSERT_EQ(rep.failed, 0);
  auto st = kernel::shapeStats(d.feature("Box1")->result.body.shape());
  EXPECT_TRUE(st.valid);
  EXPECT_EQ(st.solids, 1);
  EXPECT_EQ(st.faces, 6);
  EXPECT_EQ(st.edges, 12);
  EXPECT_EQ(st.vertices, 8);
  EXPECT_NEAR(st.volume, 60.0 * 40.0 * 30.0, 1e-6);
}

TEST(BoxFeature, EditingParameterRecomputes) {
  doc::Document d;
  d.parameters().add("W", "60 mm");
  d.addFeature(std::make_unique<doc::BoxFeature>("Box1", "W", "40", "W * 0.5"));
  d.recompute();
  d.parameters().setExpression("W", "2 in");
  d.recompute();
  auto st = kernel::shapeStats(d.feature("Box1")->result.body.shape());
  EXPECT_NEAR(st.volume, 50.8 * 40.0 * 25.4, 1e-6);
}

TEST(BoxFeature, InvalidDimensionReportsErrorAndDependentsFail) {
  doc::Document d;
  d.parameters().add("W", "0 mm");
  d.addFeature(std::make_unique<doc::BoxFeature>("Box1", "W", "40", "10"));
  d.addFeature(std::make_unique<doc::EdgeBlendFeature>("Chamfer1", doc::EdgeBlendFeature::Kind::Chamfer, "Box1",
                                                       std::vector<std::string>{"Edge(Box1.Front|Box1.Top)"}, "1"));
  auto rep = d.recompute();
  EXPECT_EQ(rep.failed, 2);
  EXPECT_EQ(d.feature("Box1")->status, doc::FeatureStatus::Error);
  EXPECT_FALSE(d.feature("Box1")->error.empty());
  EXPECT_EQ(d.feature("Chamfer1")->status, doc::FeatureStatus::Error);
}

TEST(BoxFeature, UnknownParameterIsAFeatureError) {
  doc::Document d;
  d.addFeature(std::make_unique<doc::BoxFeature>("Box1", "Missing", "40", "10"));
  d.recompute();
  EXPECT_EQ(d.feature("Box1")->status, doc::FeatureStatus::Error);
  EXPECT_NE(d.feature("Box1")->error.find("Missing"), std::string::npos);
}

TEST(History, SuppressAndRollback) {
  doc::Document d;
  doc::buildDemoDocument(d);
  d.recompute();
  const double fullVol = kernel::shapeStats(d.feature("Fillet1")->result.body.shape()).volume;
  const double boxVol = kernel::shapeStats(d.feature("Box1")->result.body.shape()).volume;
  EXPECT_LT(fullVol, boxVol);

  d.feature("Chamfer1")->suppressed = true;
  d.recompute();
  EXPECT_EQ(d.feature("Chamfer1")->status, doc::FeatureStatus::Suppressed);
  // Fillet references only Box faces, so it still applies with the chamfer suppressed.
  EXPECT_EQ(d.feature("Fillet1")->status, doc::FeatureStatus::Ok);

  d.feature("Chamfer1")->suppressed = false;
  d.setRollbackIndex(0);
  d.recompute();
  EXPECT_EQ(d.feature("Chamfer1")->status, doc::FeatureStatus::RolledBack);
  auto tips = d.tipFeatures();
  ASSERT_FALSE(tips.empty());
  EXPECT_EQ(tips.front()->id(), "Box1");
}

TEST(History, DemoDocumentComputesCompletely) {
  doc::Document d;
  doc::buildDemoDocument(d);
  auto rep = d.recompute();
  EXPECT_EQ(rep.failed, 0) << (rep.messages.empty() ? "" : rep.messages.front());
  for (const auto& f : d.features()) EXPECT_EQ(f->status, doc::FeatureStatus::Ok) << f->id() << ": " << f->error;
  EXPECT_TRUE(kernel::isValidSolid(d.feature("Fillet1")->result.body.shape()));
}
