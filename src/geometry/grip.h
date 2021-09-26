#ifndef GEOMETRY_GRIP_H
#define GEOMETRY_GRIP_H

#include "baseObject.h"

namespace geometry
{

MAKESMART(Grip);

class Grip : public ObjectToSelect
{
    bool temporal_;

public:
    Grip();

    bool isTemporal() const;
};

} // namespace geometry

#endif // GEOMETRY_GRIP_H
