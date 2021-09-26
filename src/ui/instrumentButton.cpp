#include "instrumentButton.h"
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QStyleOption>

InstrumentButton::InstrumentButton(QWidget *parent)
    : QToolButton(parent)
{
    textVisible_ = false;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

bool InstrumentButton::isTextVisible() const
{
    return textVisible_;
}

void InstrumentButton::setTextVisible(bool v)
{
    textVisible_ = v;
}

QSize InstrumentButton::sizeHint()
{
    return textVisible_ ? QSize(100, 32) : QSize(32, 32);
}

void InstrumentButton::paintEvent(QPaintEvent *evt)
{
    //    return QPushButton::paintEvent(evt);

    QPainter painter(this);
    QRect    rect = evt->rect();

    // заполнение фона
    painter.setBackground(palette().base());
    painter.fillRect(rect, palette().base());

    bool hover = rect.contains(mapFromGlobal(QCursor::pos()));
    if(hover)
    {
        // стандартная рамка
        QStyleOptionButton opt;
        opt.init(this);
        style()->drawPrimitive(QStyle::PE_FrameDefaultButton, &opt, &painter, this);
    }
    else if(isChecked())
    {
        QStyleOptionButton opt;
        opt.init(this);
        opt.state |= QStyle::State_Sunken;
        style()->drawPrimitive(QStyle::PE_FrameButtonBevel, &opt, &painter, this);
    }

    // размещение зон
    QSize iconSz = iconSize();

    QRect iconRect(QPoint(rect.left() + 2, rect.top() + (rect.height() - iconSz.height()) / 2),
                   iconSz);
    icon().paint(&painter, iconRect);

    // рисуем текст
    if(textVisible_)
    {
        QRect textRect(QPoint(iconRect.right(), rect.top()), QPoint(rect.right(), rect.bottom()));
        textRect.adjust(4, 2, -2, -2);

        style()->drawItemText(
            &painter, textRect, Qt::AlignLeft | Qt::AlignVCenter, palette(), true, text());
    }
}
