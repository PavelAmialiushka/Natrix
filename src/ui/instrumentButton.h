#ifndef INSTRUMENTBUTTON_H
#define INSTRUMENTBUTTON_H

#include <QPushButton>
#include <QToolButton>

class InstrumentButton : public QToolButton
{
    Q_OBJECT

    bool textVisible_;

public:
    explicit InstrumentButton(QWidget *parent = 0);

    bool isTextVisible() const;
    void setTextVisible(bool v);

protected:
    virtual void  paintEvent(QPaintEvent *);
    virtual QSize sizeHint();
signals:

public slots:
};

#endif // INSTRUMENTBUTTON_H
