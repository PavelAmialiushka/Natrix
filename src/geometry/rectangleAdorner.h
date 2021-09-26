#ifndef RECTANGLEADORNER_H
#define RECTANGLEADORNER_H

#include "adorner.h"

namespace geometry
{

class RectangleAdorner : public Adorner
{
    point2d lt_, rb_;

public:
    RectangleAdorner(point2d a, point2d b);
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

} // namespace geometry

#endif // RECTANGLEADORNER_H
