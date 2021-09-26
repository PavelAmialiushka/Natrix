#ifndef OBJECTFACTORY_H
#define OBJECTFACTORY_H

#include "object.h"

#include <QDomElement>

#include <functional>
using std::function;

namespace geometry
{

class Scene;
class ObjectFactory
{
public:
    // базовый способ создания объекта
    typedef function<PObject(Scene *, QVariantMap)> creatorQV;

    // создание фиксированных объектов по списку нодов
    typedef function<PObject(Scene *, QList<NodeInfo>)> creatorNL;
    static creatorQV                                    NL2QV(creatorNL);

    template <class T>
    static PObject createElement(Scene *scene, QList<NodeInfo> list)
    {
        PObject ob(new T);
        ob->setNodeList(scene, list);
        ob->setPObject(ob);
        return ob;
    }

public:
    static void      registerClass(QString className, QString typeInfoName, creatorQV creator);
    static QString   classNameFromTypeName(QString);
    static creatorQV getObjectCreator(QString className);

public:
    // создание объекта
    static PObject createObject(Scene *, QDomElement objectElement);
    static PObject createObject(Scene *, QVariantMap);

    static void saveObject(PObject object, QDomElement objectElement);
};

#define MAKE_UNIQ_NAME(classname) makeUniq##classname##Registrator
/*
 * #define CREATE_CLASS_BY_DOMELEMENT(classname) \
 *   int MAKE_UNIQ_NAME(classname) = (ObjectFactory::registerClass(#classname,
 * typeid(classname).name(), \ ObjectFactory::ObjectCreatorDE(&classname::createFrom)), 1)
 */

#define CREATE_CLASS_BY_NODELIST(classname)                                                        \
    int MAKE_UNIQ_NAME(classname) =                                                                \
        (ObjectFactory::registerClass(#classname,                                                  \
                                      typeid(classname).name(),                                    \
                                      ObjectFactory::NL2QV(&classname::createFromList)),           \
         1) //

#define CREATE_CLASS_ELEMENT_SUBCLASS(classname)                                                   \
    int MAKE_UNIQ_NAME(classname) =                                                                \
        (ObjectFactory::registerClass(                                                             \
             #classname,                                                                           \
             typeid(classname).name(),                                                             \
             ObjectFactory::NL2QV(&ObjectFactory::createElement<classname>)),                      \
         ElementFactory::registerInfo(#classname, classname::params),                              \
         1) //

} // namespace geometry

#endif // OBJECTFACTORY_H
