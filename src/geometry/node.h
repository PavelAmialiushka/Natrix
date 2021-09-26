#ifndef NODE_H
#define NODE_H

#include "point.h"
#include "worldSystem.h"

#include <QSharedPointer>
#include <QWeakPointer>

namespace geometry
{

class Object;
typedef QSharedPointer<Object> PObject;
class Scene;

struct NodeInfo
{
    point3d globalPoint;     // 3d точка
    point3d direction;       // направление (наружу)
    int     jointType{0};    // 0 - контакты с объектами
    double  weldPosition{0}; //  положение шва
};

class Node
{
    friend class Object;
    QWeakPointer<Object> object_;

    point3d global_point_;
    point3d dir_;

    point2d local_point_;
    double  weld_position_;

    int joint_type_;

public:
    Node();
    void setPObject(PObject);

    point3d globalPoint() const;
    point2d localPoint() const;
    point3d direction() const;
    int     jointType() const; // 0 -- обычный нод

    double weldPosition() const;
    void   setWeldPosition(double d);

    void setPoint(PWorldSystem sys, point3d const &pt, point3d const &dr);
    void setJointType(int);

public:
    PObject object() const;
};

typedef QSharedPointer<Node> PNode;

} // namespace geometry
#endif // NODE_H
