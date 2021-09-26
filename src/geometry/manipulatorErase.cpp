#include "manipulatorErase.h"
#include "canvasRectangle.h"
#include "markLabels.h"
#include "marker.h"
#include "rectangleAdorner.h"
#include "sceneProcedures.h"

namespace geometry
{

EraseObjects::EraseObjects(Manipulator *m)
    : ManipulatorTool(m)
{
    supportModes_ = Clicking | Dragging;
}

PManipulatorTool EraseObjects::create(Manipulator *m)
{
    auto p = PManipulatorTool(new EraseObjects(m));
    return p;
}

QString EraseObjects::helperText() const
{
    return QString::fromUtf8(
        "<b>Удаление объектов:</b> щелкните на объекте, который необходимо удалить"
        " либо обведите группу элементов");
}

void EraseObjects::makeSelection(PNeighbourhood nei)
{
    // добавляем все объекты к выделению
    foreach(PObject obj, nei->closestObjects)
    {
        selObjects_ << obj;
    }

    bool markerLabelAndItsObject = false;

    // добавляем метки к выделению
    foreach(PLabel lab, nei->closestLabels)
    {
        if(lab.dynamicCast<CanvasRectangle>())
            continue;

        // проверяем
        if(auto markLabel = lab.dynamicCast<MarkLabel>())
        {
            auto list = scene_->markersOfFollower(markLabel);

            markerLabelAndItsObject = false;
            if(selObjects_.size() == 1 && list.size() == 1)
            {
                auto leader = list[0]->leader;
                if(selObjects_.find(leader) != selObjects_.end())
                {
                    markerLabelAndItsObject = true;
                }
            }
        }
        selObjects_ << lab;
    }

    if(markerLabelAndItsObject)
    {
        foreach(PObjectToSelect obj, selObjects_)
        {
            if(!obj.dynamicCast<MarkLabel>())
            {
                selObjects_.remove(obj);
                break;
            }
        }
    }

    foreach(PObjectToSelect obj, selObjects_)
        nei->hoverState[obj] = Newby;
}

void EraseObjects::do_prepare()
{
    setCursor("erase");
    selObjects_.clear();
}

void EraseObjects::do_drag(point2d start, PNeighbourhood nei, bool started)
{
    if(started)
        lt_ = start;
    else
        rb_ = start;

    auto adorner = new RectangleAdorner(lt_, rb_);
    scene_->pushAdorner(adorner);

    // получаем список объектов, подавших под выделение
    auto n = scene_->getObjectsRectangle(lt_, rb_);
    qSwap(*nei, *n);

    makeSelection(nei);
}

void EraseObjects::do_drop(point2d start, PNeighbourhood nei)
{
    rb_ = start;

    // получаем список объектов, подавших под выделение
    nei = scene_->getObjectsRectangle(lt_, rb_);

    // делаем выделение
    makeSelection(nei);
}

void EraseObjects::do_click(point2d start, PNeighbourhood nei)
{
    makeSelection(nei);
}

void EraseObjects::do_commit()
{
    if(selObjects_.size())
    {
        commandList_.addAndExecute(cmdEraseObjectsToSelect(scene_, selObjects_));
    }
}

} // namespace geometry
