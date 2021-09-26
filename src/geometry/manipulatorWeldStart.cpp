#include "manipulatorWeldStart.h"
#include "manipulatorWeldMove.h"
#include "neighbourhood.h"

#include "sceneProcedures.h"
#include "weldProcedures.h"

#include "bendJoiner.h"
#include "global.h"
#include "line.h"
#include "nodePointAdorner.h"
#include "teeJoiner.h"
#include "weldJoiner.h"

namespace geometry
{

QString WeldStart::helperText() const
{
    return QString::fromUtf8("Укажите место для <b>вставки нового шва</b>") + helperTab +
           QString::fromUtf8(
               "<b>Перетяните</b> существующий шов, чтобы переместить или удалить его");
}

WeldStart::WeldStart(Manipulator *m)
    : ManipulatorTool(m)
{
}

PManipulatorTool WeldStart::create(Manipulator *m)
{
    return PManipulatorTool(new WeldStart(m));
}

void WeldStart::do_click(point2d point, PNeighbourhood nei)
{
    Command cmd;

    QMultiMap<PObject, WeldInfo> weldList = object2welds(scene_, nei->closestObjects);
    QList<WeldInfo>              infos    = weldList.values();

    // сортируем по расстоянию до курсора
    auto byDistanceToPoint = [&](WeldInfo const &lhs, WeldInfo const &rhs) {
        return lhs.localPoint.distance(point) < rhs.localPoint.distance(point);
    };
    std::sort(infos.begin(), infos.end(), byDistanceToPoint);

    // удаляем все, кроме ближайших, скопившихся в одной точке
    // плюс если это сварной шов, то удаляем всё, кроме него
    auto notInTheSamePoint = [&](WeldInfo const &info) {
        if(info.object != infos[0].object && infos[0].object.dynamicCast<WeldJoiner>())
            return true;
        if(info.globalPoint != infos[0].globalPoint)
            return true;
        return info.localPoint.distance(point) >= lineMinimumSize / 2;
    };
    if(infos.size() > 1)
    {
        infos.erase(std::remove_if(infos.begin(), infos.end(), notInTheSamePoint), infos.end());
    }

    if(infos.size())
    {
        WeldInfo info = infos.first();
        scene_->pushAdorner(new NodePointAdorner(info.globalPoint, AdornerMovingWeld));

        nextTool_ = WeldMove::create(manipulator_, infos);
        return;
    }

    while(!nei->closestObjects.empty())
    {
        auto neighbour = nei->closestObjects.takeFirst();
        if(auto line = neighbour.dynamicCast<Line>())
        {
            auto point = nei->objectInfo[neighbour].closestPoint;

            point3d p1 = line->globalPoint(0);
            point3d p2 = line->globalPoint(1);

            point3d el1, el2;
            bool    ok;
            std::tie(el1, el2) = breakLineBy(p1, p2, point, 0, &ok);
            double d           = qMin(el1.distance(p1), el2.distance(p2));
            if(ok && d >= lineMinimumSize - POINT_PREC)
            {
                auto direction = line->direction(1);

                auto line1 = Line::create(scene_, p1, el1, neighbour);
                auto line2 = Line::create(scene_, el2, p2, neighbour);

                cmd << cmdReplaceObject1to2(scene_, neighbour, line1, line2);

                if(scene_->isSelected(neighbour))
                {
                    QSet<PObjectToSelect> group;
                    group << line1 << line2;
                    cmd << bind(&Scene::addSelectedObjects, scene_, group);
                }

                auto weld = WeldJoiner::create(scene_, el1, direction, line1);
                cmd << cmdAttachObject(scene_, weld);
                nei->hoverState[weld] = HoverState::Newby;

                scene_->pushAdorner(new NodePointAdorner(el1, AdornerOverWeld));
            }
            break;
        }
    }

    nextTool_ = PManipulatorTool(new WeldStart(manipulator_));
    commandList_.addAndExecute(cmd);
}

} // namespace geometry
