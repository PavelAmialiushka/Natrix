#include "worldSystem.h"
#include "pointProcedures.h"
#include "worldSystemImpl.h"

#include <cmath>

#include <stdexcept>

namespace geometry
{

PWorldSystem wsByName(int type)
{
    if(type == 2)
        return WorldSystem::createIsoplaneWorldSystem(20, 60);
    else if(type == 3)
        return WorldSystem::createOrtoWorldSystem();

    Q_ASSERT(type == 1);
    return WorldSystem::createIsoplaneWorldSystem(45, 55);
}

PWorldSystem WorldSystem::createOrtoWorldSystem()
{
    return PWorldSystem(new OrtoWorldSystem());
}

// 45. 60 - изометрия
// 80 60 - диметрия :)

PWorldSystem WorldSystem::createIsoplaneWorldSystem(double a, double b)
{
    return PWorldSystem(new IsoplaneWorldSystem(a, b));
}

WorldSystem::WorldSystem()
{
}

double WorldSystem::xangle() const
{
    point2d x = toUser(point3d(100, 0, 0));
    return x.angle() * 180 / M_PI;
}

double WorldSystem::yangle() const
{
    point2d y = toUser(point3d(0, 100, 0));
    return y.angle() * 180 / M_PI;
}

point3d WorldSystem::toGlobal_atline(const point2d &pt2d, const point3d &start, const point3d &end)
{
    // процируем точку на 2д линию-проекцию прямой
    point2d start2   = toUser(start);
    point2d result2d = pt2d.project_to_line(start2, start2 + toUser(end - start));

    // преобразуем к 3d виду
    return toGlobal(result2d);
}

point3d WorldSystem::toGlobal_atplane(const point2d &pt2d,
                                      const point3d &a,
                                      const point3d &b,
                                      const point3d &c)
{
    // точки образуют плоскость
    Q_ASSERT(!(a - b).cross(a - c).empty());

    point2d a2 = toUser(a), b2 = toUser(b), c2 = toUser(c);

    // определяем вспомогательную точку x, которая нахдится на прямой bc и
    // которая является визуальным пересечением прямых a,p и b,c
    bool    insegment, nonparallel;
    point2d x2 = cross_lines2d(a2, pt2d, b2, c2, &insegment, &nonparallel);
    if(!nonparallel)
        return a;

    point3d x;
    // поскольку distance не учитывает знак, делаем это вручную
    if(c2.distance(x2) > b2.distance(x2))
    {
        // определяем ее 3d положение на прямой bc
        double fx = c2.distance(b2);
        if(fx)
            fx = c2.distance(x2) / fx;
        x = c + (b - c).resized(b.distance(c) * fx);
    }
    else
    {
        double fx = b2.distance(c2);
        if(fx)
            fx = b2.distance(x2) / fx;
        x = b + (c - b).resized(b.distance(c) * fx);
    }

    // задача свелась к нахождению проекции на линии
    return toGlobal_atline(pt2d, a, x);
}

double WorldSystem::distance2d(point2d pt2d, point3d pt3d)
{
    return toUser(pt3d).distance(pt2d);
}

point3d WorldSystem::convert(point2d const &x) const
{
    return toGlobal(x);
}
point2d WorldSystem::convert(point3d const &x) const
{
    return toUser(x);
}

/////////////////////////////////////////////////////////////////////

bool planeMakeSense(PWorldSystem pw, int m)
{
    bool isx = !(point3d(1, 0, 0) >> *pw).empty();
    bool isy = !(point3d(0, 1, 0) >> *pw).empty();
    bool isz = !(point3d(0, 0, 1) >> *pw).empty();

    switch(m)
    {
    case 1: // xy
        if(isx && isy)
            return 1;
        return 0;
    case 2: // xz
        if(isx && isz)
            return 1;
        return 0;
    case 3: // yz
        if(isy && isz)
            return 1;
        return 0;
    }
    return 1;
}

/////////////////////////////////////////////////////////////////////

wsLine::wsLine(PWorldSystem pw, point3d const &a, point3d const &b)
    : pw(pw)
    , a(a)
    , b(b)
{
}

point3d wsLine::convert(point2d const &x) const
{
    return pw->toGlobal_atline(x, a, b);
}

point2d wsLine::convert(point3d const &x) const
{
    return pw->toUser(x);
}

//////////////////////////////////////////////////////////////////////////

wsPlane::wsPlane(PWorldSystem pw, point3d const &a, point3d const &b, point3d const &c)
    : pw(pw)
    , a(a)
    , b(b)
    , c(c)
{
}

point3d wsPlane::convert(point2d const &x) const
{
    return pw->toGlobal_atplane(x, a, b, c);
}

point2d wsPlane::convert(point3d const &x) const
{
    return pw->toUser(x);
}

/////////////////////////////////////////////////////////////////////

OrtoWorldSystem::OrtoWorldSystem()
{
}

point3d OrtoWorldSystem::toGlobal(const point2d &pt) const
{
    return point3d(pt.x, pt.y, pt.z);
}
point2d OrtoWorldSystem::toUser(const point3d &pt) const
{
    return point2d(pt.x, pt.y, pt.z);
}

//////////////////////////////

IsoplaneWorldSystem::IsoplaneWorldSystem(double a, double b)
    : alpha(a)
    , beta(b)
{
    sina = sin(a * M_PI / 180);
    cosa = cos(a * M_PI / 180);
    sinb = sin(b * M_PI / 180);
    cosb = cos(b * M_PI / 180);

    m11 = -sina;
    m12 = cosa;
    m13 = 0;
    m21 = cosa * cosb;
    m22 = cosb * sina;
    m23 = -sinb;
    m31 = sinb * cosa;
    m32 = sina * sinb;
    m33 = cosb;

    b11 = m22 * m33 - m23 * m32;
    b12 = m32 * m13 - m12 * m33;
    b13 = m12 * m23 - m13 * m22;
    b21 = m23 * m31 - m21 * m33;
    b22 = m33 * m11 - m13 * m31;
    b23 = m13 * m21 - m11 * m23;
    b31 = m21 * m32 - m31 * m22;
    b32 = m31 * m12 - m11 * m32;
    b33 = m11 * m22 - m12 * m21;

    D = m11 * m22 * m33 + m12 * m23 * m31 + m13 * m21 * m32 - m13 * m22 * m31 - m23 * m32 * m11 -
        m33 * m12 * m21;

    if(fabs(D) < 1E-6)
        throw std::runtime_error("zero denominant");
}

point2d IsoplaneWorldSystem::toUser(const point3d &g) const
{
    double x = m11 * g.x + m12 * g.y + m13 * g.z;
    double y = m21 * g.x + m22 * g.y + m23 * g.z;
    double z = m31 * g.x + m32 * g.y + m33 * g.z;

    return point2d(x, y, z);
}

point3d IsoplaneWorldSystem::toGlobal(const point2d &u) const
{
    double D1 = b11 * u.x + b12 * u.y + b13 * u.z;
    double D2 = b21 * u.x + b22 * u.y + b23 * u.z;
    double D3 = b31 * u.x + b32 * u.y + b33 * u.z;
    return point3d(D1 / D, D2 / D, D3 / D);
}

WorldSystem::~WorldSystem()
{
}

} // namespace geometry
