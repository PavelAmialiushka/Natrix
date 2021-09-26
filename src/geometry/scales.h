#ifndef SCALES_H
#define SCALES_H

namespace geometry
{

inline double toScale(int sizeFactor)
{
    return pow(1.25, qMin(4, qMax(-4, sizeFactor)));
}

} // namespace geometry

#endif // SCALES_H
