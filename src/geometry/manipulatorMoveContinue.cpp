#include "manipulatorMoveContinue.h"
#include "manipulator.h"
#include "manipulatorMoveStart.h"
#include "sceneProcedures.h"

#include "moveAdorner.h"
#include "neighbourhood.h"
#include "nodePointAdorner.h"

#include "joiner.h"

#include <QSet>

namespace geometry
{

QString MoveContinue::helperText() const
{
    return QString::fromUtf8("<b>Перемещение:</b> указываем новое положение элемента") + helperTab +
           QString::fromUtf8("удерживайте <b>CTRL</b> чтобы присоединить перемещаемый элемент к "
                             "другому контуру") +
           helperTab +
           QString::fromUtf8("удерживайте <b>SHIFT</b> чтобы смещать часть контура вдоль осей") +
           helperTab + ManipulatorTool::helperText();
}

MoveContinue::MoveContinue(Manipulator *m, MoveDataInit data)
    : ManipulatorTool(m)
    , moveParams_(data)
{
    fastRedrawPossible_ = true;
    supportModes_       = Clicking | DragToClick;
}

PManipulatorTool MoveContinue::create(Manipulator *m, MoveDataInit data)
{
    return PManipulatorTool(new MoveContinue(m, data));
}

void MoveContinue::do_prepare()
{
    nextTool_ = PManipulatorTool(new MoveStart(manipulator_));
}

void MoveContinue::do_tearDown()
{
}

static QList<point3d> proposeMoveDirections(PObject object)
{
    QSet<point3d> result;
    foreach(PNode node, object->nodes())
    {
        result << node->direction();
        result << -node->direction();
    }

    return result.toList();
}

void MoveContinue::do_click(point2d clicked, PNeighbourhood nei)
{
    if(manipulator_->getMode(ControlKey))
    {
        return modifyMove(clicked, nei);
    }
    else
    {
        return simpleMove(clicked, nei);
    }
}

void MoveContinue::do_changeSize(int sizeFactor, bool absolute)
{
    if(absolute)
    {
        auto v = ScenePropertyValue::fromNumber(sizeFactor + 5);
        scene_->manipulator()->updatePropertiesDown(v);
    }
}

void MoveContinue::do_takeToolProperty(SceneProperties &props)
{
    props.addLineInfo(moveParams_.lineInfo);
}

bool MoveContinue::do_updateToolProperties(ScenePropertyValue v)
{
    if(!moveParams_.lineInfo.apply(v))
        return false;

    return true;
}

void MoveContinue::simpleMove(point2d clicked, PNeighbourhood nei)
{
    setCursor("stop");

    MoveData data = moveParams_;

    // устанаваливаем местку старта
    auto t = data.object.dynamicCast<Joiner>() ? AdornerMovingFromNode : AdornerMovingFromLine;
    scene_->pushAdorner(new NodePointAdorner(data.startPoint, t));

    // определяем конечную точку перемещения
    if(manipulator_->getMode(ShiftKey) == 0)
    {
        // свободное перемещение
        data.destination = clicked >> *scene_->worldSystem();
        scene_->pushAdorner(new MoveAdorner(data.startPoint, data.destination, false));
    }
    else
    {
        // перемещение по плоскостям
        QList<point3d> variants = generateDirections();
        variants << proposeMoveDirections(data.object);

        //        if (manipulator_->getMode(ShiftKey))
        //        {
        //            if (variants.contains( manipulator_->preferedDirection() ))
        //            {
        //                variants.clear();
        //                variants << manipulator_->preferedDirection();
        //            }
        //        }

        bool    success;
        point3d result_end;
        std::tie(result_end, success) = selectDirection(variants, data.startPoint, clicked, 2);

        // если направление не выбрано то отмена
        if(!success)
            return;

        Q_ASSERT(result_end != data.startPoint);
        nextTool_ = PManipulatorTool(new MoveStart(manipulator_));

        // определяем конец движения
        data.destination = result_end;
        scene_->pushAdorner(new MoveAdorner(data.startPoint, data.destination, true));
    }

    nextTool_ = PManipulatorTool(new MoveStart(manipulator_));
    commandList_.addAndExecute(manipulator_->getMode(ShiftKey) == 0 ? cmdShiftObject(scene_, data)
                                                                    : cmdMoveObject(scene_, data));

    // выводим подсказку
    setCursor("size");
    scene_->pushAdorner(new NodePointAdorner(data.destination, t));
}

void MoveContinue::modifyMove(point2d clicked, PNeighbourhood nei)
{
    MoveData data = moveParams_;

    setCursor("stop");
    auto t = data.object.dynamicCast<Joiner>() ? AdornerMovingFromNode : AdornerMovingFromLine;
    scene_->pushAdorner(new NodePointAdorner(data.startPoint, t));
    nextTool_ = PManipulatorTool(new MoveStart(manipulator_));

    // если есть куда прилепиться
    if(!nei->closestObjects.empty())
    {
        PObject landObject = nei->closestObjects.first();

        bool     canMove;
        MoveData localData    = data;
        localData.destination = nei->objectInfo[landObject].closestPoint;
        localData.fixedObjects << landObject;

        Command cmd = cmdMoveConnectObject(scene_, localData, landObject, &canMove);
        if(canMove)
        {
            commandList_.addAndExecute(cmd);
            setCursor("size");
            return;
        }
    }

    return simpleMove(clicked, nei);
}

void MoveContinue::do_commit()
{
}

} // namespace geometry
