#include "objectFactory.h"
#include "object.h"

#include "element.h"
#include "scene.h"

#include <QMap>
#include <QVariant>

namespace geometry
{

namespace
{
QMap<QString, QString> &mapTypeInfoName2className()
{
    static QMap<QString, QString> instance;
    return instance;
}

QMap<QString, ObjectFactory::creatorQV> &mapClassName2creatorFn()
{
    static QMap<QString, ObjectFactory::creatorQV> instance;
    return instance;
}
} // namespace

QString ObjectFactory::classNameFromTypeName(QString typeInfoName)
{
    return mapTypeInfoName2className()[typeInfoName];
}

ObjectFactory::creatorQV ObjectFactory::getObjectCreator(QString className)
{
    Q_ASSERT(mapClassName2creatorFn().contains(className));
    return mapClassName2creatorFn()[className];
}

void ObjectFactory::registerClass(QString                  className,
                                  QString                  typeInfoName,
                                  ObjectFactory::creatorQV creator)
{
    mapTypeInfoName2className()[typeInfoName] = className;
    mapClassName2creatorFn()[className]       = creator;
}

///////////////////////////////////////////////////////////////////

static QList<NodeInfo> createNodeListFromQV(QVariantMap vmap)
{
    QList<NodeInfo> resultList;

    auto children = vmap["@children"].toList();
    foreach(QVariant vnod1, children)
    {
        auto vnode = vnod1.toMap();

        bool     ok;
        NodeInfo info;

        QString gpValue  = vnode["gp"].toString();
        info.globalPoint = point3d::serialLoad(gpValue, &ok);
        Q_ASSERT(ok);

        QString drValue = vnode["dr"].toString();
        info.direction  = point3d::serialLoad(drValue, &ok);
        Q_ASSERT(ok);

        QString typeValue = vnode["type"].toString();
        info.jointType    = typeValue.toInt(&ok);
        Q_ASSERT(ok);

        QString weldValue = vnode.value("weld", "-1").toString();
        info.weldPosition = weldValue.toDouble(&ok);
        Q_ASSERT(ok);

        resultList << info;
    };
    return resultList;
}

ObjectFactory::creatorQV ObjectFactory::NL2QV(ObjectFactory::creatorNL creatorNL)
{
    Q_ASSERT(creatorNL);
    return creatorQV(
        [=](Scene *scene, QVariantMap map) { return creatorNL(scene, createNodeListFromQV(map)); });
}

static QVariantMap domElement2QVariant(QDomElement elem)
{
    QVariantMap  result;
    QVariantList children;

    QDomElement child = elem.firstChildElement();
    while(!child.isNull())
    {
        children << domElement2QVariant(child);
        child = child.nextSiblingElement();
    }
    result["@name"]     = elem.nodeName();
    result["@children"] = children;

    auto attrs = elem.attributes();
    for(int index = 0; index < attrs.size(); ++index)
    {
        auto attr               = attrs.item(index);
        result[attr.nodeName()] = attr.nodeValue();
    }

    return result;
}

PObject ObjectFactory::createObject(Scene *scene, QDomElement objectElement)
{
    return createObject(scene, domElement2QVariant(objectElement));
}

PObject ObjectFactory::createObject(Scene *scene, QVariantMap vmap)
{
    QString className = vmap["class"].toString();

    // загружаем указатель на загрузчик
    creatorQV creator = getObjectCreator(className);
    if(!creator)
    {
        Q_ASSERT(!"incorrect classname");
        return PObject();
    }

    // при помощи загрузчика создаем объект
    PObject object = creator(scene, vmap);

    // загружаем дополнительные аттрибуты
    if(!object->loadObject(vmap))
    {
        Q_ASSERT(!"loadObject failed");
        return PObject();
    }

    return object;
}

///////////////////////////////////////////////////////////////////

void ObjectFactory::saveObject(PObject object, QDomElement objectElement)
{
    Q_ASSERT(!object->typeName().isEmpty());
    objectElement.setAttribute("class", object->typeName());

    foreach(PNode node, object->nodes())
    {
        auto elementNode = objectElement.ownerDocument().createElement("node");
        objectElement.appendChild(elementNode);

        elementNode.setAttribute("gp", node->globalPoint().serialSave());
        elementNode.setAttribute("dr", node->direction().serialSave());
        elementNode.setAttribute("type", node->jointType());
        elementNode.setAttribute("weld", node->weldPosition());
    }

    QVariantMap map;
    object->saveObject(map);

    foreach(QString key, map.keys())
    {
        objectElement.setAttribute(key, map[key].toString());
    }
}

} // namespace geometry
