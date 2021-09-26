#ifndef JOINER_H
#define JOINER_H

#include "object.h"

namespace geometry
{

// класс представляет собой такой объект, который
// может связываться только с линиями

MAKESMART(Joiner);
class Joiner : public Object
{
protected:
    int lineStyle_;

public:
    Joiner(Scene *sc, int n);

    double weldPosition(int n) const;
    void   setWeldPosition(int n, double d);

    point3d weldPoint(int n) const;

    int  lineStyle() const;
    void setLineStyle(int s);

    ScenePropertyList getStyles();

public:
    virtual double width() const;
    virtual double distanceToPoint(int) const;
    point3d        distantPoint(int) const;

    virtual void applyStyle(PObject other);
    virtual bool applyStyle(ScenePropertyValue v);

    virtual void saveObject(QVariantMap &map);
    virtual bool loadObject(QVariantMap map);
};

bool joinersFirst(PObject a, PObject b);

} // namespace geometry

#endif // JOINER_H
