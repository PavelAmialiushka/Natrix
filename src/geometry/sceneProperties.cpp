#include "sceneProperties.h"
#include "element.h"
#include "label.h"
#include "line.h"
#include "textLabel.h"

#include <QDebug>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <scene.h>

#include "bendJoiner.h"
#include "object.h"
#include "sceneProcedures.h"

#include "element.h"
#include "endCupJoiner.h"
#include "lineInfo.h"
#include "teeJoiner.h"

namespace geometry
{

SceneProperties::SceneProperties()
{
    textSize.reset(ScenePropertyType::TextSize);
    textRotation.reset(ScenePropertyType::TextRotation);
    textDecoration.reset(ScenePropertyType::TextDecoration);

    valveSize.reset(ScenePropertyType::ValveSize);
    valveFlanges.reset(ScenePropertyType::ValveFlanges);

    lineStyle.reset(ScenePropertyType::LineStyle);
    bendStyle.reset(ScenePropertyType::BendStyle);
    teeStyle.reset(ScenePropertyType::TeeStyle);
    endStyle.reset(ScenePropertyType::EndStyle);
}

void SceneProperties::loadDefaults()
{
    if(textSize.empty())
        addText(defaultTextInfo);

    if(valveSize.empty())
        addElement(defaultElementInfo);

    if(lineStyle.empty())
        lineStyle.addValue(defaultLineInfo.lineStyle);
    if(bendStyle.empty())
        bendStyle.addValue(defaultLineInfo.bendStyle);
    if(teeStyle.empty())
        teeStyle.addValue(defaultLineInfo.teeStyle);
    if(endStyle.empty())
        endStyle.addValue(defaultLineInfo.endStyle);
}

void SceneProperties::loadSelected()
{
    // выделение
    foreach(auto it, selection)
    {
        addObjectToSelect(it);
    }
}
void SceneProperties::addObjectToSelect(PObjectToSelect it)
{
    if(auto label = it.dynamicCast<TextLabel>())
    {
        addText(label->info());
    }
    else if(auto joiner = it.dynamicCast<Joiner>())
    {
        lineStyle.addValue(joiner->lineStyle());

        if(auto bend = it.dynamicCast<BendJoiner>())
            bendStyle.addValue(bend->bendStyle());
        if(auto tee = it.dynamicCast<TeeJoiner>())
            teeStyle.addValue(tee->teeStyle());
        if(auto end = it.dynamicCast<EndCupJoiner>())
            endStyle.addValue(end->endStyle());
    }
    else if(auto elem = it.dynamicCast<Element>())
    {
        lineStyle.addValue(elem->lineStyle());
        addElement(elem->info());
    }
    else if(auto line = it.dynamicCast<Line>())
    {
        lineStyle.addValue(line->lineStyle());
    }
}

void SceneProperties::addElement(const ElementInfo &info)
{
    int vs = fromElementScale(info.scaleFactor);
    valveSize.addValue(vs);

    int hf = static_cast<int>(info.hasFlanges());
    valveFlanges.addValue(hf);
}

int SceneProperties::fromElementScale(int x)
{
    return (x / 2) + 2;
}

int SceneProperties::getElementScale(int x)
{
    // 0=> -4
    // 1=> -2
    // 2=> 0
    // 3=> +2
    // 4=> +4

    return (x - 2) * 2;
}

void SceneProperties::addProperty(ScenePropertyValue v)
{
    switch(v.type)
    {
    case ScenePropertyType::TextSize:
        textSize.addValue(v.current, v.matchStyleMode);
        break;
    case ScenePropertyType::TextRotation:
        textRotation.addValue(v.current, v.matchStyleMode);
        break;
    case ScenePropertyType::TextDecoration:
        textDecoration.addValue(v.current, v.matchStyleMode);
        break;

    case ScenePropertyType::ValveSize:
        valveSize.addValue(v.current, v.matchStyleMode);
        break;
    case ScenePropertyType::ValveFlanges:
        valveFlanges.addValue(v.current, v.matchStyleMode);
        break;

    case ScenePropertyType::LineStyle:
        lineStyle.addValue(v.current, v.matchStyleMode);
        break;
    case ScenePropertyType::BendStyle:
        bendStyle.addValue(v.current, v.matchStyleMode);
        break;
    case ScenePropertyType::TeeStyle:
        teeStyle.addValue(v.current, v.matchStyleMode);
        break;
    case ScenePropertyType::EndStyle:
        endStyle.addValue(v.current, v.matchStyleMode);
        break;
    }
}

ScenePropertyList SceneProperties::getStyles()
{
    ScenePropertyList r;

    auto append = [&](ScenePropertyValue const &v) {
        if(!v.empty())
            r << v;
    };

    append(lineStyle);
    append(bendStyle);
    append(teeStyle);
    append(endStyle);

    append(textSize);
    append(textRotation);
    append(textDecoration);

    append(valveSize);
    append(valveFlanges);

    return r;
}

void SceneProperties::addLineInfo(LineInfo info)
{
    lineStyle.addValue(info.lineStyle);
    bendStyle.addValue(info.bendStyle);
    teeStyle.addValue(info.teeStyle);
    endStyle.addValue(info.endStyle);
}

void SceneProperties::addText(const TextInfo &info)
{
    textSize.addValue(fromTextScale(info.scale));
    textRotation.addValue(info.rotationIndex);
    textDecoration.addValue(info.decoration);
}

int SceneProperties::fromTextScale(int x)
{
    return x + 1;
}

int SceneProperties::getTextScale(int x)
{
    return x - 1;
}

} // namespace geometry
