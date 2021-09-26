#ifndef STRAIGHTLINEADORNER_H
#define STRAIGHTLINEADORNER_H

#include "adorner.h"

namespace geometry
{

class StraightLineAdorner : public Adorner
{
    point3d start_, end_;

public:
    StraightLineAdorner(point3d start, point3d end);
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
    virtual bool isTemporal() const;
};

typedef QSharedPointer<StraightLineAdorner> PStraightLineAdorner;

} // namespace geometry

#endif // STRAIGHTLINEADORNER_H
