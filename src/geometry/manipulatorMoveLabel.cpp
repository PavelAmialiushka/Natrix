#include "manipulatorMoveLabel.h"

#include "manipulatorLabelRotate.h"
#include "manipulatorMoveStart.h"
#include "sceneProcedures.h"

#include "element.h"
#include "label.h"
#include "markLabels.h"
#include "textLabel.h"

#include "moveAdorner.h"
#include "neighbourhood.h"
#include "nodePointAdorner.h"

#include "global.h"
#include "manipulator.h"
#include "utilites.h"

namespace geometry
{

QString MoveLabel::helperText() const
{
    return QString::fromUtf8("<b>Укажите новое местоположение элемента</b>") + helperTab +
           QString::fromUtf8("<b>CTRL</b> чтобы копировать элемент");
}

PManipulatorTool MoveLabel::create(Manipulator *m, point2d pt, PLabel label)
{
    return PManipulatorTool(new MoveLabel(m, pt, label));
}

MoveLabel::MoveLabel(Manipulator *m, point2d point, PLabel label)
    : ManipulatorTool(m)
    , start_point_(point)
    , label_(label)
{
    supportModes_ = DragToClick | Clicking;
}

void MoveLabel::do_prepare()
{
    nextTool_ = manipulator_->prevTool();
}

void MoveLabel::do_click(point2d pt, PNeighbourhood n)
{
    do_drag(pt, n);
}

static point2d pointOf(PLabel label)
{
    if(auto text = label.dynamicCast<TextLabel>())
    {
        return text->info().markerPoint();
    }
    else if(auto mark = label.dynamicCast<MarkLabel>())
    {
        return mark->basePoint();
    }

    return 0;
}

static bool canAcceptText(PObject object)
{
    if(auto el = object.dynamicCast<Element>())
        return el->canBreakLine(0);

    return 1;
}

Command MoveLabel::cmdMoveLabel(Scene *        scene,
                                PNeighbourhood nei,
                                PLabel         label,
                                point2d        pt,
                                point2d        delta,
                                bool           copyLabel,
                                bool           dropMarkers,
                                bool           dontStick)
{
    Command cmd;
    PLabel  newby;
    bool    checkMarkers = true;
    point2d start        = pointOf(label);
    point2d land         = start + delta;
    auto    ws           = scene->worldSystem();

    // попала ли точка на объект
    PNeighbourhood neiTarget = scene->getClosestObjects(pt);
    QList<PObject> objects   = utils::filtered(neiTarget->closestObjects, canAcceptText);

    if(objects.isEmpty())
    {
        // попытка вторая по методу
        neiTarget = scene->getClosestObjects(land);
        objects   = utils::filtered(neiTarget->closestObjects, canAcceptText);
    }

    // новый маркер, если решим его перемещать
    PMarker newbyMarker;

    bool tearedOffFromMarker = false;
    if(objects.size() && !dontStick)
    {
        PObject object = objects.first();
        point3d point  = neiTarget->objectInfo[object].closestPoint;

        if(auto markLabel = label.dynamicCast<MarkLabel>())
        {
            PObject object = neiTarget->closestObjects[0];

            // получается, всегда крутить лучше
            bool sameObject = false;
            auto markers    = scene->markersOfFollower(label);
            foreach(PMarker marker, markers)
            {
                if(marker->leader == object)
                    sameObject = true;
            }

            int rtype = markLabel->rotationType();
            if(!rtype)
            {
                point3d delta = point - markLabel->objectPoint();
                newby         = label->clone(scene, delta);
                newbyLabel_   = newby;
                newby->setLabelActive();
            }
            else
            {
                point3d dir     = getMarkLandingDirection(object);
                point3d lineDir = object->direction(0);
                point3d delta   = point - markLabel->objectPoint();

                PMarkLabel newbyMarkMarker;
                if(sameObject)
                {
                    dir             = markLabel->direction();
                    newbyMarkMarker = markLabel->clone(scene, delta).dynamicCast<MarkLabel>();
                }
                else if(rtype == InLineRotation)
                    newbyMarkMarker = markLabel->clone(scene, delta, dir);
                else
                {
                    newbyMarkMarker = markLabel->clone(scene, delta, bestNormalDirection(dir));
                    newbyMarkMarker->setLineDirection(lineDir);
                }
                newby = newbyMarkMarker;

                // небольшое перемещение, возможно требуется вращение
                if(rtype == InLineRotation)
                {
                    auto *tool =
                        RotateLabel::flipAlongDirection(manipulator_, newbyMarkMarker, dir);
                    if(sameObject)
                        tool->setMarkSpecialDirection();
                    nextTool_.reset(tool);
                }
                else
                    nextTool_.reset(
                        RotateLabel::rotateOverObject(manipulator_, newbyMarkMarker, object));
            }
        }
        else
        {
            // текстовый маркер приземляется
            land        = point >> *ws;
            auto delta  = land - start;
            newby       = label->clone(scene, delta);
            newbyLabel_ = newby;
            newby->setLabelActive();
        }

        // приклеиваем маркер к объекту
        if(copyLabel)
            cmd << cmdAttachLabelToObject(scene, PLabel(), newby, object, point);
        else
            cmd << cmdAttachLabelToObject(scene, label, newby, object, point);
        checkMarkers = false;
    }
    else // нет нового объекта, к которому нужно прилипать
    {
        // если оторвали от объекта, то возможно надо убрать маркер
        if(!copyLabel && label.dynamicCast<MarkLabel>())
        {
            foreach(PMarker marker, scene->markersOfFollower(label))
            {
                cmd << cmdDetachMarker(scene, marker);
            }
            checkMarkers = false;
        }

        if(PTextLabel textLabel = label.dynamicCast<TextLabel>())
        {
            PMarker m =
                takeStickyMarker(scene_, textLabel->info(), scene->markersOfFollower(label));

            if(m)
            { // липкий маркер найден, отрываем
                tearedOffFromMarker = true;

                // пробуем переместить точку маркераs
                PElement elem = m->leader.dynamicCast<Element>();
                if(elem && elem->nodeCount() == 2)
                {
                    point3d p;
                    if(elem->localPoint(0).distance(land) < elem->localPoint(1).distance(land))
                        p = elem->globalPoint(0) * 0.67 + elem->globalPoint(1) * 0.33;
                    else
                        p = elem->globalPoint(0) * 0.33 + elem->globalPoint(1) * 0.67;

                    newbyMarker = Marker::create(p, m->leader, m->follower);
                }
            }
        }
    }

    if(!newby)
        newby = label->clone(scene, delta);
    newbyLabel_ = newby;
    newby->setLabelActive();
    nei->hoverState[newby] = HoverState::Newby;

    if(PTextLabel textLabel = newby.dynamicCast<TextLabel>())
        if(tearedOffFromMarker)
        {
            TextInfo newInfo = textLabel->info();
            newInfo.setAngle(scene_, 0);
            textLabel->setInfo(newInfo);

            if(newbyMarker)
            { // есть альтернативна маркеру, заменяем

                newbyMarker->follower = textLabel;
                cmd << cmdAttachMarker(scene, newbyMarker);

                // это, конечно, хак, но вроде бы так будет работать
                // удаляем все маркеры
                dropMarkers = true;
            }
        }

    if(dropMarkers)
        checkMarkers = false;

    if(copyLabel)
        cmd << cmdAttachLabel(scene, newby);
    else
        cmd << cmdReplaceLabel(scene, label, newby, checkMarkers, dropMarkers);

    return cmd;
}

void MoveLabel::do_drag(point2d pt, PNeighbourhood nei, bool started)
{
    bool copyLabel   = false;
    bool dropMarkers = false;
    bool dontStick   = false;

    if(manipulator_->getMode(ControlKey))
        copyLabel = true;
    else if(manipulator_->getMode(ShiftKey))
        dropMarkers = true;
    if(manipulator_->getMode(AltKey))
        dontStick = true;

    Command cmd =
        cmdMoveLabel(scene_, nei, label_, pt, pt - start_point_, copyLabel, dropMarkers, dontStick);
    commandList_.addAndExecute(cmd);

    setCursor("size");
    scene_->pushAdorner(new MoveAdorner(start_point_, pt));
}

void MoveLabel::do_drop(point2d pt, PNeighbourhood n)
{
    do_drag(pt, n);
}

void MoveLabel::do_tearDown()
{
    if(newbyLabel_)
        newbyLabel_->setLabelActive(false);
}

} // namespace geometry
