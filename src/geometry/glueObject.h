#ifndef GEOMETRY_FAKEELEMENT_H
#define GEOMETRY_FAKEELEMENT_H

#include "element.h"

namespace geometry
{

MAKESMART(GlueObject);

class GlueObject : public Object
{
public:
    GlueObject(Scene *s);
    static PObject createFromList(Scene *, QList<NodeInfo>);
    static PObject create(Scene *, point3d a, point3d b);

    PObject      clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);

    virtual double width() const;

    virtual PObject cloneMove(Scene *scene, point3d delta);
    virtual PObject cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle);
};

} // namespace geometry

#endif // GEOMETRY_FAKEELEMENT_H
