#include "propertyTreeWidget.h"
#include "ui_propertyTreeWidget.h"

#include "geometry/scene.h"

#include <QToolBar>
#include <QToolButton>
#include <qstyleditemdelegate.h>

#include "geometry/manipulator.h"
#include "geometry/sceneProperties.h"

using namespace geometry;

PropertyTreeWidget::PropertyTreeWidget(QWidget *parent, Scene *scene)
    : QWidget(parent)
    , scene_(scene)
    , ui(new Ui::PropertyTreeWidget)
{
    ui->setupUi(this);

    auto addButton = [&](QHBoxLayout *box, PropertyBarModel *prop, QString iconName) {
        auto button = new QToolButton(this);
        button->setAutoRaise(true);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setIcon(QIcon(iconName));
        button->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        button->setIconSize(QSize(32, 32));

        prop->addButton(button);
        box->addWidget(button);
    };

    auto createToolbar = [&](QHBoxLayout *box, PropertyBarModel *&bar, QString templ, int maximum) {
        bar = new PropertyBarModel(this);
        for(int index = 0; index <= maximum; ++index)
            addButton(box, bar, QString(":/icon-%1%2.png").arg(templ).arg(index));

        box->setSpacing(0);
        box->addStretch(2);

        connect(bar,
                SIGNAL(onPropertyValueChanged(ScenePropertyValue)),
                this,
                SLOT(onPropertyValueChanged(ScenePropertyValue)));
    };

    createToolbar(ui->horizontalLayout_11, textSizeBar_, "text-size", 5);
    createToolbar(ui->horizontalLayout_12, textRotationBar_, "text-rot", 3);
    createToolbar(ui->horizontalLayout_13, textDecorationBar_, "text-dec", 3);

    createToolbar(ui->horizontalLayout_21, valveSizeBar_, "valve", 4);
    createToolbar(ui->horizontalLayout_22, valveFlangesBar_, "flanges", 1);

    createToolbar(ui->horizontalLayout_31, lineStyleBar_, "line-style", 3);
    createToolbar(ui->horizontalLayout_32, bendStyleBar_, "bend-style", 2);
    createToolbar(ui->horizontalLayout_33, teeStyleBar_, "tee-style", 2);
    createToolbar(ui->horizontalLayout_34, endStyleBar_, "end-style", 1);

    connect(scene_,
            SIGNAL(updatePropertiesAfterSceneChange()),
            this,
            SLOT(updatePropertiesAfterSceneChange()));
}

PropertyTreeWidget::~PropertyTreeWidget()
{
    delete ui;
}

void PropertyTreeWidget::updatePropertiesAfterSceneChange()
{
    SceneProperties props = scene_->getPropertiesFromScene();

    textSizeBar_->update(props.textSize);
    textRotationBar_->update(props.textRotation);
    textDecorationBar_->update(props.textDecoration);

    valveSizeBar_->update(props.valveSize);
    valveFlangesBar_->update(props.valveFlanges);

    lineStyleBar_->update(props.lineStyle);
    bendStyleBar_->update(props.bendStyle);
    teeStyleBar_->update(props.teeStyle);
    endStyleBar_->update(props.endStyle);
}

void PropertyTreeWidget::onPropertyValueChanged(ScenePropertyValue v)
{
    scene_->manipulator()->updatePropertiesDown(v);
}
