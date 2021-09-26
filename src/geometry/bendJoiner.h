#ifndef BENDJOINER_H
#define BENDJOINER_H

#include "joiner.h"
#include "lineInfo.h"

namespace geometry
{

// отвод
MAKESMART(BendJoiner);
class BendJoiner : public Joiner
{
    int bendStyle_;

    BendJoiner(Scene *sc, point3d p0, point3d d1, point3d d2);

public:
    static PObject create(Scene * sc,
                          point3d p0,
                          point3d d1,
                          point3d d2,
                          PObject sample,
                          double  w1 = -1,
                          double  w2 = -1);
    static PObject createFromList(Scene *sc, QList<NodeInfo> list);

    int               bendStyle() const;
    void              setBendStyle(int s);
    bool              applyStyle(ScenePropertyValue v);
    ScenePropertyList getStyles();

public:
    virtual void    draw(GraphicsScene *gscene, GItems &gitems, int level);
    virtual PObject cloneMove(Scene *, point3d delta);
    virtual PObject cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle);

    virtual double interactWith(PObject) const;

    virtual void applyStyle(PObject);
    virtual void applyStyle(LineInfo);
    virtual void saveObject(QVariantMap &map);
    virtual bool loadObject(QVariantMap map);
};

} // namespace geometry

#endif // BENDJOINER_H
