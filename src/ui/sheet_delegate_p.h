#ifndef SHEET_DELEGATE_H
#define SHEET_DELEGATE_H

#include <QItemDelegate>
#include <QTreeView>

QT_BEGIN_NAMESPACE

class QTreeView;

class SheetDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    SheetDelegate(QTreeView *view, QWidget *parent);

    virtual void  paint(QPainter *                  painter,
                        const QStyleOptionViewItem &option,
                        const QModelIndex &         index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const;

private:
    QTreeView *m_view;
};

QT_END_NAMESPACE

#endif // SHEET_DELEGATE_H
