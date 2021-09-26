#include "baseObject.h"

#include "graphics.h"

namespace geometry
{

ObjectToDraw::~ObjectToDraw()
{
}

QBrush ObjectToDraw::originalColour() const
{
    return graphics::originalColour();
}

bool ObjectToSelect::applyStyle(PObjectToSelect sample)
{
    bool result = false;
    foreach(auto v, sample->getStyles())
        if(applyStyle(v))
            result = true;

    return result;
}

bool ObjectToSelect::applyStyle(ScenePropertyValue)
{
    return false;
}

ScenePropertyList geometry::ObjectToSelect::getStyles()
{
    return ScenePropertyList();
}

} // namespace geometry
