#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include "point.h"
#include "worldSystem.h"

#include <QGraphicsItem>
#include <QGraphicsScene>

#include "graphicsClipItem.h"

namespace geometry
{

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT

private:
    PWorldSystem pws_;
    point2d      cursorPoint_;
    bool         noCursorPoint_;
    bool         textEditMode_;

public:
    GraphicsScene(PWorldSystem pw);

    void         noCursorPoint();
    void         setCursorPoint(point2d p);
    PWorldSystem ws() const;

    void setTextEditMode(bool mode);

private:
    virtual void drawForeground(QPainter *painter, const QRectF &rect);
    virtual void focusInEvent(QFocusEvent *event);
};

} // namespace geometry

#endif // GRAPHICSSCENE_H
