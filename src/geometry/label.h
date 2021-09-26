#ifndef LABEL_H
#define LABEL_H

#include "baseObject.h"
#include "point.h"
#include "primitives.h"

#include <functional>
using std::function;

#include <QSharedPointer>

#include <QDomElement>

namespace geometry
{

MAKESMART(Marker);

class Scene;
typedef QSharedPointer<class Label> PLabel;

struct MoveRotateContext
{
    // исходные маркеры
    QList<PMarker> ms;

    // исходная сцена
    Scene *otherScene;
};

class Label : public ObjectToSelect
{
protected:
    point2d leftTop_;
    point2d rightBottom_;
    bool    activeLabel_;
    Scene * scene_;

public:
    Label(Scene *scene);
    void replaceScene(Scene *newby);

    point2d center() const;
    void    moveCenterTo(point2d);
    void    setSize(double, double);

    bool isActiveLabel() const;
    void setLabelActive(bool = true);

    point2d leftTop() const;
    point2d rightTop() const;
    point2d leftBottom() const;
    point2d rightBottom() const;

    virtual void   draw(GraphicsScene *gscene, GItems &gitems, int level) = 0;
    virtual PLabel clone(Scene *, point2d delta) const                    = 0;
    virtual PLabel cloneMoveRotate(Scene *            scene,
                                   point3d            delta,
                                   point3d            center,
                                   double             angle,
                                   MoveRotateContext &context) const;
    virtual PLabel clone(Scene *, point3d delta) const;

    // загрузка из XML
    typedef function<PLabel(Scene *, QDomElement)> labelCreatorDE;
    static void registerLabelCreator(QString name, QString type, labelCreatorDE);
    QString     typeName() const;

    static PLabel createLabel(Scene *scene, QDomElement);
    static void   saveLabel(PLabel, QDomElement);

    virtual void saveLabel(QDomElement);
    virtual bool loadLabel(QDomElement);
};

#define MAKE_UNIQ_NAME(classname) makeUniq##classname##Registrator
#define CREATE_LABEL_BY_DOMELEMENT(classname)                                                      \
    int MAKE_UNIQ_NAME(classname) =                                                                \
        (Label::registerLabelCreator(                                                              \
             #classname, typeid(classname).name(), Label::labelCreatorDE(&classname::createFrom)), \
         1)

} // namespace geometry
#endif // LABEL_H
