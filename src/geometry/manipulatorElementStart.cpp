#include "manipulatorElementStart.h"
#include "element.h"
#include "global.h"
#include "glueObject.h"
#include "manipulator.h"
#include "manipulatorElementRotate.h"
#include "manipulatorTools.h"
#include "nodePointAdorner.h"

#include "bendJoiner.h"
#include "elementFactory.h"
#include "elementTypes.h"
#include "endCupJoiner.h"
#include "line.h"
#include "sceneProcedures.h"

#include <QDebug>
#include <qalgorithms.h>

namespace geometry
{

StartElement::StartElement(Manipulator *m, ElementInfo info)
    : ManipulatorTool(m)
    , sample_(info)
{
}

StartElement::StartElement(Manipulator *m, QString elementName)
    : ManipulatorTool(m)
{
    ElementInfo info = scene_->defaultElementInfo();
    info.elementName = elementName;

    sample_ = info;
}

QString StartElement::helperText() const
{
    return QString::fromUtf8("<b>Вставляем элемент:</b> укажите место вставки") + helperTab +
           QString::fromUtf8("<b>Ctrl + или -</b> изменить размер, ") + helperTab +
           QString::fromUtf8("<b>Ctrl-цифра</b> установить размер, "
                             "<b>Ctrl-*</b>,<b>Ctrl-5</b> вернуть начальный размер");
}

void StartElement::do_changeRotation(int d)
{
    sample_.setFlanges(d);
    scene_->updatePropertiesAfterSceneChange();
}

void StartElement::do_changeSize(int sizeF, bool absolute)
{
    int factor = qMin(4, qMax(-4, absolute ? sizeF : sample_.scaleFactor + 2 * sizeF));
    sample_.setScaleFactor(factor - factor % 2);

    scene_->updatePropertiesAfterSceneChange();
}

void StartElement::do_toggleMode()
{
    sample_ = sample_.nextSiblingOptions();
}

static bool shouldDeleteTargetElement(PElement target, PElement elem)
{
    if(elem->info().elementJointType == ElementJoint::Flanged &&
       target.dynamicCast<FlangePairElement>())
    {
        return true;
    }

    return false;
}

static bool canConnectElements(PElement target, PElement elem)
{
    if(elem.dynamicCast<FlangePairElement>() &&
       target->info().elementJointType == ElementJoint::Flanged)
        return false;

    return true;
}

void StartElement::do_click(point2d pt, PNeighbourhood n)
{
    // если кликнули
    // конец линии, отвод или продолждение линии
    // свободный нод, прикрепляемся к ноду
    // середина линии - тройник продолжаем
    // отвод - тройник из отвода

    point3d elementStart = scene_->worldSystem()->toGlobal(pt);

    nextTool_ = PManipulatorTool(new StartElement(manipulator_, sample_));

    Command cmd;

    PElement proto = ElementFactory::createElement(scene_, sample_);
    proto          = proto->cloneResize(scene_, proto->globalPoint(0), sample_.scaleFactor);

    point3d direction, direction2 = 0;
    bool    canBeFreeRotated    = false;
    bool    canBreak            = proto->canBreakLine(sample_.startNode);
    bool    hasDirection        = !proto->params().canNonBeFlipped;
    bool    canBeRotatedOverAxe = proto->canBeRotated(0);

    bool bendLike =
        proto.dynamicCast<SafetyValveElement>() || proto.dynamicCast<AngleValveElement>();
    bool    bendLikeSuccess = false;
    PObject replaceObject;

    double  size = proto->breakingSize(0);
    PObject neighbour;

    if(n->closestObjects.empty())
    { // под мышкой ничего нет

        // определяем центр элемента
        point3d center       = proto->center();
        direction            = point3d::nx;
        point3d adornerStart = elementStart;

        // положение элемента с учетом смещения от центра
        if(!canBeRotatedOverAxe)
            elementStart -= center;

        // адорнер будет располагаться в видимом центре
        auto adorner = new NodePointAdorner(adornerStart, AdornerLineFromFree);
        scene_->pushAdorner(adorner);

        canBeFreeRotated = true;
    }
    else
    {
        // элемент будет несвободным
        neighbour       = n->closestObjects.first();
        ObjectInfo info = n->objectInfo[neighbour];
        elementStart    = info.closestPoint;
        bool canStart   = false;

        if(neighbour.dynamicCast<EndCupJoiner>())
        { // прикрепляется к свободному концу линии
            scene_->pushAdorner(new NodePointAdorner(elementStart, AdornerLineFromNode));
            direction = -info.node(neighbour)->direction();

            cmd << cmdDetachObject(scene_, neighbour);
            canStart = true;
        }
        else if(auto element = neighbour.dynamicCast<Element>())
        { // вставляем рядом с существующей арматурой
            PNode neighbourNode = info.node(neighbour);
            if(neighbourNode && neighbourNode->jointType() == 0)
            {
                // указывает на часть арматуры
                direction = neighbourNode->direction();
                canStart  = true;

                // занят ли этот нод арматуры
                ConnectionData data   = scene_->findConnectedNode(neighbourNode);
                PObject        object = data.object;

                double sizeDefect = getSizeDefect(proto, neighbour);
                size -= sizeDefect;

                if(!canConnectElements(element, proto))
                {
                    canStart = false;
                }
                else if(object && !canBreak)
                { // нод занят, а элемент не позволяет раздвижение
                    canStart = false;
                }
                else if(!object && info.isNodeFound)
                {
                    // вставляем элемент
                    // со свободной стороны арматуры

                    auto p0 = info.node(neighbour)->globalPoint();
                    auto d0 = info.node(neighbour)->direction();

                    if(shouldDeleteTargetElement(element, proto))
                    {
                        // вставляем _вместо_ фланца
                        size += sizeDefect;
                        cmd << cmdDetachObject(scene_, element);

                        // ищем первоначальную точку присоединения фланца
                        point3d p3 = secondNode(element->nodeAtPoint(elementStart))->globalPoint();
                        elementStart = p3;
                    }
                    else if(sizeDefect)
                    {
                        auto p3   = p0.polar_to(p0 - d0, sizeDefect);
                        auto glue = GlueObject::create(scene_, p0, p3);
                        cmd << cmdAttachObject(scene_, glue);

                        elementStart = p3;
                    }
                }
                else if(auto line = object.dynamicCast<Line>())
                {
                    // к существующему элементу присоединяется линия
                    // попытаемся раздвинуть линию
                    point3d p2 = data.point;
                    point3d p1 = line->globalPoint(0);
                    if(p2 == p1)
                        p1 = line->globalPoint(1);

                    if(p1.distance(p2) - size < lineMinimumSize)
                    {
                        // места недостаточно
                        canStart = false;
                    }
                    else if(shouldDeleteTargetElement(element, proto))
                    {
                        // нельзя поглотить фланцец, раздвигая линию
                        canStart = false;
                    }
                    else
                    {
                        // места достаточно, сдвигаем линию
                        point3d el2   = p2.polar_to(p1, size);
                        PObject line1 = Line::create(scene_, p1, el2, line);

                        cmd << cmdReplaceObject(scene_, line, line1);
                        cmd << cmdTouchObject(scene_, element);
                    }

                    if(sizeDefect)
                    {
                        auto p3   = p2.polar_to(p1, -sizeDefect);
                        auto glue = GlueObject::create(scene_, p2, p3);
                        cmd << cmdAttachObject(scene_, glue);

                        elementStart = p3;
                    }
                }
                else if(object.dynamicCast<Element>() || object.dynamicCast<GlueObject>())
                {
                    // TODO вставка между двумя элементами пока не возможна
                    canStart = false;
                }

                if(canStart)
                    scene_->pushAdorner(new NodePointAdorner(elementStart, AdornerLineFromNode));
            }
            else if(!neighbourNode && !element->canBeRotated(0))
            {
                // указывает на саму арматуру
                auto tool = RotateElement::flipAlongNode(manipulator_, element, 0);
                tool->setPrevTool(manipulator_->tool());
                tool->setJoinToPrevious(false);
                nextTool_ = PManipulatorTool(tool);

                auto adorner = new NodePointAdorner(element->center(), AdornerLineFromNone);
                scene_->pushAdorner(adorner);
                return;
            }
        }
        else if(canBreak && neighbour.dynamicCast<Line>())
        { // вставляем в середину линии, для этого линию нужно разорвать
            auto    line = neighbour.dynamicCast<Line>();
            point3d p1   = line->nodeAt(0)->globalPoint();
            point3d p2   = line->nodeAt(1)->globalPoint();

            point3d el1, el2;
            std::tie(el1, el2) = breakLineBy(p1, p2, elementStart, size);

            double d = qMin(el1.distance(p1), el2.distance(p2));
            if(el1 != el2 && d >= lineMinimumSize - POINT_PREC)
            {
                elementStart = el1;
                direction    = line->nodeAt(1)->direction();

                auto line1 = Line::create(scene_, p1, el1, line);
                auto line2 = Line::create(scene_, el2, p2, line);

                cmd << cmdReplaceObject1to2(scene_, neighbour, line1, line2);

                // TODO нужно ли сохранять выделение, если разорванная линия была выделена
                if(scene_->isSelected(neighbour))
                {
                    QSet<PObjectToSelect> group;
                    group << line1 << line2;
                    cmd << bind(&Scene::addSelectedObjects, scene_, group);
                }

                scene_->pushAdorner(new NodePointAdorner((el1 + el2) / 2, AdornerLineFromLine));

                canStart = true;
            }
        }
        else if(bendLike && neighbour.dynamicCast<BendJoiner>())
        {
            auto line1 = neighbour->neighbour(0);
            auto line2 = neighbour->neighbour(1);

            Q_ASSERT(line1.dynamicCast<Line>());
            Q_ASSERT(line2.dynamicCast<Line>());

            bool    ok1, ok2;
            point3d start = neighbour->globalPoint(0);

            double  size1 = proto->center().distance(proto->globalPoint(0));
            PNode   n1    = commonNode(line1, neighbour);
            point3d e11, p11 = n1->globalPoint();
            point3d e12, p12 = secondNode(n1)->globalPoint();

            std::tie(e11, e12) = breakOrShortLineBy(p11, p12, start, size1, true, &ok1);

            double  size2 = proto->center().distance(proto->globalPoint(1));
            PNode   n2    = commonNode(line2, neighbour);
            point3d e21, p21 = n2->globalPoint();
            point3d e22, p22 = secondNode(n2)->globalPoint();

            std::tie(e21, e22) = breakOrShortLineBy(p21, p22, start, size2, true, &ok2);

            if(ok1 && ok2)
            {
                auto line1x = Line::create(scene_, e12, p12, line1);
                cmd << cmdReplaceObject(scene_, line1, line1x);

                auto line2x = Line::create(scene_, e22, p22, line2);
                cmd << cmdReplaceObject(scene_, line2, line2x);

                elementStart = e12;

                canStart        = true;
                bendLikeSuccess = true;
                replaceObject   = neighbour;

                direction  = n1->direction();
                direction2 = -n2->direction();

                // TODO нужно ли сохранять выделение, если разорванная линия была выделена
                if(scene_->isSelected(line1))
                {
                    QSet<PObjectToSelect> group;
                    group << line1x;
                    cmd << bind(&Scene::addSelectedObjects, scene_, group);
                }

                if(scene_->isSelected(line2))
                {
                    QSet<PObjectToSelect> group;
                    group << line2x;
                    cmd << bind(&Scene::addSelectedObjects, scene_, group);
                }
            }
        }

        if(!canStart)
        {
            scene_->pushAdorner(new NodePointAdorner(elementStart, AdornerLineFromNone));
            return;
        }
    }

    // вставляем в центр
    PElement newbyElement =
        proto->cloneMoveRotate(scene_, sample_.startNode, elementStart, direction, direction2);

    // выделяем элемент
    n->hoverState[newbyElement] = HoverState::Newby;

    if(canBeRotatedOverAxe)
    { // несимметричен относительно ноды 0 (насос, ППК)

        // в любом случае нужно вращать
        if(bendLikeSuccess)
        {
            if(newbyElement->canBeFlipped(0))
            {
                auto tool =
                    RotateElement::flipAlongNode(manipulator_, newbyElement, sample_.startNode);
                tool->setPrevTool(manipulator_->tool());
                nextTool_ = PManipulatorTool(tool);
            }
            // ничего больше не надо
        }
        else if(canBeFreeRotated)
        { // свободное положение
            auto tool =
                RotateElement::overNodeTwoStages(manipulator_, newbyElement, sample_.startNode);
            tool->setPrevTool(manipulator_->tool());
            nextTool_ = PManipulatorTool(tool);
        }
        else if(canBreak && hasDirection) // эксцентричный переход
        {
            auto tool = RotateElement::flipAlongNodeTwoStages(
                manipulator_, newbyElement, sample_.startNode);
            tool->setPrevTool(manipulator_->tool());
            nextTool_ = PManipulatorTool(tool);
        }
        else
        { // вставка к соседу
            auto tool =
                RotateElement::rotateOverNodeAxis(manipulator_, newbyElement, sample_.startNode);
            tool->setPrevTool(manipulator_->tool());
            nextTool_ = PManipulatorTool(tool);
        }
    }
    else
    {
        // симметричен относительно ноды 0 (задвижка, клапан)
        if(canBeFreeRotated)
        {
            //  свободное положение
            auto tool = RotateElement::inAnyDirection(manipulator_, newbyElement);
            tool->setPrevTool(manipulator_->tool());
            nextTool_ = PManipulatorTool(tool);
        }
        else if(newbyElement->canBeFlipped(0))
        {
            // вставка к соседу
            auto tool = RotateElement::flipAlongNode(manipulator_, newbyElement, 0);
            tool->setPrevTool(manipulator_->tool());
            nextTool_ = PManipulatorTool(tool);
        }
    }

    if(replaceObject)
        cmd << cmdReplaceObject(scene_, replaceObject, newbyElement, 0, true);
    else
        cmd << cmdAttachObject(scene_, newbyElement);
    commandList_.addAndExecute(cmd);
}

void StartElement::do_takeToolProperty(SceneProperties &props)
{
    props.addElement(sample_);
}

bool StartElement::do_updateToolProperties(ScenePropertyValue v)
{
    if(!sample_.applyStyle(v))
        return false;

    return true;
}

} // namespace geometry
