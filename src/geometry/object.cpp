#include "object.h"
#include "objectFactory.h"
#include "scene.h"

#include <QMap>

#include "glueObject.h"
#include "sceneProcedures.h"

namespace geometry
{

const PObject Object::NoSample = PObject{};

Object::Object(Scene *s, int n)
    : scene_(s)
    , generation(0)
{
    nodes_.reserve(n);
    while(n--)
    {
        nodes_ << PNode(new Node());
    }
}

Scene *Object::scene() const
{
    return scene_;
}

void Object::replaceScene(Scene *newby)
{
    qSwap(scene_, newby);
}

void Object::setNodeList(Scene *scene, QList<NodeInfo> list)
{
    scene_ = scene;

    // если сцена задана, то отмечаем точки в соответствии со сценой
    // если нет, то используем заглушку, чтоб были хоть какие нибудь 2d точки
    PWorldSystem pws = scene ? scene->worldSystem() : WorldSystem::createOrtoWorldSystem();

    nodes_.clear();
    foreach(NodeInfo const &info, list)
    {
        PNode node{new Node};
        node->setPoint(pws, info.globalPoint, info.direction);
        node->setJointType(info.jointType);
        nodes_ << node;
    }
}

void Object::setPObject(PObject object)
{
    typeName_ = ObjectFactory::classNameFromTypeName(typeid(*this).name());
    foreach(PNode node, nodes_)
        node->setPObject(object);
}

PObject Object::pObject() const
{
    Q_ASSERT(nodes_.count());
    return nodes_.first()->object();
}

Object::~Object()
{
}

int Object::nodeCount() const
{
    return nodes_.count();
}
PNode Object::nodeAt(int index) const
{
    return nodes_[index];
}

double Object::size(int i, int j) const
{
    Q_ASSERT(i == 0 && j == 1);
    return globalPoint(i).distance(globalPoint(j));
}

bool Object::hasPoint(point3d p) const
{
    foreach(auto n, nodes())
        if(n->globalPoint() == p)
            return true;
    return false;
}

point3d Object::globalPoint(int index) const
{
    return nodeAt(index)->globalPoint();
}

point3d Object::direction(int index) const
{
    return nodeAt(index)->direction();
}

point2d Object::localPoint(int index) const
{
    return nodeAt(index)->localPoint();
}

PObject Object::neighbour(int i, bool overGlue) const
{
    PNode n1 = geometry::connectedNode(scene_, nodes_[i]);
    // не присоединен к чему либо
    if(!n1)
        return PObject();

    PObject nei = n1->object();
    if(nei.dynamicCast<GlueObject>() && overGlue)
    {
        // если соединяется через glue элемент
        PNode t = connectedNode(scene_, secondNode(commonNode(nei, pObject())));

        if(t)
            nei = t->object();
    }

    return nei;
}

PNode Object::nodeAtPoint(point3d pp) const
{
    for(int index = 0; index < nodeCount(); ++index)
    {
        if(nodeAt(index)->globalPoint() == pp)
            return nodeAt(index);
    }

    return PNode();
}

double Object::distance(point2d const &)
{
    return 0;
}

QList<PNode> Object::nodes() const
{
    return nodes_.toList();
}

double Object::getInteraction(PObject other) const
{
    double x = interactWith(other);
    if(x != 0)
        return x;
    else
        return -other->interactWith(nodes_.first()->object());
}

double Object::interactWith(PObject) const
{
    return 0;
}

void Object::applyStyle(PObject)
{
}

void Object::applyStyle(LineInfo)
{
}

QString Object::typeName() const
{
    return typeName_;
}

void Object::saveObject(QVariantMap &)
{
}

bool Object::loadObject(QVariantMap map)
{
    return 1;
}

} // namespace geometry
