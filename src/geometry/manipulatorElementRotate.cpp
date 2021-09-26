#include "manipulatorElementRotate.h"
#include "element.h"
#include "manipulator.h"
#include "manipulatorElementStart.h"
#include "manipulatorTools.h"
#include "moveProcedures.h"
#include "nodePointAdorner.h"

#include "global.h"

#include "moveAdorner.h"
#include "sceneProcedures.h"

#include "scales.h"
#include <algorithm>

namespace geometry
{

RotateElement::RotateElement(Manipulator *m, PElement element)
    : ManipulatorTool(m)
    , element_(element)
    , scaleFactor_(element->info().scaleFactor)
    , activeNodeNo_(0)
    , rotateAfterFlip_(0)
    , rotateOverCenter_(0)
{
    joinToPrevious_ = true;
}

RotateElement *RotateElement::inAnyDirection(Manipulator *m, PElement e)
{
    auto tool   = new RotateElement(m, e);
    tool->mode_ = Mode::OverCenterFree;

    tool->inAnyDirection_ = true;
    return tool;
}

RotateElement *RotateElement::flipAlongNode(Manipulator *m, PElement e, int node)
{
    auto tool   = new RotateElement(m, e);
    tool->mode_ = Mode::Flip;

    tool->activeNodeNo_ = node;

    return tool;
}

RotateElement *RotateElement::flipAlongNodeTwoStages(Manipulator *m, PElement e, int node)
{
    auto tool              = new RotateElement(m, e);
    tool->mode_            = Mode::Flip;
    tool->activeNodeNo_    = 0;
    tool->rotateAfterFlip_ = true;

    return tool;
}

RotateElement *RotateElement::rotateOverNodeAxis(Manipulator *m, PElement e, int node)
{
    auto tool   = new RotateElement(m, e);
    tool->mode_ = Mode::OverNodeAxis;

    tool->activeNodeNo_ = node;
    return tool;
}

RotateElement *RotateElement::overNodeTwoStages(Manipulator *m, PElement e, int node)
{
    auto tool   = new RotateElement(m, e);
    tool->mode_ = Mode::OverNodeFree;

    tool->inAnyDirection_ = true;
    tool->activeNodeNo_   = node;
    return tool;
}

void RotateElement::setPrevTool(PManipulatorTool t)
{
    prevTool_ = t;
}

QString RotateElement::helperText() const
{
    return QString::fromUtf8("<b>Выберите направление</b>, куда будет направлен элемент") +
           helperTab + ManipulatorTool::helperText();
}

void RotateElement::do_prepare()
{
    QString type = element_->elementName();
    nextTool_    = prevTool_ ? prevTool_ : PManipulatorTool(new StartElement(manipulator_, type));
}

void RotateElement::do_click(point2d clicked, PNeighbourhood n)
{
    //////////////////////////////////////
    // соседи и масштабирование

    // анализируем соседей
    auto n1 = element_->neighbour(0);
    auto n2 = element_->neighbour(1);

    // если присутствуют оба соседа, то не изменяем масштаб
    bool acceptScaling = !n1 || !n2;
    if(!acceptScaling)
    {
        scaleFactor_ = element_->info().scaleFactor;
    }

    int    factorDelta = scaleFactor_ - element_->info().scaleFactor;
    double scale       = toScale(factorDelta);

    // создаем прототип
    auto proto = factorDelta ? element_->cloneResize(scene_, 0, scaleFactor_) : element_;

    //////////////////////////////////////
    // центры и направления

    // точка, вокруг которой будем вращать
    point3d rotateCenter = 0;
    // точка, относитеьно которой будем изменять масштаб
    point3d scaleCenter = 0;
    switch(mode_)
    {
    case Mode::Flip:
        if(acceptScaling && (n1 || n2))
        {
            activeNodeNo_ = n1 ? 0 : 1;
            scaleCenter   = element_->globalPoint(activeNodeNo_);
            rotateCenter  = element_->center().scale(scaleCenter, scale);
            break;
        }
        // no break
        [[fallthrough]];
    case Mode::OverCenterFree:
        scaleCenter = rotateCenter = element_->center();
        break;
    case Mode::OverNodeAxis:
        rotateCenter = scaleCenter = element_->center();
        break;
    case Mode::OverNodeFree:
        // для двухмерных элементов (ППК, насос)
        rotateCenter = scaleCenter = element_->globalPoint(activeNodeNo_);
        break;
    }

    // выбираем все свободные направления
    QList<point3d> variants = generateDirections();
    if(mode_ == Mode::Flip)
    {
        variants.clear();
        variants << -element_->direction(activeNodeNo_);
        variants << -element_->direction(activeNodeNo_ == 0 ? 1 : 0);
    }

    // ключевые точки вращаемого элемента
    point3d d0 = element_->direction(activeNodeNo_);

    // отбрасываем запрещенные
    if(mode_ == Mode::OverNodeAxis)
    { // вращение должно сохранить "прямой угол" элемента
        auto nonorto = [&](point3d const &dir) { return !dir.isNormal(d0); };
        variants.erase(std::remove_if(variants.begin(), variants.end(), nonorto), variants.end());
    }

    // выбор направления
    bool    success;
    point3d result_end;
    double  size                  = proto->breakingSize(0);
    std::tie(result_end, success) = selectDirection(variants, rotateCenter, clicked, size);
    if(!success)
        return;
    point3d cursor_direction = (result_end - rotateCenter).normalized();

    //////////////////////////////////////
    // создание элемента

    PElement newByElement;
    point3d  p0 = element_->globalPoint(activeNodeNo_);
    switch(mode_)
    {
    case Mode::OverNodeFree: {
        // вращение вокруг оси, используется только для тех элементов,
        // которые являются двухмерными (например ППК)

        // первая стадия выбирает направление выбранного нода
        // вращение нелинейного элемента
        newByElement = proto->cloneMoveRotate(scene_, activeNodeNo_, p0, cursor_direction);
    }
    break;
    case Mode::OverNodeAxis: {
        // вторая стадия (или единственная) изменяет положение
        // фиксируя выбранный нод, и вращая элемент вокруг него
        newByElement = proto->cloneMoveRotate(scene_, activeNodeNo_, p0, -d0, cursor_direction);
    }
    break;
    case Mode::OverCenterFree:
    case Mode::Flip: {
        // определяем положение первого и второго нода элемента
        point3d d0 = proto->direction(0);
        point3d d1 = proto->direction(1);

        if(mode_ != Mode::Flip || d0.isParallel(d1))
        {
            // если это флип линейной фигуры, или
            // свободное вращение
            auto nodePoint = rotateCenter - cursor_direction * size / 2;
            newByElement =
                proto->cloneMoveRotate(scene_, activeNodeNo_, nodePoint, cursor_direction);
            break;
        }

        // хитрая фигня для флипа треугольной фигуры
        if(cursor_direction.isCoaimed(-proto->direction(0)))
        { // ничего не меняем
            newByElement =
                proto->cloneMoveRotate(scene_, activeNodeNo_, proto->globalPoint(0), -d0, d1);
        }
        else
        {
            // меняем местами ноды
            newByElement =
                proto->cloneMoveRotate(scene_, activeNodeNo_, proto->globalPoint(1), -d1, d0);
        }
    }
    }

    // выделяем элемент
    n->hoverState[newByElement] = HoverState::Newby;

    scene_->pushAdorner(new NodePointAdorner(rotateCenter, AdornerLineFromNone));
    scene_->pushAdorner(new MoveAdorner(rotateCenter, result_end));

    Command cmd;
    cmd << cmdReplaceObject(scene_, element_, newByElement);
    commandList_.addAndExecute(cmd);

    if(mode_ == Mode::OverNodeFree || rotateAfterFlip_)
    {
        auto tool = RotateElement::rotateOverNodeAxis(manipulator_, newByElement, activeNodeNo_);
        tool->setPrevTool(prevTool_);
        nextTool_ = PManipulatorTool(tool);
    }
    else
    {
        QString type = element_->elementName();
        nextTool_ = prevTool_ ? prevTool_ : PManipulatorTool(new StartElement(manipulator_, type));
    }
}

void RotateElement::do_changeSize(int sizeF, bool absolute)
{
    int factor   = qMin(4, qMax(-4, absolute ? sizeF : scaleFactor_ + 2 * sizeF));
    scaleFactor_ = factor;

    scene_->updatePropertiesAfterSceneChange();
}

void RotateElement::do_takeToolProperty(SceneProperties &props)
{
    ElementInfo info = element_->info();
    info.setScaleFactor(scaleFactor_);
    props.addElement(info);
}

bool RotateElement::do_updateToolProperties(ScenePropertyValue v)
{
    if(v.type == ScenePropertyType::ValveSize)
    {
        auto current = SceneProperties::getElementScale(v.current);

        if(scaleFactor_ == current)
            return false;

        scaleFactor_ = current;
        return true;
    }
    return false;
}

} // namespace geometry
