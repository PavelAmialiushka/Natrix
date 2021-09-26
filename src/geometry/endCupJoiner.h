#ifndef ENDCUPJOINER_H
#define ENDCUPJOINER_H

#include "joiner.h"

namespace geometry
{

// конец линии
MAKESMART(EndCupJoiner);
class EndCupJoiner : public Joiner
{
    int endStyle_;

    EndCupJoiner(Scene *sc, point3d p0, point3d dir);

public:
    static PObject create(Scene *sc, point3d p0, point3d dir);
    static PObject createFromList(Scene *sc, QList<NodeInfo> list);

    int               endStyle() const;
    void              setEndStyle(int s);
    bool              applyStyle(ScenePropertyValue v);
    ScenePropertyList getStyles();

    virtual void    draw(GraphicsScene *gscene, GItems &gitems, int level);
    virtual double  distanceToPoint(int) const;
    virtual PObject cloneMove(Scene *scene, point3d delta);
    virtual PObject cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle);

    virtual void applyStyle(PObject);
    virtual void applyStyle(LineInfo);
    virtual void saveObject(QVariantMap &map);
    virtual bool loadObject(QVariantMap map);
};

MAKESMART(EndCupJoiner);

} // namespace geometry

#endif // ENDCUPJOINER_H
