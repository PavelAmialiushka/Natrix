#ifndef LINEINFO_H
#define LINEINFO_H

#include "scenePropertyValue.h"

namespace geometry
{

enum LineStyle
{
    LineNormalStyle,
    LineThinStyle,
    LineSelectedStyle,
    LineDashDotStyle,
};

enum BendJoinerStyle
{
    BendWeldedStyle,
    BendSimpleStyle,
    BendCastStyle,
};

enum TeeJoinerStyle
{
    TeeInsetStyle,
    TeeWeldedStyle,
    TeeCastStyle,
};

enum EndJoinerStyle
{
    EndNormalStyle,
    EndNoStyle
};

class LineInfo
{
public:
    int lineStyle;
    int bendStyle;
    int teeStyle;
    int endStyle;

public:
    LineInfo()
    {
        lineStyle = LineStyle::LineNormalStyle;
        bendStyle = BendJoinerStyle::BendWeldedStyle;
        teeStyle  = TeeJoinerStyle::TeeInsetStyle;
        endStyle  = EndJoinerStyle::EndNormalStyle;
    }

    ScenePropertyList getStyles() const
    {
        ScenePropertyList result;
        result << ScenePropertyValue(ScenePropertyType::LineStyle, lineStyle);
        result << ScenePropertyValue(ScenePropertyType::BendStyle, bendStyle);
        result << ScenePropertyValue(ScenePropertyType::TeeStyle, teeStyle);
        result << ScenePropertyValue(ScenePropertyType::EndStyle, teeStyle);
        return result;
    }

    bool apply(ScenePropertyValue v)
    {
        if(v.type == ScenePropertyType::LineStyle)
        {
            if(lineStyle == v.current)
                return false;
            lineStyle = v.current;
            return true;
        }
        else if(v.type == ScenePropertyType::BendStyle)
        {
            if(bendStyle == v.current)
                return false;
            bendStyle = v.current;
            return true;
        }
        else if(v.type == ScenePropertyType::TeeStyle)
        {
            if(teeStyle == v.current)
                return false;
            teeStyle = v.current;
            return true;
        }
        else if(v.type == ScenePropertyType::EndStyle)
        {
            if(endStyle == v.current)
                return false;
            endStyle = v.current;
            return true;
        }
        return false;
    }
};

} // namespace geometry

#endif // LINEINFO_H
