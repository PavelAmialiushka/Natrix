#include "graphicsClipItem.h"
#include "graphicsScene.h"

#include "graphics.h"
#include "point.h"

#include "lineInfo.h"

#include <QPainter>

using namespace geometry;

GraphicsClipItem::GraphicsClipItem(QGraphicsItem *item)
    : QGraphicsItem(0)
    , subitem(item)
{
}

GraphicsClipItem::~GraphicsClipItem()
{
}

QSharedPointer<QGraphicsItem> GraphicsClipItem::subItem() const
{
    return subitem;
}

QGraphicsItem *GraphicsClipItem::create(geometry::GraphicsScene *gscene, QLineF line, QPen p)
{
    auto item = new QGraphicsLineItem(line);
    item->setPen(p);

    auto result = new GraphicsClipItem(item);
    gscene->addItem(result);
    return result;
}

QGraphicsItem *GraphicsClipItem::create(geometry::GraphicsScene *gscene, QPainterPath path, QPen p)
{
    auto item = new QGraphicsPathItem(path);
    item->setPen(p);

    auto result = new GraphicsClipItem(item);
    gscene->addItem(result);
    return result;
}

QList<QGraphicsItem *> GraphicsClipItem::create(geometry::GraphicsScene *gscene,
                                                QLineF                   l,
                                                int                      lineStyle)
{
    QList<QGraphicsItem *> items;

    QPen pen = graphics::linePen();
    if(lineStyle == LineThinStyle)
        pen = graphics::elementPen();
    else if(lineStyle == LineDashDotStyle)
        pen = graphics::dashDotPen();
    items << GraphicsClipItem::create(gscene, l, pen);
    if(lineStyle == LineSelectedStyle)
    {
        auto a = point2d::fromQPoint(l.p1());
        auto b = point2d::fromQPoint(l.p2());

        if(a.x > b.x)
            std::swap(a, b);
        else if(a.x == b.x && a.y > b.y)
            std::swap(a, b);

        bool ok = 0;
        auto d  = (a - b).rotate2dAround(0, M_PI_2).normalized(&ok) * 4;

        if(ok)
            items << GraphicsClipItem::create(
                gscene, makeQLine(a + d, b + d), graphics::lineSecondPen());
    }

    return items;
}

QList<QGraphicsItem *> GraphicsClipItem::create(geometry::GraphicsScene *gscene,
                                                QPainterPath             path,
                                                int                      lineStyle)
{
    QList<QGraphicsItem *> items;

    QPen pen = graphics::linePen();
    if(lineStyle == LineThinStyle)
        pen = graphics::elementPen();
    else if(lineStyle == LineDashDotStyle)
        pen = graphics::dashDotPen();

    items << GraphicsClipItem::create(gscene, path, pen);
    if(lineStyle == LineSelectedStyle)
    {
        point2d next, prev;

        QList<QPointF> qpoints;
        for(int index = 0; index < path.elementCount(); ++index)
        {
            next = point2d::fromQPoint(path.elementAt(index));

            auto a = prev;
            auto b = next;
            prev   = next;

            if(index == 0)
                continue;

            bool swap = false;
            if(a.x > b.x)
                swap = true;
            else if(a.x == b.x && a.y > b.y)
                swap = true;

            bool ok = 0;
            auto d  = (a - b).rotate2dAround(0, swap ? -M_PI_2 : M_PI_2).normalized(&ok) * 4;
            if(ok)
            {
                if(qpoints.empty())
                    qpoints << (a + d).toQPoint();
                qpoints << (b + d).toQPoint();
            }
        }
        for(int index = 0; index < qpoints.size() - 1; ++index)
        {
            QPointF a = qpoints[index];
            QPointF b = qpoints[index + 1];
            items << GraphicsClipItem::create(gscene, QLineF(a, b), graphics::lineSecondPen());
        }
    }

    return items;
}

void GraphicsClipItem::resetNegativePath()
{
    clipPath = QPainterPath();
    clipPath.setFillRule(Qt::WindingFill);
    clipPath.addRect(subitem->boundingRect());

    update();
}

void GraphicsClipItem::addNegativePath(QPainterPath path)
{
    clipPath -= path;
    update();
}

QRectF GraphicsClipItem::boundingRect() const
{
    return subitem->boundingRect();
}
QPainterPath GraphicsClipItem::shape() const
{
    return subitem->shape();
}
bool GraphicsClipItem::contains(const QPointF &point) const
{
    return subitem->contains(point);
}
bool GraphicsClipItem::collidesWithItem(const QGraphicsItem * other,
                                        Qt::ItemSelectionMode mode) const
{
    return subitem->collidesWithItem(other, mode);
}
bool GraphicsClipItem::collidesWithPath(const QPainterPath &path, Qt::ItemSelectionMode mode) const
{
    return subitem->collidesWithPath(path, mode);
}
bool GraphicsClipItem::isObscuredBy(const QGraphicsItem *item) const
{
    return subitem->isObscuredBy(item);
}
QPainterPath GraphicsClipItem::opaqueArea() const
{
    return subitem->opaqueArea();
}

// Drawing
void GraphicsClipItem::paint(QPainter *                      painter,
                             const QStyleOptionGraphicsItem *option,
                             QWidget *                       widget)
{
    if(!clipPath.isEmpty())
    {
        painter->setClipPath(clipPath);
        painter->setClipping(true);
    }

    // вызываем оригинальный рисователь
    subitem->paint(painter, option, widget);

    if(!clipPath.isEmpty())
        painter->setClipping(false);
}

int GraphicsClipItem::type() const
{
    return subitem->type();
}

void GraphicsClipItem::setPen(QPen pen)
{
    if(auto i = subitem.dynamicCast<QGraphicsLineItem>())
    {
        i->setPen(pen);
    }
    else if(auto i = subitem.dynamicCast<QGraphicsPathItem>())
    {
        i->setPen(pen);
    }
    else if(auto i = subitem.dynamicCast<QGraphicsTextItem>())
    {
        i->setDefaultTextColor(pen.brush().color());
    }
}

void GraphicsClipItem::setPenBrush(QBrush penBrush)
{
    if(auto i = subitem.dynamicCast<QGraphicsLineItem>())
    {
        auto pen = i->pen();
        pen.setBrush(penBrush);
        i->setPen(pen);
    }
    else if(auto i = subitem.dynamicCast<QGraphicsPathItem>())
    {
        auto pen = i->pen();
        pen.setBrush(penBrush);
        i->setPen(pen);
        auto bru = i->brush();
        bru.setColor(penBrush.color());
        i->setBrush(bru);
    }
    else if(auto i = subitem.dynamicCast<QGraphicsTextItem>())
    {
        i->setDefaultTextColor(penBrush.color());
    }
}
