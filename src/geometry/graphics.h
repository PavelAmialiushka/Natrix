#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <QGraphicsItem>
#include <QGraphicsScene>

#include "baseObject.h"
#include "point.h"
#include "worldSystem.h"

namespace geometry
{

point2d bestElementNormal(WorldSystem &ws, point3d gp0, point3d gp1);
point2d bestElementNormal(WorldSystem &ws, point3d d, double size);
point3d bestNormalDirection(WorldSystem &ws, point3d d);
point2d elementNormal(WorldSystem &ws, point3d d);

struct GItemsList : QList<PGItem>
{
    GraphicsScene *gscene;
    GItemsList(GraphicsScene *);
};

GItemsList operator*(GraphicsScene *gscene, QList<QGraphicsItem *> const &items);

GItemsList &operator<<(GItemsList &, QGraphicsItem *item);
GItemsList &operator<<(GItemsList &, QList<QGraphicsItem *> const &items);

PGItem makePGItem(QGraphicsItem *item, QGraphicsScene *scene);

struct GItems
{
    bool           update;
    GraphicsScene *gscene;

    // элементы, образующие объект
    GItemsList items, prevItems;

    // контур объекта
    GItemsList contur, prevContur;

    GItems(GraphicsScene *gscene);

    bool tryToUpdate();
};

struct graphics
{
    static QBrush transparentBrush();
    static QBrush blackBrush();

    static QBrush selectionBrush();
    static QBrush selectionPenBrush();
    static QBrush toBeSelectedBrush();
    static QBrush toBeSelectedPenBrush();
    static QBrush newbyBrush();
    static QBrush newbyPenBrush();
    static QBrush hoverBrush();
    static QBrush hoverPenBrush();
    static QBrush toModifyBrush();
    static QBrush toModifyPenBrush();
    static QBrush canvasAdornerBrush();

    static QBrush originalColour();

    static QPen linePen();
    static QPen lineSecondPen();
    static QPen elementPen();
    static QPen dashDotPen();

    static QPen vesselPen();
    static QPen viewportPen();

    static QPen ghostAdornerPen();

    // линия, которая подсвечивает направление перемещения
    static QPen moveLineAdornerPen();

    // линия, которая подрисовывае оси для участков прямой непараллельной осям
    static QPen straightLineAdornerPen();

    // прямоугольник, который выделяет области при удалении или выделении
    // вариант 1
    static QPen rectangle1AdornerPen();

    // вариант 2
    static QPen rectangle2AdornerPen();

    static QBrush textGripBrush();
};

// образ
QLineF makeQLine(point2d a, point2d b);

QPainterPath makeSimplePathFromPoints(QList<point2d>);
QPainterPath makeQuadPathFromPoints(QList<point2d>);

QList<QGraphicsItem *> drawSpline(QGraphicsScene *gscene,
                                  QList<point2d>,
                                  QPen   pen   = QPen(),
                                  QBrush brush = QBrush());

QList<QGraphicsItem *> drawQuadSpline(QGraphicsScene *gscene,
                                      QList<point2d>,
                                      QPen   pen   = QPen(),
                                      QBrush brush = QBrush());

QList<QGraphicsItem *> drawSelectedContur(GraphicsScene *gscene,
                                          QList<point2d>,
                                          point2d a,
                                          point2d b,
                                          QPen    pen   = QPen(),
                                          QBrush  brush = QBrush());

QList<QGraphicsItem *> drawSemiIsocircle(QGraphicsScene *gscene,
                                         point2d         a,
                                         point2d         b,
                                         point2d         c,
                                         QPen            pen   = QPen(),
                                         QBrush          brush = QBrush());

QList<QGraphicsItem *> drawCircle(QGraphicsScene *gscene,
                                  point2d         a,
                                  double          radius,
                                  QPen            pen   = QPen(),
                                  QBrush          brush = QBrush());

void addEllipsePoints(QList<point2d> &list);

QList<QGraphicsItem *> drawEllipse(QGraphicsScene *gscene,
                                   point2d         a,
                                   point2d         b,
                                   point2d         c,
                                   point2d         d,
                                   QPen            pen   = QPen(),
                                   QBrush          brush = QBrush());

QList<QGraphicsItem *> drawBox(QGraphicsScene *gscene,
                               point2d         a,
                               double          radius,
                               QPen            pen   = QPen(),
                               QBrush          brush = QBrush());

QList<QGraphicsItem *> drawDiamond(QGraphicsScene *gscene,
                                   point2d         a,
                                   double          radius,
                                   QPen            pen   = QPen(),
                                   QBrush          brush = QBrush());

QList<QGraphicsItem *> drawRectangle(QGraphicsScene *gscene,
                                     point2d         a,
                                     point2d         b,
                                     point2d         c,
                                     point2d         d,
                                     QPen            pen   = QPen(),
                                     QBrush          brush = QBrush());

QList<QGraphicsItem *> drawRectangle(QGraphicsScene *gscene,
                                     point2d         a,
                                     point2d         b,
                                     QPen            pen   = QPen(),
                                     QBrush          brush = QBrush());

QList<QGraphicsItem *> drawPumpCircle(QGraphicsScene *gscene,
                                      WorldSystem &   ws,
                                      point3d         c1,
                                      point3d         c2,
                                      point3d         direction);

// используется, чтобы визуально расширить регион из точек для создания контура

bool           isHull(QList<point2d> points);
QList<point2d> makeAHull(QList<point2d> points);
void           enlargeRegion_hull(QList<point2d> &points, double sz);
void           enlargeRegion_oneSide(QList<point2d> &points, double sz);

QList<point2d> makePointListFromItemsList(QList<PGItem> items);

void compressContur(GItemsList &contur);

} // namespace geometry

#endif // GRAPHICS_H
