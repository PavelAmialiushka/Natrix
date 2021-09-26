#ifndef POINTPROCEDURES_CPP
#define POINTPROCEDURES_CPP

#include "pointProcedures.h"

namespace geometry
{

point3d cross_lines3d(point3d r11,
                      point3d r12,
                      point3d r21,
                      point3d r22,
                      bool *  has_cross,
                      bool *  non_parallel)
{
    bool    ok1, ok2;
    point3d r1 = r11;
    point3d a1 = (r12 - r11).normalized(&ok1);
    point3d r2 = r21;
    point3d a2 = (r22 - r21).normalized(&ok2);
    point3d dr = r1 - r2;

    if(!ok1 || !ok2)
    {
        if(has_cross)
            *has_cross = false;
        if(non_parallel)
            *non_parallel = true;
        return point3d();
    }

    // http://www.mathworks.com/matlabcentral/newsreader/view_thread/246420
    // nA = dot(cross(a2,r11-r21),cross(a1,a2));
    // nB = dot(cross(a1,r11-r21),cross(a1,a2));
    // d = dot(cross(a1,a2),cross(a1,a2));
    // A0 = r11 + (nA/d)*(a1);
    // B0 = r21 + (nB/d)*(a2);

    point3d craa = a1.cross(a2);
    double  nA   = a2.cross(dr).dot(craa);
    double  nB   = a1.cross(dr).dot(craa);
    double  det  = craa.dot(craa);

    if(std::abs(det) < POINT_PREC)
    {
        if(has_cross)
            *has_cross = false; // не пересекаются
        if(non_parallel)
            *non_parallel = false; // параллельны
        return point3d();
    }
    if(non_parallel)
        *non_parallel = true; // не параллельны

    point3d A0 = r11 + a1 * nA / det;
    point3d B0 = r21 + a2 * nB / det;
    if(A0 == B0)
    {
        if(has_cross)
            *has_cross = true; // пересекаются
        return A0;
    }
    if(has_cross)
        *has_cross = false; // скрещиваются
    return point3d();
}

point2d cross_lines2d(point2d a, point2d b, point2d c, point2d d, bool *in_segment, bool *exists)
{
    double x11 = a.x, y11 = a.y;
    double x12 = b.x, y12 = b.y;
    double x21 = c.x, y21 = c.y;
    double x22 = d.x, y22 = d.y;

    if(in_segment)
        *in_segment = 1;
    if(exists)
        *exists = 1;

    double dx1 = x12 - x11, dy1 = y12 - y11;
    double dx2 = x22 - x21, dy2 = y22 - y21;
    double dxx = x11 - x21, dyy = y11 - y21;

    double div = dy2 * dx1 - dx2 * dy1;
    if(fabs(div) < POINT_PREC * POINT_PREC)
    {
        // параллельны
        if(in_segment)
            *in_segment = 0;
        if(exists)
            *exists = 0;
        return point2d();
    }

    double mul1 = dx1 * dyy - dy1 * dxx;
    double mul2 = dx2 * dyy - dy2 * dxx;

    if(div > 0)
    {
        // если первый отрезок пересекается за своими границами...
        if(mul1 < 0 || mul1 > div)
        {
            if(in_segment)
                *in_segment = 0;
        }

        // если второй отрезок пересекается за своими границами...
        if(mul2 < 0 || mul2 > div)
        {
            if(in_segment)
                *in_segment = 0;
        }
    }
    else
    {
        // если первый отрезок пересекается за своими границами...
        if(mul1 > 0 || -mul1 > -div)
        {
            if(in_segment)
                *in_segment = 0;
        }

        // если второй отрезок пересекается за своими границами...
        if(mul2 > 0 || -mul2 > -div)
        {
            if(in_segment)
                *in_segment = 0;
        }
    }

    // всетаки пересекаются
    return point2d(x11 + mul2 * dx1 / div, y11 + mul2 * dy1 / div);
}

point2d replacePointBase3to3(point2d p,
                             point2d r1,
                             point2d r2,
                             point2d r3,
                             point2d d1,
                             point2d d2,
                             point2d d3)
{
    bool   ok;
    double posA = p.fraction_at_projection(r1, r2, &ok);
    if(!ok)
        return d1;

    auto   rc   = r3.project_to_line(r1, r2);
    double posB = p.fraction_at_projection(rc, r3, &ok);
    if(!ok)
        return d1;

    auto dc = d3.project_to_line(d1, d2);

    auto A = d1.partition_by(d2, posA);
    auto B = dc.partition_by(d3, posB);

    auto result = A + (B - dc);

    return result;
}

} // namespace geometry

#endif // POINTPROCEDURES_CPP
