#include "point.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

#include <QStringList>

namespace geometry
{

template <>
const point3d point3d::n{};
template <>
const point3d point3d::nx{1, 0, 0};
template <>
const point3d point3d::ny{0, 1, 0};
template <>
const point3d point3d::nz{0, 0, 1};

template <>
const point2d point2d::n{};
template <>
const point2d point2d::nx{1, 0, 0};
template <>
const point2d point2d::ny{0, 1, 0};
template <>
const point2d point2d::nz{0, 0, 1};

template <>
QString point2d::serialSave()
{
    return QString("%1,%2").arg(x, 0, 'f', 12).arg(y, 0, 'f', 12);
}

template <>
QString point3d::serialSave()
{
    return QString("%1,%2,%3").arg(x, 0, 'f', 12).arg(y, 0, 'f', 12).arg(z, 0, 'f', 12);
}

template <>
point2d point2d::serialLoad(QString str, bool *ok)
{
    QStringList lst = str.split(",");
    if(lst.size() != 2)
    {
        if(ok)
            *ok = false;
        return point2d();
    }

    if(ok)
        *ok = true;
    bool ok1, ok2;
    auto x = lst[0].toDouble(&ok1);
    auto y = lst[1].toDouble(&ok2);
    if(!ok1 || !ok2)
    {
        if(ok)
            *ok = false;
        return point2d();
    }
    return point2d(x, y);
}

template <>
point3d point3d::serialLoad(QString str, bool *ok)
{
    QStringList lst = str.split(",");
    if(lst.size() != 3)
    {
        if(ok)
            *ok = false;
        return point3d();
    }

    if(ok)
        *ok = true;
    bool ok1, ok2, ok3;
    auto x = lst[0].toDouble(&ok1);
    auto y = lst[1].toDouble(&ok2);
    auto z = lst[2].toDouble(&ok3);
    if(!ok1 || !ok2 || !ok3)
    {
        if(ok)
            *ok = false;
        return point3d();
    }

    return point3d(x, y, z);
}

template <>
QString point3d::toQString() const
{
    return QString("{%1,%2,%3}").arg(x, 0, 'f', 3).arg(y, 0, 'f', 3).arg(z, 0, 'f', 3);
}

template <>
QString point2d::toQString() const
{
    return QString("{%1,%2,[%3]}").arg(x, 0, 'f', 1).arg(y, 0, 'f', 1).arg(z, 0, 'f', 1);
}

template <>
QPointF point2d::toQPoint() const
{
    return QPointF(x, y);
}

template <>
QPointF point3d::toQPoint() const
{
    return QPointF(x, y);
}

template <>
point2d point2d::fromQPoint(QPointF p)
{
    return point2d(p.x(), p.y());
}

template <>
point3d point3d::fromQPoint(QPointF p)
{
    return point3d(p.x(), p.y());
}

uint qHash(point2d const &p)
{
    return static_cast<uint>(p.x * POINT_PREC) + 1e4 * static_cast<uint>(p.y * POINT_PREC);
}

uint qHash(point3d const &p)
{
    return static_cast<uint>(p.x * POINT_PREC) + 1e4 * static_cast<uint>(p.y * POINT_PREC) +
           1e8 * static_cast<uint>(p.z * POINT_PREC);
}

} // namespace geometry
