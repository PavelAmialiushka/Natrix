#ifndef WIDGETBOXTREEWIDGET_H
#define WIDGETBOXTREEWIDGET_H

#include <QTreeWidget>

QT_BEGIN_NAMESPACE

class WidgetBoxTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    WidgetBoxTreeWidget(QWidget *parent = 0);

    virtual void resizeEvent(QResizeEvent *event);

private slots:
    void          handleMousePress(QTreeWidgetItem *item);
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
};

QT_END_NAMESPACE

#endif // WIDGETBOXTREEWIDGET_H
