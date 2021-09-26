#include "endCupJoiner.h"
#include "global.h"
#include "objectFactory.h"
#include "scene.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

CREATE_CLASS_BY_NODELIST(EndCupJoiner);

EndCupJoiner::EndCupJoiner(Scene *sc, point3d p0, point3d dir)
    : Joiner(sc, 1)
    , endStyle_(sc->defaultLineInfo().endStyle)
{
    nodeAt(0)->setPoint(sc->worldSystem(), p0, dir);
}

PObject EndCupJoiner::create(Scene *sc, point3d p0, point3d dir)
{
    PObject object(new EndCupJoiner(sc, p0, dir));
    object->setPObject(object);
    return object;
}

PObject EndCupJoiner::createFromList(Scene *sc, QList<NodeInfo> list)
{
    Q_ASSERT(list.size() == 1);
    return create(sc, list[0].globalPoint, list[0].direction);
}

int EndCupJoiner::endStyle() const
{
    return endStyle_;
}

void EndCupJoiner::setEndStyle(int s)
{
    endStyle_ = s;
}

bool EndCupJoiner::applyStyle(ScenePropertyValue v)
{
    if(v.assignTo(ScenePropertyType::EndStyle, endStyle_))
        return true;

    return Joiner::applyStyle(v);
}

ScenePropertyList EndCupJoiner::getStyles()
{
    ScenePropertyList r;
    r << Joiner::getStyles();
    r << ScenePropertyValue(ScenePropertyType::EndStyle, endStyle_);
    return r;
}

PObject EndCupJoiner::cloneMove(Scene *scene, point3d delta)
{
    auto result = create(scene, globalPoint(0) + delta, direction(0));
    result->applyStyle(pObject());
    return result;
}

PObject EndCupJoiner::cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle)
{
    auto a  = delta + globalPoint(0).rotate3dAround(center, point3d::nz, angle);
    auto da = direction(0).rotate3dAround(0, point3d::nz, angle);

    auto result = create(scene, a, da);
    result->applyStyle(pObject());
    return result;
}

void EndCupJoiner::applyStyle(PObject other)
{
    Joiner::applyStyle(other);
    if(auto end = other.dynamicCast<EndCupJoiner>())
    {
        setEndStyle(end->endStyle());
    }
}

void EndCupJoiner::applyStyle(LineInfo info)
{
    setLineStyle(info.lineStyle);
    setEndStyle(info.endStyle);
}

void EndCupJoiner::saveObject(QVariantMap &map)
{
    Joiner::saveObject(map);

    map["endStyle"] = endStyle_;
}

bool EndCupJoiner::loadObject(QVariantMap map)
{
    Joiner::loadObject(map);

    if(map.contains("endStyle"))
    {
        int s = map["endStyle"].toInt();
        setEndStyle(s);
    }

    return 1;
}

double EndCupJoiner::distanceToPoint(int) const
{
    return 0.1;
}

void EndCupJoiner::draw(GraphicsScene *gscene, GItems &g, int level)
{
    QPen linePen(QBrush(Qt::black), 1.5, Qt::SolidLine, Qt::RoundCap);

    point3d pp = nodeAt(0)->globalPoint();
    point3d dd = nodeAt(0)->direction();

    point2d p = nodeAt(0)->localPoint();
    point2d d = gscene->ws()->toUser(pp + endCupStrokeHalfSize * dd);

    GItemsList items(gscene);
    if(endStyle_ == EndNormalStyle || endStyle_ == EndNoStyle)
    {
        point2d a = d.rotate2dAround(p, M_PI_2);
        point2d b = d.rotate2dAround(p, -M_PI_2);

        point2d dt = gscene->ws()->toUser(pp + endCupStrokeHalfSize * dd / 2);
        point2d m  = dt.rotate2dAround(p, M_PI_2 * 60 / 90);
        point2d n  = dt.rotate2dAround(p, -M_PI_2 * 120 / 90);

        QList<point2d> points;
        points << a << m << p << n << b;

        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        path.moveTo(points[0].x, points[0].y);
        for(int index = 1; index < points.size(); ++index)
        {
            path.lineTo(points[index].x, points[index].y);
        }

        items << GraphicsClipItem::create(gscene, path, linePen);

        if(lineStyle_ != LineDashDotStyle && endStyle_ != EndNoStyle)
            g.items << items;
    }

    if(!level)
        return;

    // рисуем контур
    QList<point2d> points = makePointListFromItemsList(items);

    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);
}

} // namespace geometry
