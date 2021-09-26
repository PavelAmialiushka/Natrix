#include "manipulatorTextLine.h"
#include "manipulator.h"
#include "manipulatorTextStart.h"
#include "manipulatortextedit.h"
#include "marker.h"
#include "neighbourhood.h"
#include "sceneProcedures.h"
#include "textLabel.h"
#include "utilites.h"

#include <QDebug>

namespace geometry
{

QString TextLine::helperText() const
{
    return QString::fromUtf8("проведите линию текста") + helperTab +
           QString::fromUtf8(
               "удерживайте <b>CTRL</b> чтобы присоединить линию к существующей метке");
}

TextLine::TextLine(Manipulator *m, PObjectToSelect object, point3d point)
    : ManipulatorTool(m)
    , startPoint(point)
    , objectToConnect(object)
{
    supportModes_ = Dragging;
}

PManipulatorTool TextLine::create(Manipulator *m, PObjectToSelect object, point3d point)
{
    return PManipulatorTool(new TextLine(m, object, point));
}

void TextLine::do_prepare()
{
    nextTool_ = manipulator_->prevTool();
}

void TextLine::do_drag(point2d point, PNeighbourhood nei, bool)
{
    return do_drop(point, nei);
}

void TextLine::do_drop(point2d point, PNeighbourhood nei)
{
    Command cmd;
    setCursor("text-new");

    PLabel newbyLabel;

    auto labelList = utils::filtered(nei->closestLabels, isText);
    if(labelList.size())
    {
        newbyLabel = labelList.first();
    }

    if(newbyLabel && (manipulator_->getMode(ControlKey)))
    {
        // если существует метка и дана команда на перезапись
        nextTool_ = TextStart::create(manipulator_);

        // прикрепляем новый маркер к метке
        auto marker = Marker::create(startPoint, objectToConnect, newbyLabel);
        cmd << cmdAttachMarker(scene_, marker);
    }
    else
    {
        // создаем текст заново

        TextInfo tinfo  = scene_->defaultTextInfo();
        tinfo.text      = "text";
        tinfo.basePoint = point;
        auto text       = new TextLabel(scene_, tinfo);

        // создаем метку
        newbyLabel = PLabel(text);
        newbyLabel->setLabelActive();
        cmd << cmdAttachLabel(scene_, newbyLabel);

        nextTool_ = TextEdit::createOnExistingLabel(manipulator_,
                                                    newbyLabel,
                                                    point,
                                                    /*selectAll=*/true);
        // создаем маркер
        auto marker = Marker::create(startPoint, objectToConnect, newbyLabel);
        cmd << cmdAttachMarker(scene_, marker);
    }

    commandList_.addAndExecute(cmd);
}

} // namespace geometry
