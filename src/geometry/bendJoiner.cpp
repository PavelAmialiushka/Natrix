#include "bendJoiner.h"
#include "global.h"
#include "graphics.h"
#include "graphicsScene.h"
#include "line.h"
#include "object.h"
#include "objectFactory.h"
#include "scene.h"
#include "sceneProcedures.h"

namespace geometry
{

CREATE_CLASS_BY_NODELIST(BendJoiner);

BendJoiner::BendJoiner(Scene *sc, point3d p0, point3d d1, point3d d2)
    : Joiner(sc, 2)
    , bendStyle_(sc->defaultLineInfo().bendStyle)
{
    if(d1 == d2 || d1 == -d2)
    {
        Q_ASSERT(!"incorrect BendJoiner");
        throw std::runtime_error("incorrect BendJoiner");
    }
    nodeAt(0)->setPoint(sc->worldSystem(), p0, d1);
    nodeAt(1)->setPoint(sc->worldSystem(), p0, d2);
}

PObject BendJoiner::create(Scene * sc,
                           point3d p0,
                           point3d d1,
                           point3d d2,
                           PObject sample,
                           double  w1,
                           double  w2)
{
    auto bend = new BendJoiner(sc, p0, d1, d2);
    if(w1 >= 0)
        bend->setWeldPosition(0, w1);
    if(w2 >= 0)
        bend->setWeldPosition(1, w2);
    PObject object(bend);
    object->setPObject(object);
    if(sample)
        object->applyStyle(sample);
    return object;
}

PObject BendJoiner::createFromList(Scene *sc, QList<NodeInfo> list)
{
    Q_ASSERT(list.size() == 2);
    return create(sc,
                  list[0].globalPoint,
                  list[0].direction,
                  list[1].direction,
                  Object::NoSample,
                  list[0].weldPosition,
                  list[1].weldPosition);
}

int BendJoiner::bendStyle() const
{
    return bendStyle_;
}

void BendJoiner::setBendStyle(int s)
{
    bendStyle_ = s;
}

bool BendJoiner::applyStyle(ScenePropertyValue v)
{
    if(v.assignTo(ScenePropertyType::BendStyle, bendStyle_))
        return true;

    return Joiner::applyStyle(v);
}

ScenePropertyList BendJoiner::getStyles()
{
    ScenePropertyList r;
    r << Joiner::getStyles();
    r << ScenePropertyValue(ScenePropertyType::BendStyle, bendStyle_);
    return r;
}

PObject BendJoiner::cloneMove(Scene *scene, point3d delta)
{
    auto result = create(scene,
                         globalPoint(0) + delta,
                         direction(0),
                         direction(1),
                         this->pObject(),
                         weldPosition(0),
                         weldPosition(1));
    result->applyStyle(pObject());
    return result;
}

PObject BendJoiner::cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle)
{
    auto a  = delta + globalPoint(0).rotate3dAround(center, point3d::nz, angle);
    auto da = direction(0).rotate3dAround(0, point3d::nz, angle);
    auto db = direction(1).rotate3dAround(0, point3d::nz, angle);

    auto result = create(scene, a, da, db, this->pObject(), weldPosition(0), weldPosition(1));
    return result;
}

void BendJoiner::draw(GraphicsScene *gscene, GItems &g, int level)
{
    QPen         weldLinePen(QBrush(Qt::black), 1.5, Qt::SolidLine, Qt::RoundCap);
    WorldSystem &ws = *gscene->ws();

    GItemsList welds(gscene);

    // рисуем сварные швы по краям
    for(int index = 0; index < 2; ++index)
    {
        point3d pp = globalPoint(index);
        point3d dd = direction(index);
        point2d p  = localPoint(index);

        point2d n  = ((pp + bendStrokeHalfSize * dd) >> ws) - p;
        n          = n.rotate2dAround(0, M_PI_2);
        point2d d0 = weldPoint(index) >> ws;

        point2d a = d0 + n;
        point2d b = d0 - n;

        if(weldPosition(index) != 0)
            welds << GraphicsClipItem::create(
                gscene, QLineF(a.toQPoint(), b.toQPoint()), weldLinePen);
    }

    bool dashDot = lineStyle_ == LineDashDotStyle;
    if(!dashDot && bendStyle_ == BendWeldedStyle)
    {
        g.items << welds;
    }

    // плавный отвод
    point3d ce = globalPoint(0);
    point3d ax = weldPoint(0);
    point3d bx = weldPoint(1);
    point3d a  = (ce + ax) / 2;
    point3d b  = (ce + bx) / 2;

    QPainterPath path;
    path.moveTo(ax * ws >> toQPoint());
    path.lineTo(a * ws >> toQPoint());
    path.quadTo(ce * ws >> toQPoint(), b * ws >> toQPoint());
    path.lineTo(bx * ws >> toQPoint());
    g.items << GraphicsClipItem::create(gscene, path, lineStyle_);

    if(!level)
        return;

    // залитый контур
    bool ok = 0;
    auto n1 = (direction(0) * ws).rotate2dAround(0, M_PI_2).resized(bendStrokeHalfSize / 2, &ok);
    auto n2 = (direction(1) * ws).rotate2dAround(0, -M_PI_2).resized(bendStrokeHalfSize / 2, &ok);
    if(!dashDot && bendStyle_ == BendCastStyle)
    {
        QPainterPath path1;
        path1.moveTo((ax * ws - n1).toQPoint());
        path1.lineTo((ax * ws + n1).toQPoint());
        path1.lineTo((a * ws + n1).toQPoint());
        path1.quadTo((ce * ws + (n1 + n2) / 2).toQPoint(), (b * ws + n2).toQPoint());
        path1.lineTo((bx * ws + n2).toQPoint());
        path1.lineTo((bx * ws - n2).toQPoint());
        path1.lineTo((b * ws - n2).toQPoint());
        path1.quadTo((ce * ws - (n1 + n2) / 2).toQPoint(), (a * ws - n1).toQPoint());
        path1.lineTo((ax * ws - n1).toQPoint());
        g.items << GraphicsClipItem::create(gscene, path1, graphics::linePen());
    }

    // рисуем контур
    QList<point2d> points;
    points << (ce * ws + (n1 + n2) / 2) << (ce * ws - (n1 + n2) / 2) << (ax * ws - n1)
           << (ax * ws + n1);
    points = makeAHull(points);
    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);

    points.clear();
    points << (ce * ws + (n1 + n2) / 2) << (ce * ws - (n1 + n2) / 2) << (bx * ws - n2)
           << (bx * ws + n2);
    points = makeAHull(points);
    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);

    compressContur(g.contur);
}

double BendJoiner::interactWith(PObject other) const
{
    if(!other.dynamicCast<Line>())
        return 0;

    int n = commonNode(pObject(), other) == nodeAt(0) ? 0 : 1;
    return std::max<double>(weldPosition(n), bendStrokeDistance);
}

void BendJoiner::applyStyle(PObject other)
{
    Joiner::applyStyle(other);
    if(auto bend = other.dynamicCast<BendJoiner>())
    {
        setBendStyle(bend->bendStyle());
    }
}

void BendJoiner::applyStyle(LineInfo info)
{
    setLineStyle(info.lineStyle);
    setBendStyle(info.bendStyle);
}

void BendJoiner::saveObject(QVariantMap &map)
{
    Joiner::saveObject(map);

    map["bendStyle"] = bendStyle_;
}

bool BendJoiner::loadObject(QVariantMap map)
{
    Joiner::loadObject(map);

    if(map.contains("bendStyle"))
    {
        int s = map["bendStyle"].toInt();
        setBendStyle(s);
    }

    return 1;
}

} // namespace geometry
