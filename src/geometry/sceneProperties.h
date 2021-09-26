#ifndef SCENEPROPERTIES_H
#define SCENEPROPERTIES_H

#include <QObject>
#include <QSet>

#include "baseObject.h"

#include "element.h"
#include "lineInfo.h"
#include "scenePropertyValue.h"
#include "textInfo.h"

class QtProperty;

namespace geometry
{

class SceneProperties
{
public:
    SceneProperties();

    QSet<PObjectToSelect> selection;

    LineInfo    defaultLineInfo;
    TextInfo    defaultTextInfo;
    ElementInfo defaultElementInfo;

    ScenePropertyValue textSize;
    ScenePropertyValue textRotation;
    ScenePropertyValue textDecoration;

    ScenePropertyValue valveSize;
    ScenePropertyValue valveFlanges;

    ScenePropertyValue lineStyle;
    ScenePropertyValue bendStyle;
    ScenePropertyValue teeStyle;
    ScenePropertyValue endStyle;

    void       loadSelected();
    void       addElement(struct ElementInfo const &);
    static int fromElementScale(int x);
    static int getElementScale(int x);

    void              addProperty(ScenePropertyValue v);
    ScenePropertyList getStyles();

    void addLineInfo(LineInfo);
    void addObjectToSelect(PObjectToSelect obj);

    void       addText(struct TextInfo const &);
    static int fromTextScale(int x);
    static int getTextScale(int x);

public:
    void loadDefaults();
};

} // namespace geometry

#endif // SCENEPROPERTIES_H
