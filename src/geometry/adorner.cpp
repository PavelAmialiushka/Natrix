#include "adorner.h"

namespace geometry
{

Adorner::Adorner()
{
}

bool Adorner::isTemporal() const
{
    return true;
}

double Adorner::distance(point2d const &pt)
{
    return 0.0;
}

} // namespace geometry
