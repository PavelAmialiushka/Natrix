#include "markLabels.h"

#include "graphics.h"
#include "graphicsScene.h"
#include "label.h"
#include "object.h"
#include "scene.h"

#include "global.h"
#include "graphics.h"
#include "sceneProcedures.h"

#include "marker.h"

#include <QDomElement>

namespace geometry
{

CREATE_LABEL_BY_DOMELEMENT(MarkLabel);

MarkLabel::MarkLabel(Scene *scene, point3d point, point3d direction, point3d lineDir)
    : Label(scene)
    , objectPoint_(point)
    , basePoint_(scene->worldSystem()->convert(point))
    , direction_(direction)
    , lineDirection_(lineDir)
    , type_(Support)
{
}

int MarkLabel::type() const
{
    return type_;
}

void MarkLabel::setType(int t)
{
    type_ = t;
}

point2d MarkLabel::basePoint() const
{
    return basePoint_;
}

point3d MarkLabel::objectPoint() const
{
    return objectPoint_;
}

int MarkLabel::rotationType() const
{
    return rotationType(type_);
}

int MarkLabel::rotationType(int t)
{
    switch(t)
    {
    case Arrow:
    case Underground:
        return InLineRotation;
    case NosslePI:
    case NosslePlug:
    case NossleTI:
    case NossleValve:
        return NormalRotation;
    default:
        return NoRotation;
    }
}

point3d MarkLabel::direction() const
{
    return direction_;
}

void MarkLabel::setLineDirection(point3d d)
{
    lineDirection_ = d;
}

point3d MarkLabel::lineDirection() const
{
    return lineDirection_;
}

point3d MarkLabel::locateTextGripPoint(point2d pt) const
{
    WorldSystem &ws     = *scene_->worldSystem();
    point3d      normal = bestNormalDirection(ws, direction_);

    switch(rotationType())
    {
    case InLineRotation:
    case NoRotation:
        return ws.toGlobal_atline(pt, objectPoint_, objectPoint_ + normal);
    case NormalRotation:
        return ws.toGlobal_atline(pt, objectPoint_, objectPoint_ + direction_);
    default: {
        // направление direction параллельно линии
        // поэтому ищем два перпендикулярных направления
        // и строим ищем точку на этой плоскости
        point3d a, b;
        for(point3d z : {point3d::nx, point3d::ny, point3d::nz})
            if(z.isNormal(direction_))
            {
                a = z;
                b = direction_.rotate3dAround(point3d{}, z, M_PI_2);
                break;
            }
        return ws.toGlobal_atplane(pt, objectPoint_, objectPoint_ + b, objectPoint_ + a);
    }
    }
}

point3d MarkLabel::transformPoint(point3d point, PMarkLabel label2)
{
    point3d delta = point - objectPoint_;

    if(rotationType() == NoRotation || rotationType() == InLineRotation)
    {
        point3d normal1 = bestNormalDirection(*scene_->worldSystem(), direction());
        point3d normal2 = bestNormalDirection(*scene_->worldSystem(), label2->direction());

        double dd = delta.fraction_at_projection(0, normal1);
        return label2->objectPoint() + normal2 * dd;
    }

    double dd = delta.fraction_at_projection(0, direction_);
    return label2->objectPoint() + label2->direction() * dd;
}

static QList<QGraphicsItem *> makeSpringPoints(GraphicsScene *gscene,
                                               point2d        mG, // начальная точка
                                               point2d        z,  // направление z
                                               point2d        x)
{
    double         cofs[] = {0, 0, 0.7, -0.7, 0.7, -0.7, 0, 0};
    QList<point2d> points;
    points << mG;
    for(unsigned index = 0; index < sizeof(cofs) / sizeof(*cofs); ++index)
    {
        points << mG + z.resized(1.5 * (index + 1)) + x.resized(cofs[index] * 7);
    }

    QList<QGraphicsItem *> items;
    for(int index = 1; index < points.size(); ++index)
        items << gscene->addLine(makeQLine(points[index - 1], points[index]));
    return items;
}

static void drawSupport(GraphicsScene *gscene, GItems &g, point3d d0, point3d p0, bool neg = false)
{
    auto    ws = gscene->ws();
    point2d z  = point2d::ny * (neg ? -7 : 7);
    point2d x  = point2d::nx * 8;
    point2d d  = (d0 * 7) >> *ws;

    if(!point2d::nx.isNormal(d0 >> *ws))
    { // находится в горизонтальной оси

        point2d mG0 = p0 >> *ws;
        point2d mG = mG0 + z, mGl = mG + d, mGr = mG - d, mGb = mG + z;

        g.items << gscene->addLine(makeQLine(mG, mGb));
        g.items << gscene->addLine(makeQLine(mGl, mGr));
    }
    else
    {
        point2d mG  = (p0 >> *ws);
        point2d mGl = mG - x, mGr = mG + x, mGlt = mG - x * 5 / 8, mGrt = mG + x * 5 / 8;

        g.items << gscene->addLine(makeQLine(mGl, mGr));
        g.items << gscene->addLine(makeQLine(mGlt, mGlt + z));
        g.items << gscene->addLine(makeQLine(mGrt, mGrt + z));
    }
}

static void drawSupportSpring(GraphicsScene *gscene,
                              GItems &       g,
                              point3d        d0,
                              point3d        p0,
                              bool           neg = false)
{
    auto    ws = gscene->ws();
    point2d z  = point2d::ny * (neg ? -7 : 7);
    point2d x  = point2d::nx * 8;
    point2d d  = (d0 * 7) >> *ws;

    if(!point2d::nx.isNormal(d0 >> *ws))
    {
        // линия направлена в сторону
        point2d mG = (p0 >> *ws) + z, mGl = mG - d, mGr = mG + d;

        g.items << gscene->addLine(makeQLine(mGl, mGr));

        g.items << makeSpringPoints(gscene, mG, z, d);
    }
    else
    {
        point2d mG  = (p0 >> *ws);
        point2d mGl = mG - x, mGr = mG + x, mGlt = mG - x * 0.625, mGrt = mG + x * 0.625;

        g.items << gscene->addLine(makeQLine(mGl, mGr));

        g.items << makeSpringPoints(gscene, mGlt, z, x);
        g.items << makeSpringPoints(gscene, mGrt, z, x);
    }
}

static void drawClip(GraphicsScene *gscene, GItems &g, point3d d0, point3d p0)
{
    return drawSupport(gscene, g, d0, p0, true);
}

static void drawClipSpring(GraphicsScene *gscene, GItems &g, point3d d0, point3d p0)
{
    return drawSupportSpring(gscene, g, d0, p0, true);
}

static void drawConSupport(GraphicsScene *gscene, GItems &g, point3d dir, point3d d0, point3d p0)
{
    constexpr int length = 25;

    auto ws = gscene->ws();
    auto p1 = p0 + dir * length;
    g.items << gscene->addLine(
        makeQLine(p0.partition_by(p1, 0.25) >> *ws, p1.partition_by(p0, -0.5) >> *ws));

    return drawSupport(gscene, g, dir, p1);
}

static void drawConSpring(GraphicsScene *gscene, GItems &g, point3d dir, point3d d0, point3d p0)
{
    constexpr int length = 25;

    auto ws = gscene->ws();
    auto p1 = p0 + dir * length;
    g.items << gscene->addLine(
        makeQLine(p0.partition_by(p1, 0.25) >> *ws, p1.partition_by(p0, -0.5) >> *ws));

    return drawSupportSpring(gscene, g, dir, p1);
}

static void drawConClip(GraphicsScene *gscene, GItems &g, point3d dir, point3d d0, point3d p0)
{
    constexpr int length = 25;

    auto ws = gscene->ws();
    auto p1 = p0 + dir * length;
    g.items << gscene->addLine(
        makeQLine(p0.partition_by(p1, 0.25) >> *ws, p1.partition_by(p0, -0.5) >> *ws));

    return drawClip(gscene, g, dir, p1);
}

static void drawConClipSpring(GraphicsScene *gscene, GItems &g, point3d dir, point3d d0, point3d p0)
{
    constexpr int length = 25;

    auto ws = gscene->ws();
    auto p1 = p0 + dir * length;
    g.items << gscene->addLine(
        makeQLine(p0.partition_by(p1, 0.25) >> *ws, p1.partition_by(p0, -0.5) >> *ws));

    return drawClipSpring(gscene, g, dir, p1);
}

static void drawWall(GraphicsScene *gscene, GItems &g, point3d d0, point3d p0)
{
    constexpr int height  = 5;
    constexpr int xnear_v = 5;
    constexpr int xfar_v  = 12;

    auto    ws    = gscene->ws();
    point2d z     = (d0 >> *ws) * height;
    point2d xnear = bestElementNormal(*ws, d0, xnear_v);
    point2d xfar  = bestElementNormal(*ws, d0, xfar_v);
    double  k     = xnear.length() / xnear_v;
    if(k < 0.75)
    {
        xfar  = xfar.resized(xfar.length() * 1.35);
        xnear = xnear.resized(xnear.length() * 1.35);
    }

    QList<QList<point2d>> points;
#define NEXT_LINE                                                                                  \
    points << QList<point2d>();                                                                    \
    points.last()

    NEXT_LINE << xfar + z << xnear + z << xnear - z << xfar - z;

    NEXT_LINE << -xfar + z << -xnear + z << -xnear - z << -xfar - z;

    // штриховка
    NEXT_LINE << xfar << (xfar + xnear) / 2 + z;
    NEXT_LINE << xfar - z << xnear + z;
    NEXT_LINE << (xfar + xnear) / 2 - z << xnear;

    NEXT_LINE << -xnear << -(xfar + xnear) / 2 + z;
    NEXT_LINE << -xnear - z << -xfar + z;
    NEXT_LINE << -(xfar + xnear) / 2 - z << -xfar;

#undef NEXT_LINE

    point2d mG = p0 >> *ws;
    foreach(auto list, points)
    {
        for(int index = list.size(); index-- > 0;)
            list[index] += mG;

        for(int index = 1; index < list.size(); ++index)
        {
            g.items << gscene->addLine(makeQLine(list[index - 1], list[index]));
        }
    }
}

static void drawUnderground(GraphicsScene *gscene, GItems &g, point3d d0, point3d p0)
{
    constexpr int height  = 5;
    constexpr int xnear_v = 5;
    constexpr int xfar_v  = 17;

    auto    ws    = gscene->ws();
    point2d z     = -(d0 >> *ws) * height;
    point2d xnear = bestElementNormal(*ws, d0, xnear_v);
    point2d xfar  = bestElementNormal(*ws, d0, xfar_v);
    double  k     = xnear.length() / xnear_v;
    if(k < 0.75)
    {
        xfar  = xfar.resized(xfar.length() * 1.35);
        xnear = xnear.resized(xnear.length() * 1.35);
    }

    QList<QList<point2d>> points;
#define NEXT_LINE                                                                                  \
    points << QList<point2d>();                                                                    \
    points.last()

    NEXT_LINE << xfar + z << xnear + z << xnear - z;

    NEXT_LINE << -xfar + z << -xnear + z << -xnear - z;

    // штриховка
    auto a1 = xfar;
    auto b1 = (xfar + xnear) / 2 + z;
    auto c1 = xfar - z;
    auto d1 = xnear + z;
    auto e1 = (xfar + xnear) / 2 - z;
    auto f1 = xnear;
    NEXT_LINE << a1.partition_by(b1, 0.25) << b1;
    NEXT_LINE << c1.partition_by(d1, 0.35) << d1;
    NEXT_LINE << e1.partition_by(f1, 0.25) << f1;

    auto a2 = -xnear;
    auto b2 = -(xfar + xnear) / 2 + z;
    auto c2 = -xnear - z;
    auto d2 = -xfar + z;
    auto e2 = -(xfar + xnear) / 2 - z;
    auto f2 = -xfar;
    NEXT_LINE << a2 << b2;
    NEXT_LINE << c2 << d2;
    NEXT_LINE << e2.partition_by(f2, 0.25) << f2.partition_by(e2, 0.25);

#undef NEXT_LINE

    point2d mG = p0 >> *ws;
    foreach(auto list, points)
    {
        for(int index = list.size(); index-- > 0;)
            list[index] += mG;

        for(int index = 1; index < list.size(); ++index)
        {
            g.items << gscene->addLine(makeQLine(list[index - 1], list[index]));
        }
    }
}

static void drawArrow(GraphicsScene *gscene, GItems &g, point3d d0, point3d p0)
{
    constexpr int length = 15;

    auto ws      = gscene->ws();
    auto base    = (p0 >> *ws);
    auto farside = ((p0 + d0) >> *ws);

    bool ok         = true;
    auto arrowpoint = base.polar_to(farside, length / 2, &ok);
    if(!ok)
        return;

    auto arrowbase = arrowpoint.polar_to(farside, length, &ok);
    if(!ok)
        return;

    double angle = 15 * M_PI / 180;
    auto   wind1 = arrowbase.rotate2dAround(arrowpoint, angle);
    auto   wind2 = arrowbase.rotate2dAround(arrowpoint, -angle);

    g.items << gscene->addLine(makeQLine(wind1, arrowpoint), graphics::linePen());
    g.items << gscene->addLine(makeQLine(wind2, arrowpoint), graphics::linePen());
    g.items << gscene->addLine(makeQLine(arrowbase, arrowpoint), graphics::linePen());
}

static void drawNossle(GraphicsScene *gscene,
                       GItems &       g,
                       point3d        d0,
                       point3d        ld,
                       point3d        p0,
                       int            type)
{
    constexpr int nosslelength = 12;
    constexpr int nossleWidth  = 6;
    constexpr int plug         = 4;
    constexpr int plugWidth    = 6;
    constexpr int gridLength   = 8;
    constexpr int radiusTI     = 10;
    constexpr int valveHWidth  = 3;
    constexpr int valveLength  = 12;
    auto          ws           = gscene->ws();

    point3d a = p0;
    point3d b = p0 + d0 * nosslelength;

    point2d ax = ws->convert(a);
    point2d bx = ws->convert(b);

    bool    ok = 0;
    point2d tx = ws->convert(ld).normalized(&ok);
    point2d nx = (bx - ax).normalized(&ok);

    if(!tx)
        tx = nx.rotate2dAround(0, M_PI_2);

    QPolygonF r;
    r << (ax + tx * nossleWidth / 2).toQPoint() << (bx + tx * nossleWidth / 2).toQPoint()
      << (bx - tx * nossleWidth / 2).toQPoint() << (ax - tx * nossleWidth / 2).toQPoint();

    auto brush = graphics::linePen().brush();
    g.items << gscene->addPolygon(r, QPen(), brush);

    switch(type)
    {
    case NosslePlug:
        // просто пробка
        g.items << gscene->addLine(makeQLine(bx, bx + nx * plug), graphics::elementPen());
        g.items << gscene->addLine(
            makeQLine(bx + nx * plug + tx * plugWidth / 2, bx + nx * plug - tx * plugWidth / 2),
            graphics::elementPen());
        break;
    case NossleTI:
    case NosslePI: {
        auto center = bx + nx * (radiusTI + gridLength);
        g.items << gscene->addLine(makeQLine(bx, bx + nx * gridLength), graphics::elementPen());
        g.items << drawCircle(gscene, center, radiusTI, graphics::elementPen());

        QFont font("Tahoma", 12);
        font.setKerning(0);
        font.setBold(true);
        font.setPixelSize(TextNormalHeight);

        auto item  = gscene->addText(type == NossleTI ? "T" : "P", font);
        auto delta = center.toQPoint() - item->boundingRect().center();
        item->moveBy(delta.x(), delta.y());
        g.items << item;
    }
    break;
    case NossleValve: {
        auto c      = b.polar_to(a, -gridLength);
        auto d      = c.polar_to(b, -valveLength);
        auto cx     = ws->convert(c);
        auto dx     = ws->convert(d);
        auto normal = bestElementNormal(*ws, d0, valveHWidth);

        auto ex = cx + normal;
        auto fx = cx - normal;
        auto kx = dx + normal;
        auto lx = dx - normal;

        g.items << gscene->addLine(makeQLine(bx, cx), graphics::elementPen());
        g.items << gscene->addLine(makeQLine(ex, fx), graphics::elementPen());
        g.items << gscene->addLine(makeQLine(fx, kx), graphics::elementPen());
        g.items << gscene->addLine(makeQLine(kx, lx), graphics::elementPen());
        g.items << gscene->addLine(makeQLine(lx, ex), graphics::elementPen());

        break;
    }
    }
}

void MarkLabel::draw(GraphicsScene *gscene, GItems &g, int level)
{
    // определяем, привязана ли опора к к.-л. элементу
    auto ws = gscene->ws();

    point3d d0 = point3d::nx;
    point3d p0 = basePoint_ >> *ws;

    auto markers = scene_->markersOfFollower(sharedFromThis().dynamicCast<MarkLabel>());
    if(markers.size())
    {
        // направление элемента
        auto marker = markers.first();
        if(PObject object = marker->leader.dynamicCast<Object>())
        {
            d0 = getMarkLandingDirection(object);
        }
    }

    g.items.clear();
    if(type_ == SupportType::Support)
    {
        drawSupport(gscene, g, d0, p0);
    }
    else if(type_ == SupportType::SupportSpring)
    {
        drawSupportSpring(gscene, g, d0, p0);
    }
    else if(type_ == SupportType::Clip)
    {
        drawClip(gscene, g, d0, p0);
    }
    else if(type_ == SupportType::ClipSpring)
    {
        drawClipSpring(gscene, g, d0, p0);
    }
    else if(type_ == SupportType::Wall)
    {
        drawWall(gscene, g, d0, p0);
    }
    else if(type_ == SupportType::ConSupport)
    {
        drawConSupport(gscene, g, d0, d0, p0);
    }
    else if(type_ == SupportType::ConSupportSpring)
    {
        drawConSpring(gscene, g, d0, d0, p0);
    }
    else if(type_ == SupportType::ConClip)
    {
        drawConClip(gscene, g, d0, d0, p0);
    }
    else if(type_ == SupportType::ConClipSpring)
    {
        drawConClipSpring(gscene, g, d0, d0, p0);
    }
    else if(type_ == SupportType::Underground)
    {
        drawUnderground(gscene, g, direction_, p0);
    }
    else if(type_ == SupportType::Arrow)
    {
        drawArrow(gscene, g, direction_, p0);
    }
    else if(type_ == SupportType::NosslePlug || type_ == SupportType::NossleValve ||
            type_ == SupportType::NosslePI || type_ == SupportType::NossleTI)
    {
        drawNossle(gscene, g, direction_, lineDirection_, p0, type_);
    }

    if(g.items.empty())
        g.items << drawCircle(gscene, basePoint_, 7);

    if(!level)
        return;

    // контур
    QList<point2d> points = makePointListFromItemsList(g.items);
    enlargeRegion_oneSide(points, 2);

    if(points.size())
    {
        g.contur.clear();
        g.contur << drawSpline(gscene, points);
    }
}

PLabel MarkLabel::clone(Scene *scene, point2d delta) const
{
    //    Q_ASSERT(!"must not be used");
    return clone(scene,
                 scene->worldSystem()->convert(delta) - scene->worldSystem()->convert(point2d{0}));
}

PLabel MarkLabel::clone(Scene *scene, point3d delta) const
{
    auto p = new MarkLabel(scene, objectPoint_ + delta, direction_, lineDirection());
    p->setType(type());
    return PLabel(p);
}

PMarkLabel MarkLabel::clone(Scene *, point3d delta, point3d direction) const
{
    PMarkLabel label{new MarkLabel(scene_, objectPoint_ + delta, direction, lineDirection())};
    label->setType(type_);
    return label;
}

PLabel MarkLabel::cloneMoveRotate(Scene *            scene,
                                  point3d            delta,
                                  point3d            center,
                                  double             angle,
                                  MoveRotateContext &context) const
{
    point3d localObjectPoint = objectPoint_;
    if(context.ms.size())
    {
        // сначала приводим метку к точке объекта
        // (на случай, если она переехала в другое место)
        if(auto subdelta = context.ms.first()->point - objectPoint())
            localObjectPoint += subdelta;
    }

    auto point   = delta + localObjectPoint.rotate3dAround(center, point3d::nz, angle);
    auto dir     = direction_.rotate3dAround(0, point3d::nz, angle);
    auto lineDir = lineDirection_.rotate3dAround(0, point3d::nz, angle);

    PMarkLabel label{new MarkLabel(scene, point, dir, lineDir)};
    label->setType(type_);
    return label;
}

double MarkLabel::distance(const point2d &pt)
{
    return 999;
}

PLabel MarkLabel::createFrom(Scene *scene, class QDomElement elm)
{
    auto label = PLabel(new MarkLabel(scene, 0, 1.0, 1.0));
    label->loadLabel(elm);
    return label;
}

void MarkLabel::saveLabel(QDomElement elm)
{
    elm.setAttribute("point", basePoint_.serialSave());
    elm.setAttribute("object_point", objectPoint_.serialSave());
    elm.setAttribute("type", type_);
    elm.setAttribute("dir", direction_.serialSave());
    elm.setAttribute("linedir", lineDirection_.serialSave());
}

bool MarkLabel::loadLabel(QDomElement elm)
{
    bool ok    = true;
    basePoint_ = point2d::serialLoad(elm.attribute("point"), &ok);
    if(!ok)
        return false;

    // считываем или восстанавливаем точку привязки
    objectPoint_ = point3d::serialLoad(elm.attribute("object_point"), &ok);
    if(!ok)
        objectPoint_ = scene_->worldSystem()->convert(basePoint_);
    ok = true;

    leftTop_ = rightBottom_ = basePoint_;
    type_                   = elm.attribute("type", "0").toInt();
    direction_              = point3d::serialLoad(elm.attribute("dir"));
    lineDirection_          = point3d::serialLoad(elm.attribute("linedir"));
    return ok;
}

int MarkLabel::typeToInt(QString et)
{
#define _elementTypeToEnum(a) et == #a ? SupportType::a:
    return _elementTypeToEnum(Support) _elementTypeToEnum(SupportSpring) _elementTypeToEnum(Clip)
        _elementTypeToEnum(ClipSpring) _elementTypeToEnum(Wall) _elementTypeToEnum(ConSupport)
            _elementTypeToEnum(ConSupportSpring) _elementTypeToEnum(ConClip)
                _elementTypeToEnum(ConClipSpring) _elementTypeToEnum(Arrow)
                    _elementTypeToEnum(Underground) _elementTypeToEnum(NosslePlug)
                        _elementTypeToEnum(NossleValve) _elementTypeToEnum(NossleTI)
                            _elementTypeToEnum(NosslePI) SupportType::Wall; // and default
}

QString MarkLabel::typeFromInt(int type)
{
#define _elementTypeFromEnum(a) type == SupportType::a ? #a:
    return _elementTypeFromEnum(Support) _elementTypeFromEnum(SupportSpring)
        _elementTypeFromEnum(Clip) _elementTypeFromEnum(ClipSpring) _elementTypeFromEnum(Wall)
            _elementTypeFromEnum(ConSupport) _elementTypeFromEnum(ConSupportSpring)
                _elementTypeFromEnum(ConClip) _elementTypeFromEnum(ConClipSpring)
                    _elementTypeFromEnum(Arrow) _elementTypeFromEnum(Underground)
                        _elementTypeFromEnum(NosslePlug) _elementTypeFromEnum(NossleValve)
                            _elementTypeFromEnum(NossleTI)
                                _elementTypeFromEnum(NosslePI) "WallAndGround";
}

} // namespace geometry
