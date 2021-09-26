#include "element.h"
#include "elementFactory.h"
#include "global.h"
#include "glueObject.h"
#include "objectFactory.h"
#include "pointProcedures.h"
#include "scales.h"
#include "scene.h"
#include "sceneProcedures.h"
#include "tmatrix.h"

#include <QDebug>
#include <QVariant>
#include <math.h>

namespace geometry
{

ElementInfo::ElementInfo()
{
    scaleFactor      = 0;
    scale            = 1.0;
    startNode        = 0;
    elementJointType = ElementJoint::Flanged;
}

ElementInfo::ElementInfo(QString t)
    : ElementInfo()
{
    elementName = t;
}

bool ElementInfo::hasFlanges() const
{
    return elementJointType == ElementJoint::Flanged;
}

ElementParams ElementInfo::params() const
{
    return ElementFactory::inst().getInfo(elementName);
}

void ElementInfo::assign(ElementInfo other)
{
    bool acceptAny = elementName.isEmpty();
    auto p         = params();

    // меняем фланцы
    if(acceptAny || p.isFlangeMutable)
    {
        elementJointType = other.elementJointType;
    }
    else
    {
        elementJointType = p.defaultJoint;
    }

    // меняем масштаб
    setScaleFactor(other.scaleFactor);
}

void ElementInfo::setScaleFactor(int sf)
{
    scaleFactor = sf;
    scale       = toScale(sf);
}

void ElementInfo::setFlanges(bool flanges)
{
    if(elementName.isEmpty() || params().isFlangeMutable)
    {
        if(flanges)
            elementJointType = ElementJoint::Flanged;
        else
            elementJointType = ElementJoint::Welded;
    }
}

bool ElementInfo::applyStyle(ScenePropertyValue v)
{
    if(v.type == ScenePropertyType::ValveSize)
    {
        int sf = SceneProperties::getElementScale(v.current);

        if(sf == scaleFactor)
            return false;

        setScaleFactor(sf);
        return true;
    }
    else if(v.type == ScenePropertyType::ValveFlanges)
    {
        if(!params().isFlangeMutable ||
           v.current == (elementJointType == ElementJoint::Welded ? 0 : 1))
            return false;

        setFlanges(v.current);
        return true;
    }

    return false;
}

ElementInfo ElementInfo::nextSiblingOptions() const
{
    static QString names = "Valve,CheckValve,KipValve,ElectricValve,,"
                           "FlangePair,FlangePairBlind,Diaphragm,,"
                           "Pump,Pump2,Pump2/2,,"
                           "Valve3,Valve3/2,,"
                           "VesselNossle,VesselNossle2,VesselNossle3,,"
                           "Trans,ExTrans,Semisphere,,"
                           "SafetyValve,AngleValve,AngleValve/1,,";
    static QStringList items = names.split(",");

    QString name = elementName.section("Element", 0, 0);
    if(startNode != 0)
        name += QString("/%1").arg(startNode);

    QString siblingName;
    int     index = items.indexOf(name);
    if(index == -1)
        siblingName = name;
    else
    {
        if(++index == items.size() || items[index].isEmpty())
        {
            while(index != 0 && !items[index - 1].isEmpty())
                --index;
        }

        siblingName = items[index];
    }

    ElementInfo sibling = *this;

    int sn              = siblingName.section("/", 1, 1).toInt();
    sibling.startNode   = sn;
    sibling.elementName = siblingName.section("/", 0, 0) + "Element";

    return sibling;
}

QString Element::elementName() const
{
    return elementName_;
}

Element::Element(QString name)
    : Object(0, 0)
    , elementName_(name)
    , info_(name)
    , lineStyle_(0)
{
}

PObject Element::cloneMove(Scene *scene, point3d delta)
{
    point3d start = globalPoint(0) + delta;
    point3d dir   = -direction(0);
    return cloneMoveRotate(scene, 0, start, dir);
}

// повернуть элемент так,
// чтобы Node под номером nodeNo
// 1 оказалась в точке position
// 2 её direction стал равен dir0
// 3 а первый перпендикулярный нод стал указывать в dir1
PElement Element::cloneMoveRotate(Scene * scene,
                                  int     nodeNo,
                                  point3d position,
                                  point3d dir0,
                                  point3d dir1)
{
    //    Q_ASSERT(dir1.empty() || dir0.isNormal(dir1));
    Q_ASSERT(!dir0.empty());
    Q_ASSERT(!dir0.isCoaimed(dir1));

    dir0 = dir0.normalized();

    // исходная точка, от которой пляшем
    point3d start    = globalPoint(nodeNo);
    point3d startDir = direction(nodeNo);

    // текущее ортогональное направление (если есть)
    // будет найдено ниже
    point3d ortoDir;

    // получаем перечень исходных точек элемента
    QList<NodeInfo> nodeList;
    foreach(PNode n, nodes())
    {
        nodeList << NodeInfo{n->globalPoint(), n->direction(), n->jointType()};

        // подыскиваем первое направление, непараллельное заданному
        if(dir1 && !ortoDir && !n->direction().isParallel(startDir))
        {
            ortoDir = n->direction();
        }
    }

    // изменение направлений
    point3d dir0start = -direction(nodeNo);

    // вращение, которое меняет направление точки
    // перед вращением необходимо сместиться в центр вращения
    TMatrix rotate1 = ortoDir && dir1 ? TMatrix::makeFromPoints(dir0start, ortoDir, dir0, dir1)
                                      : TMatrix::makeFromPoints(dir0start, dir0);

    // функция вращения
    auto rotate = [&](NodeInfo &info) {
        info.globalPoint = (info.globalPoint - start) * rotate1 + position;
        info.direction   = info.direction * rotate1;
        return info;
    };

    // выполняем заданное вращение
    std::transform(nodeList.begin(), nodeList.end(), nodeList.begin(), rotate);

    return clone(scene, nodeList);
}

PObject Element::cloneMoveRotate(Scene *scene, point3d delta, point3d center, double angle)
{
    QList<NodeInfo> nodeList;
    foreach(auto node, nodes())
    {
        NodeInfo info;
        info.direction    = node->direction().rotate3dAround(0, point3d::nz, angle);
        info.globalPoint  = delta + node->globalPoint().rotate3dAround(center, point3d::nz, angle);
        info.jointType    = node->jointType();
        info.weldPosition = node->weldPosition();
        nodeList << info;
    }
    return clone(scene, nodeList);
}

PElement Element::clone(Scene *scene, ElementInfo opt)
{
    PObject n1 = neighbour(0);
    PObject n2 = neighbour(1);

    bool noway   = n1 && n2;
    bool somehow = n1 || n2;

    int scaleFactor = opt.scaleFactor;

    point3d scenter;
    if(noway)
    {
        scaleFactor = info().scaleFactor;
        opt.setScaleFactor(scaleFactor);
        scenter = center();
    }
    else if(somehow)
    {
        if(n1)
            scenter = globalPoint(0);
        else
            scenter = globalPoint(1);
    }
    else
    {
        scenter = center();
    }

    auto newbyElement = cloneResize(scene, scenter, scaleFactor);
    newbyElement->setInfo(opt);

    return newbyElement;
}

PElement Element::cloneResize(Scene *scene, point3d centerOfResize, int scaleFactor)
{
    auto opt = info();
    if(scaleFactor != opt.scaleFactor)
    {
        opt.setScaleFactor(scaleFactor);
    }
    double scale = opt.scale / info().scale;

    QList<NodeInfo> nodeList;
    foreach(PNode n, nodes())
    {
        auto gp = n->globalPoint().scale(centerOfResize, scale);
        nodeList << NodeInfo{gp, n->direction(), n->jointType()};
    }

    auto copy = clone(scene, nodeList);
    copy->setInfo(opt);

    return copy;
}

PElement Element::cloneMoveResize(Scene *scene, point3d delta1, point3d delta2)
{
    QList<NodeInfo> nodeList;
    nodeList << NodeInfo{globalPoint(0) + delta1, direction(0), nodeAt(0)->jointType()};
    nodeList << NodeInfo{globalPoint(1) + delta2, direction(1), nodeAt(1)->jointType()};

    auto copy = clone(scene, nodeList);
    copy->setInfo(info());

    return copy;
}

void Element::setElementName(QString n)
{
    elementName_ = n;
}

void Element::setInfo(ElementInfo options)
{
    info_.assign(options);
}

bool Element::canBreakLine(int ix) const
{
    for(int index = nodeCount(); index-- > 0;)
    {
        if(direction(index) == -direction(ix) &&
           (globalPoint(index) - globalPoint(ix)).isParallel(direction(ix)))
            return true;
    }
    return false;
}

double Element::breakingSize(int) const
{
    return globalPoint(0).distance(globalPoint(1));
}

bool Element::canBeFlipped(int) const
{
    return !info_.params().canNonBeFlipped;
}

bool Element::canBeRotated(int ix) const
{
    point3d d0 = direction(ix);
    for(int index = nodeCount(); index-- > 0;)
    {
        if(!direction(index).isParallel(d0))
        {
            return true;
        }
    }
    return false;
}

point3d Element::center(int i, int j) const
{
    Q_ASSERT((unsigned)i < (unsigned)nodeCount() && (unsigned)j < (unsigned)nodeCount());

    bool cross, nonparallel;
    auto result = cross_lines3d(globalPoint(i),
                                globalPoint(i) + direction(i),
                                globalPoint(j),
                                globalPoint(j) + direction(j),
                                &cross,
                                &nonparallel);
    Q_ASSERT(cross && nonparallel);
    return result;
}

point3d Element::center(int i) const
{
    if(!direction(0).isParallel(direction(i)))
        return center(0, i);
    else if(!direction(1).isParallel(direction(i)))
        return center(1, i);
    else if(nodeCount() > 2 && !direction(2).isParallel(direction(i)))
        return center(2, i);

    return globalPoint(i) - direction(i);
}

point3d Element::center() const
{
    if(nodeCount() == 1)
    {
        return globalPoint(0);
    }
    else if(nodeCount() == 2)
    {
        if(direction(0).isParallel(direction(1)))
        {
            return (globalPoint(0) + globalPoint(1)) / 2;
        }
        else
        {
            return center(0, 1);
        }
    }

    int  counter = 0;
    auto cent    = point3d();

    if(!direction(0).isParallel(direction(1)))
        ++counter, cent += center(0, 1);
    if(!direction(0).isParallel(direction(2)))
        ++counter, cent += center(0, 2);
    if(!direction(1).isParallel(direction(2)))
        ++counter, cent += center(1, 2);

    return cent / counter;
}

ElementInfo Element::info() const
{
    return info_;
}

ElementParams Element::params() const
{
    return info_.params();
}

int Element::lineStyle() const
{
    return lineStyle_;
}

void Element::setLineStyle(int s)
{
    lineStyle_ = s;
}

bool Element::applyStyle(ScenePropertyValue v)
{
    if(v.type == ScenePropertyType::LineStyle)
    {
        if(v.current == lineStyle_)
            return false;

        setLineStyle(v.current);
        return true;
    }

    // запрещаем изменение размера
    if(v.type == ScenePropertyType::ValveSize)
        return false;

    ElementInfo info = info_;
    if(info.applyStyle(v))
    {
        setInfo(info);
        return true;
    }

    return false;
}

void Element::applyStyle(PObject obj)
{
    if(PElement elem = obj.dynamicCast<Element>())
    {
        setInfo(elem->info());
        setLineStyle(elem->lineStyle());
    }
}

ScenePropertyList Element::getStyles()
{
    ScenePropertyList r;
    r << ScenePropertyValue(ScenePropertyType::LineStyle, lineStyle_);
    r << ScenePropertyValue(ScenePropertyType::ValveSize,
                            SceneProperties::fromElementScale(info_.scaleFactor));
    r << ScenePropertyValue(ScenePropertyType::ValveFlanges, info_.hasFlanges());
    return r;
}

double Element::width() const
{
    return weldStrokeHalfSize * 2 * info_.scale + 2;
}

double Element::interactWith(PObject other) const
{
    if(auto el = other.dynamicCast<Element>())
    {
        auto op1 = info();
        auto op2 = el->info();
        if(op1.elementJointType == ElementJoint::Flanged &&
           op2.elementJointType == ElementJoint::Flanged)
        {
            int scaleF = (op1.scale + op2.scale) / 2;
            return flangesWidth * toScale(scaleF);
        }
    }

    return 0;
}

void Element::saveObject(QVariantMap &map)
{
    Object::saveObject(map);

    map["lineStyle"] = lineStyle_;

    map["scale"] = info().scaleFactor;
    if(info().elementJointType == ElementJoint::Flanged)
        map["flanges"] = 1;
}

bool Element::loadObject(QVariantMap map)
{
    Object::loadObject(map);

    auto opt = info();

    if(map.contains("scale"))
    {
        int size = map["scale"].toInt();
        opt.setScaleFactor(size);
    }

    if(map.contains("lineStyle"))
    {
        int s = map["lineStyle"].toInt();
        setLineStyle(s);
    }

    if(map.contains("flanges"))
    {
        opt.elementJointType = ElementJoint::Flanged;
    }
    else
        opt.elementJointType = ElementJoint::Welded;

    setInfo(opt);

    return 1;
}

// наличие фланца у соседнего элемента
bool Element::isFlangeVisible(int i) const
{
    if(info_.elementJointType != ElementJoint::Flanged)
        return false;

    if(i < 0 || i >= nodes_.size())
        return true;

    PObject n1 = neighbour(i);
    if(!n1)
        return true;

    PGlueObject g = n1.dynamicCast<GlueObject>();
    if(g)
    {
        auto set = neighbours(scene_, g);
        if(set.size() < 2)
            return false;
        if(set[0] == sharedFromThis())
            n1 = set[1];
        else
            n1 = set[0];
    }

    PElement p1 = n1.dynamicCast<Element>();
    // присоединен не к элементу
    if(!p1)
        return true;

    // присоединен к элементу с фланцами
    bool isNeighbourFlanged = p1->info().elementJointType == ElementJoint::Flanged;

    if(isNeighbourFlanged)
    {
        return false;
    }

    return true;
}

bool hasFlanges(PObject obj)
{
    return hasFlanges(obj.dynamicCast<Element>());
}

bool hasFlanges(PElement el)
{
    return el->info().elementJointType == ElementJoint::Flanged;
}

double getSizeDefect(PObject obj1, PObject obj2)
{
    if(auto el = obj1.dynamicCast<Element>())
        return getSizeDefect(el, obj2);

    return 0;
}

double getSizeDefect(PElement el, PObject obj2)
{
    if(auto el2 = obj2.dynamicCast<Element>())
    {
        return el->getInteraction(obj2);
    }
    return 0;
}

} // namespace geometry
