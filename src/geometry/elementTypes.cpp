#include "elementTypes.h"
#include "objectFactory.h"
#include "scene.h"

#include "global.h"
#include "pointProcedures.h"

#include <QStringList>
#include <typeinfo>

#include "elementFactory.h"
#include "graphics.h"
#include "graphicsScene.h"
#include "line.h"

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

namespace geometry
{

///////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
struct flange
{
    point2d p;
    point2d top_center, bottom_center;
    point2d top_internal, bottom_internal;
    point2d top_out, bottom_out;
    point2d internal, out;
    bool    isVisible;

    flange(point2d p, point2d normal, point2d m, bool is)
        : p(p)
        , isVisible(is)
    {
        top_internal = p + normal + m;               // внутренний фланец
        top_out      = p + normal;                   // наружный фланец
        top_center   = (top_internal + top_out) / 2; // середина

        bottom_internal = p - normal + m;
        bottom_out      = p - normal;
        bottom_center   = (bottom_internal + bottom_out) / 2;

        internal = (top_internal + bottom_internal) / 2;
        out      = (top_out + bottom_out) / 2;
    }
    point2d bottom_external() const
    {
        return isVisible ? bottom_out : bottom_internal;
    }
    point2d top_external() const
    {
        return isVisible ? top_out : top_internal;
    }

    QList<point2d> points(bool invert = false) const
    {
        QList<point2d> result;
        if(isVisible)
            result << bottom_internal << bottom_external() << top_external() << top_internal;
        else
            result << bottom_internal << out << top_internal;

        if(invert)
        {
            for(int index = result.size() / 2; index-- > 0;)
                std::swap(result[index], result[result.size() - index - 1]);
        }
        return result;
    }

    GItemsList drawExternal(GraphicsScene *gscene, Element *self)
    {
        GItemsList items{gscene};

        if(isVisible)
        {
            items << GraphicsClipItem::create(
                gscene, makeQLine(top_out, bottom_out), graphics::elementPen());
        }
        else if(self->info().elementJointType == ElementJoint::Welded)
        {
            items << GraphicsClipItem::create(
                gscene,
                makeQLine((top_out + bottom_out) / 2, (top_internal + bottom_internal) / 2),
                graphics::linePen());
        }

        return items;
    }

    GItemsList drawInternal(GraphicsScene *gscene, Element *self)
    {
        GItemsList items{gscene};

        items << GraphicsClipItem::create(
            gscene, makeQLine(top_internal, bottom_internal), graphics::elementPen());

        return items;
    }
};
} // namespace
///////////////////////////////////////////////////////////////////////////////////////////////

CREATE_CLASS_ELEMENT_SUBCLASS(ValveElement);
ElementParams ValveElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    true, /*canNonBeFlipped*/
    true  /*isFlangeMutable*/
};

ValveElement::ValveElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    info_.setFlanges();
    setNodeList(0, list);
}

PElement ValveElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new ValveElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void ValveElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));

    // направление "перпендикуляра"
    auto normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;

    // направление арматуры
    bool   ok = 0;
    auto   m  = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1));

    point2d ce = (p1 + p2) / 2;

    // если нужно рисовать дальние фланцы
    g.items << left.drawExternal(gscene, this);
    g.items << right.drawExternal(gscene, this);

    // непосредственно арматура
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.top_internal, right.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.bottom_internal, right.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.top_internal, left.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.bottom_internal, left.top_internal), graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << ce << left.points();
    enlargeRegion_hull(points1, 2);
    g.contur << drawSpline(gscene, points1);

    QList<point2d> points2;
    points2 << ce << right.points();
    enlargeRegion_hull(points2, 2);
    g.contur << drawSpline(gscene, points2);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points1 + points2, p1, p2);

    compressContur(g.contur);
}

// обратный клапан
CREATE_CLASS_ELEMENT_SUBCLASS(CheckValveElement);
ElementParams CheckValveElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true,  /*isFlangeMutable*/
};

CheckValveElement::CheckValveElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    info_.setFlanges();
    setNodeList(0, list);
}

PElement CheckValveElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new CheckValveElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void CheckValveElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));
    point2d ce = center() >> ws;

    // направление "перпендикуляра"
    auto normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;

    // направление фланца
    bool   ok = false;
    auto   m  = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1));

    // если нужно рисовать дальние фланцы

    g.items << left.drawExternal(gscene, this);
    g.items << right.drawExternal(gscene, this);

    // арматура
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.top_internal, left.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.top_internal, right.bottom_internal), graphics::elementPen());

    // диагональ
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.bottom_internal, left.top_internal), graphics::elementPen());

    // стрелки
    point2d f = (left.top_internal - right.bottom_internal).normalized() * flangesHalfSize *
                info().scale * 1.5;
    auto f1 = f.rotate2dAround(point2d(), 15 * M_PI / 180);
    auto f2 = f.rotate2dAround(point2d(), -15 * M_PI / 180);

    g.items << GraphicsClipItem::create(
        gscene,
        makeQLine(right.bottom_internal, right.bottom_internal + f1),
        graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene,
        makeQLine(right.bottom_internal, right.bottom_internal + f2),
        graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << ce << left.points();
    enlargeRegion_hull(points1, 2);
    g.contur << drawSpline(gscene, points1);

    QList<point2d> points2;
    points2 << ce << right.points();
    enlargeRegion_hull(points2, 2);
    g.contur << drawSpline(gscene, points2);

    QList<point2d> points3;
    points3 << left.top_internal << right.bottom_internal + f1 << right.bottom_internal
            << right.bottom_internal + f2;
    enlargeRegion_hull(points3, 2);
    g.contur << drawSpline(gscene, points3);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points1 + points2 + points3, p1, p2);

    compressContur(g.contur);
}

///////////////////////////////////////////////////////////////////////////////////////////////

CREATE_CLASS_ELEMENT_SUBCLASS(KipValveElement);
ElementParams KipValveElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true   /*isFlangeMutable*/
};

KipValveElement::KipValveElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    info_.setFlanges();
    setNodeList(0, list);
}

PElement KipValveElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new KipValveElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void KipValveElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));

    // направление "перпендикуляра"
    auto normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;

    // направление арматуры
    bool   ok = false;
    auto   m  = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1));

    point2d ce = (p1 + p2) / 2;

    // рисуем то, что определяет клапан КИП
    auto cem0 = ce - normal * 2;
    g.items << GraphicsClipItem::create(gscene, makeQLine(ce, cem0), graphics::elementPen());

    auto cem1 = cem0 + (p2 - p1).resized(normal.length(), &ok);
    auto cem2 = cem1.rotate2dAround(cem0, M_PI);
    g.items << GraphicsClipItem::create(gscene, makeQLine(cem1, cem2), graphics::elementPen());

    // если нужно рисовать дальние фланцы
    g.items << left.drawExternal(gscene, this);
    g.items << right.drawExternal(gscene, this);

    // непосредственно арматура
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.top_internal, right.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.bottom_internal, right.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.top_internal, left.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.bottom_internal, left.top_internal), graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << ce << left.points();
    enlargeRegion_hull(points1, 2);
    g.contur << drawSpline(gscene, points1);

    QList<point2d> points2;
    points2 << ce << right.points();
    enlargeRegion_hull(points2, 2);
    g.contur << drawSpline(gscene, points2);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points1 + points2, p1, p2);

    compressContur(g.contur);
}
///////////////////////////////////////////////////////////////////////////////////////////////

CREATE_CLASS_ELEMENT_SUBCLASS(ElectricValveElement);
ElementParams ElectricValveElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true   /*isFlangeMutable*/
};

ElectricValveElement::ElectricValveElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    info_.setFlanges();
    setNodeList(0, list);
}

PElement ElectricValveElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new ElectricValveElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void ElectricValveElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));

    // направление "перпендикуляра"
    auto normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;

    // направление арматуры
    bool   ok = false;
    auto   m  = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1));

    point2d ce = (p1 + p2) / 2;

    // рисуем то, что определяет электрозадвижку
    auto cem0 = ce - normal;
    g.items << GraphicsClipItem::create(gscene, makeQLine(ce, cem0), graphics::elementPen());

    auto cex = cem0 - normal.resized(1.5 * flangesWidth * info().scale, &ok);
    g.items << drawCircle(gscene, cex, (cex - cem0).length(), graphics::elementPen());

    // если нужно рисовать дальние фланцы
    g.items << left.drawExternal(gscene, this);
    g.items << right.drawExternal(gscene, this);

    // непосредственно арматура
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.top_internal, right.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.bottom_internal, right.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.top_internal, left.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.bottom_internal, left.top_internal), graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << ce << left.points();
    enlargeRegion_hull(points1, 2);
    g.contur << drawSpline(gscene, points1);

    QList<point2d> points2;
    points2 << ce << right.points();
    enlargeRegion_hull(points2, 2);
    g.contur << drawSpline(gscene, points2);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points1 + points2, p1, p2);

    compressContur(g.contur);
}

///////////////////////////////////////////////////////////////////////////////////////////////

CREATE_CLASS_ELEMENT_SUBCLASS(Valve3Element);
ElementParams Valve3Element::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    true, /*canNonBeFlipped*/
    true  /*isFlangeMutable*/
};

Valve3Element::Valve3Element()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    list << NodeInfo{point3d::nx * size / 2 + point3d::ny * size / 2, point3d::ny};
    info_.setFlanges();
    setNodeList(0, list);
}

PElement Valve3Element::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new Valve3Element);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void Valve3Element::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));
    point2d p3 = ws.toUser(globalPoint(2));

    // направление "перпендикуляра"
    auto normal  = elementNormal(ws, direction(2)) * info().scale;
    auto normal2 = elementNormal(ws, direction(0)) * info().scale;

    // направление арматуры
    bool ok = 0;
    auto m  = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;

    point2d ce = (p1 + p2) / 2;
    auto    k  = (ce - p3).normalized(&ok) * flangesWidth * info().scale;
    flange  left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1)),
        third(p3, normal2, k, isFlangeVisible(2));

    // если нужно рисовать дальние фланцы
    g.items << left.drawExternal(gscene, this);
    g.items << right.drawExternal(gscene, this);
    g.items << third.drawExternal(gscene, this);

    // непосредственно арматура
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.top_internal, right.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.bottom_internal, right.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(right.top_internal, left.bottom_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.bottom_internal, left.top_internal), graphics::elementPen());

    g.items << GraphicsClipItem::create(
        gscene, makeQLine(third.bottom_internal, ce), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(third.bottom_internal, third.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ce, third.top_internal), graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << ce << left.points();
    enlargeRegion_hull(points1, 2);
    g.contur << drawSpline(gscene, points1);

    QList<point2d> points2;
    points2 << right.points() << ce;
    enlargeRegion_hull(points2, 2);
    g.contur << drawSpline(gscene, points2);

    QList<point2d> points3;
    points3 << ce << third.points();
    enlargeRegion_hull(points3, 2);
    g.contur << drawSpline(gscene, points3);

    // обрисовка по выделенными линиями
    // if (lineStyle_ == LineSelectedStyle)
    //    g.items << makeSimplePathFromPoints(points1);
    // compressContur(g.contur);

    QPainterPath path = makeSimplePathFromPoints(points1) + makeSimplePathFromPoints(points2) +
                        makeSimplePathFromPoints(points3);

    g.contur << gscene->addPath(path);
}

// переход
CREATE_CLASS_ELEMENT_SUBCLASS(TransElement);
ElementParams TransElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Welded,
    false, /*canNonBeFlipped*/
    false, /*isFlangeMutable*/
};

TransElement::TransElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 2;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);
}

PElement TransElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new TransElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void TransElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));

    // направление "перпендикуляра"
    auto normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;

    g.items << GraphicsClipItem::create(
        gscene, makeQLine(p1 + normal, p1 - normal), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(p1 + normal, p2), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(p1 - normal, p2), graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> points;
    points << p1 + normal << p1 - normal << p2;
    enlargeRegion_hull(points, 2);

    g.contur << drawSpline(gscene, points);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points, p1, p2);
}

// эксцентричный переход
CREATE_CLASS_ELEMENT_SUBCLASS(ExTransElement);
ElementParams ExTransElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Welded,
    false, /*canNonBeFlipped*/
    false, /*isFlangeMutable*/
};

ExTransElement::ExTransElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 2;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    // положение лишней ноды важно, для определения центра элемента
    // три ноды не должны лежать на одной плоскости
    list << NodeInfo{point3d::nx * size / 2 + point3d::nz, point3d::nz, -1};
    setNodeList(0, list);
}

PElement ExTransElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new ExTransElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void ExTransElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));

    // направление "перпендикуляра"
    const double size    = 2 * flangesHalfSize * info().scale;
    auto         normal  = ws.toUser(globalPoint(0) + direction(2) * size) - localPoint(0);
    auto         normal3 = normal * 0.3;

    g.items << GraphicsClipItem::create(gscene, makeQLine(p1 + normal, p1), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(p1 + normal, p2 + normal3), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(p2, p1), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(p2, p2 + normal3), graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> points;
    points << p1 + normal << p1 << p2 << p2 + normal3;
    enlargeRegion_hull(points, 2);

    g.contur << drawSpline(gscene, points);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points, p1, p2);
}

// сферическая заглушка
CREATE_CLASS_ELEMENT_SUBCLASS(SemisphereElement);
ElementParams SemisphereElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Welded,
    false, /*canNonBeFlipped*/
    false, /*isFlangeMutable*/
};

SemisphereElement::SemisphereElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 4;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);
}

PElement SemisphereElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new SemisphereElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void SemisphereElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));

    // направление "перпендикуляра"
    auto normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;

    // фланцы
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(p1 + normal, p1 - normal), graphics::elementPen());

    // эллипс
    g.items << drawSemiIsocircle(gscene, p1 + normal, p1 - normal, p2, graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> points = makePointListFromItemsList(g.items);
    enlargeRegion_oneSide(points, 2);
    g.contur << drawSpline(gscene, points);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points, p1, p2);
}

// фланцевая пара
CREATE_CLASS_ELEMENT_SUBCLASS(FlangePairElement);
ElementParams FlangePairElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    true,  /*canNonBeFlipped*/
    false, /*isFlangeMutable*/
};

FlangePairElement::FlangePairElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = flangesWidth;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement FlangePairElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new FlangePairElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void FlangePairElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));

    // направление "перпендикуляра"
    auto normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;

    // направление арматуры
    bool   ok = false;
    auto   m  = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1));

    // если нужно рисовать дальние фланцы
    g.items << left.drawExternal(gscene, this);
    g.items << right.drawExternal(gscene, this);

    if(!level)
        return;

    // контур
    QList<point2d> points;
    points << left.points() << right.points(1);
    points = makeAHull(points);
    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points, p1, p2);
}

// фланцы с заглушкой
CREATE_CLASS_ELEMENT_SUBCLASS(FlangePairBlindElement);
ElementParams FlangePairBlindElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    false, /*isFlangeMutable*/
};

FlangePairBlindElement::FlangePairBlindElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = flangesWidth * 2;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement FlangePairBlindElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new FlangePairBlindElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void FlangePairBlindElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));

    // середина элемента
    point2d px = (p1 + p2) / 2;

    // направление "перпендикуляра"
    auto normal  = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;
    auto normal2 = normal * 2;

    if(!level)
        return;

    // контур
    QList<point2d> points;
    points << p1 + normal << p1 - normal << px - normal2 << p2 - normal << p2 + normal
           << px + normal2;
    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);

    // фланцы
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(p1 + normal, p1 - normal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(p2 + normal, p2 - normal), graphics::elementPen());
    // заглушка
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(px + normal2, px - normal2), graphics::elementPen());
    g.items << drawCircle(
        gscene, px + normal2, blindCircleRadius * info().scale, QPen(), graphics::blackBrush());

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points, p1, p2);
}

// диафрагма
CREATE_CLASS_ELEMENT_SUBCLASS(DiaphragmElement);
ElementParams DiaphragmElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    false, /*isFlangeMutable*/
};

DiaphragmElement::DiaphragmElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 2;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement DiaphragmElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new DiaphragmElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void DiaphragmElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));
    auto    ce = (p1 + p2) / 2;

    // направление "перпендикуляра"
    bool   ok     = false;
    auto   normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;
    auto   m      = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1));

    if(!level)
        return;

    // контур
    QList<point2d> points;
    points << p1 + normal << p1 - normal << p2 - normal << p2 + normal;
    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points, p1, p2);

    g.items << drawSemiIsocircle(gscene, left.p, right.p, ce + normal, graphics::elementPen());
    g.items << drawSemiIsocircle(gscene, left.p, right.p, ce - normal, graphics::elementPen());

    auto r1 = p2 + normal - p1;
    r1      = r1.resized(r1.length() * 0.8, &ok);
    auto r2 = p2 - normal - p1;
    r2      = r2.resized(r2.length() * 0.8, &ok);

    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.p, left.p + r1), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(left.p, left.p + r2), graphics::elementPen());

    if(left.isVisible)
        g.items << GraphicsClipItem::create(
            gscene, makeQLine(left.bottom_out, left.top_out), graphics::elementPen());
    if(right.isVisible)
        g.items << GraphicsClipItem::create(
            gscene, makeQLine(right.bottom_out, right.top_out), graphics::elementPen());
}

// конденсатоотводчик
CREATE_CLASS_ELEMENT_SUBCLASS(CondTapperElement);
ElementParams CondTapperElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Flanged,
    true, /*canNonBeFlipped*/
    true, /*isFlangeMutable*/
};

CondTapperElement::CondTapperElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize * 0.75;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement CondTapperElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new CondTapperElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void CondTapperElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = localPoint(0);
    point2d p2 = localPoint(1);
    auto    ce = (p1 + p2) / 2;

    // направление "перпендикуляра"
    bool   ok     = false;
    auto   normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;
    auto   m      = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1));

    // если нужно рисовать дальние фланцы
    g.items << left.drawExternal(gscene, this);
    g.items << right.drawExternal(gscene, this);

    // окружность
    auto a = ce + normal;
    auto b = ce - normal;

    // внутренние фланцы
    if(info_.elementJointType == ElementJoint::Flanged)
    {
        g.items << GraphicsClipItem::create(
            gscene, makeQLine(left.top_internal, left.bottom_internal), graphics::elementPen());
        g.items << GraphicsClipItem::create(
            gscene, makeQLine(right.top_internal, right.bottom_internal), graphics::elementPen());
    }

    // наружные фланцы
    g.items << drawSemiIsocircle(gscene,
                                 left.internal,
                                 right.internal,
                                 a,
                                 graphics::elementPen(),
                                 a.y < b.y ? graphics::transparentBrush() : graphics::blackBrush());
    g.items << drawSemiIsocircle(gscene,
                                 left.internal,
                                 right.internal,
                                 b,
                                 graphics::elementPen(),
                                 a.y < b.y ? graphics::blackBrush() : graphics::transparentBrush());

    if(!level)
        return;

    // контур
    QList<point2d> points;
    points << left.points() << right.points();
    points = makeAHull(points);
    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points, p1, p2);
}

// коппенсатор
CREATE_CLASS_ELEMENT_SUBCLASS(CancelerElement);
ElementParams CancelerElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Welded,
    true,  /*canNonBeFlipped*/
    false, /*isFlangeMutable*/
};

CancelerElement::CancelerElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize * 0.75;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);
    info_.setFlanges();
}

PElement CancelerElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new CancelerElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void CancelerElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // начало и конец арматуры
    point2d p1 = localPoint(0);
    point2d p2 = localPoint(1);

    // направление "перпендикуляра"
    auto normal = bestElementNormal(ws, globalPoint(0), globalPoint(1)) * info().scale;

    // направление арматуры
    bool   ok = false;
    auto   m  = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0)), right(p2, normal, -m, isFlangeVisible(1));

    // если нужно рисовать дальние фланцы
    g.items << left.drawExternal(gscene, this);
    g.items << right.drawExternal(gscene, this);

    constexpr int N = 4;
    point2d       pts[N + 1];
    pts[0] = left.internal;
    pts[N] = right.internal;
    for(int index = 1; index < N; ++index)
        pts[index] = pts[0].partition_by(pts[N], index / (N + 0.0));

    QList<point2d> points;
    points << pts[0] + normal << pts[N] + normal;
    points << pts[0] - normal << pts[N] - normal;
    for(int index = 0; index <= N; ++index)
        points << pts[index] - normal << pts[index] + normal;

    for(int index = 0; index < points.size(); index += 2)
        g.items << GraphicsClipItem::create(
            gscene, makeQLine(points[index], points[index + 1]), graphics::elementPen());

    if(!level)
        return;

    // контур
    QList<point2d> hullPoints;
    hullPoints << left.points() << right.points();
    hullPoints = makeAHull(hullPoints);
    enlargeRegion_hull(hullPoints, 2);
    g.contur << drawSpline(gscene, hullPoints);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, hullPoints, p1, p2);
}

// ППК
CREATE_CLASS_ELEMENT_SUBCLASS(SafetyValveElement);
ElementParams SafetyValveElement::params = {
    ElementConfiguration::CornerType,
    ElementJoint::Flanged,
    true, /*canNonBeFlipped*/
    true  /*isFlangeMutable*/
};

SafetyValveElement::SafetyValveElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 2;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::ny * size, point3d::ny};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement SafetyValveElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new SafetyValveElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void SafetyValveElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    bool    cross  = 0;
    point3d center = cross_lines3d(globalPoint(0),
                                   globalPoint(0) - direction(0),
                                   globalPoint(1),
                                   globalPoint(1) - direction(1),
                                   &cross,
                                   0);
    //    Q_ASSERT(cross && "cannot be parallel or skewd");

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));
    point2d c  = ws.toUser(center);
    auto    normal1 =
        bestElementNormal(ws, globalPoint(0), globalPoint(0) + direction(0)) * info().scale;
    auto normal2 =
        bestElementNormal(ws, globalPoint(1), globalPoint(1) + direction(1)) * info().scale;

    bool ok = false;
    auto m1 = (c - p1).normalized(&ok) * flangesWidth * info().scale;
    auto m2 = (c - p2).normalized(&ok) * flangesWidth * info().scale;

    flange first(p1, normal1, m1, isFlangeVisible(0)), second(p2, normal2, m2, isFlangeVisible(1));

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << c << first.points();
    enlargeRegion_hull(points1, 2);
    g.contur << drawSpline(gscene, points1);

    // вторая часть
    QList<point2d> points2;
    points2 << c << second.points();
    enlargeRegion_hull(points2, 2);
    g.contur << drawSpline(gscene, points2);

    compressContur(g.contur);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points1 + points2, p1, p2);

    // направление "перпендикуляра"
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(first.bottom_internal, first.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(first.top_internal, c), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(first.bottom_internal, c), graphics::elementPen());

    g.items << GraphicsClipItem::create(
        gscene, makeQLine(second.bottom_internal, second.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(second.top_internal, c), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(second.bottom_internal, c), graphics::elementPen());

    // если нужно рисовать дальние фланцы
    g.items << first.drawExternal(gscene, this);
    g.items << second.drawExternal(gscene, this);
}

// угловой клапан
CREATE_CLASS_ELEMENT_SUBCLASS(AngleValveElement);
ElementParams AngleValveElement::params = {
    ElementConfiguration::CornerType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true   /*isFlangeMutable*/
};

AngleValveElement::AngleValveElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 2;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::ny * size, point3d::ny};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement AngleValveElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new AngleValveElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void AngleValveElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    bool    cross  = 0;
    point3d center = cross_lines3d(globalPoint(0),
                                   globalPoint(0) - direction(0),
                                   globalPoint(1),
                                   globalPoint(1) - direction(1),
                                   &cross,
                                   0);

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(globalPoint(1));
    point2d c  = ws.toUser(center);
    auto    normal1 =
        bestElementNormal(ws, globalPoint(0), globalPoint(0) + direction(0)) * info().scale;
    auto normal2 =
        bestElementNormal(ws, globalPoint(1), globalPoint(1) + direction(1)) * info().scale;

    bool ok = false;
    auto m1 = (c - p1).normalized(&ok) * flangesWidth * info().scale;
    auto m2 = (c - p2).normalized(&ok) * flangesWidth * info().scale;

    flange first(p1, normal1, m1, isFlangeVisible(0)), second(p2, normal2, m2, isFlangeVisible(1));

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << c << first.points();
    enlargeRegion_hull(points1, 2);
    g.contur << drawSpline(gscene, points1);

    // вторая часть
    QList<point2d> points2;
    points2 << c << second.points();
    enlargeRegion_hull(points2, 2);
    g.contur << drawSpline(gscene, points2);

    // рисуем краник
    double kranSize = 1.5;
    auto   cem0     = c + m1 * kranSize * 2;
    g.items << GraphicsClipItem::create(gscene, makeQLine(c, cem0), graphics::elementPen());

    auto cem1 = cem0 + (p2 - c).resized(kranSize * m1.length(), &ok);
    auto cem2 = cem1.rotate2dAround(cem0, M_PI);
    g.items << GraphicsClipItem::create(gscene, makeQLine(cem1, cem2), graphics::elementPen());

    QList<point2d> points3;
    points3 << cem0 << cem1 << cem2;
    g.contur << drawSpline(gscene, points3);

    compressContur(g.contur);

    // обрисовка по выделенными линиями
    if(lineStyle_ == LineSelectedStyle)
        g.items << drawSelectedContur(gscene, points1 + points2, p1, p2);

    // направление "перпендикуляра"
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(first.bottom_internal, first.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(first.top_internal, c), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(first.bottom_internal, c), graphics::elementPen());

    g.items << GraphicsClipItem::create(
        gscene, makeQLine(second.bottom_internal, second.top_internal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(second.top_internal, c), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(second.bottom_internal, c), graphics::elementPen());

    // если нужно рисовать дальние фланцы
    g.items << first.drawExternal(gscene, this);
    g.items << second.drawExternal(gscene, this);
}

///////////////////////////////////////////////////////////////////////////////
// штуцер обечайки
CREATE_CLASS_ELEMENT_SUBCLASS(VesselNossleElement);
ElementParams VesselNossleElement::params = {
    ElementConfiguration::SpecialType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true,  /*isFlangeMutable*/
};

VesselNossleElement::VesselNossleElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 2 * info().scale;
    QList<NodeInfo> list;
    list << NodeInfo{point3d(size), point3d::nx};

    // а к этим нодам нельзя прислединиться
    list << NodeInfo{point3d(0, size), point3d::ny, -1};
    list << NodeInfo{point3d(0, 0), -point3d::ny, -1};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement VesselNossleElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new VesselNossleElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void VesselNossleElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    point3d g0 = globalPoint(0) - valveSize * direction(0) * info().scale / 2;
    point3d g1 = g0 + valveSize * direction(1) * info().scale;
    point3d g2 = g0 - valveSize * direction(1) * info().scale;

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(g0);
    point2d q1 = ws.toUser(g1);
    point2d q2 = ws.toUser(g2);

    // направление "перпендикуляра"
    bool ok = false;
    auto normal =
        bestElementNormal(ws, globalPoint(0), globalPoint(0) + direction(0)) * info().scale;
    auto   m = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0));
    auto   p3 = left.out;

    // фланцы
    if(info_.elementJointType == ElementJoint::Flanged)
    {
        p3 = left.internal;

        g.items << left.drawInternal(gscene, this);

        // наружный рисуется только если нет пары
        if(left.isVisible)
            g.items << GraphicsClipItem::create(
                gscene, makeQLine(left.bottom_out, left.top_out), graphics::elementPen());
    }

    // пенёк
    g.items << GraphicsClipItem::create(gscene, makeQLine(p3, p2), graphics::elementPen());

    // аппарат
    g.items << GraphicsClipItem::create(gscene, makeQLine(q1, q2), graphics::elementPen());

    // сплайн
    point3d t      = g0 - globalPoint(0);
    point3d deltag = (g2 - g1);

    double marks[] = {0.3, 0.4, 0.55, 0.7, 0.65, 0.50, 0.40, 0.45};

    QList<point2d> points2;
    int            count = sizeof(marks) / sizeof(*marks) - 1;
    for(int index = 0; index <= count; ++index)
    {
        points2 << ws.toUser(g1 + t * marks[index] + deltag * index / count);
    }
    points2.prepend(q1);
    points2.append(q2);
    g.items << drawSpline(gscene, points2, graphics::vesselPen());

    if(!level)
        return;

    // штуцер
    QList<point2d> points1;
    points1 << p1 + normal << p1 - normal << p2;
    enlargeRegion_hull(points1, 2);

    // контур аппарата
    QList<point2d> points3;
    points3 << points2[0] << points2[1] << points2[3] // самая выступающая точка
            << points2[points2.size() - 2] << points2[points2.size() - 1];
    // добавляем к контуру
    enlargeRegion_hull(points3, 2);

    // обрисовка по выделенными линиями НЕ ТРЕБУЕТСЯ

    QPainterPath path = makeSimplePathFromPoints(points1) + makeSimplePathFromPoints(points3);
    g.contur << gscene->addPath(path);
}

///////////////////////////////////////////////////////////////////////////////
// штуцер элиптического днища
CREATE_CLASS_ELEMENT_SUBCLASS(VesselNossle2Element);
ElementParams VesselNossle2Element::params = {
    ElementConfiguration::SpecialType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true,  /*isFlangeMutable*/
};

VesselNossle2Element::VesselNossle2Element()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 2 * info().scale;
    QList<NodeInfo> list;
    list << NodeInfo{point3d(size), point3d::nx};

    // а к этим нодам нельзя прислединиться
    list << NodeInfo{point3d(0, size), point3d::ny, -1};
    list << NodeInfo{point3d(0, 0), -point3d::ny, -1};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement VesselNossle2Element::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new VesselNossle2Element);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void VesselNossle2Element::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    point3d vesselDir = valveSize * direction(0) * info().scale;
    point3d normDir   = valveSize * direction(1) * info().scale;

    point3d g0 = globalPoint(0) - vesselDir / 2;
    point3d g1 = g0 + normDir / 3 - vesselDir / 2;
    point3d g2 = g0 - normDir / 3 - vesselDir / 2;

    // начало и конец арматуры
    point2d p1 = ws.toUser(globalPoint(0));
    point2d p2 = ws.toUser(g0);
    point2d q1 = ws.toUser(g1);
    point2d q2 = ws.toUser(g2);

    // направление "перпендикуляра"
    bool ok = false;
    auto normal =
        bestElementNormal(ws, globalPoint(0), globalPoint(0) + direction(0)) * info().scale;
    auto   m = (p2 - p1).normalized(&ok) * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0));
    auto   p3 = left.out;

    // фланцы
    if(info_.elementJointType == ElementJoint::Flanged)
    {
        p3 = left.internal;

        g.items << left.drawInternal(gscene, this);

        if(left.isVisible)
            g.items << GraphicsClipItem::create(
                gscene, makeQLine(left.bottom_out, left.top_out), graphics::elementPen());
    }

    // пенёк
    g.items << GraphicsClipItem::create(gscene, makeQLine(p3, p2), graphics::elementPen());

    // контур аппарата
    g.items << drawSemiIsocircle(gscene, q1, q2, p2, graphics::elementPen());
    QList<point2d> points4 = makePointListFromItemsList({g.items.last()});

    // сплайн
    point3d t      = g0 - globalPoint(0);
    point3d deltag = (g2 - g1);

    double marks[] = {0.0, 0.05, -0.05, 0.1, 0, 0.05, -0.05, 0.00};

    QList<point2d> points2;
    int            count = sizeof(marks) / sizeof(*marks) - 1;
    for(int index = 0; index <= count; ++index)
    {
        points2 << ws.toUser(g1 + t * marks[index] + deltag * index / count);
    }
    g.items << drawSpline(gscene, points2, graphics::vesselPen());

    if(!level)
        return;

    // штуцер
    QList<point2d> points1;
    points1 << p1 + normal << p1 - normal << p2;
    enlargeRegion_hull(points1, 2);

    // добавляем к контуру
    enlargeRegion_hull(points4, 2);

    QPainterPath path = makeSimplePathFromPoints(points1) + makeSimplePathFromPoints(points4);
    g.contur << gscene->addPath(path);
}

///////////////////////////////////////////////////////////////////////////////
// коробка
CREATE_CLASS_ELEMENT_SUBCLASS(VesselNossle3Element);
ElementParams VesselNossle3Element::params = {
    ElementConfiguration::SpecialType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true,  /*isFlangeMutable*/
};

VesselNossle3Element::VesselNossle3Element()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize / 2 * info().scale;
    QList<NodeInfo> list;
    list << NodeInfo{point3d(size), point3d::nx};

    list << NodeInfo{point3d(-size, size * 2), point3d::ny};
    list << NodeInfo{point3d(0, 0), -point3d::ny, -1};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement VesselNossle3Element::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new VesselNossle3Element);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void VesselNossle3Element::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    point3d vesselDir = valveSize * direction(0) * info().scale;
    point3d normDir   = valveSize * direction(1) * info().scale * 2 / 3;

    point3d g0  = globalPoint(0) - vesselDir / 2;
    point3d g1  = g0 + normDir;
    point3d g2  = g0 - normDir;
    point3d gn1 = g0 + normDir - vesselDir;
    point3d gn2 = g0 - normDir - vesselDir;

    // начало и конец арматуры
    point2d p1  = ws.toUser(globalPoint(0));
    point2d p2  = ws.toUser(g0);
    point2d q1  = ws.toUser(g1);
    point2d q2  = ws.toUser(g2);
    point2d qn1 = ws.toUser(gn1);
    point2d qn2 = ws.toUser(gn2);

    // направление "перпендикуляра"
    auto normal =
        bestElementNormal(ws, globalPoint(0), globalPoint(0) + direction(0)) * info().scale;
    auto   m = (p2 - p1).normalized() * flangesWidth * info().scale;
    flange left(p1, normal, m, isFlangeVisible(0));
    auto   p3 = left.out;

    // фланцы
    if(info_.elementJointType == ElementJoint::Flanged)
    {
        p3 = left.internal;

        g.items << left.drawInternal(gscene, this);

        if(left.isVisible)
            g.items << GraphicsClipItem::create(
                gscene, makeQLine(left.bottom_out, left.top_out), graphics::elementPen());
    }

    // пенёк
    g.items << GraphicsClipItem::create(gscene, makeQLine(p3, p2), graphics::elementPen());

    // штуцер
    QList<point2d> points1;
    points1 << p1 + normal << p1 - normal << p2;
    enlargeRegion_hull(points1, 2);

    // собственно сам "ящик"s
    g.items << GraphicsClipItem::create(gscene, makeQLine(q1, q2), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(q2, qn2), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(qn2, qn1), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(qn1, q1), graphics::elementPen());

    QList<point2d> points4;
    points4 << q1 << q2 << qn2 << qn1;

    // добавляем к контуру
    enlargeRegion_hull(points4, 2);

    QPainterPath path = makeSimplePathFromPoints(points1) + makeSimplePathFromPoints(points4);
    g.contur << gscene->addPath(path);
}

// насос
CREATE_CLASS_ELEMENT_SUBCLASS(PumpElement);
ElementParams PumpElement::params = {
    ElementConfiguration::SpecialType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true,  /*isFlangeMutable*/
};

PumpElement::PumpElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize * 0.75;
    QList<NodeInfo> list;

    // штуцера торчат вверх
    list << NodeInfo{point3d(size, 0), point3d::nx};
    list << NodeInfo{point3d(size, valveSize), point3d::nx};

    // а к этому ноду нельзя прислединиться
    list << NodeInfo{point3d(0, size), point3d::ny, -1};
    setNodeList(0, list);

    info_.setFlanges();
}

PElement PumpElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new PumpElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void PumpElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // направление "перпендикуляра"
    point3d f1 = globalPoint(0), f2 = globalPoint(1);
    point2d ff1 = ws.toUser(f1), ff2 = ws.toUser(f2);
    point2d v1     = bestElementNormal(ws, f1, f1 + direction(0)) * info().scale,
            normal = bestElementNormal(ws, f2, f2 + direction(1)) * info().scale;
    point3d c1 = center(0, 2), c2 = center(1, 2);
    point2d /*c = ws.toUser(center()),*/
        b1     = ws.toUser(c1),
        b2     = ws.toUser(c2);
    bool    ok = false;
    point2d m1 = (b1 - ff1).normalized(&ok) * flangesWidth * info().scale,
            m2 = (b2 - ff2).normalized(&ok) * flangesWidth * info().scale;
    // фланцы
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ff1 + v1, ff1 - v1), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ff1 + v1 + m1, ff1 - v1 + m1), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ff2 + normal, ff2 - normal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ff2 + normal + m2, ff2 - normal + m2), graphics::elementPen());

    // пеньки
    g.items << GraphicsClipItem::create(gscene, makeQLine(b1, ff1 + m1), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(b2, ff2 + m2), graphics::elementPen());

    // рисуем кружочек
    auto pumpc = gscene * drawPumpCircle(gscene, ws, c1, c2, direction(0));
    g.items << pumpc;

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << ff1 + v1 << ff1 - v1 << b1;
    enlargeRegion_hull(points1, 2);

    QList<point2d> points2;
    points2 << ff2 + normal << ff2 - normal << b2;
    enlargeRegion_hull(points2, 2);

    // оприходуем кружочек в контур
    QList<point2d> points3 = makePointListFromItemsList(pumpc);
    enlargeRegion_oneSide(points3, 2);

    QPainterPath path = makeSimplePathFromPoints(points1) + makeSimplePathFromPoints(points2) +
                        makeSimplePathFromPoints(points3);

    g.contur << gscene->addPath(path);
}

// насос
CREATE_CLASS_ELEMENT_SUBCLASS(Pump2Element);
ElementParams Pump2Element::params = {
    ElementConfiguration::SpecialType,
    ElementJoint::Flanged,
    false, /*canNonBeFlipped*/
    true,  /*isFlangeMutable*/
};
Pump2Element::Pump2Element()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize * 0.75;
    QList<NodeInfo> list;

    // штуцера торчат вверх и вбок
    list << NodeInfo{point3d(size, 0), point3d::nx};
    list << NodeInfo{point3d(size, valveSize), point3d::nx, -1};
    list << NodeInfo{point3d(0, size * 1.75), point3d::ny};

    setNodeList(0, list);

    info_.setFlanges();
}

PElement Pump2Element::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new Pump2Element);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void Pump2Element::draw(GraphicsScene *gscene, GItems &g, int level)
{
    WorldSystem &ws = *gscene->ws();

    // направление "перпендикуляра"
    point3d f1 = globalPoint(0), f2 = globalPoint(2);
    point2d ff1 = ws.toUser(f1), ff2 = ws.toUser(f2);
    point2d v1     = bestElementNormal(ws, f1, f1 + direction(0)) * info().scale,
            normal = bestElementNormal(ws, f2, f2 + direction(2)) * info().scale;
    point3d c = center(), c1 = center(0, 2), c2 = center(1, 2);
    point2d /*b = ws.toUser(c),*/
        b1     = ws.toUser(c1),
        b2     = ws.toUser(c);
    bool    ok = false;
    point2d m1 = (b1 - ff1).normalized(&ok) * flangesWidth * info().scale,
            m2 = (b2 - ff2).normalized(&ok) * flangesWidth * info().scale;
    // фланцы
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ff1 + v1, ff1 - v1), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ff1 + v1 + m1, ff1 - v1 + m1), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ff2 + normal, ff2 - normal), graphics::elementPen());
    g.items << GraphicsClipItem::create(
        gscene, makeQLine(ff2 + normal + m2, ff2 - normal + m2), graphics::elementPen());

    // пеньки
    g.items << GraphicsClipItem::create(gscene, makeQLine(b1, ff1 + m1), graphics::elementPen());
    g.items << GraphicsClipItem::create(gscene, makeQLine(b2, ff2 + m2), graphics::elementPen());

    // кружочек
    GItemsList pumpc = gscene * drawPumpCircle(gscene, ws, c1, c2, direction(0));
    g.items << pumpc;

    if(!level)
        return;

    // контур
    QList<point2d> points1;
    points1 << ff1 + v1 << ff1 - v1 << b1;
    enlargeRegion_hull(points1, 2);

    QList<point2d> points2;
    points2 << ff2 + normal << ff2 - normal << b2;
    enlargeRegion_hull(points2, 2);

    // оприходуем кружочек в контур
    QList<point2d> points3 = makePointListFromItemsList(pumpc);
    enlargeRegion_oneSide(points3, 2);

    QPainterPath path = makeSimplePathFromPoints(points1) + makeSimplePathFromPoints(points2) +
                        makeSimplePathFromPoints(points3);

    g.contur << gscene->addPath(path);
}

///////////////////////////////////////////////////////////////////////////////////////////////

CREATE_CLASS_ELEMENT_SUBCLASS(DrainageElement);
ElementParams DrainageElement::params = {
    ElementConfiguration::ValveType,
    ElementJoint::Welded,
    false, /*canNonBeFlipped*/
    false  /*isFlangeMutable*/
};

DrainageElement::DrainageElement()
    : Element(QString(__PRETTY_FUNCTION__).section("::", 1, 1))
{
    double          size = valveSize;
    QList<NodeInfo> list;
    list << NodeInfo{point3d::n, -point3d::nx};
    list << NodeInfo{point3d::nx * size, point3d::nx};
    info_.setFlanges();
    setNodeList(0, list);
}

PElement DrainageElement::clone(Scene *scene, QList<NodeInfo> nodeList) const
{
    PElement elem(new DrainageElement);
    elem->setNodeList(scene, nodeList);
    elem->setPObject(elem);
    elem->applyStyle(pObject());
    return elem;
}

void DrainageElement::draw(GraphicsScene *gscene, GItems &g, int level)
{
    point2d px1 = localPoint(0);
    point2d px2 = localPoint(1);

    point2d c  = px2.partition_by(px1, 0.4);
    point2d a1 = px2;
    point2d a2 = a1.rotate2dAround(c, +M_PI_2 * 135 / 90);
    point2d a3 = a1.rotate2dAround(c, -M_PI_2 * 135 / 90);

    GItemsList items(gscene);

    items << GraphicsClipItem::create(gscene, makeQLine(a1, c), graphics::elementPen());
    items << GraphicsClipItem::create(gscene, makeQLine(a2, c), graphics::elementPen());
    items << GraphicsClipItem::create(gscene, makeQLine(a3, c), graphics::elementPen());
    g.items << items;

    items << GraphicsClipItem::create(gscene, makeQLine(px1, c), graphics::elementPen());

    QList<point2d> points = makePointListFromItemsList(items);

    enlargeRegion_hull(points, 2);
    g.contur << drawSpline(gscene, points);
}
} // namespace geometry
