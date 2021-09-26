#include "neighbourhood.h"

namespace geometry
{

PNode ConnectionData::node() const
{
    return info.isNodeFound ? object->nodeAt(info.nodeNo) : PNode();
}

PNode ObjectInfo::node(PObject object) const
{
    return isNodeFound ? object->nodeAt(nodeNo) : PNode();
}

} // namespace geometry
