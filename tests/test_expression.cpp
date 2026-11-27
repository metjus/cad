#include "core/Expression.h"
#include "document/ParameterTable.h"
#include <gtest/gtest.h>

using namespace hcad;

static std::optional<double> none(const std::string&) { return std::nullopt; }

TEST(Expression, ArithmeticAndPrecedence) {
  EXPECT_DOUBLE_EQ(*evaluateExpression("1 + 2 * 3", none).value, 7.0);
  EXPECT_DOUBLE_EQ(*evaluateExpression("(1 + 2) * 3", none).value, 9.0);
  EXPECT_DOUBLE_EQ(*evaluateExpression("-2 ^ 2", none).value, -4.0);
  EXPECT_DOUBLE_EQ(*evaluateExpression("2 ^ 3 ^ 2", none).value, 512.0);
}

TEST(Expression, UnitsConvertToMillimetres) {
  EXPECT_DOUBLE_EQ(*evaluateExpression("10 mm", none).value, 10.0);
  EXPECT_DOUBLE_EQ(*evaluateExpression("1 in", none).value, 25.4);
  EXPECT_DOUBLE_EQ(*evaluateExpression("2cm + 5mm", none).value, 25.0);
  EXPECT_DOUBLE_EQ(*evaluateExpression("0.5 m", none).value, 500.0);
}

TEST(Expression, ErrorsAreReportedNotThrown) {
  EXPECT_FALSE(evaluateExpression("1 / 0", none).ok());
  EXPECT_FALSE(evaluateExpression("1 +", none).ok());
  EXPECT_FALSE(evaluateExpression("Foo * 2", none).ok());
  EXPECT_NE(evaluateExpression("Foo * 2", none).error.find("Foo"), std::string::npos);
  EXPECT_FALSE(evaluateExpression("(1 + 2", none).ok());
}

TEST(Expression, ReferencedIdentifiers) {
  auto ids = referencedIdentifiers("Width * 0.5 + Gap - 2 mm");
  EXPECT_EQ(ids, (std::set<std::string>{"Width", "Gap"}));
}

TEST(ParameterTable, DrivenParameters) {
  doc::ParameterTable t;
  EXPECT_TRUE(t.add("Width", "100 mm").empty());
  EXPECT_TRUE(t.add("Height", "Width * 0.5").empty());
  t.evaluate();
  EXPECT_DOUBLE_EQ(*t.value("Height"), 50.0);
  t.setExpression("Width", "4 in");
  t.evaluate();
  EXPECT_DOUBLE_EQ(*t.value("Height"), 50.8);
}

TEST(ParameterTable, RejectsBadNamesAndDetectsCycles) {
  doc::ParameterTable t;
  EXPECT_FALSE(t.add("1abc", "1").empty());
  EXPECT_FALSE(t.add("mm", "1").empty());
  t.add("A", "B + 1");
  t.add("B", "A * 2");
  t.add("C", "5");
  t.evaluate();
  EXPECT_FALSE(t.value("A").has_value());
  EXPECT_FALSE(t.value("B").has_value());
  EXPECT_DOUBLE_EQ(*t.value("C"), 5.0);
  EXPECT_NE(t.error("A").find("Circular"), std::string::npos);
}
