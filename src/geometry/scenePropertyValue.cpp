#include "scenePropertyValue.h"

namespace geometry
{

ScenePropertyValue::ScenePropertyValue(ScenePropertyType t, int c)
{
    reset(t, c);
}

bool ScenePropertyValue::empty() const
{
    return current == -1;
}

void ScenePropertyValue::reset(ScenePropertyType t, int c)
{
    type = t;

    active.clear();
    exclusive      = true;
    matchStyleMode = false;
    current        = c;
}

ScenePropertyValue ScenePropertyValue::fromNumber(int n)
{
    switch(n)
    {
    case 1:
        return ScenePropertyValue{ScenePropertyType::LineStyle, 0};
    case 2:
        return ScenePropertyValue{ScenePropertyType::LineStyle, 1};
    case 3:
        return ScenePropertyValue{ScenePropertyType::LineStyle, 2};
    case 4:
        return ScenePropertyValue{ScenePropertyType::LineStyle, 3};
    case 5:
        return ScenePropertyValue{ScenePropertyType::BendStyle, 0};
    case 6:
        return ScenePropertyValue{ScenePropertyType::BendStyle, 1};
    case 7:
        return ScenePropertyValue{ScenePropertyType::BendStyle, 2};
    case 8:
        return ScenePropertyValue{ScenePropertyType::TeeStyle, 0};
    case 9:
        return ScenePropertyValue{ScenePropertyType::TeeStyle, 1};
    case 10:
        return ScenePropertyValue{ScenePropertyType::TeeStyle, 2};
    default:
        return ScenePropertyValue{};
    }
}

bool ScenePropertyValue::assignTo(ScenePropertyType t, int &external_value)
{
    if(t != type)
        return false;
    if(external_value == current)
        return false;

    external_value = current;
    return true;
}

void ScenePropertyValue::addValue(int index, bool m)
{
    Q_ASSERT(index >= 0);
    while(index >= active.size())
        active << false;

    matchStyleMode = m;

    if(current == -1)
        current = index;
    else if(current != index)
        exclusive = false;

    active[index] = true;
}

ScenePropertyList propertyMapToList(ScenePropertyMap map)
{
    ScenePropertyList result;
    foreach(ScenePropertyValue v, map)
        result << v;
    return result;
}

ScenePropertyMap propertyListToMap(ScenePropertyList list)
{
    ScenePropertyMap result;
    foreach(ScenePropertyValue v, list)
        result[v.type] = v;
    return result;
}

} // namespace geometry
