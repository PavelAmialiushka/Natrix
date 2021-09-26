#include "elementFactory.h"
#include "element.h"
#include "elementTypes.h"
#include "objectFactory.h"

namespace geometry
{

namespace
{
QMap<QString, ElementParams> &elementName2Info()
{
    static QMap<QString, ElementParams> instance;
    return instance;
}
} // namespace

void ElementFactory::registerInfo(QString name, ElementParams info)
{
    elementName2Info()[name] = info;
}

ElementFactory::ElementFactory()
{
#define ADD_ELEMENT(classname)                                                                     \
    do                                                                                             \
    {                                                                                              \
        prototypeMap[#classname] = []() {                                                          \
            PElement e = PElement(new classname);                                                  \
            e->setPObject(e);                                                                      \
            return e;                                                                              \
        };                                                                                         \
    } while(0)

    ADD_ELEMENT(ValveElement);
    ADD_ELEMENT(Valve3Element);
    ADD_ELEMENT(DrainageElement);
    ADD_ELEMENT(CheckValveElement);
    ADD_ELEMENT(KipValveElement);
    ADD_ELEMENT(ElectricValveElement);
    ADD_ELEMENT(TransElement);
    ADD_ELEMENT(ExTransElement);
    ADD_ELEMENT(SemisphereElement);
    ADD_ELEMENT(FlangePairElement);
    ADD_ELEMENT(FlangePairBlindElement);
    ADD_ELEMENT(DiaphragmElement);
    ADD_ELEMENT(CondTapperElement);
    ADD_ELEMENT(CancelerElement);
    ADD_ELEMENT(SafetyValveElement);
    ADD_ELEMENT(AngleValveElement);
    ADD_ELEMENT(VesselNossleElement);
    ADD_ELEMENT(VesselNossle2Element);
    ADD_ELEMENT(VesselNossle3Element);
    ADD_ELEMENT(PumpElement);
    ADD_ELEMENT(Pump2Element);
}

ElementFactory &ElementFactory::inst()
{
    static ElementFactory instance;
    return instance;
}

PElement ElementFactory::prototype(QString name) const
{
    Q_ASSERT(prototypeMap.contains(name));
    auto defaultProto = prototypeMap.value(QString("ValveElement"));
    auto fn_creator   = prototypeMap.value(name, defaultProto);
    return fn_creator();
}

QStringList ElementFactory::getList() const
{
    QStringList list = prototypeMap.keys();
    return list;
}

ElementParams ElementFactory::getInfo(QString name) const
{
    return elementName2Info().value(name);
}

PElement ElementFactory::createElement(Scene *scene, ElementInfo opt)
{
    PElement proto = inst().prototype(opt.elementName);
    proto          = proto->cloneResize(scene, 0, opt.scaleFactor);
    proto->setInfo(opt);
    return proto;
}

PElement ElementFactory::createElement(Scene *scene, QString name, QList<NodeInfo> nodes)
{
    PElement proto = inst().prototype(name);
    proto          = proto->clone(scene, nodes);
    proto->setPObject(proto);
    return proto;
}

} // namespace geometry
