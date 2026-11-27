#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace hcad {

// All lengths are stored internally in millimetres. Angles in degrees at the
// document level, converted to radians only when handed to the kernel.
enum class LengthUnit { Millimetre, Inch };

constexpr double kMmPerInch = 25.4;

// Factor that converts a value expressed in `unit` into millimetres.
// Returns nullopt for an unknown suffix.
std::optional<double> lengthUnitFactor(std::string_view suffix);

double toDisplay(double mm, LengthUnit unit);
double fromDisplay(double value, LengthUnit unit);
std::string unitSuffix(LengthUnit unit);
std::string formatLength(double mm, LengthUnit unit, int decimals = 3);

} // namespace hcad
