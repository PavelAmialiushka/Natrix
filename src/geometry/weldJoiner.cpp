#include "weldJoiner.h"
#include "global.h"
#include "objectFactory.h"
#include "scene.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

CREATE_CLASS_BY_NODELIST(WeldJoiner);

WeldJoiner::WeldJoiner(Scene *sc, point3d p0, point3d dir)
    : Joiner(sc, 2)
{
    nodeAt(0)->setPoint(sc->worldSystem(), p0, dir);
    nodeAt(1)->setPoint(sc->worldSystem(), p0, -dir);
}

PObject WeldJoiner::create(Scene *sc, point3d p0, point3d dir, PObject sample)
{
    PObject object(new WeldJoiner(sc, p0, dir));
    object->setPObject(object);
    if(sample)
        object->applyStyle(sample);
    return object;
}

PObject WeldJoiner::createFromList(Scene *sc, QList<NodeInfo> list)
{
    Q_ASSERT(list.size() >= 1);
    return create(sc, list[0].globalPoint, list[0].direction, NoSample);
}

PObject WeldJoiner::cloneMove(Scene *scene, point3d delta)
{
    return create(scene, globalPoint(0) + delta, direction(0), this->pObject());
}

PObject WeldJoiner::cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle)
{
    auto a  = delta + globalPoint(0).rotate3dAround(center, point3d::nz, angle);
    auto da = direction(0).rotate3dAround(0, point3d::nz, angle);
    return create(scene, a, da, this->pObject());
}

void WeldJoiner::applyStyle(LineInfo info)
{
    setLineStyle(info.lineStyle);
}

void WeldJoiner::draw(GraphicsScene *gscene, GItems &g, int level)
{
    point3d pp = globalPoint(0);
    point3d dd = direction(0);

    point2d p = localPoint(0);
    point2d d = gscene->ws()->toUser(pp + weldStrokeHalfSize * dd);

    point2d a = d.rotate2dAround(p, M_PI_2);
    point2d b = d.rotate2dAround(p, -M_PI_2);

    QPen linePen(QBrush(Qt::black), 1.5, Qt::SolidLine, Qt::RoundCap);
    g.items << GraphicsClipItem::create(gscene, QLineF(a.toQPoint(), b.toQPoint()), linePen);

    if(!level)
        return;

    // рисуем контур
    QList<point2d> points;
    points << a << b;

    enlargeRegion_hull(points, 3);
    g.contur << drawSpline(gscene, points);
}

} // namespace geometry
