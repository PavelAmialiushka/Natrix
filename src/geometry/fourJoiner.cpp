#include "fourJoiner.h"
#include "global.h"
#include "line.h"
#include "objectFactory.h"
#include "scene.h"
#include "sceneProcedures.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

CREATE_CLASS_BY_NODELIST(FourJoiner);

FourJoiner::FourJoiner(Scene *sc, point3d p0, point3d d1, point3d d2)
    : Joiner(sc, 4)
    , teeStyle_(sc->defaultLineInfo().teeStyle)
{
    if(d1 == d2 || d1 == -d2)
    {
        Q_ASSERT(!"incorrect FourJoiner");
        throw std::runtime_error("incorrect FourJoiner");
    }
    nodeAt(0)->setPoint(sc->worldSystem(), p0, d1);
    nodeAt(1)->setPoint(sc->worldSystem(), p0, -d1);
    nodeAt(2)->setPoint(sc->worldSystem(), p0, d2);
    nodeAt(3)->setPoint(sc->worldSystem(), p0, -d2);
}

PObject FourJoiner::create(Scene * sc,
                           point3d p0,
                           point3d d1,
                           point3d d2,
                           PObject sample,
                           double  a,
                           double  b,
                           double  c,
                           double  d)
{
    auto four = new FourJoiner(sc, p0, d1, d2);

    //    if (tee->teeStyle() == TeeWeldedStyle)
    //    {
    //        if (!a) a = teeStrokeDistance;
    //        if (!b) b = teeStrokeDistance;
    //        if (!c) c = teeStrokeDistance;
    //        if (!d) d = teeStrokeDistance;
    //    }

    if(a >= 0)
        four->setWeldPosition(0, a);
    if(b >= 0)
        four->setWeldPosition(1, b);
    if(c >= 0)
        four->setWeldPosition(2, c);
    if(d >= 0)
        four->setWeldPosition(3, c);

    PObject object(four);
    object->setPObject(object);
    if(sample)
        object->applyStyle(sample);
    return object;
}

PObject FourJoiner::createFromList(Scene *sc, QList<NodeInfo> list)
{
    Q_ASSERT(list.size() == 4);

    // коррекция поведения v1
    if(!list[0].direction.isParallel(list[1].direction))
        qSwap(list[0], list[2]);

    return create(sc,
                  list[0].globalPoint,
                  list[0].direction,
                  list[2].direction,
                  NoSample,
                  list[0].weldPosition,
                  list[1].weldPosition,
                  list[2].weldPosition,
                  list[3].weldPosition);
}

bool FourJoiner::applyStyle(ScenePropertyValue v)
{
    if(v.assignTo(ScenePropertyType::TeeStyle, teeStyle_))
        return true;

    return Joiner::applyStyle(v);
}

ScenePropertyList FourJoiner::getStyles()
{
    ScenePropertyList r;
    r << Joiner::getStyles();
    r << ScenePropertyValue(ScenePropertyType::TeeStyle, teeStyle_);
    return r;
}

PObject FourJoiner::cloneMove(Scene *scene, point3d delta)
{
    auto result = create(scene,
                         globalPoint(0) + delta,
                         direction(0),
                         direction(2),
                         this->pObject(),
                         weldPosition(0),
                         weldPosition(1),
                         weldPosition(2),
                         weldPosition(3));
    return result;
}

PObject FourJoiner::cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle)
{
    auto a  = delta + globalPoint(0).rotate3dAround(center, point3d::nz, angle);
    auto da = direction(0).rotate3dAround(0, point3d::nz, angle);
    auto dc = direction(2).rotate3dAround(0, point3d::nz, angle);

    auto result = create(scene,
                         a,
                         da,
                         dc,
                         this->pObject(),
                         weldPosition(0),
                         weldPosition(1),
                         weldPosition(2),
                         weldPosition(3));
    return result;
}

void FourJoiner::applyStyle(PObject other)
{
    Joiner::applyStyle(other);
}

void FourJoiner::applyStyle(LineInfo info)
{
    setLineStyle(info.lineStyle);
}

void FourJoiner::saveObject(QVariantMap &map)
{
    Joiner::saveObject(map);
}

bool FourJoiner::loadObject(QVariantMap map)
{
    Joiner::loadObject(map);

    return 1;
}

void FourJoiner::draw(GraphicsScene *gscene, GItems &g, int level)
{
    QPen         weldLinePen(QBrush(Qt::black), 1.5, Qt::SolidLine, Qt::RoundCap);
    WorldSystem &ws = *gscene->ws();

    //    GItemsList  welds(gscene);
    //    for(int index=0; index<4; ++index)
    //    {
    //        point3d pp = globalPoint(index);
    //        point3d dd = direction(index);
    //        point2d p = localPoint(index);

    //        point2d n = ((pp + bendStrokeHalfSize * dd) >> ws) - p;
    //        n = n.rotate2dAround(0, M_PI_2);
    //        point2d d0 = weldPoint(index) >> ws;

    //        point2d a = d0 + n;
    //        point2d b = d0 - n;

    //        if (weldPosition(index) != 0 || teeStyle_ == TeeWeldedStyle)
    //            welds << GraphicsClipItem::create(gscene, QLineF(a.toQPoint(), b.toQPoint()),
    //            weldLinePen);
    //    }

    //    bool dashDot = lineStyle_ == LineDashDotStyle;
    // рисуем швы
    //    if (!dashDot && teeStyle_ == TeeWeldedStyle)
    //        g.items << welds;

    auto a  = weldPoint(0);
    auto b  = weldPoint(1);
    auto c  = weldPoint(2);
    auto d  = weldPoint(3);
    auto ce = globalPoint(0);

    g.items << GraphicsClipItem::create(
        gscene, QLineF(a * ws >> toQPoint(), b * ws >> toQPoint()), lineStyle_);
    g.items << GraphicsClipItem::create(
        gscene, QLineF(c * ws >> toQPoint(), d * ws >> toQPoint()), lineStyle_);

    bool ok = false;
    auto n1 =
        (a * ws - ce * ws).rotate2dAround(point2d(), M_PI_2).resized(teeStrokeHalfSize / 2, &ok);
    auto n3 =
        (c * ws - ce * ws).rotate2dAround(point2d(), M_PI_2).resized(teeStrokeHalfSize / 2, &ok);

    // жирный
    //    if (!dashDot && teeStyle_==TeeCastStyle)
    //    {
    //        QPainterPath path1;
    //        path1.moveTo( (a * ws - n1).toQPoint() );
    //        path1.lineTo( (a * ws + n1).toQPoint() );
    //        path1.lineTo( (b * ws + n1).toQPoint() );
    //        path1.lineTo( (b * ws - n1).toQPoint() );
    //        path1.lineTo( (a * ws - n1).toQPoint() );

    //        path1.moveTo( (d * ws - n3).toQPoint() );
    //        path1.lineTo( (c * ws - n3).toQPoint() );
    //        path1.lineTo( (c * ws + n3).toQPoint() );
    //        path1.lineTo( (d * ws + n3).toQPoint() );
    //        g.items << GraphicsClipItem::create(gscene, path1, graphics::linePen());
    //    }

    if(!level)
        return;

    // рисуем контур
    QList<point2d> points;
    points << a * ws - n1 << a * ws + n1 << b * ws + n1 << b * ws - n1;
    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);

    points.clear();
    points << c * ws - n3 << c * ws + n3 << d * ws + n3 << d * ws - n3;
    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);

    compressContur(g.contur);
}

double FourJoiner::interactWith(PObject other) const
{
    if(!other.dynamicCast<Line>())
        return 0;

    auto node = commonNode(pObject(), other);
    int  n    = node == nodeAt(0) ? 0 : node == nodeAt(1) ? 1 : 2;
    return std::max<double>(weldPosition(n), teeStrokeDistance);
}

} // namespace geometry
