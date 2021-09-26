#include "propertyBarModel.h"

#include "geometry/sceneProperties.h"

#include <QPainter>

using namespace geometry;

PropertyBarModel::PropertyBarModel(QObject *parent)
    : QObject(parent)
    , exclusiveMode_(true)
    , lockChanges_(false)
{
}

void PropertyBarModel::update(ScenePropertyValue const &d)
{
    ++lockChanges_;

    value_         = d;
    exclusiveMode_ = d.exclusive;

    Q_ASSERT(d.active.size() <= buttons_.size());

    for(int index = 0; index < buttons_.size(); ++index)
    {
        bool active = (uint)index < (uint)d.active.size() ? d.active[index] : false;

        QString style;

        auto b = buttons_[index];

        if(d.current == index)
        {
            if(d.matchStyleMode)
            {
                style = "QToolButton { "
                        "  border-style: solid; "
                        "  border-width: 3px; "
                        "  border-color: blue; "
                        "} "
                        "QToolButton:hover {"
                        "  background-color: blue; "
                        "} ";
            }
            else
            {
                style = "QToolButton { "
                        "  border-style: solid; "
                        "  border-width: 3px; "
                        "  border-color: black; "
                        "} "
                        "QToolButton:hover {"
                        "  background-color: black; "
                        "} ";
            }
        }
        else if(active)
        {
            style = "QToolButton {"
                    "  border-style: solid; "
                    "  border-width: 2px; "
                    "  border-color: rgb(162,162,162); "
                    "} "
                    "QToolButton:hover {"
                    "  background-color: rgb(128,128,128); "
                    "} ";
        }
        else
        {
            style = "QToolButton {"
                    "} "
                    "QToolButton:hover {"
                    "  border-style: solid; "
                    "  border-width: 1px;"
                    "  border-color: gray; "
                    "}";
        }

        b->setStyleSheet(style);
    }

    --lockChanges_;
}

void PropertyBarModel::addButton(QToolButton *b)
{
    buttons_ << b;

    connect(b, SIGNAL(clicked(bool)), this, SLOT(onTriggered(bool)));
}

void PropertyBarModel::setExclusiveMode(bool m)
{
    exclusiveMode_ = m;
}

void PropertyBarModel::onTriggered(bool state)
{
    if(lockChanges_)
        return;
    ++lockChanges_;

    auto *from     = qobject_cast<QToolButton *>(this->sender());
    int   index    = buttons_.indexOf(from);
    value_.current = index;

    // сообщаем о выборе главной кнопки
    emit onPropertyValueChanged(value_);

    --lockChanges_;
}
