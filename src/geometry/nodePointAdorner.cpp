#include "nodePointAdorner.h"

#include "scene.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

NodePointAdorner::NodePointAdorner(point3d point, NodePointAdornerStyle style)
    : point_(point)
    , style_(style)
{
}

void NodePointAdorner::draw(GraphicsScene *gscene, GItems &g, int /*level*/)
{
    WorldSystem &ws    = *gscene->ws();
    auto         point = ws.toUser(point_);

    QPen pen;
    switch(style_)
    {
    case AdornerLineFromFree: // черный
        pen = QPen(QBrush(QColor(0, 0, 0, 128)), 3, Qt::SolidLine, Qt::RoundCap);
        break;

    case AdornerLineFromNode:
    case AdornerMovingToNode:
    case AdornerMovingFromNode: // синий
        pen = QPen(QBrush(QColor(0, 0, 255, 128)), 3, Qt::SolidLine, Qt::RoundCap);
        break;

    case AdornerMovingToFree:
    case AdornerLineFromNone: // красный
        pen = QPen(QBrush(QColor(255, 0, 0, 128)), 3, Qt::SolidLine, Qt::RoundCap);
        break;

    case AdornerLineFromLine:
    case AdornerMovingToLine:
    case AdornerMovingFromLine: // зеленоватый
        pen = QPen(QBrush(QColor(21, 179, 0, 128)), 3, Qt::SolidLine, Qt::RoundCap);
        break;

    case AdornerOverWeld:
    case AdornerMovingWeld: // оранжевый
        pen = QPen(QBrush(QColor(255, 90, 255, 128)), 3, Qt::SolidLine, Qt::RoundCap);
        break;

    case AdornerLineShortenerInactive:
    case AdornerLineShortener: // shorner
        pen = QPen(QBrush(QColor(118, 34, 221, 128)), 3, Qt::SolidLine, Qt::RoundCap);
        break;
    }

    switch(style_)
    {
    case AdornerLineFromNone:
    case AdornerLineFromNode:
    case AdornerLineFromLine:
    case AdornerLineFromFree:
        // line styles
        g.items << drawBox(gscene, point, 4, pen);
        break;

    case AdornerMovingFromNode:
    case AdornerMovingFromLine:
    case AdornerMovingToFree:
    case AdornerMovingToNode:
    case AdornerMovingToLine: // move

        g.items << drawCircle(gscene, point, 4, pen);
        break;
    case AdornerOverWeld:
    case AdornerMovingWeld: // weld

        g.items << drawDiamond(gscene, point, 4, pen);
        break;
    case AdornerLineShortenerInactive:
    case AdornerLineShortener: // shorner
        g.items << drawDiamond(gscene, point, 4, pen);
        break;
    }
}

bool NodePointAdorner::isTemporal() const
{
    return true;
}
} // namespace geometry
