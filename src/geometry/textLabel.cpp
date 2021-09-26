#include "textLabel.h"
#include "scene.h"

#include <QDomElement>
#include <QTextCursor>
#include <QTextDocument>

#include "object.h"

#include <QSet>

#include "graphics.h"
#include "graphicsScene.h"
#include "graphicsTextItem.h"
#include "sceneProcedures.h"

#include <QDebug>

namespace geometry
{

CREATE_LABEL_BY_DOMELEMENT(TextLabel);

TextLabel::TextLabel(Scene *scene, TextInfo info)
    : Label(scene)
{
    info_        = info;
    leftTop_     = info.basePoint;
    rightBottom_ = info.basePoint;

    setActualSize(0, 0);
}

TextLabel::~TextLabel()
{
}

PLabel TextLabel::create(Scene *const scene, point3d pt, TextInfo info)
{
    auto pw        = scene->worldSystem();
    info.basePoint = pt >> *pw;

    return PTextLabel(new TextLabel(scene, info));
}

PLabel TextLabel::createFrom(Scene *scene, class QDomElement elm)
{
    TextInfo info;
    info.load(elm);

    info.setAngle(scene, info.rotationAngleDegrees);

    return create(scene, info);
}

void TextLabel::saveLabel(QDomElement elm)
{
    info_.save(elm);
}

bool TextLabel::loadLabel(QDomElement elm)
{
    info_.load(elm);
    return 1;
}

QList<point2d> TextLabel::getCornerPoints()
{
    QTransform trans;
    trans.translate(info_.basePoint.x, info_.basePoint.y);
    trans.rotate(info_.rotationAngleDegrees);

    QList<point2d> result;
    auto           item = item_.data();
    if(!item)
        return result;

    QRectF rect = item->boundingRect();
    result << point2d::fromQPoint(trans.map(rect.topLeft()))
           << point2d::fromQPoint(trans.map(rect.topRight()))
           << point2d::fromQPoint(trans.map(rect.bottomRight()))
           << point2d::fromQPoint(trans.map(rect.bottomLeft()));
    return result;
}

void TextLabel::setActualSize(double w, double h)
{
    width_  = w;
    height_ = h;
}

double TextLabel::distance(const point2d &pt)
{
    QList<point2d> points = getCornerPoints();

    // проверяем, находится ли точка внутри полигона
    QVector<QPointF> qpoints;
    foreach(auto point, points)
        qpoints << point.toQPoint();
    QPolygonF poly = qpoints;
    if(poly.containsPoint(pt.toQPoint(), Qt::WindingFill))
        return 0;

    double d = 999;
    if(points.size())
    {
        points << points.first();
        for(int index = 0; index < points.size() - 1; ++index)
        {
            d = std::min(d, pt.distance_to_line(points[index], points[index + 1]));
        }
    }

    return d;
}

PLabel TextLabel::create(Scene *scene, TextInfo info)
{
    return PTextLabel(new TextLabel(scene, info));
}

void TextLabel::updateStickyMode(QList<PMarker> markers, TextLayout &self) const
{
    auto ws = scene_->worldSystem();

    if(PMarker stickyMarker = takeStickyMarker(scene_, info_, markers))
    {
        self.leaderElement  = stickyMarker->leader.dynamicCast<Object>();
        self.leaderPoint    = stickyMarker->point;
        self.stickyTextMode = self.leaderElement;
    }
    else
    {
        self.leaderElement.reset();
        self.stickyTextMode = false;
    }

    double dx = 0;
    double dy = 0;
    if(self.stickyTextMode)
    {
        // ищем центр
        point3d center         = 0;
        bool    isMarkerAtNode = 0;
        foreach(auto node, self.leaderElement->nodes())
        {
            center += node->globalPoint();
            if(node->globalPoint() == self.leaderPoint)
                isMarkerAtNode = true;
        }
        center = center / self.leaderElement->nodeCount();

        //определяем направление и выясняем, к какой ноде мы присоединены
        point3d dirToText = center - self.leaderPoint;
        bool    leftNode  = false;
        if(abs((dirToText >> *ws).angle() - info_.rotationAngleDegrees * M_PI / 180) < 0.1)
            leftNode = true;

        // корректируем положение текста
        dy = self.leaderElement->width() / 2;
        dx = info_.width / 2;
        if(isMarkerAtNode)
        {
            double sz =
                self.leaderElement->localPoint(0).distance(self.leaderElement->localPoint(1));
            if(leftNode)
                dx -= sz / 2 /*- 4*/;
            else
                dx += sz / 2 /* - 5*/;
        }
    }
    else if(info_.alignment == 1) // выравнивание по центру
    {
        dx = info_.width / 2 + 4;
    }
    else if(info_.alignment == 2) // выравнивание по правому краю
    {
        dx = info_.width + 8;
    }
    point2d delta = point2d(-dx, -dy).rotate2dAround(0, (info_.rotationAngleDegrees) * M_PI / 180);
    self.stickyDelta = QPointF{delta.x, delta.y};
}

PLabel TextLabel::cloneMoveRotate(Scene *            scene,
                                  point3d            delta,
                                  point3d            center,
                                  double             angle,
                                  MoveRotateContext &context) const
{
    auto &ws  = *scene->worldSystem();
    auto &ows = *context.otherScene->worldSystem();

    point2d delta2;
    point2d c0    = this->center();
    point3d point = ws.toGlobal(c0);

    TextInfo info = info_;

    if(context.ms.empty())
    { // без маркера

        point  = delta + point.rotate3dAround(center, point3d::nz, angle);
        delta2 = (point >> ws) - c0;
    }
    else if(PMarker m = takeStickyMarker(scene_, info_, context.ms))
    { // приклеенный к объекту

        point3d mpoint = m->point;
        mpoint         = delta + mpoint.rotate3dAround(center, point3d::nz, angle);

        PObject leader = m->leader.dynamicCast<Object>();
        if(leader)
        {
            point3d direction = leader->direction(0).rotate3dAround(0, point3d::nz, angle);
            info.setAngle(scene_, toDegrees(direction >> ws));
            info.setMarkerPoint(mpoint >> ws);
        }
    }
    else
    {
        // выноска
        point3d marker = context.ms.first()->point;

        // поворот вместе с маркерной точкой, с соблюдением положения
        point3d marker2 = delta + marker.rotate3dAround(center, point3d::nz, angle);

        // 3d точка текстовой метки, в горизонтальной плоскости привязки
        point = ws.toGlobal_atplane(c0, marker, marker + point3d::nx, marker + point3d::ny);

        // расстояние от старого центра до маркерной точки
        auto length = c0.distance(marker >> ows);

        // 3d направление на точку привязки
        bool ok;
        auto d1 = marker.polar_to(point, 1, &ok);
        if(!ok)
            d1 = point + point3d::nx;

        // новое направление привязки
        auto d2 = delta + d1.rotate3dAround(center, point3d::nz, angle);

        // новый центр, образованный поворотом
        auto c1 = (marker2 >> ws).polar_to(d2 >> ws, length, &ok);

        if(ok)
            delta2 = c1 - c0;
        else
            delta2 = 0;
    }

    info.basePoint += delta2;

    auto newby = new TextLabel(scene, info);
    newby->setActualSize(width_, height_);
    return PLabel(newby);
}

void TextLabel::draw(GraphicsScene *gscene, GItems &g, int level)
{
    auto ws = gscene->ws();

    if(activeLabel_)
        level = 1;

    // маркер
    QList<PMarker> markers = scene_->markersOfFollower(sharedFromThis().dynamicCast<Label>());

    // пытаемся понять, не привязаны ли мы к элементу
    TextLayout self;
    updateStickyMode(markers, self);

    // либо создаем либо получаем доступ к уже существующему
    // текстовому элементу
    QSharedPointer<GraphicsTextItem> item;
    if(level)
    {
        if(g.tryToUpdate())
        {
            item = g.items.first().dynamicCast<GraphicsTextItem>();
        }

        // контур в любом случае строится заново
        g.contur.clear();
        // элемент уже извлечен
        g.items.clear();

        Q_ASSERT(g.contur.gscene);

        bool justCreated = false;
        // если элемент не создан, создаем заново
        if(!item)
        {
            item_ = item = QSharedPointer<GraphicsTextItem>(new GraphicsTextItem(info_));
            item->setTextInfo(info_, true);
            justCreated = true;
        }
        else
        {
            item->setTextInfo(info_);
        }

        // устанавливаем положение текста
        QPointF point = info_.basePoint.toQPoint();
        double  width = item->boundingRect().width();
        if(width != info_.width)
        {
            updateStickyMode(markers, self);
            info_.width = width;
        }
        point += self.stickyDelta;
        item->setPos(point);

        // запоминаем
        QRectF rect = item->boundingRect();
        setActualSize(rect.width(), rect.height());

        if(justCreated)
        {
            gscene->addItem(item.data());
            item->setTextInfo(info_, 1);
        }

        g.items << item;
    }

    // рисование рамок и линеек
    QString text      = item ? item->toPlainText() : info_.text;
    int     lineCount = text.count('\n') + 1;
    double  section   = 1.0 / lineCount;

    // контур преобразуется согласно этой матрице
    QTransform trans;
    QPointF    point = info_.basePoint.toQPoint() + self.stickyDelta;
    trans.translate(point.x(), point.y());
    trans.rotate(info_.rotationAngleDegrees);

    // размеры и положение
    QRectF rect =
        item ? item->boundingRect()
             : QRectF(QPointF(0, 0), QSizeF(std::max(20.0, width_), std::max(15.0, height_)));
    QPointF pointB = trans.map(rect.bottomLeft() * section + rect.topLeft() * (1 - section));
    QPointF pointC = trans.map(rect.bottomRight() * section + rect.topRight() * (1 - section));

    QList<point2d> rectanglePoints;
    QRectF         textRect;
    switch(info_.decoration)
    {
    case TextInfo::NormalStyle:
        textRect = rect;
        break;
    case TextInfo::SquareStyle:
        textRect = rect.adjusted(-2, 0, 2, 0);
        break;
    case TextInfo::EllipseStyle:

        int dx = std::max(0., rect.height() - rect.width() - 2) / 2;
        if(lineCount > 2)
            dx = 0;

        textRect = rect.adjusted(-dx - 2, 0, 2 + dx, 0);
        break;
    }

    rectanglePoints << point2d::fromQPoint(trans.map(textRect.topLeft()));
    rectanglePoints << point2d::fromQPoint(trans.map(textRect.topRight()));
    rectanglePoints << point2d::fromQPoint(trans.map(textRect.bottomRight()));
    rectanglePoints << point2d::fromQPoint(trans.map(textRect.bottomLeft()));

    switch(info_.decoration)
    {
    case TextInfo::NormalStyle:
        if(level != 0)
            break;
        [[fallthrough]];
    case TextInfo::SquareStyle:
        g.items << drawRectangle(gscene,
                                 rectanglePoints[0],
                                 rectanglePoints[1],
                                 rectanglePoints[2],
                                 rectanglePoints[3],
                                 graphics::elementPen());
        break;
    case TextInfo::EllipseStyle:
        g.items << drawEllipse(gscene,
                               rectanglePoints[0],
                               rectanglePoints[1],
                               rectanglePoints[2],
                               rectanglePoints[3],
                               graphics::elementPen());
        break;
    }

    if(level)
    {
        // рисуем контур
        QList<point2d> points = info_.decoration == TextInfo::NormalStyle
                                    ? rectanglePoints
                                    : makePointListFromItemsList(g.items);
        enlargeRegion_hull(points, 4);
        g.contur << drawSpline(gscene, points);
    }

    if(markers.size())
    {
        bool anyMarkers = false;

        // рисуем вспомогательные линии
        foreach(PMarker marker, markers)
        {
            point2d markerPoint = marker->point >> *ws;
            if(markerPoint.distance(info_.markerPoint()) < 3)
                continue;
            QPointF mpoint = markerPoint.toQPoint();

            if(info_.decoration == TextInfo::NormalStyle)
            {
                auto b = point2d::fromQPoint(pointB);
                auto c = point2d::fromQPoint(pointC);

                auto tryToMoveLineAround =
                    [&](QPointF mpoint2, QPointF close, QPointF dist) -> QPointF {
                    auto p1 = point2d::fromQPoint(close);
                    auto p2 = point2d::fromQPoint(mpoint2);
                    auto p3 = point2d::fromQPoint(dist);
                    auto x  = p1.project_to_line(markerPoint, p2);
                    if(p3.distance(p1) > p3.distance(x) &&
                       markerPoint.distance(p2) > markerPoint.distance(p1))
                    {
                        g.items << gscene->addLine(QLineF(close, mpoint2), graphics::elementPen());
                        mpoint2 = close;
                    }

                    return mpoint2;
                };

                QPointF mpoint2;
                if(markerPoint.distance(b) < markerPoint.distance(c))
                {
                    mpoint2 = pointB;
                    mpoint2 = tryToMoveLineAround(
                        mpoint2, trans.map(rect.topLeft()), trans.map(rect.topRight()));
                    mpoint2 = tryToMoveLineAround(
                        mpoint2, trans.map(rect.bottomLeft()), trans.map(rect.bottomRight()));
                }
                else
                {
                    mpoint2 = pointC;
                    mpoint2 = tryToMoveLineAround(
                        mpoint2, trans.map(rect.topRight()), trans.map(rect.topLeft()));
                    mpoint2 = tryToMoveLineAround(
                        mpoint2, trans.map(rect.bottomRight()), trans.map(rect.bottomLeft()));
                }

                g.items << gscene->addLine(QLineF(mpoint2, mpoint), graphics::elementPen());
            }
            else
            {
                if(info_.decoration == TextInfo::EllipseStyle)
                {
                    addEllipsePoints(rectanglePoints);
                }

                auto minPoint = *std::min_element(
                    rectanglePoints.begin(), rectanglePoints.end(), [&](point2d a, point2d b) {
                        return a.distance(markerPoint) < b.distance(markerPoint);
                    });

                auto mpoint2 = minPoint.toQPoint();
                g.items << gscene->addLine(QLineF(mpoint2, mpoint), graphics::elementPen());
            }
            anyMarkers = true;
        }

        // маркеры есть, рисуем линию подчеркивания
        if(anyMarkers && info_.decoration == TextInfo::NormalStyle)
        {
            g.items << gscene->addLine(QLineF(pointB, pointC), graphics::elementPen());
        }
    }
}

PLabel TextLabel::clone(Scene *scene, point2d delta) const
{
    TextInfo copy = info_;
    copy.basePoint += delta;

    auto newby = new TextLabel(scene, copy);
    newby->setActualSize(width_, height_);
    return PLabel(newby);
}

void TextLabel::updateText(QString text)
{
    info_.text = text;
}

TextInfo TextLabel::info() const
{
    return info_;
}

QSharedPointer<GraphicsTextItem> TextLabel::item() const
{
    return item_.toStrongRef();
}

void TextLabel::setInfo(TextInfo inf)
{
    info_ = inf;
}

bool TextLabel::applyStyle(ScenePropertyValue v)
{
    TextInfo info = info_;
    if(info.applyStyle(scene_, v))
    {
        setInfo(info);
        return true;
    }
    return false;
}

ScenePropertyList TextLabel::getStyles()
{
    ScenePropertyList r;
    r << ScenePropertyValue(ScenePropertyType::TextSize,
                            SceneProperties::fromTextScale(info_.scale));
    r << ScenePropertyValue(ScenePropertyType::TextRotation, info_.rotationIndex);
    r << ScenePropertyValue(ScenePropertyType::TextDecoration, info_.decoration);
    return r;
}

void TextLabel::selectAll()
{
    QSharedPointer<GraphicsTextItem> gitem = item();

    QTextCursor cursor = gitem->textCursor();
    cursor.select(QTextCursor::Document);
    gitem->setTextCursor(cursor);
}

void attachTextLabelTo(Scene *scene, PTextLabel text, point3d point, point3d direction)
{
    TextInfo info = text->info();
    auto     ws   = scene->worldSystem();
    info.setAngle(scene, toDegrees(direction >> *ws));
    info.setMarkerPoint(point >> *ws);
    text->setInfo(info);
}

} // namespace geometry
