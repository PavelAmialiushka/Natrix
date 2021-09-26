#ifndef TEXTINFO_H
#define TEXTINFO_H

#include <QList>
#include <QString>
#include <QtGlobal>

#include "makeSmart.h"
#include "point.h"

#include "scenePropertyValue.h"

class QDomElement;

namespace geometry
{

MAKESMARTS(TextInfo);

struct TextInfo
{
    enum TextDecorationStyle
    {
        NormalStyle,
        SquareStyle,
        EllipseStyle,
    };

    enum TextAlignment
    {
        LeftTextAlignment   = 0,
        CenterTextAlignment = 1,
        RightTextAlignment  = 2
    };

    QString text;
    // 0 - normal,
    // -1,-2... smaller
    // +1,+2 bigger
    int scale;

    // привязка
    // 0 влево
    // 1 центр
    int alignment;

    int decoration;

    // точка, к которой привязан текст
    // эта точка - верний левый угол текста
    point2d basePoint;

    // угол, на который наклонен текст
    //
    double rotationAngleDegrees;
    int    rotationIndex;
    void   setAngle(class Scene *scene, double a);
    void   setRotationIndex(class Scene *scene, int index);
    bool   applyStyle(class Scene *scene, ScenePropertyValue v);

    ////////////////////////////////////////////

    // количество строк
    int lines; // calculate from text

    // actual size
    double height; // calculate from scale
    double width;
    ////////////////////////////////////////////

    TextInfo();
    void setText(QString text);

    void    setRotationPoint(point2d);
    point2d rotationPoint() const;

    point2d markerPoint() const;
    void    setMarkerPoint(point2d);

    bool operator==(TextInfo const &inf) const;
    bool operator!=(TextInfo const &inf) const
    {
        return !operator==(inf);
    }

    void copyStyleFrom(TextInfo const &);
    void setTextScaleNo(int sc);

    void setWidth(int);

    //
    void save(QDomElement);
    void load(QDomElement);
};

/////////////////////////////////
//
//  интерфейс используется как промежуточное звено между редактором текста
//  и отображением текста.
//    редактор текста владеет указателем на этот интерфейс и через него
//    сообщает об изменениях текста, которые должны быть отражены
//
class ITextEditor
{
public:
    // текущее состояние текста
    virtual TextInfo textInfo() const = 0;

    // точка, с которой началось редактирование
    virtual point2d clickedPoint() const = 0;

    // текст изменился
    virtual void checkChanged() = 0;
    virtual void checkFocus()   = 0;

    virtual bool eventInItem(int x, int y) = 0;

    // изменение размера и угла поворота текста
    virtual void changeRotation(int delta) = 0;

    // успешное окончание редактирования
    virtual void editFinished() = 0;
};

} // namespace geometry

#endif // TEXTINFO_H
