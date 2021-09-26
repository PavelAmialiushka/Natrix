#ifndef WORLDSYSTEMIMPL_H
#define WORLDSYSTEMIMPL_H

#include "point.h"
#include "worldSystem.h"

namespace geometry
{

/////////////////////////////////////////////////////////////////////////

class OrtoWorldSystem : public WorldSystem
{
public:
    OrtoWorldSystem();

public:
    virtual point3d toGlobal(const point2d &pt) const;
    virtual point2d toUser(const point3d &pt) const;
};

/////////////////////////////////////////////////////////////////////////

class IsoplaneWorldSystem : public WorldSystem
{
    double alpha, beta;
    double cosa, cosb, sina, sinb;
    double m11, m12, m13, m21, m22, m23, m31, m32, m33;
    double b11, b12, b13, b21, b22, b23, b31, b32, b33;
    double D;

public:
    IsoplaneWorldSystem(double a, double b);

public:
    virtual point3d toGlobal(const point2d &pt) const;
    virtual point2d toUser(const point3d &pt) const;
};

/////////////////////////////////////////////////////////////////////////

} // namespace geometry
#endif // WORLDSYSTEMIMPL_H
