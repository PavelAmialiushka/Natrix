#include "manipulatorLineShortener.h"

#include "global.h"
#include "line.h"
#include "manipulator.h"
#include "moveProcedures.h"
#include "neighbourhood.h"
#include "nodePointAdorner.h"

#include "joiner.h"
#include "sceneProcedures.h"

namespace geometry
{

QString LineShortener::helperText() const
{
    return QString::fromUtf8("<b>Укоротить линию:</b> укажите линию, которую нужно укоротить") +
           helperTab + QString::fromUtf8("<b>CTRL+click</b> чтобы удлинить линию");
}

LineShortener::LineShortener(Manipulator *m)
    : ManipulatorTool(m)
{
    supportModes_ = Clicking | Dragging;
    dragMode_     = false;
}

PManipulatorTool LineShortener::create(Manipulator *m)
{
    auto p = PManipulatorTool(new LineShortener(m));
    return p;
}

struct LineShortener::Parameters
{
    PObject        object;
    PNeighbourhood nei;
    point3d        start;
    point3d        end;
    double         size;
    bool           extendMode;

    // результат
    MoveData data;
    Command  cmd;
};

void LineShortener::makeLineShorter(Parameters &param, bool &ok)
{
    point3d p1 = param.object->globalPoint(0);
    point3d p2 = param.object->globalPoint(1);

    bool  points_ok = true;
    PNode n1        = connectedNode(scene_, param.object->nodes()[0]);
    auto  i1        = -param.object->getInteraction(n1->object());
    auto  pp1       = p1.polar_to(p2, i1, &points_ok);

    PNode n2  = connectedNode(scene_, param.object->nodes()[1]);
    auto  i2  = -param.object->getInteraction(n2->object());
    auto  pp2 = p2.polar_to(p1, i2, &points_ok);

    if(pp1 == pp2)
    {
        pp1 = p1;
        pp2 = p2;
    }

    double lenThird   = pp1.distance(pp2) / 3;
    bool   firstThird = pp1.distance(param.start) < lenThird;
    bool   lastThird  = pp2.distance(param.start) < lenThird;

    if(!param.end)
    {
        param.extendMode = (manipulator_->getMode(ControlKey)) != 0;
    }
    else
    {
        point3d n1       = param.start - (pp1 + pp2) / 2;
        point3d n2       = param.start - param.end;
        param.extendMode = n1.isCoaimed(n2);
    }

    if(!points_ok || (param.extendMode && p1.distance(p2) < lineMinimumSize * 2))
    {
        firstThird = lastThird = false;
    }

    point3d d1, d2;
    if(firstThird)
    {
        d2 = p2;
        d1 = p1.polar_to(p2, param.extendMode ? param.size : -param.size);
        if(d1.distance(d2) < lineMinimumSize)
        {
            d1 = p1;
            d2 = d1.polar_to(p2, lineMinimumSize);
        }
    }
    else if(lastThird)
    {
        d1 = p1;
        d2 = p2.polar_to(p1, param.extendMode ? param.size : -param.size);
        if(d1.distance(d2) < lineMinimumSize)
        {
            d2 = p2;
            d1 = p2.polar_to(p1, lineMinimumSize);
        }
    }
    else
    {
        d1 = p1.polar_to(p2, param.extendMode ? param.size / 2 : -param.size / 2);
        d2 = p2.polar_to(p1, param.extendMode ? param.size / 2 : -param.size / 2);
        if(d1.distance(d2) < lineMinimumSize)
        {
            d1 = ((p1 + p2) / 2).polar_to(p1, lineMinimumSize / 2);
            d2 = d1.polar_to(p2, lineMinimumSize);
        }
    }

    d1 -= p1;
    d2 -= p2;

    MoveParameters mprms{ObjectRestrictionType::Rigid};
    mprms.nodeMoves[param.object->nodeAt(0)] = NodeMovement{d1};
    mprms.nodeMoves[param.object->nodeAt(1)] = NodeMovement{d2};
    mprms.objectRules[param.object]          = ObjectRestriction{0, ObjectRestrictionType::Resize};

    param.cmd = cmdMoveNodes(scene_, param.data, mprms, &ok);
}

void LineShortener::do_move(point2d, PNeighbourhood nei)
{
    foreach(PObject object, nei->closestObjects)
    {
        ObjectInfo info  = nei->objectInfo[object];
        point3d    start = info.closestPoint;

        if(object.dynamicCast<Line>())
        {
            Parameters param;
            param.object = object;
            param.nei    = nei;
            param.start  = start;
            param.size   = lineMinimumSize;

            bool ok = true;
            makeLineShorter(param, ok);

            if(!ok)
            {
                // сигналим о невозможности укорачивания
                scene_->pushAdorner(new NodePointAdorner(start, AdornerLineShortenerInactive));
            }
            else
            {
                nei->hoverState[object] = HoverState::ToModify;
                scene_->pushAdorner(new NodePointAdorner(start, AdornerLineShortener));
            }

            return;
        }
    }
}

void LineShortener::do_click(point2d, PNeighbourhood nei)
{
    foreach(PObject object, nei->closestObjects)
    {
        ObjectInfo info  = nei->objectInfo[object];
        point3d    start = info.closestPoint;

        if(object.dynamicCast<Line>())
        {
            Parameters param;
            param.object = object;
            param.nei    = nei;
            param.start  = start;
            param.size   = lineMinimumSize;

            bool ok = true;
            makeLineShorter(param, ok);

            if(!ok)
                return;
            commandList_.addAndExecute(param.cmd);

            return;
        }
    }
}

void LineShortener::do_drag(point2d pt, PNeighbourhood nei, bool started)
{
    if(started)
    {
        dragMode_ = false;
        foreach(PObject object, nei->closestObjects)
        {
            if(object.dynamicCast<Line>())
            {
                ObjectInfo info  = nei->objectInfo[object];
                point3d    start = info.closestPoint;

                dragMode_       = true;
                dragStartPoint_ = start;
                dragObject_     = object;
            }
        }
    }
    if(!dragMode_)
        return;

    dragFinish(pt, nei, false);
}

void LineShortener::do_drop(point2d pt, PNeighbourhood nei)
{
    if(!dragMode_)
        return;

    dragFinish(pt, nei, true);
}

void LineShortener::dragFinish(point2d pt, PNeighbourhood nei, bool /*isdrop*/)
{
    point3d endPoint = scene_->worldSystem()->toGlobal_atline(
        pt, dragObject_->globalPoint(0), dragObject_->globalPoint(1));
    double size = endPoint.distance(dragStartPoint_);

    Parameters param;
    param.object = dragObject_;
    param.nei    = nei;
    param.start  = dragStartPoint_;
    param.end    = endPoint;
    param.size   = size;

    bool ok = true;
    makeLineShorter(param, ok);

    if(!ok)
    {
        // сигналим о невозможности укорачивания
        scene_->pushAdorner(new NodePointAdorner(dragStartPoint_, AdornerLineShortenerInactive));
    }
    else
    {
        nei->hoverState[dragObject_]                      = HoverState::ToModify;
        nei->hoverState[param.data.replaced(dragObject_)] = HoverState::ToModify;
        scene_->pushAdorner(new NodePointAdorner(dragStartPoint_, AdornerLineShortener));
        scene_->pushAdorner(new NodePointAdorner(endPoint, AdornerLineShortener));

        commandList_.addAndExecute(param.cmd);
        insertAdorners(scene_, param.data);
    }
}

} // namespace geometry
