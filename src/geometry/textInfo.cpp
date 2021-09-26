#include "textInfo.h"
#include "global.h"
#include "scene.h"

#include <cmath>

#include <QDomElement>

namespace geometry
{

// TODО: избавиться от класса TextInvoValue

void TextInfo::setAngle(Scene *scene, double a)
{
    rotationAngleDegrees = a;
    rotationIndex        = scene->textAngles().indexOf((int)round(a));
}

void TextInfo::setRotationIndex(Scene *scene, int index)
{
    auto list = scene->textAngles();

    rotationIndex        = index % list.size();
    rotationAngleDegrees = list[rotationIndex];
}

TextInfo::TextInfo()
    : alignment(0)
    , rotationAngleDegrees(0)
    , rotationIndex(2)
    , lines(1)
    , decoration(TextInfo::NormalStyle)
{
    setTextScaleNo(0);
}

bool TextInfo::operator==(TextInfo const &inf) const
{
    return text == inf.text && scale == inf.scale &&
           rotationAngleDegrees == inf.rotationAngleDegrees;
}

void TextInfo::copyStyleFrom(const TextInfo &sample)
{
    alignment            = sample.alignment;
    rotationAngleDegrees = sample.rotationAngleDegrees;
    decoration           = sample.decoration;
    setTextScaleNo(sample.scale);
}

// static const double TextNormalHeight = 13;
void TextInfo::setTextScaleNo(int sc)
{
    scale  = qMin(4, qMax(-1, sc));
    height = TextNormalHeight * pow(1.5, scale);
}

bool TextInfo::applyStyle(Scene *scene, ScenePropertyValue v)
{
    if(v.type == ScenePropertyType::TextSize)
    {
        int current = SceneProperties::getTextScale(v.current);
        if(current == scale)
            return false;
        setTextScaleNo(current);
        return true;
    }
    else if(v.type == ScenePropertyType::TextRotation)
    {
        if(v.current == rotationIndex)
            return false;
        setRotationIndex(scene, v.current);
        return true;
    }
    else if(v.type == ScenePropertyType::TextDecoration)
    {
        if(v.current == decoration)
            return false;
        decoration = v.current;
        return true;
    }
    return false;
}

void TextInfo::setWidth(int w)
{
    width = w;
}

point2d TextInfo::rotationPoint() const
{
    return basePoint + point2d(0, height).rotate2dAround(0, rotationAngleDegrees * M_PI / 180);
}

void TextInfo::setRotationPoint(point2d rp)
{
    point2d start = rotationPoint();
    point2d delta = rp - start;

    basePoint += delta;
}

point2d TextInfo::markerPoint() const
{
    return basePoint +
           point2d(0, 1.3 * height).rotate2dAround(0, rotationAngleDegrees * M_PI / 180);
}

void TextInfo::setMarkerPoint(point2d point)
{
    point2d start = markerPoint();
    point2d delta = point - start;

    basePoint += delta;
}

void TextInfo::save(QDomElement elm)
{
    elm.setAttribute("text", text);
    elm.setAttribute("scale", scale);
    elm.setAttribute("align", alignment);
    elm.setAttribute("rotation", rotationAngleDegrees);
    elm.setAttribute("width", width);
    elm.setAttribute("point", basePoint.serialSave());
    elm.setAttribute("decoration", (int)decoration);
}

void TextInfo::load(QDomElement elm)
{
    setText(elm.attribute("text", ""));
    scale                = elm.attribute("scale").toDouble();
    alignment            = elm.attribute("align").toInt();
    width                = elm.attribute("width").toDouble();
    rotationAngleDegrees = elm.attribute("rotation").toDouble();
    decoration = static_cast<TextDecorationStyle>(elm.attribute("decoration", "0").toInt());

    bool ok;
    basePoint = point2d::serialLoad(elm.attribute("point"), &ok);

    setTextScaleNo(scale);
    setWidth(width);
}

void TextInfo::setText(QString txt)
{
    text = txt;

    lines = 1;
    qCount(text.begin(), text.end(), '\n', lines);
}

} // namespace geometry
