#include "solar_core/math/vector3.hpp"
#include "solar_core/export.hpp"

#include <iomanip>
#include <sstream>

namespace SolarSystem::Math {

// Explicit template instantiations for common types
template class SOLAR_CORE_API Vector3<float>;
template class SOLAR_CORE_API Vector3<double>;
template class SOLAR_CORE_API Vector3<long double>;

// Free function implementations
template <typename T>
std::string to_string(const Vector3<T>& vec) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(6);
  oss << "(" << vec.x() << ", " << vec.y() << ", " << vec.z() << ")";
  return oss.str();
}

// Explicit instantiations for to_string
template SOLAR_CORE_API std::string to_string(const Vector3<float>&);
template SOLAR_CORE_API std::string to_string(const Vector3<double>&);
template SOLAR_CORE_API std::string to_string(const Vector3<long double>&);

}  // namespace SolarSystem::Math
