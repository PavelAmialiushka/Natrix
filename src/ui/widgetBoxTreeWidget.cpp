#include "widgetBoxTreeWidget.h"
#include "sheet_delegate_p.h"

#include <QApplication>
#include <QHeaderView>
#include <QSettings>
#include <qevent.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

//------------------------------ WidgetBoxTreeWidget ----------------------------------------

WidgetBoxTreeWidget::WidgetBoxTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setIndentation(0);
    setRootIsDecorated(false);
    setColumnCount(1);
    header()->hide();
    // header()->setResizeMode(QHeaderView::Stretch);
    setTextElideMode(Qt::ElideMiddle);
    setVerticalScrollMode(ScrollPerPixel);

    setIconSize(QSize(32, 32));

    setItemDelegate(new SheetDelegate(this, this));

    connect(this,
            SIGNAL(itemPressed(QTreeWidgetItem *, int)),
            this,
            SLOT(handleMousePress(QTreeWidgetItem *)));
}

void WidgetBoxTreeWidget::resizeEvent(QResizeEvent *event)
{
    QTreeWidget::resizeEvent(event);

    QSettings sett;
    sett.setValue("ToolsWindowWidth", event->size().width());
}

void WidgetBoxTreeWidget::handleMousePress(QTreeWidgetItem *item)
{
    if(item == 0)
        return;

    if(QApplication::mouseButtons() != Qt::LeftButton)
        return;

    if(item->parent() == 0)
    {
        setItemExpanded(item, !isItemExpanded(item));
        return;
    }
}

QSize WidgetBoxTreeWidget::sizeHint() const
{
    return QSize(150, 10);

    //    QSettings sett;
    //    int width = sett.value("ToolsWindowWidth", 123).toInt();

    //    return QSize(width, size().height());
}

QSize WidgetBoxTreeWidget::minimumSizeHint() const
{
    QRect r = childrenRect();
    r.adjust(0, 0, 5, 0);
    return r.size();
}

QT_END_NAMESPACE
