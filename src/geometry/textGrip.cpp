#include "textGrip.h"

#include "graphics.h"
#include "graphicsScene.h"

namespace geometry
{

TextGrip::TextGrip(point2d p, PMarker m)
    : point_(p)
    , marker_(m)
{
}

point2d TextGrip::point() const
{
    return point_;
}

PMarker TextGrip::marker() const
{
    return marker_;
}

void TextGrip::draw(GraphicsScene *gscene, GItems &gitems, int /*level*/)
{
    gitems.items << drawBox(
        gscene, point_, 2, QPen(graphics::textGripBrush(), 1), graphics::textGripBrush());

    gitems.contur << drawBox(
        gscene, point_, 2, QPen(graphics::textGripBrush(), 1), graphics::textGripBrush());
    ;
}

double TextGrip::distance(const point2d &pt)
{
    return pt.distance(point_);
}

} // namespace geometry
