#include "canvasPropertiesDialog.h"
#include "ui_canvasPropertiesDialog.h"

#include <geometry/canvasRectangle.h>
#include <geometry/manipulator.h>
#include <geometry/manipulatorCanvasStart.h>
#include <geometry/scene.h>

#include <geometry/sceneProcedures.h>

#include <QPainter>
#include <QSettings>

CanvasPropertiesDialog::CanvasPropertiesDialog(Scene *scene, QWidget *parent)
    : QDialog(parent)
    , scene_(scene)
    , ui(new Ui::CanvasPropertiesDialog)
{
    ui->setupUi(this);

    QObject::connect(ui->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    QObject::connect(ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    lock_ = true;
    ui->pageSize->addItem("A5");
    ui->pageSize->addItem("A4");
    ui->pageSize->addItem("A3");

    // настройка списка страниц
    updateCanvasList();

    if(ui->canvasNo->selectionModel()->selectedIndexes().empty())
    {
        auto rootIndex = ui->canvasNo->rootIndex();
        ui->canvasNo->selectionModel()->select(rootIndex, QItemSelectionModel::Select);
    }

    // обнуляем список Undo
    scene_->manipulator()->setTool(CanvasStart::create(scene_->manipulator()), true, true);
    scene_->manipulator()->appendCommand(Command(), false);

    lock_ = false;
}

void CanvasPropertiesDialog::updateCanvasList()
{
    int canvasCount = scene_->canvasCount();
    int bvc         = scene_->bestVisibleCanvas();
    for(int index = 0; index < canvasCount; ++index)
    {
        QListWidgetItem *item = 0;
        if(index < ui->canvasNo->count())
            item = ui->canvasNo->item(index);
        else
        {
            item = new QListWidgetItem;
            ui->canvasNo->addItem(item);
        }

        // рисуем изображение страницы
        QPixmap  image{64, 64};
        QPainter paint(&image);
        paint.fillRect(QRect(0, 0, 64, 64), Qt::white);

        QFont font{"Tahoma", 48};
        paint.setFont(font);

        auto text = QString(QString::fromUtf8("%1").arg(scene_->canvas(index)->info().index));
        paint.drawText(0, 0, 64, 64, Qt::AlignCenter, text);
        paint.end();

        item->setIcon(QIcon(image));

        // если текущий канвас, то выделяем его
        if(index == bvc)
        {
            ui->canvasNo->setCurrentItem(item);
            ui->canvasNo->selectionModel()->select(ui->canvasNo->currentIndex(),
                                                   QItemSelectionModel::Select);
        }
    }

    while(ui->canvasNo->count() > scene_->canvasCount())
    {
        auto row = ui->canvasNo->count() - 1;
        ui->canvasNo->takeItem(row);
    }
}

CanvasPropertiesDialog::~CanvasPropertiesDialog()
{
    delete ui;
}

void CanvasPropertiesDialog::on_canvasNo_currentRowChanged(int /*index*/)
{
}

void CanvasPropertiesDialog::on_canvasNo_itemSelectionChanged()
{
    CanvasInfo info = scene_->canvas(0)->info();
    switch(info.size)
    {
    case CanvasSize::A5:
        ui->pageSize->setCurrentIndex(0);
        break;
    case CanvasSize::A4:
        ui->pageSize->setCurrentIndex(1);
        break;
    case CanvasSize::A3:
        ui->pageSize->setCurrentIndex(2);
        break;
    }
    if(info.landscape)
        ui->landscapeOrientation->setChecked(true);
    else
        ui->portraitOrientation->setChecked(true);
}

CanvasInfo CanvasPropertiesDialog::getCurrentInfo(PCanvasRectangle canvas)
{
    CanvasInfo info = canvas->info();

    CanvasSize size = CanvasSize::A4;
    switch(ui->pageSize->currentIndex())
    {
    case 0:
        size = CanvasSize::A5;
        break;
    case 1:
        size = CanvasSize::A4;
        break;
    case 2:
        size = CanvasSize::A3;
        break;
    }

    bool land = ui->landscapeOrientation->isChecked();
    info.setSize(size, land);

    return info;
}

void CanvasPropertiesDialog::updateCurrentCanvas()
{
    for(int index = 0; index < scene_->canvasCount(); ++index)
    {
        PCanvasRectangle canvas = scene_->canvas(index);
        CanvasInfo       info   = getCurrentInfo(canvas);

        PLabel  newby = canvas->create(scene_, info);
        Command cmd   = cmdReplaceLabel(scene_, canvas, newby);
        cmd << cmdModify(scene_);
        registerCommand(cmd);
    }
}

void CanvasPropertiesDialog::on_portraitOrientation_clicked()
{
    updateCurrentCanvas();
}

void CanvasPropertiesDialog::on_landscapeOrientation_clicked()
{
    updateCurrentCanvas();
}

void CanvasPropertiesDialog::on_pageSize_currentIndexChanged(int /*pageSizeIndex*/)
{
    if(!lock_)
        updateCurrentCanvas();
}

void CanvasPropertiesDialog::registerCommand(Command cmd)
{
    scene_->manipulator()->appendCommand(cmd, true);

    // необходимо, чтобы инструмент отобразил изменения холстов
    scene_->manipulator()->moveToTheSamePoint();
}

void CanvasPropertiesDialog::on_addButton_clicked()
{
    PCanvasRectangle canvas = scene_->canvas(0);
    CanvasInfo       info   = getCurrentInfo(canvas);

    point2d delta = info.rightBottom - info.leftTop;

    // выбираем первый свободный номер
    QSet<int> numbers;
    for(int index = 0; index < scene_->canvasCount(); ++index)
        numbers << scene_->canvas(index)->info().index;
    for(int index = 1;; ++index)
    {
        if(numbers.contains(index))
            continue;
        info.index = index;
        break;
    }

    // выбираем положение относительно первого листа
    for(int count = 0;; ++count)
    {
        bool overlapes = false;

        if(count % 2 == 1)
            info.shiftBy(point2d(-delta.x, delta.y));
        else
            info.shiftBy(point2d(delta.x, 0));

        for(int index = 0; index < scene_->canvasCount(); ++index)
        {
            if(info.equalTo(scene_->canvas(index)->info()))
                overlapes = true;
        }
        if(!overlapes)
            break;
    }

    PLabel  newby = canvas->create(scene_, info);
    Command cmd   = cmdAttachLabel(scene_, newby);
    cmd << cmdModify(scene_);
    registerCommand(cmd);

    updateCanvasList();
}

void CanvasPropertiesDialog::on_delButton_clicked()
{
    int index = ui->canvasNo->currentRow();
    if(index >= scene_->canvasCount())
        return;

    PCanvasRectangle canvas = scene_->canvas(index);
    Command          cmd    = cmdDetachLabel(scene_, canvas);
    cmd << cmdModify(scene_);
    registerCommand(cmd);

    updateCanvasList();
}
