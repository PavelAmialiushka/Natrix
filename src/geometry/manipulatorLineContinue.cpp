#include "manipulatorLineContinue.h"
#include "joiners.h"
#include "manipulator.h"
#include "manipulatorLineStart.h"
#include "manipulatorTools.h"

#include "sceneProcedures.h"

#include "bendJoiner.h"
#include "element.h"
#include "endCupJoiner.h"
#include "global.h"
#include "line.h"
#include "moveProcedures.h"
#include "nodePointAdorner.h"

#include <QDebug>
#include <qalgorithms.h>

namespace geometry
{
//////////////////////////////////////////////////////////////////////

static QList<point3d> proposeLineDirections(ConnectionData data)
{
    PObject object = data.object;

    QList<point3d> result;
    if(object.dynamicCast<Line>() || object.dynamicCast<WeldJoiner>())
    {
        return result;
    }
    else if(auto element = object.dynamicCast<Element>())
    {
        result << data.node()->direction();
        return result;
    }
    else if(auto end = object.dynamicCast<EndCupJoiner>())
    {
        result << end->nodeAt(0)->direction();
        return result;
    }
    else if(auto bend = object.dynamicCast<BendJoiner>())
    {
        // делаем тройник, можно двигаться в стороны противоположные
        // существующим
        result << -bend->nodeAt(0)->direction();
        result << -bend->nodeAt(1)->direction();
        result << bend->nodeAt(0)->direction();
        result << bend->nodeAt(1)->direction();
        return result;
    }
    else if(auto tee = object.dynamicCast<TeeJoiner>())
    {
        result << tee->nodeAt(2)->direction();
        result << -tee->nodeAt(2)->direction();
        return result;
    }

    Q_ASSERT(!"unexpected type");
    return result;
}

////////////////////////////////////////////////////////////////

LineContinue::LineContinue(Manipulator *m, LineInfo info)
    : ManipulatorTool(m)
    , lineInfo_(info)
{
    supportModes_ = Clicking | DragToClick;
}

QString LineContinue::helperText() const
{
    return QString::fromUtf8("<b>Pисуем линию:</b> щелкните мышкой на конечной точке линии") +
           helperTab +
           QString::fromUtf8(
               "удерживайте <b>CTRL</b> чтобы присоединить линию к существующему контуру") +
           helperTab +
           QString::fromUtf8("изменить свойства <b>CTRL+1..4</b> линий, <b>CTRL+5..7</b> отводов, "
                             "<b>CTRL+8..0</b> тройников") +
           helperTab + ManipulatorTool::helperText();
}

PManipulatorTool LineContinue::create_free(Manipulator *m, const point3d &start, LineInfo info)
{
    LineContinue *self = new LineContinue(m, info);
    self->start_       = start;

    return PManipulatorTool(self);
}

PManipulatorTool LineContinue::createStartingFromLine(Manipulator *  m,
                                                      ConnectionData data,
                                                      LineInfo       info)
{
    LineContinue *self = new LineContinue(m, info);
    self->start_       = data.point;
    self->data_        = data;

    return PManipulatorTool(self);
}

void LineContinue::do_click(point2d clicked, PNeighbourhood nei)
{
    // на выбор:
    // 1 направление по одной из 3х осей
    // 2 если нажат фиксаж, то фиксируется одна из трех осей
    // 3 если нажато отклоннение , то отклонение от одной из трех осей
    // 4 точка

    // свободные направления
    QList<point3d> variants = generateDirections();

    if(data_.object)
    {
        variants << proposeLineDirections(data_);
    }

    if(data_.object)
    {
        for(int index = 0; index < variants.size(); ++index)
        {
            point3d dir     = variants[index];
            bool    success = false;
            if(auto line = data_.object.dynamicCast<Line>())
            {
                point3d d = line->nodeAt(0)->direction();

                // тройник можно сделать если направление
                // не параллельно существующей линии
                success = d != dir && -d != dir;
            }
            else if(auto element = data_.object.dynamicCast<Element>())
            {
                auto node = data_.node();
                if(!node)
                    success = false;
                else
                    success = node->direction() == dir;
            }
            else if(auto cup = data_.object.dynamicCast<EndCupJoiner>())
            {
                // можно либо продолжить линию
                // либо создать отвод, нельзя только повернуть назад
                point3d d = cup->nodeAt(0)->direction();
                success   = d != dir                 // назад нельзя
                          && d.dot(dir) < POINT_PREC // нельзя под острым углом
                    ;
            }
            else if(auto weld = data_.object.dynamicCast<WeldJoiner>())
            {
                point3d d = weld->nodeAt(0)->direction();
                success   = d != dir && dir != -d;
            }
            else if(auto bend = data_.object.dynamicCast<BendJoiner>())
            {
                // отвод можно превратить в тройник
                // направление должно продолжать одну из линий
                point3d d1 = bend->nodeAt(0)->direction();
                point3d d2 = bend->nodeAt(1)->direction();
                success    = dir == -d1 || dir == -d2;
            }
            else if(auto tee = data_.object.dynamicCast<TeeJoiner>())
            {
                point3d d1 = tee->nodeAt(2)->direction();
                success    = dir == -d1;
            }

            if(!success)
            {
                variants.removeAt(index);
                --index;
            }
        }
    }

    // здесь причина нарушения регрессионных тестов
    if(manipulator_->getMode(ShiftKey, ControlKey))
    {
        if(variants.contains(manipulator_->preferedDirection()))
        {
            variants.clear();
            variants << manipulator_->preferedDirection();
        }
    }

    scene_->pushAdorner(new NodePointAdorner(start_, AdornerLineFromNode));

    bool    success;
    point3d result_end;
    std::tie(result_end, success) = selectDirection(variants, start_, clicked, lineMinimumSize);
    setCursor("stop");
    if(!success)
    {
        return;
    }

    Q_ASSERT(result_end != start_);

    /////////////////////////////////////

    Command cmd;
    // объекты для следующих этапов
    PObject line = Line::create(scene_, start_, result_end, Object::NoSample);
    line->applyStyle(lineInfo_);
    nei->hoverState[line] = HoverState::Newby;

    PObject newJoiner;
    cmd << cmdAttachObject(scene_, line);
    if(!data_.object)
    {
        auto cup1 = EndCupJoiner::create(
            scene_, line->nodeAt(0)->globalPoint(), -line->nodeAt(0)->direction());
        cup1->applyStyle(lineInfo_);
        cmd << cmdAttachObject(scene_, cup1);
    }
    else if(data_.object.dynamicCast<Line>())
    {
        PObject baseLine = data_.object;
        PNode   lineNode = line->nodeAtPoint(data_.point);
        Q_ASSERT(lineNode);

        point3d d0 = data_.info.closestPoint;

        PObject newbyTee = TeeJoiner::create(scene_,
                                             d0,
                                             baseLine->nodeAt(0)->direction(), // old line
                                             -lineNode->direction(),           // creating line
                                             Object::NoSample);
        newbyTee->applyStyle(lineInfo_);

        PObject line1 = Line::create(scene_, baseLine->nodeAt(0)->globalPoint(), d0, baseLine);
        PObject line2 = Line::create(scene_, d0, baseLine->nodeAt(1)->globalPoint(), baseLine);

        newbyTee->applyStyle(baseLine);

        cmd << cmdReplaceObject1to2(scene_, baseLine, line1, line2);
        cmd << cmdAttachObject(scene_, newbyTee, baseLine);
    }
    else if(data_.object.dynamicCast<EndCupJoiner>())
    {
        PNode lineNode = line->nodeAtPoint(data_.point);
        Q_ASSERT(lineNode);

        PObject cup = data_.object;

        point3d d1 = cup->nodeAt(0)->direction();
        point3d d2 = lineNode->direction();

        PObject newbyBend;
        if(d1 == d2) // продолжаем линию
        {
            PObject secondLine = neighbours(scene_, cup).first();

            // если тип линий совпадает
            if(canMergeLines(secondLine, line))
            {
                point3d a, b;
                mostDistantPoints(line, secondLine, a, b);

                // удаляем старое окончание
                cmd << cmdDetachObject(scene_, cup, true);

                // вставляем линию вместо двух
                PObject newbyLine = Line::create(scene_, a, b, line);
                cmd << cmdReplaceObject2to1(scene_, line, secondLine, newbyLine);

                nei->hoverState[newbyLine] = HoverState::Newby;
            }
            else
            {
                point3d a = cup->globalPoint(0);
                PObject newWeld =
                    WeldJoiner::create(scene_, a, cup->direction(0), Object::NoSample);
                newWeld->applyStyle(lineInfo_);
                cmd << cmdReplaceObject(scene_, cup, newWeld);
            }
        }
        else
        {
            newbyBend = BendJoiner::create(scene_,
                                           cup->nodeAt(0)->globalPoint(),
                                           cup->nodeAt(0)->direction(),
                                           -lineNode->direction(),
                                           /*sample*/ cup);
            newbyBend->applyStyle(lineInfo_);

            cmd << cmdReplaceObject(scene_, cup, newbyBend);
        }

        newJoiner = newbyBend;
    }
    else if(data_.object.dynamicCast<WeldJoiner>())
    {
        // шов можно превратить в тройник
        // направление НЕ ДОЛЖНО совпадать с направлением линии

        PObject weld = data_.object;
        // Q_ASSERT(data_.connecting_point == weld->nodeAt(0)->globalPoint());

        // куда направлена новая линия
        PNode   lineNode = line->nodeAtPoint(data_.point);
        point3d lineDir  = lineNode->direction();

        // ориентация шва
        point3d weldDir = weld->nodeAt(0)->direction();
        if(weldDir == -lineDir || weldDir == lineDir)
        {
            Q_ASSERT(!"incorrect logic");
            throw std::logic_error("incorrect tee-from-weld direction");
        }
        else
        {
            auto newbyTee = TeeJoiner::create(scene_, data_.point, weldDir, -lineDir, weld);

            cmd << cmdReplaceObject(scene_, weld, newbyTee);

            newJoiner = newbyTee;
        }
    }
    else if(data_.object.dynamicCast<BendJoiner>())
    {
        // отвод можно превратить в тройник
        // направление должно продолжать одну из линий

        PObject bend = data_.object;
        // Q_ASSERT(data_.connecting_point == bend->nodeAt(0)->globalPoint());

        // куда направлена новая линия
        PNode   lineNode = line->nodeAtPoint(data_.point);
        point3d lineDir  = lineNode->direction();

        // направления отвода
        point3d bendDir0 = bend->nodeAt(0)->direction();
        point3d bendDir1 = bend->nodeAt(1)->direction();
        int     nodeNo   = bendDir0 == lineDir ? 0 : bendDir1 == lineDir ? 1 : -1;
        if(nodeNo == -1)
        {
            Q_ASSERT(!"incorrect logic");
            throw std::logic_error("incorrect tee-from-bend direction");
        }
        else
        {
            PObject newbyTee;
            if(nodeNo == 0)
            {
                newbyTee = TeeJoiner::create(scene_,
                                             data_.point,
                                             bendDir0,
                                             bendDir1,
                                             bend
#ifdef _COPY_JOINER_WELD_POSITION
                                             ,
                                             bend->nodeAt(0)->weldPosition(),
                                             -1,
                                             bend->nodeAt(1)->weldPosition()
#endif
                );
            }
            else
            {
                newbyTee = TeeJoiner::create(scene_,
                                             data_.point,
                                             bendDir1,
                                             bendDir0,
                                             bend
#ifdef _COPY_JOINER_WELD_POSITION
                                             ,
                                             bend->nodeAt(1)->weldPosition(),
                                             -1,
                                             bend->nodeAt(0)->weldPosition()
#endif
                );
            }

            cmd << cmdReplaceObject(scene_, bend, newbyTee);

            newJoiner = newbyTee;
        }
    }
    else if(data_.object.dynamicCast<TeeJoiner>())
    {
        // тройник превращаем в четверник

        PObject tee      = data_.object;
        PNode   lineNode = line->nodeAtPoint(data_.point);
        point3d lineDir  = lineNode->direction();

        point3d teeDir0 = tee->nodeAt(0)->direction();
        point3d teeDir2 = tee->nodeAt(2)->direction();

        if(lineDir != teeDir2)
        {
            Q_ASSERT(!"incorrect logic");
            throw std::logic_error("incorrect tee-from-weld direction");
        }

        PObject newbyFour = FourJoiner::create(scene_, data_.point, teeDir0, teeDir2, tee);

        cmd << cmdReplaceObject(scene_, tee, newbyFour);
        newJoiner = newbyFour;
    }

    // точка, на которой заканчиваем линию
    PObject cup2 =
        EndCupJoiner::create(scene_, line->nodeAt(1)->globalPoint(), -line->nodeAt(1)->direction());
    cup2->applyStyle(lineInfo_);
    cmd << cmdAttachObject(scene_, cup2);

    // информацию для следующего движения
    // (начало следующей линии)
    ObjectInfo     info = {0.0, true, 0};
    ConnectionData data = {result_end, info, cup2};

    // setting next tool
    nextTool_ = LineContinue::createStartingFromLine(manipulator_, data, lineInfo_);
    commandList_.addAndExecute(cmd);

    // режим присоединения линий
    auto isModifyMode = manipulator_->getMode(ControlKey);
    if(isModifyMode && nei->closestObjects.size())
    {
        MoveData data;
        data.lineInfo   = lineInfo_;
        data.startPoint = result_end;

        PObject landObject = nei->closestObjects.first();
        data.destination   = nei->objectInfo[landObject].closestPoint;
        data.object        = cup2;

        // если "посчасливилось" ткнуть на объект, который
        // изменился в процессе рисования линии
        if(!scene_->contains(landObject))
            return;

        bool    ok;
        Command cmd = cmdMoveConnectObject(scene_, data, landObject, &ok);
        if(!ok)
            return;

        nextTool_ = PManipulatorTool(new LineStart(manipulator_));
        commandList_.addAndExecute(cmd);
    }

    setCursor("");
}

void LineContinue::do_prepare()
{
    nextTool_ = PManipulatorTool(new LineStart(manipulator_));
}

void LineContinue::do_changeSize(int sizeFactor, bool absolute)
{
    if(absolute)
    {
        auto v = ScenePropertyValue::fromNumber(sizeFactor + 5);
        scene_->manipulator()->updatePropertiesDown(v);
    }
}

void LineContinue::do_takeToolProperty(SceneProperties &props)
{
    props.addLineInfo(lineInfo_);
}

bool LineContinue::do_updateToolProperties(ScenePropertyValue v)
{
    if(!lineInfo_.apply(v))
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////////////
} // namespace geometry
