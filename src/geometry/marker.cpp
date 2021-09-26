#include "marker.h"

#include "scene.h"

#include <QString>

namespace geometry
{

PMarker Marker::create()
{
    return PMarker(new Marker);
}

PMarker Marker::create(point3d point, PObjectToSelect leader, PLabel follower)
{
    return PMarker(new Marker{point, leader, follower});
}

} // namespace geometry
