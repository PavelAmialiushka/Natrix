#ifndef TEEJOINER_H
#define TEEJOINER_H

#include "joiner.h"
#include "lineInfo.h"

namespace geometry
{

// тройник
MAKESMART(TeeJoiner);
class TeeJoiner : public Joiner
{
    int teeStyle_;

    TeeJoiner(Scene *sc, point3d p0, point3d d1, point3d d2);

public:
    // ноды 0 и 1 - параллельные направления
    static PObject create(Scene * sc,
                          point3d p0,
                          point3d d1,
                          point3d d2,
                          PObject sample,
                          double = 0,
                          double = 0,
                          double = 0);
    static PObject createFromList(Scene *sc, QList<NodeInfo> list);

    int  teeStyle() const;
    void setTeeStyle(int s);

    bool              applyStyle(ScenePropertyValue v);
    ScenePropertyList getStyles();

public:
    virtual void    draw(GraphicsScene *gscene, GItems &gitems, int level);
    virtual double  interactWith(PObject) const;
    virtual PObject cloneMove(Scene *, point3d delta);
    virtual PObject cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle);

    virtual void applyStyle(PObject);
    virtual void applyStyle(LineInfo);
    virtual void saveObject(QVariantMap &map);
    virtual bool loadObject(QVariantMap map);
};

} // namespace geometry

#endif // TEEJOINER_H
