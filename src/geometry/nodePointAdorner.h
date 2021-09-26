#ifndef NODEPOINTADORNER_H
#define NODEPOINTADORNER_H

#include "adorner.h"

namespace geometry
{

// enum NodePointType { ConnectToNodeActivePoint,
//                     ConnectToLineActivePoint
//                    };

enum NodePointAdornerStyle
{
    // creating line
    AdornerLineFromNone,
    AdornerLineFromNode,
    AdornerLineFromLine,
    AdornerLineFromFree,
    // moving
    AdornerMovingFromNode,
    AdornerMovingFromLine,
    AdornerMovingToFree,
    AdornerMovingToNode,
    AdornerMovingToLine,
    // welds
    AdornerOverWeld,
    AdornerMovingWeld,
    // shortener
    AdornerLineShortener,
    AdornerLineShortenerInactive,
};

class NodePointAdorner : public Adorner
{
    point3d               point_;
    NodePointAdornerStyle style_;

public:
    NodePointAdorner(point3d, NodePointAdornerStyle);

    void draw(GraphicsScene *gscene, GItems &gitems, int level);
    bool isTemporal() const;
};

} // namespace geometry

#endif // NODEPOINTADORNER_H
