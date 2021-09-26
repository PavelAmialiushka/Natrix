#ifndef SCENEPROPERTYVALUE_H
#define SCENEPROPERTYVALUE_H

#include <QList>
#include <QMap>

namespace geometry
{

class SceneProperties;

enum class ScenePropertyType
{
    TextSize,
    TextRotation,
    TextDecoration,
    ValveSize,
    ValveFlanges,
    LineStyle,
    BendStyle,
    TeeStyle,
    EndStyle,
};

struct ScenePropertyValue
{
    ScenePropertyType type;
    QList<bool>       active;
    bool              exclusive; // выбран один активный
    bool matchStyleMode; // этот стиль используется инструментом MatchStyle
    int current;         // активный

    ScenePropertyValue(ScenePropertyType t = ScenePropertyType::TextSize, int current = -1);
    void reset(ScenePropertyType t, int current = -1);

    static ScenePropertyValue fromNumber(int n);

    // изменяем свойства объектов
    bool assignTo(ScenePropertyType t, int &external_value);

    bool empty() const;
    void addValue(int index, bool m = false);
};

using ScenePropertyList = QList<ScenePropertyValue>;
using ScenePropertyMap  = QMap<ScenePropertyType, ScenePropertyValue>;

ScenePropertyList propertyMapToList(ScenePropertyMap map);
ScenePropertyMap  propertyListToMap(ScenePropertyList list);

} // namespace geometry

#endif // SCENEPROPERTYVALUE_H
