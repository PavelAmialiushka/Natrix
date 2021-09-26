#ifndef ELEMENTFACTORY_H
#define ELEMENTFACTORY_H

#include "element.h"

#include <QList>
#include <QMap>
#include <QStringList>

#include <functional>

namespace geometry
{

class ElementFactory
{
    QMap<QString, std::function<PElement()>> prototypeMap;

    ElementFactory();

public:
    static ElementFactory &inst();

    QStringList getList() const;

    ElementParams getInfo(QString name) const;

    PElement prototype(QString name) const;

    static PElement createElement(Scene *, ElementInfo);
    static PElement createElement(Scene *, QString name, QList<NodeInfo> nodes);

    static void registerInfo(QString q, ElementParams);
};

} // namespace geometry
#endif // ELEMENTFACTORY_H
