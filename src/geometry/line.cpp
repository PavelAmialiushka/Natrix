#include "line.h"
#include "global.h"
#include "joiner.h"
#include "objectFactory.h"
#include "scene.h"
#include "worldSystem.h"

#include "graphics.h"
#include "graphicsClipItem.h"
#include "graphicsScene.h"

#include <QDebug>

namespace geometry
{

CREATE_CLASS_BY_NODELIST(Line);

PObject Line::create(Scene *sc, point3d p1, point3d p2, PObject sample)
{
    // точки отличаются
    if(p1.distance(p2) < lineMinimumSize - POINT_PREC)
    {
        Q_ASSERT(!"слишком короткая линия");
        return PObject();
    }

    PObject object(new Line(sc, p1, p2));
    object->setPObject(object);

    if(sample)
        object->applyStyle(sample);
    return object;
}

PObject Line::createFromList(Scene *sc, QList<NodeInfo> list)
{
    Q_ASSERT(list.size() == 2);
    return create(sc, list[0].globalPoint, list[1].globalPoint, NoSample);
}

Line::Line(Scene *scene, point3d p1, point3d p2)
    : Object(scene, 2)
{
    lineStyle_ = scene->defaultLineInfo().lineStyle;
    Q_ASSERT(p1.distance(p2) >= lineMinimumSize - POINT_PREC);

    point3d dr = (p1 - p2).normalized();
    nodes_[0]->setPoint(scene->worldSystem(), p1, dr);
    nodes_[1]->setPoint(scene->worldSystem(), p2, -dr);
}

PObject Line::cloneMove(Scene *scene, point3d delta)
{
    PObject result = create(scene, globalPoint(0) + delta, globalPoint(1) + delta, this->pObject());
    return result;
}

PObject Line::cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle)
{
    auto    a      = delta + globalPoint(0).rotate3dAround(center, point3d::nz, angle);
    auto    b      = delta + globalPoint(1).rotate3dAround(center, point3d::nz, angle);
    PObject result = create(scene, a, b, this->pObject());
    return result;
}

int Line::lineStyle() const
{
    return lineStyle_;
}

void Line::setLineStyle(int s)
{
    lineStyle_ = s;
}

bool Line::apply(ScenePropertyValue v)
{
    if(v.type == ScenePropertyType::LineStyle)
    {
        if(v.current == lineStyle_)
            return false;

        setLineStyle(v.current);
        return true;
    }
    return false;
}

void Line::applyStyle(LineInfo info)
{
    setLineStyle(info.lineStyle);
}

bool Line::applyStyle(ScenePropertyValue v)
{
    if(v.assignTo(ScenePropertyType::LineStyle, lineStyle_))
        return true;

    return false;
}

ScenePropertyList Line::getStyles()
{
    ScenePropertyList r;
    r << ScenePropertyValue(ScenePropertyType::LineStyle, lineStyle_);
    return r;
}

double Line::width() const
{
    return lineWidth;
}

void Line::applyStyle(PObject object)
{
    if(auto line = object.dynamicCast<Line>())
    {
        setLineStyle(line->lineStyle());
    }
    else if(auto join = object.dynamicCast<Joiner>())
    {
        setLineStyle(join->lineStyle());
    }
}

void Line::draw(GraphicsScene *gscene, GItems &g, int level)
{
    point3d ga = globalPoint(0), gb = globalPoint(1);

    auto nei1 = neighbour(0);
    auto nei2 = neighbour(1);

    if(nei1)
        ga -= direction(0).resized(nei1->getInteraction(pObject()));
    if(nei2)
        gb -= direction(1).resized(nei2->getInteraction(pObject()));

    auto a = ga >> *gscene->ws();
    auto b = gb >> *gscene->ws();

    g.items << GraphicsClipItem::create(gscene, makeQLine(a, b), lineStyle_);

    if(!level)
        return;

    // контур
    QList<point2d> points;
    points << a << b;
    enlargeRegion_hull(points, 4);
    g.contur << drawSpline(gscene, points, Qt::NoPen);
}

void Line::saveObject(QVariantMap &map)
{
    Object::saveObject(map);

    map["lineStyle"] = lineStyle_;
}

bool Line::loadObject(QVariantMap map)
{
    Object::loadObject(map);

    if(map.contains("lineStyle"))
    {
        int s = map["lineStyle"].toInt();
        setLineStyle(s);
    }

    return 1;
}

} // namespace geometry
