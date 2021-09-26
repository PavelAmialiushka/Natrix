#ifndef WELDJOINER_H
#define WELDJOINER_H

#include "joiner.h"

namespace geometry
{

// отвод
MAKESMART(WeldJoiner);
class WeldJoiner : public Joiner
{
    WeldJoiner(Scene *sc, point3d p0, point3d d1);

public:
    static PObject create(Scene *sc, point3d p0, point3d d1, PObject sample);
    static PObject createFromList(Scene *sc, QList<NodeInfo> list);

    virtual void    draw(GraphicsScene *gscene, GItems &gitems, int level);
    virtual PObject cloneMove(Scene *, point3d delta);
    virtual PObject cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle);

    virtual void applyStyle(LineInfo);
};

} // namespace geometry
#endif // WELDJOINER_H
