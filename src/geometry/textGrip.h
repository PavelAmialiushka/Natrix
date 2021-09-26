#ifndef GEOMETRY_TEXTGRIP_H
#define GEOMETRY_TEXTGRIP_H

#include "grip.h"
#include "marker.h"

namespace geometry
{

MAKESMART(TextGrip);

class TextGrip : public Grip
{
    point2d point_;
    PMarker marker_;

public:
    TextGrip(point2d, PMarker marker);

    point2d point() const;
    PMarker marker() const;

private:
    virtual void   draw(GraphicsScene *gscene, GItems &gitems, int level) override;
    virtual double distance(const point2d &pt) override;
};

inline auto toTextGrip = [&](PGrip a) -> PTextGrip { return a.dynamicCast<TextGrip>(); };

} // namespace geometry

#endif // GEOMETRY_TEXTGRIP_H
