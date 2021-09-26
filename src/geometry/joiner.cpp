#include "joiner.h"
#include "element.h"
#include "global.h"
#include "line.h"
#include "scene.h"

#include <QVariant>

namespace geometry
{

Joiner::Joiner(Scene *sc, int n)
    : Object(sc, n)
    , lineStyle_(sc->defaultLineInfo().lineStyle)
{
    while(n-- > 0)
        setWeldPosition(n, teeStrokeDistance);
}

double Joiner::weldPosition(int n) const
{
    if(0 <= n && n <= nodeCount())
        return nodes_[n]->weldPosition();

    Q_ASSERT(!"WTF");
    return 0;
}

void Joiner::setWeldPosition(int n, double d)
{
    if(0 <= n && n <= nodeCount())
        nodes_[n]->setWeldPosition(d);
    else
        Q_ASSERT(!"WTF");
}

point3d Joiner::weldPoint(int n) const
{
    if(0 <= n && n <= nodeCount())
        return globalPoint(n) +
               direction(n) * std::max<double>(nodes_[n]->weldPosition(), bendStrokeDistance);

    Q_ASSERT(!"WTF");
    return globalPoint(0);
}

int Joiner::lineStyle() const
{
    return lineStyle_;
}

void Joiner::setLineStyle(int s)
{
    lineStyle_ = s;
}

bool Joiner::applyStyle(ScenePropertyValue v)
{
    if(v.assignTo(ScenePropertyType::LineStyle, lineStyle_))
        return true;

    return false;
}

ScenePropertyList Joiner::getStyles()
{
    ScenePropertyList r;
    r << ScenePropertyValue(ScenePropertyType::LineStyle, lineStyle_);
    return r;
}

double Joiner::distanceToPoint(int index) const
{
    return 0.1;
}

double Joiner::width() const
{
    return lineWidth;
}

point3d Joiner::distantPoint(int index) const
{
    return globalPoint(0) + distanceToPoint(index) * direction(index);
}

void Joiner::applyStyle(PObject other)
{
    if(auto line = other.dynamicCast<Line>())
    {
        setLineStyle(line->lineStyle());
    }
    else if(auto joiner = other.dynamicCast<Joiner>())
    {
        setLineStyle(joiner->lineStyle());
    }
}

void Joiner::saveObject(QVariantMap &map)
{
    Object::saveObject(map);

    map["lineStyle"] = lineStyle_;
}

bool Joiner::loadObject(QVariantMap map)
{
    Object::loadObject(map);

    if(map.contains("lineStyle"))
    {
        int s = map["lineStyle"].toInt();
        setLineStyle(s);
    }

    return 1;
}

static int objectStatus(PObject a)
{
    return a.dynamicCast<Joiner>() ? 0 : a.dynamicCast<Element>() ? 1 : 2;
}

bool joinersFirst(PObject a, PObject b)
{
    return objectStatus(a) < objectStatus(b);
}

} // namespace geometry
