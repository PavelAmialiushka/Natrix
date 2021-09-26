#include "graphics.h"

#include <QSet>
#include <algorithm>

#include <QDebug>
#include <functional>
#include <tuple>

#include "global.h"
#include "graphicsScene.h"

namespace geometry
{

GItemsList::GItemsList(GraphicsScene *g)
    : gscene(g)
{
}

GItemsList operator*(GraphicsScene *gscene, QList<QGraphicsItem *> const &items)
{
    Q_ASSERT(gscene);
    GItemsList list{gscene};
    list << items;
    return list;
}

GItems::GItems(GraphicsScene *gscene)
    : update(0)
    , gscene(gscene)
    , items(gscene)
    , prevItems(gscene)
    , contur(gscene)
    , prevContur(gscene)
{
}

bool GItems::tryToUpdate()
{
    if(update)
    {
        contur = prevContur;
        items  = prevItems;

        Q_ASSERT(contur.gscene && items.gscene);
        return items.size();
    }

    return 0;
}

PGItem makePGItem(QGraphicsItem *item, QGraphicsScene *scene)
{
    auto deleter = [=](QGraphicsItem *item) {
        scene->removeItem(item);
        delete item;
    };
    return PGItem(item, deleter);
}

GItemsList &operator<<(GItemsList &list, QGraphicsItem *item)
{
    GraphicsScene *gscene = list.gscene;

    Q_ASSERT(gscene);
    Q_ASSERT(item->scene() == gscene);

    list.append(makePGItem(item, gscene));
    return list;
}

GItemsList &operator<<(GItemsList &list, QList<QGraphicsItem *> const &items)
{
    GraphicsScene *gscene = list.gscene;
    Q_ASSERT(gscene);

    foreach(auto item, items)
    {
        Q_ASSERT(item->scene() == gscene);
        list.append(makePGItem(item, gscene));
    }
    return list;
}

QBrush graphics::transparentBrush()
{
    return QBrush(Qt::NoBrush);
}
QBrush graphics::blackBrush()
{
    return QBrush(Qt::black, Qt::SolidPattern);
}

QBrush graphics::originalColour()
{
    return QBrush(Qt::black, Qt::SolidPattern);
}

QBrush graphics::selectionBrush()
{
    return QBrush(QColor(198, 235, 255));
}
QBrush graphics::selectionPenBrush()
{
    return QBrush(QColor(0, 0, 255));
}

QBrush graphics::toBeSelectedBrush()
{
    return QBrush(QColor(198, 235, 255));
}
QBrush graphics::toBeSelectedPenBrush()
{
    return QBrush(QColor(64, 64, 192, 255));
}

QBrush graphics::toModifyBrush()
{
    return QBrush(QColor(249, 172, 251));
}
QBrush graphics::toModifyPenBrush()
{
    return QBrush(QColor(176, 11, 181, 255));
}

QBrush graphics::newbyBrush()
{
    return QBrush(QColor(255, 223, 223));
}
QBrush graphics::newbyPenBrush()
{
    return QBrush(QColor(255, 0, 0, 255));
}

QBrush graphics::hoverBrush()
{
    return QBrush(QColor(255, 255, 192));
}
QBrush graphics::hoverPenBrush()
{
    return QBrush(QColor(32, 32, 64));
}

QBrush graphics::canvasAdornerBrush()
{
    return QBrush(QColor(128, 128, 128, 128));
}

QPen graphics::linePen()
{
    return QPen(QBrush(Qt::black), 4, Qt::SolidLine, Qt::FlatCap);
}
QPen graphics::lineSecondPen()
{
    return QPen(QBrush(Qt::black), 1, Qt::DashLine, Qt::FlatCap);
}
QPen graphics::elementPen()
{
    return QPen(QBrush(Qt::black), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}

QPen graphics::dashDotPen()
{
    return QPen(QBrush(Qt::black), 1.5, Qt::DashDotLine, Qt::RoundCap);
}
QPen graphics::vesselPen()
{
    return QPen(QBrush(Qt::black), 0.5, Qt::DashDotLine, Qt::RoundCap);
}
QPen graphics::viewportPen()
{
    return QPen(QBrush(Qt::gray), 8, Qt::SolidLine, Qt::RoundCap);
}

QPen graphics::ghostAdornerPen()
{
    return QPen(QBrush(QColor(108, 126, 255)), 1, Qt::DashLine, Qt::RoundCap);
}

QPen graphics::moveLineAdornerPen()
{
    return QPen(QBrush(Qt::green), 1, Qt::DotLine, Qt::RoundCap);
}
QPen graphics::straightLineAdornerPen()
{
    return QPen(QBrush(Qt::red), 1, Qt::DotLine, Qt::RoundCap);
}
QPen graphics::rectangle1AdornerPen()
{
    return QPen(QBrush(Qt::blue), 1, Qt::DashLine, Qt::RoundCap);
}
QPen graphics::rectangle2AdornerPen()
{
    return QPen(QBrush(Qt::green), 1, Qt::DashLine, Qt::RoundCap);
}

QBrush graphics::textGripBrush()
{
    return QBrush(Qt::red);
}

QLineF makeQLine(point2d a, point2d b)
{
    return QLineF(a.toQPoint(), b.toQPoint());
}

QPainterPath makeSimplePathFromPoints(QList<point2d> points)
{
    QPainterPath path;
    if(points.size())
    {
        path.setFillRule(Qt::WindingFill);
        path.moveTo(points[0].x, points[0].y);
        for(int index = 1; index < points.size(); ++index)
        {
            path.lineTo(points[index].x, points[index].y);
        }
    }
    return path;
}

QPainterPath makeQuadPathFromPoints(QList<point2d> points)
{
    QPainterPath path;
    if(points.size())
    {
        points << points[0];
        path.setFillRule(Qt::WindingFill);
        path.moveTo(points[0].x, points[0].y);
        for(int index = 1; index < points.size() - 1; index += 2)
        {
            path.quadTo(points[index].x, points[index].y, points[index + 1].x, points[index + 1].y);
        }
    }
    return path;
}

void compressContur(GItemsList &contur)
{
    QSharedPointer<QGraphicsPathItem> compressedPath;
    foreach(PGItem item, contur)
    {
        if(auto currPath = item.dynamicCast<QGraphicsPathItem>())
        {
            if(!compressedPath)
                compressedPath = currPath;
            else
            {
                QPainterPath sumPath = compressedPath->path();
                sumPath.addPath(currPath->path());
                compressedPath->setPath(sumPath);
            }
        }
    }

    if(compressedPath)
    {
        contur.clear();
        contur << compressedPath;
    }
}

QList<QGraphicsItem *> drawSpline(QGraphicsScene *gscene,
                                  QList<point2d>  points,
                                  QPen            pen,
                                  QBrush          brush)
{
    QList<QGraphicsItem *> items;
    if(points.size())
    {
        QPainterPath path = makeSimplePathFromPoints(points);
        items << gscene->addPath(path, pen, brush);
    }
    return items;
}

QList<QGraphicsItem *> drawSelectedContur(GraphicsScene *gscene,
                                          QList<point2d> points,
                                          point2d        a,
                                          point2d        b,
                                          QPen           pen,
                                          QBrush         brush)
{
    QList<QGraphicsItem *> items;

    points = makeAHull(points);
    enlargeRegion_hull(points, 2);

    auto norm = a.rotate2dAround(b, M_PI / 2) - b;

    // увеличиваем размер под обводку
    a = a.polar_to(b, -2);
    b = b.polar_to(a, -2);

    // выбираем точки только с одной стороны оси
    QList<point2d> leftPoints;
    foreach(point2d it, points)
    {
        if(norm.dot(it - b) > 0)
            leftPoints << it;
    }

    // находим ближайший к началу элемент
    int index = std::min_element(leftPoints.begin(),
                                 leftPoints.end(),
                                 [&](point2d const &it1, point2d const &it2) {
                                     return it1.distance(a) < it2.distance(a);
                                 }) -
                leftPoints.begin();

    // прокручиваем к нему
    while(index-- > 0)
    {
        leftPoints.push_back(leftPoints.front());
        leftPoints.pop_front();
    }
    leftPoints.push_front(a);
    leftPoints.push_back(b);

    for(int index = 0; index < leftPoints.size() - 1; ++index)
    {
        auto item = new QGraphicsLineItem(makeQLine(leftPoints[index], leftPoints[index + 1]));
        item->setPen(graphics::lineSecondPen());

        gscene->addItem(item);
        items << item;
    }

    return items;
}

QList<QGraphicsItem *> drawQuadSpline(QGraphicsScene *gscene,
                                      QList<point2d>  points,
                                      QPen            pen,
                                      QBrush          brush)
{
    QList<QGraphicsItem *> items;
    if(points.size())
    {
        QPainterPath path = makeQuadPathFromPoints(points);
        items << gscene->addPath(path, pen, brush);
    }
    return items;
}

QList<QGraphicsItem *> drawSemiIsocircle(QGraphicsScene *gscene,
                                         point2d         a,
                                         point2d         b,
                                         point2d         c,
                                         QPen            pen,
                                         QBrush          brush)
{
    // принимаем, что а - дальше, b - ближе
    if(a.distance(c) < b.distance(c))
        qSwap(a, b);

    auto center = (a + b) / 2;

    auto r_point = [&](point2d a, point2d c, double mul) {
        auto ac        = (a + c) / 2;
        auto ac_radius = ac - center;
        auto cca       = center + ac_radius * mul;
        return cca;
    };

    QList<point2d> points;
    points << a << c << b;

    QList<double> mulValues;
    for(int index = 0; index < points.size() - 1; index += 2)
        points.insert(1 + index, r_point(points[index], points[index + 1], 1.4142));

    for(int index = 0; index < points.size() - 1; index += 2)
        points.insert(1 + index, r_point(points[index], points[index + 1], 1.0824));

    return drawSpline(gscene, points, pen, brush);
}

QList<QGraphicsItem *> drawCircle(QGraphicsScene *gscene,
                                  point2d         a,
                                  double          radius,
                                  QPen            pen,
                                  QBrush          brush)
{
    QList<QGraphicsItem *> items;
    items << gscene->addEllipse(a.x - radius, a.y - radius, radius * 2, radius * 2, pen, brush);
    return items;
}

void addEllipsePoints(QList<point2d> &list)
{
    point2d a = list[0], b = list[1], c = list[2], d = list[3];

    point2d center = (a + c) / 2;
    auto    ab     = (a + b) / 2;
    auto    bc     = (b + c) / 2;
    auto    cd     = (c + d) / 2;
    auto    da     = (d + a) / 2;

    auto aa     = a.distance(b);
    auto bb     = b.distance(c);
    bool normal = aa > bb;
    if(!normal)
        qSwap(aa, bb);

    auto k = bb / aa;
    auto e = sqrt(1 - k * k);

    auto r     = sqrt(aa * aa + bb * bb) / 2;
    auto cosfi = cos(atan(bb / aa));

    auto b1 = r * sqrt(1 - e * e * cosfi * cosfi);
    auto a1 = b1 / k;
    if(!normal)
        qSwap(a1, b1);

    auto abx = center.polar_to(ab, b1);
    auto bcx = center.polar_to(bc, a1);
    auto cdx = center.polar_to(cd, b1);
    auto dax = center.polar_to(da, a1);

    if(1 / k > 10)
    {
        list << abx << bcx << cdx << dax;
    }
    else
    {
        list << abx << bcx << cdx << dax;
    }
}

QList<QGraphicsItem *> drawEllipse(QGraphicsScene *gscene,
                                   point2d         a,
                                   point2d         b,
                                   point2d         c,
                                   point2d         d,
                                   QPen            pen,
                                   QBrush          brush)
{
    point2d center = (a + c) / 2;
    auto    ab     = (a + b) / 2;
    auto    bc     = (b + c) / 2;
    auto    cd     = (c + d) / 2;
    auto    da     = (d + a) / 2;

    auto aa     = a.distance(b);
    auto bb     = b.distance(c);
    bool normal = aa > bb;
    if(!normal)
        qSwap(aa, bb);

    auto k = bb / aa;
    auto e = sqrt(1 - k * k);

    auto r     = sqrt(aa * aa + bb * bb) / 2;
    auto cosfi = cos(atan(bb / aa));

    auto b1 = r * sqrt(1 - e * e * cosfi * cosfi);
    auto a1 = b1 / k;
    if(!normal)
        qSwap(a1, b1);

    auto abx = center.polar_to(ab, b1);
    auto bcx = center.polar_to(bc, a1);
    auto cdx = center.polar_to(cd, b1);
    auto dax = center.polar_to(da, a1);

    if(1 / k > 10)
    {
        QList<point2d> points;
        points << a << abx << b << bcx << c << cdx << d << dax << a;

        return drawQuadSpline(gscene, points, pen, brush);
    }
    else
    {
        auto item = gscene->addEllipse(center.x - a1, center.y - b1, 2 * a1, 2 * b1, pen, brush);
        item->setTransformOriginPoint(center.toQPoint());
        item->setRotation((a - b).angle() * 180 / M_PI);

        QList<QGraphicsItem *> items;
        items << item;
        return items;
    }
}

QList<QGraphicsItem *> drawRectangle(QGraphicsScene *gscene,
                                     point2d         a,
                                     point2d         b,
                                     point2d         c,
                                     point2d         d,
                                     QPen            pen,
                                     QBrush          brush)
{
    QList<QGraphicsItem *> items;

    QList<point2d> points;
    points << a << b << c << d << a;

    QPainterPath path = makeSimplePathFromPoints(points);
    items << gscene->addPath(path, pen, brush);
    return items;
}

QList<QGraphicsItem *> drawBox(QGraphicsScene *gscene,
                               point2d         a,
                               double          radius,
                               QPen            pen,
                               QBrush          brush)
{
    QList<QGraphicsItem *> items;
    items << gscene->addRect(a.x - radius, a.y - radius, radius * 2, radius * 2, pen, brush);
    return items;
}

QList<QGraphicsItem *> drawDiamond(QGraphicsScene *gscene,
                                   point2d         a,
                                   double          radius,
                                   QPen            pen,
                                   QBrush          brush)
{
    QList<QGraphicsItem *> items;

    QPainterPath path;
    path.moveTo(a.x - radius, a.y);
    path.lineTo(a.x, a.y - radius);
    path.lineTo(a.x + radius, a.y);
    path.lineTo(a.x, a.y + radius);
    path.lineTo(a.x - radius, a.y);

    return items << gscene->addPath(path, pen, brush);
}

QList<QGraphicsItem *> drawPumpCircle(QGraphicsScene *gscene,
                                      WorldSystem &   ws,
                                      point3d         c1,
                                      point3d         c2,
                                      point3d         direction)
{
    QList<QGraphicsItem *> gitems;
    point3d                c = (c1 + c2) / 2;
    point2d                b = ws.toUser(c), b1 = ws.toUser(c1), b2 = ws.toUser(c2);

    point3d du = c - direction * c1.distance(c2) / 2;
    point2d ax = ws.toUser(du);

    // point2d ax = c + m1.normalized() * (b1.distance(b2));
    point2d bx = 2 * b - ax;
    gitems << drawSemiIsocircle(gscene, b1, b2, ax, graphics::elementPen());
    gitems << drawSemiIsocircle(gscene, b1, b2, bx, graphics::elementPen());

    // ножки
    point2d n1 = b1 - (b - ax), n2 = b2 - (b - ax);

    gitems << gscene->addLine(makeQLine(n1, n2), graphics::elementPen());

    point2d cu1 = b + (n1 - b).normalized() * (n1 - b).length() * 0.73,
            cu2 = b + (n2 - b).normalized() * (n2 - b).length() * 0.73;
    gitems << gscene->addLine(makeQLine(n1, cu1), graphics::elementPen());
    gitems << gscene->addLine(makeQLine(n2, cu2), graphics::elementPen());
    return gitems;
}

// Возвращает увеличинный контур
//   для каждого конкретного узла расширение контура происходит
//   в направлении тупого узла

void enlargeRegion(QList<point2d> &                                         points,
                   double                                                   sz,
                   std::function<QList<point2d>(point2d, point2d, point2d)> make)
{
    QList<point2d> src;
    std::swap(points, src);

    // удяляем одинаковые элементы, идущие подряд
    for(int index = 1; index < src.size(); ++index)
        if(src[index - 1] == src[index])
            src.takeAt(index--);

    // гарантируем, чтобы первый элемент отличался от последнего
    while(src.size() > 0 && src.first() == src.last())
        src.takeLast();

    if(src.empty())
        return;

    // вставляем в начало и конец копии, чтобы
    // удобно обрабатывать next и prev
    src.prepend(src.last());
    src.append(src[1]);

    for(int index = 1, size = src.size(); index < size - 1; ++index)
    {
        point2d prev = src[index - 1];
        point2d curr = src[index];
        point2d next = src[index + 1];

        // вычисление вспомогательных точек
        QList<point2d> list = make(prev, curr, next);

        if(list.size() == 2)
        {
            point2d x = (list[0] + list[1]) / 2 - curr;
            if(x)
                list.insert(1, curr + x.resized(sz));
            else
                // такое может быть только если угол = 180 град
                list.insert(1, curr + (curr - prev).resized(sz));
        }

        points << list;
    }

    if(!points.empty())
        // зацикливаем
        points << points.first();
}

bool isHull(QList<point2d> points)
{
    if(points.isEmpty())
        return false;

    for(int index = 0; index < points.size(); ++index)
    {
        point2d x = points[index];
        point2d y = points[(index + 1) % points.size()];

        // перпендикуляр к прямой
        point2d norm      = (y - x).rotate2dAround(point2d(), M_PI / 2);
        int     glob_sign = 0;
        foreach(point2d z, points)
        {
            double curr_sign = (z - x).dot(norm);
            curr_sign        = fabs(curr_sign) < POINT_PREC ? 0 : curr_sign > 0 ? 1 : -1;

            if(!curr_sign)
                continue;
            else if(!glob_sign)
                glob_sign = curr_sign;
            else if(curr_sign != glob_sign)
                return false;
        }
    }
    return true;
}

void enlargeRegion_hull(QList<point2d> &points, double sz)
{
    // Q_ASSERT(isHull(points));

    // workaround по поводу бага определения оболочки отрезка
    if(points.size() == 2)
    {
        point2d a = points[0];
        point2d b = points[1];

        bool    ok1 = true;
        bool    ok2 = true;
        point2d ax  = a.polar_to(b, -sz, &ok1);
        point2d bx  = b.polar_to(a, -sz, &ok2);

        points.clear();
        if(ok1 && ok2)
        {
            points << ax.rotate2dAround(a, M_PI / 2) << ax << ax.rotate2dAround(a, -M_PI / 2)
                   << bx.rotate2dAround(b, M_PI / 2) << bx << bx.rotate2dAround(b, -M_PI / 2);
        }
        else
        {
            points << a + point2d(0, sz) << a + point2d(sz, 0) << a + point2d(0, -sz)
                   << a + point2d(-sz, 0);
        }

        return;
    }

    auto make = [&](point2d prev, point2d curr, point2d next) -> QList<point2d> {
        // определяем перпендикуляр в заданной точке
        QList<point2d> result;

        if(prev == curr)
            return result << curr;

        point2d forw = next - curr;
        point2d back = prev - curr;

        // определяем отдельно случай "иголки"
        point2d normal = back.rotate2dAround(point2d(), M_PI_2).resized(sz);

        // направляем перпендикуляр в направлении не совпадающем с направлением
        // второй направляющей
        auto x = normal.dot(forw);

        bool ok;
        auto fi = forw.angleTo(back, &ok);
        if(fabs(x) < 1e-3 && (!ok || fi < M_PI_2))
        {
            result << curr - normal << curr - back.resized(sz) << curr + normal;
            return result;
        }

        if(x > 0)
            return result << curr - normal;
        else
            return result << curr + normal;
    };

    auto make2 = [&](point2d prev, point2d curr, point2d next) -> QList<point2d> {
        QList<point2d> result;

        result << make(prev, curr, next);
        result << make(next, curr, prev);

        return result;
    };

    enlargeRegion(points, sz, make2);
    points = makeAHull(points);
}

void enlargeRegion_oneSide(QList<point2d> &points, double sz)
{
    if(points.size() > 3)
    {
        // определяем геометрический центр
        point2d center;
        foreach(point2d p, points)
            center += p;
        center = center / points.size();

        // если направление роста не направлено наружу объекта
        // то меняем направление на противоположное
        point2d pr = (points[0] - points[1]).rotate2dAround(point2d(), M_PI_2);
        if(pr.dot(points[0] - center) < 0)
            std::reverse(points.begin(), points.end());
    }

    auto make = [&](point2d prev, point2d curr, point2d next) -> QList<point2d> {
        // определяем перпендикуляр в заданной точке
        point2d pr1 = (prev == curr) ? point2d()
                                     : (prev - curr).rotate2dAround(point2d(), M_PI_2).resized(sz);
        point2d pr2 = (next == curr) ? point2d()
                                     : (next - curr).rotate2dAround(point2d(), -M_PI_2).resized(sz);

        // всегда в одну сторону
        QList<point2d> result;
        result << curr + pr1 << curr + pr2;
        return result;
    };

    return enlargeRegion(points, sz, make);
}

QList<point2d> makeAHull(QList<point2d> points)
{
    QSet<point2d> src = points.toSet();

    // делаем из набора точек полигон
    point2d current = *src.begin();
    foreach(point2d p, src)
    {
        // ищем самую нижнюю и левую точку
        if((fabs(p.y - current.y) < 1e-6 && p.x < current.x) || p.y < current.y)
            current = p;
    }

    // итак оболочка
    QList<point2d> hull;
    hull << current;
    src.remove(current);

    double zeroAngle = 0.0;

    auto angleTo = [&](double a, double base) -> double {
        //        Q_ASSERT( -M_PI <= a && a <= 2*M_PI && -M_PI <= base && base <= 2*M_PI );
        return fmod(a - base + M_PI * 2 * 8, M_PI * 2);
    };

    auto lessByPolarAngle = [&](point2d x, point2d y) -> bool {
        // отсчитываем полярный угол от предыдущей грани
        double xa = angleTo((x - current).angle(), zeroAngle);
        double ya = angleTo((y - current).angle(), zeroAngle);

        //        if ( fabs(xa-ya)<POINT_PREC * POINT_PREC)
        //            return current.distance(x) < current.distance(y);

        return xa < ya;
    };

    while(src.size())
    {
        // ищем элемент с самым маленьким полярным углом, относительно текущей точки
        auto   p           = *std::min_element(src.begin(), src.end(), lessByPolarAngle);
        double polar_angle = (p - current).angle();
        //        double ta = angleTo(polar_angle, zeroAngle);

        // проверяем подходит ли найденная точка лучше чем исходная точка
        // если да, то круг замкнулся, оболочка завершена
        if(hull.size() > 1 && !lessByPolarAngle(p, hull.first()))
        {
            Q_ASSERT(isHull(hull));
            return hull;
        }

        // устанавливаем как текущий элемент
        current = p;
        src.remove(current);

        // добавляем найденную точку в оболочку и продолжнаем поиски
        hull << current;

        // определяем угол
        zeroAngle = polar_angle;
    }

    Q_ASSERT(isHull(hull));
    return hull;
}

QList<point2d> makePointListFromItemsList(QList<PGItem> items)
{
    QList<point2d> src;
    foreach(auto item, items)
    {
        if(auto clip = item.dynamicCast<GraphicsClipItem>())
            item = clip->subItem();

        if(auto line = item.dynamicCast<QGraphicsLineItem>())
        {
            src << point2d::fromQPoint(line->line().p1());
            src << point2d::fromQPoint(line->line().p2());
        }
        else if(auto path = item.dynamicCast<QGraphicsPathItem>())
        {
            QPainterPath ppath = path->path();
            for(int index = 0; index < ppath.elementCount(); ++index)
            {
                src << point2d::fromQPoint(ppath.elementAt(index));
            }
        }
        else if(auto pitem = item.dynamicCast<QGraphicsPolygonItem>())
        {
            foreach(auto p, pitem->polygon())
                src << point2d::fromQPoint(p);
        }
        else if(auto ellipse = item.dynamicCast<QGraphicsEllipseItem>())
        {
            QPainterPath ppath = ellipse->shape();
            for(int index = 0; index < ppath.elementCount(); ++index)
            {
                auto p = (QPointF)ppath.elementAt(index);
                p      = item->mapToParent(p);
                src << point2d::fromQPoint(p);
            }
        }
    }

    return makeAHull(src);
}

QList<QGraphicsItem *> drawRectangle(QGraphicsScene *gscene,
                                     point2d         a,
                                     point2d         b,
                                     QPen            pen,
                                     QBrush          brush)
{
    return drawRectangle(gscene, a, point2d(a.x, b.y), b, point2d(b.x, a.y), pen, brush);
}

point2d bestElementNormal(WorldSystem &ws, point3d gp0, point3d gp1)
{
    // определяем лучшее направление перпендикуляра
    point3d d = (gp1 - gp0).normalized();
    return bestElementNormal(ws, d, flangesHalfSize);
}

point2d bestElementNormal(WorldSystem &ws, point3d d, double size)
{
    point3d norm = bestNormalDirection(ws, d);

    auto gp0 = point3d();
    auto gpx = gp0 + norm * size;

    auto r = ws.toUser(gpx) - ws.toUser(gp0);
    return r;
}

point2d elementNormal(WorldSystem &ws, point3d d)
{
    bool ok  = false;
    auto gp0 = point3d();
    auto gpx = gp0 + d.normalized(&ok) * flangesHalfSize;
    return ws.toUser(gp0) - ws.toUser(gpx);
}

point3d bestNormalDirection(WorldSystem &ws, point3d d)
{
    QList<point3d> list;
    list << point3d(0, 0, 1);
    list << point3d(1, 0, 0);
    list << point3d(0, 1, 0);

    auto status         = [&](point3d const &x) -> int { return fabs(x.dot(d)) / POINT_PREC; };
    auto minimal_status = [&](point3d const &x, point3d const &y) { return status(x) < status(y); };
    auto best           = *std::min_element(list.begin(), list.end(), minimal_status);

    return d.cross(best);
}

} // namespace geometry
