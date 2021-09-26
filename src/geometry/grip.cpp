#include "grip.h"

namespace geometry
{

Grip::Grip()
{
    temporal_ = true;
}

bool Grip::isTemporal() const
{
    return temporal_;
}

} // namespace geometry
