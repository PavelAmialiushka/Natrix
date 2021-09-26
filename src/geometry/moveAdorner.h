#ifndef MOVEADORNER_H
#define MOVEADORNER_H

#include "adorner.h"

namespace geometry
{

class MoveAdorner : public Adorner
{
    bool    showStraight;
    point3d first, second;
    point2d first2d, second2d;

public:
    MoveAdorner(point3d f, point3d s, bool showStraight = true);
    MoveAdorner(point2d f, point2d s);

    void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

} // namespace geometry
#endif // MOVEADORNER_H
