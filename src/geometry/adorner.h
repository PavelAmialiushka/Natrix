#ifndef ADORNER_H
#define ADORNER_H

#include <QSharedPointer>

#include "baseObject.h"
#include "primitives.h"

class QGraphicsItem;

namespace geometry
{
class Scene;
class GraphicsScene;

class Adorner : public ObjectToDraw
{
public:
    Adorner();

public:
    virtual bool   isTemporal() const;
    virtual double distance(point2d const &pt);
};

typedef QSharedPointer<Adorner> PAdorner;

} // namespace geometry

#endif // ADORNER_H
