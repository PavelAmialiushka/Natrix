#ifndef ELEMENT_H
#define ELEMENT_H

#include "object.h"

namespace geometry
{

class Element;
typedef QSharedPointer<Element> PElement;

MAKESMARTS(ElementInfo);

enum class ElementJoint
{
    Welded,
    Flanged,
};

enum class ElementConfiguration
{
    ValveType,
    CornerType,
    SpecialType,
};

struct ElementParams
{
    ElementConfiguration elementNodeType; // допустимы преобразовывания в прелах группы
    ElementJoint defaultJoint;

    bool canNonBeFlipped; // можно ли менять направление вдоль оси
    bool isFlangeMutable; // можно ли менять фланцы/сварка
};

struct ElementInfo
{
    QString elementName;

    double scale;
    int    scaleFactor;
    int    startNode;

    ElementJoint elementJointType;

public:
    bool          hasFlanges() const;
    ElementParams params() const;

public:
    void assign(ElementInfo other);
    void setScaleFactor(int);
    void setFlanges(bool flanges = true);

    bool applyStyle(ScenePropertyValue v);

    ElementInfo nextSiblingOptions() const;

public:
    ElementInfo();
    ElementInfo(QString);
    ElementInfo(ElementInfo const &) = default;
};

bool hasFlanges(PObject);
bool hasFlanges(PElement);

double getSizeDefect(PObject, PObject);
double getSizeDefect(PElement, PObject);

class Element : public Object
{
protected:
    QString elementName_;

protected:
    ElementInfo info_;
    int         lineStyle_;

    Element(QString name);

public:
    // ElementCreator typeCreator() const;

    QString elementName() const;
    void    setElementName(QString name);

    void          setInfo(ElementInfo info);
    ElementInfo   info() const;
    ElementParams params() const;

    int  lineStyle() const;
    void setLineStyle(int s);
    void applyStyle(PObject elem);

    bool              applyStyle(ScenePropertyValue v);
    ScenePropertyList getStyles();

    // имеет две ноды на одной прямой
    bool canBreakLine(int i) const;
    // расстояние между этими двумя нодами
    double breakingSize(int i) const;

    // элемент может вращаться вокруг главной оси
    // например насос, ППК
    bool canBeRotated(int i) const;

    // элемент несиметричен вдоль главной оси
    // (имеет направление, например обратный клапан)
    bool canBeFlipped(int i) const;

    // наличие фланца у соседнего элемента
    bool isFlangeVisible(int i) const;

    point3d center() const;
    // центр, лежащий на продолжении ноды с номером i
    point3d center(int i) const;
    // пересечение осей нодов с номерами i и j
    point3d center(int i, int j) const;

public:
    virtual double width() const;

    virtual double interactWith(PObject) const;

    virtual void saveObject(QVariantMap &);
    virtual bool loadObject(QVariantMap);

    // создает копию элемента с заданными нодами
    virtual PElement clone(Scene *, QList<NodeInfo>) const = 0;

    // наследует от PObject
    virtual PObject  cloneMove(Scene *, point3d delta);
    virtual PElement cloneMoveRotate(Scene *,
                                     int,
                                     point3d position,
                                     point3d dir0,
                                     point3d dir1 = point3d(0, 0));
    virtual PObject  cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle);

    PElement clone(Scene *, ElementInfo info);

    PElement cloneResize(Scene *, point3d position, int scaleFactor);
    PElement cloneMoveResize(Scene *, point3d delta1, point3d delta2);
};

} // namespace geometry

#endif // ELEMENT_H
