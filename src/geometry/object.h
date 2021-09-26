#ifndef OBJECT_H
#define OBJECT_H

#include "baseObject.h"
#include "command.h"
#include "node.h"
#include "primitives.h"

#include <QSharedPointer>
#include <QVariant>
#include <QVector>

#include <QDomElement>

class GraphicsScene;
class QGraphicsItem;

namespace geometry
{

class Scene;
class Object;
class LineInfo;

MAKESMART(Object);
MAKESMART(BaseObject);

class Object : public ObjectToSelect
{
protected:
    QVector<PNode> nodes_;
    Scene *        scene_;
    QString        typeName_;

public:
    int                  generation;
    static const PObject NoSample;

    Object();
    Object(Scene *, int n);
    virtual ~Object();

    // объект запоминает умный указатель на себя
    void    setPObject(PObject);
    PObject pObject() const;

    // функция окончательно устанавливает положение нодов объекта
    void setNodeList(Scene *scene, QList<NodeInfo> list);

public:
    Scene *scene() const;
    void   replaceScene(Scene *newby);

    QString typeName() const;

    int   nodeCount() const;
    PNode nodeAt(int index) const;
    PNode nodeAtPoint(point3d) const;

    point3d globalPoint(int) const;
    point3d direction(int) const;
    point2d localPoint(int) const;

    double size(int i, int j) const;
    bool   hasPoint(point3d) const;

    PObject neighbour(int, bool overGlue = false) const;
    double  getInteraction(PObject) const;

    QList<PNode> nodes() const;

public:
    virtual double width() const = 0;
    virtual double interactWith(PObject) const;

    virtual PObject cloneMove(Scene *, point3d delta)                                     = 0;
    virtual PObject cloneMoveRotate(Scene *, point3d delta, point3d center, double angle) = 0;

    using ObjectToSelect::applyStyle;

    template <class T>
    void applyStyle(QSharedPointer<T> p)
    {
        applyStyle(p.template staticCast<Object>());
    }

    virtual void applyStyle(PObject);
    virtual void applyStyle(LineInfo);

public:
    virtual void saveObject(QVariantMap &);
    virtual bool loadObject(QVariantMap);

public:
    virtual double distance(point2d const &pt);
};

} // namespace geometry

#endif // OBJECT_H
