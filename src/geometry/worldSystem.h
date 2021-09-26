#ifndef PLANE_H
#define PLANE_H

#include "point.h"

#include <QSharedPointer>

namespace geometry
{

typedef QSharedPointer<class WorldSystem> PWorldSystem;

PWorldSystem wsByName(int type);

class WorldSystem
{
protected:
    WorldSystem();

public:
    virtual ~WorldSystem();

    static PWorldSystem createOrtoWorldSystem();

    // a - наклон оси z на зрителя
    // b - поворот относительно оси z
    // a,b=45,60 - изометрия
    // a,b=80,60 - диметрия (одна ось близка к горизонтали)
    static PWorldSystem createIsoplaneWorldSystem(double a, double b);

    // преобразование в точку, лежащую на прямой (start,direction)
    point3d toGlobal_atline(const point2d &pt2d, const point3d &start, const point3d &end);

    // преобазование в точку, лежащую на плоскости образованную тремя точками
    point3d toGlobal_atplane(const point2d &pt2d,
                             const point3d &a,
                             const point3d &b,
                             const point3d &c);

    // расстояние от 2d точки до 3d точки на плоскости
    double distance2d(point2d pt2d, point3d pt3d);

    double xangle() const;
    double yangle() const;

    point3d convert(point2d const &) const;
    point2d convert(point3d const &) const;

public:
    // преобразование к 3d виду
    virtual point3d toGlobal(const point2d &pt2d) const = 0;

    // преобразование к пользовательскому плоскому виду
    virtual point2d toUser(const point3d &pt3d) const = 0;
};

bool planeMakeSense(PWorldSystem pw, int m);

class wsLine
{
    PWorldSystem pw;
    point3d      a, b;

public:
    wsLine(PWorldSystem pw, point3d const &a, point3d const &b);

public:
    point3d convert(point2d const &) const;
    point2d convert(point3d const &) const;
};

class wsPlane
{
    PWorldSystem pw;
    point3d      a, b, c;

public:
    wsPlane(PWorldSystem pw, point3d const &a, point3d const &b, point3d const &c);

public:
    point3d convert(point2d const &) const;
    point2d convert(point3d const &) const;
};

} // namespace geometry

#endif // PLANE_H
