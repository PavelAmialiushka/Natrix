#ifndef ELEMENTTYPES_H
#define ELEMENTTYPES_H

#include "element.h"

namespace geometry
{

// запорная арматура
class ValveElement : public Element
{
public:
    static ElementParams params;

public:
    ValveElement();

    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

class KipValveElement : public Element
{
public:
    static ElementParams params;

public:
    KipValveElement();

    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

class ElectricValveElement : public Element
{
public:
    static ElementParams params;

public:
    ElectricValveElement();

    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

class CheckValveElement : public Element
{
public:
    static ElementParams params;

public:
    CheckValveElement();

    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

// трёх-ходовой клапан арматура
class Valve3Element : public Element
{
public:
    static ElementParams params;

public:
    Valve3Element();

    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

// переход
class TransElement : public Element
{
public:
    static ElementParams params;

public:
    TransElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

// эксцентричный переход
class ExTransElement : public Element
{
public:
    static ElementParams params;

public:
    ExTransElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

// сферическая заглушка
class SemisphereElement : public Element
{
public:
    static ElementParams params;

public:
    SemisphereElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

// фланцевая пара
class FlangePairElement : public Element
{
public:
    static ElementParams params;

public:
    FlangePairElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

// фланцы с заглушкой
class FlangePairBlindElement : public Element
{
public:
    static ElementParams params;

public:
    FlangePairBlindElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

// диафрагма
class DiaphragmElement : public Element
{
public:
    static ElementParams params;

public:
    DiaphragmElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

////////////////////////////////////////////////////////
// конденсатоотводчик
class CondTapperElement : public Element
{
public:
    static ElementParams params;

public:
    CondTapperElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

////////////////////////////////////////////////////////
// конденсатоотводчик
class CancelerElement : public Element
{
public:
    static ElementParams params;

public:
    CancelerElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

////////////////////////////////////////////////////////
// ППК
class SafetyValveElement : public Element
{
public:
    static ElementParams params;

public:
    SafetyValveElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

////////////////////////////////////////////////////////
// Угловой клапан
class AngleValveElement : public Element
{
public:
    static ElementParams params;

public:
    AngleValveElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

////////////////////////////////////////////////////////
// аппарат
class VesselNossleElement : public Element
{
public:
    static ElementParams params;

public:
    VesselNossleElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

class VesselNossle2Element : public Element
{
public:
    static ElementParams params;

public:
    VesselNossle2Element();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

class VesselNossle3Element : public Element
{
public:
    static ElementParams params;

public:
    VesselNossle3Element();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

////////////////////////////////////////////////////////
// насос
class PumpElement : public Element
{
public:
    static ElementParams params;

public:
    PumpElement();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

class Pump2Element : public Element
{
public:
    static ElementParams params;

public:
    Pump2Element();
    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

// Дренаж
class DrainageElement : public Element
{
public:
    static ElementParams params;

public:
    DrainageElement();

    PElement     clone(Scene *, QList<NodeInfo>) const;
    virtual void draw(GraphicsScene *gscene, GItems &gitems, int level);
};

} // namespace geometry

#endif // ELEMENTTYPES_H
