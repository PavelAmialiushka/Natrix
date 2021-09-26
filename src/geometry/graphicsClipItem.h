#ifndef GRAPHICSCLIPITEM_H
#define GRAPHICSCLIPITEM_H

#include <QGraphicsItem>
namespace geometry
{
class GraphicsScene;
}

class GraphicsClipItem : public QGraphicsItem
{
    QSharedPointer<QGraphicsItem> subitem;
    QPainterPath                  clipPath;

public:
    explicit GraphicsClipItem(QGraphicsItem *item);
    ~GraphicsClipItem();

    static QGraphicsItem *create(geometry::GraphicsScene *gscene, QLineF l, QPen p);
    static QGraphicsItem *create(geometry::GraphicsScene *gscene, QPainterPath l, QPen p);

    static QList<QGraphicsItem *> create(geometry::GraphicsScene *gscene, QLineF l, int lineStyle);
    static QList<QGraphicsItem *> create(geometry::GraphicsScene *gscene,
                                         QPainterPath             l,
                                         int                      lineStyle);

    void resetNegativePath();
    void addNegativePath(QPainterPath path);

    void setPen(QPen pen);
    void setPenBrush(QBrush brush);

    QSharedPointer<QGraphicsItem> subItem() const;

private:
    virtual QRectF       boundingRect() const;
    virtual QPainterPath shape() const;
    virtual bool         contains(const QPointF &point) const;
    virtual bool         collidesWithItem(const QGraphicsItem * other,
                                          Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    virtual bool         collidesWithPath(const QPainterPath &  path,
                                          Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    virtual bool         isObscuredBy(const QGraphicsItem *item) const;
    virtual QPainterPath opaqueArea() const;

    // Drawing
    virtual void paint(QPainter *                      painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *                       widget = 0);

    virtual int type() const;
signals:
public slots:
};

#endif // GRAPHICSCLIPITEM_H
