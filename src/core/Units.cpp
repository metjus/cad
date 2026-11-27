#include "core/Units.h"
#include <cstdio>

namespace hcad {

std::optional<double> lengthUnitFactor(std::string_view s) {
  if (s == "mm") return 1.0;
  if (s == "cm") return 10.0;
  if (s == "m") return 1000.0;
  if (s == "in" || s == "\"") return kMmPerInch;
  return std::nullopt;
}

double toDisplay(double mm, LengthUnit u) { return u == LengthUnit::Inch ? mm / kMmPerInch : mm; }
double fromDisplay(double v, LengthUnit u) { return u == LengthUnit::Inch ? v * kMmPerInch : v; }
std::string unitSuffix(LengthUnit u) { return u == LengthUnit::Inch ? "in" : "mm"; }

std::string formatLength(double mm, LengthUnit unit, int decimals) {
  char buf[64];
  std::snprintf(buf, sizeof buf, "%.*f %s", decimals, toDisplay(mm, unit), unitSuffix(unit).c_str());
  return buf;
}

} // namespace hcad
