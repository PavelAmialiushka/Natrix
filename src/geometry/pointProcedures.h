#ifndef POINTPROCEDURES_H
#define POINTPROCEDURES_H

#include "point.h"

namespace geometry
{

point3d cross_lines3d(point3d r11,
                      point3d r12,
                      point3d r21,
                      point3d r22,
                      bool *  has_cross    = 0,
                      bool *  non_parallel = 0);

point2d cross_lines2d(point2d a,
                      point2d b,
                      point2d c,
                      point2d d,
                      bool *  in_segment = 0,
                      bool *  exists     = 0);

point2d replacePointBase3to3(point2d point,
                             point2d r1,
                             point2d r2,
                             point2d r3,
                             point2d d1,
                             point2d d2,
                             point2d d3);

} // namespace geometry

#endif // POINTPROCEDURES_H
