#ifndef WELDPROCEDURES_H
#define WELDPROCEDURES_H

#include "object.h"

#include <QList>
#include <QMultiMap>

namespace geometry
{

struct WeldInfo
{
    PObject object;
    int     nodeIndex;
    point3d globalPoint;
    point2d localPoint;

    bool operator==(WeldInfo const &rhs) const
    {
        return object == rhs.object;
    }
    bool operator==(PObject const &rhs) const
    {
        return object == rhs;
    }
};

QMultiMap<PObject, WeldInfo> object2welds(Scene *scene, QList<PObject> objects);

} // namespace geometry
#endif // WELDPROCEDURES_H
