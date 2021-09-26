#ifndef CANVASADORNER_H
#define CANVASADORNER_H

#include "canvasRectangle.h"
#include "grip.h"

namespace geometry
{

MAKESMART(CanvasGrip);

class CanvasGrip : public Grip
{
    point2d          lt_, rb_;
    int              index_;
    PCanvasRectangle canvas_;

public:
    CanvasGrip(int index, PCanvasRectangle canvas);

    PCanvasRectangle canvas() const;

    void   draw(GraphicsScene *gscene, GItems &gitems, int level) override;
    double distance(const point2d &pt) override;
};

inline auto toCanvasGrip = [](PGrip l) { return l.dynamicCast<CanvasGrip>(); };

} // namespace geometry

#endif // CANVASADORNER_H
