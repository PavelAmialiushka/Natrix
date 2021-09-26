#include "manipulatorMatch.h"
#include "canvasRectangle.h"
#include "rectangleAdorner.h"
#include "sceneProcedures.h"

#include "manipulator.h"
#include "scenePropertyValue.h"

namespace geometry
{

MatchStyles::MatchStyles(Manipulator *m)
    : ManipulatorTool(m)
{
    supportModes_ = Clicking;
}

MatchStyles::MatchStyles(Manipulator *m, ScenePropertyList l)
    : ManipulatorTool(m)
{
    valueMap_     = propertyListToMap(l);
    supportModes_ = Clicking | Dragging;
}

bool MatchStyles::shouldTakeSample()
{
    return manipulator_->getMode(AltKey) || manipulator_->getMode(ControlKey) ||
           valueMap_.isEmpty();
}

PManipulatorTool MatchStyles::create(Manipulator *m)
{
    auto p = PManipulatorTool(new MatchStyles(m));
    return p;
}

QString MatchStyles::helperText() const
{
    return QString::fromUtf8("Чтобы <b>копировать свойства объектов</b> щелкните на объекте, "
                             "который необходимо скопировать,") +
           helperTab + QString::fromUtf8("а затем на элементе, которму нужно передать свойства") +
           helperTab +
           QString::fromUtf8("нажмите <b>Ctrl</b> или <b>Alt</b> чтобы снова выделить свойства") +
           helperTab +
           QString::fromUtf8("изменить свойства <b>CTRL+1..4</b> линий, <b>CTRL+5..7</b> отводов, "
                             "<b>CTRL+8..0</b> тройников");
}

void MatchStyles::makeSelection(PNeighbourhood nei)
{
    // добавляем все объекты к выделению
    foreach(PObject obj, nei->closestObjects)
    {
        selObjects_ << obj;
    }

    // добавляем метки к выделению
    foreach(PLabel lab, nei->closestLabels)
    {
        if(lab.dynamicCast<CanvasRectangle>())
            continue;
        selObjects_ << lab;
    }

    foreach(PObjectToSelect obj, selObjects_)
        nei->hoverState[obj] = Newby;
}

void MatchStyles::do_prepare()
{
    if(!shouldTakeSample())
        setCursor("match");
    else
        setCursor("match-empty");
    selObjects_.clear();
}

void MatchStyles::do_drag(point2d start, PNeighbourhood nei, bool started)
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

void MatchStyles::do_drop(point2d start, PNeighbourhood nei)
{
    rb_ = start;

    // получаем список объектов, подавших под выделение
    nei = scene_->getObjectsRectangle(lt_, rb_);

    // делаем выделение
    makeSelection(nei);
}

void MatchStyles::do_changeSize(int sizeFactor, bool absolute)
{
    if(absolute)
    {
        auto v = ScenePropertyValue::fromNumber(sizeFactor + 5);
        scene_->manipulator()->updatePropertiesDown(v);
    }
}

void MatchStyles::do_takeToolProperty(SceneProperties &props)
{
    foreach(auto it, valueMap_)
    {
        it.matchStyleMode = true;
        props.addProperty(it);
    }
}

// обновить манипулятор в соответствии с выбором
bool MatchStyles::do_updateToolProperties(ScenePropertyValue v)
{
    if(!valueMap_.contains(v.type))
    {
        valueMap_[v.type] = v;

        // теперь можно выделять
        supportModes_ = Clicking | Dragging;
        return true; // подхватили свойство
    }
    else
    {
        ScenePropertyValue &local = valueMap_[v.type];
        if(local.current == v.current)
            return false;

        // изменили свойство
        local.current = v.current;
        return true;
    }
}

void MatchStyles::do_click(point2d /*start*/, PNeighbourhood nei)
{
    if(shouldTakeSample())
    {
        PObjectToSelect s;
        if(nei->closestObjects.size())
            s = nei->closestObjects.first();
        if(nei->closestLabels.size())
            s = nei->closestLabels.first();
        if(s)
        {
            nextTool_.reset(new MatchStyles(manipulator_, s->getStyles()));
            nei->hoverState[s] = ToModify;
        }
    }
    else
    {
        makeSelection(nei);
    }
}

void MatchStyles::do_commit()
{
    if(selObjects_.size())
    {
        commandList_.addAndExecute(cmdMatchStyles(scene_, selObjects_, valueMap_.values()));
    }
}

} // namespace geometry
