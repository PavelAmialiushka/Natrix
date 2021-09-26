#ifndef PROPERTYBAR_H
#define PROPERTYBAR_H

#include <QAction>
#include <QObject>
#include <QToolButton>

#include "geometry/sceneProperties.h"
using geometry::ScenePropertyValue;

class PropertyBarModel : public QObject
{
    Q_OBJECT

    QList<QToolButton *> buttons_;
    ScenePropertyValue   value_;

    int exclusiveMode_;
    int lockChanges_;

public:
    PropertyBarModel(QObject *parent = 0);

    void update(geometry::ScenePropertyValue const &d);
    void addButton(QToolButton *a);

    void setExclusiveMode(bool);

private:
signals:
    void onPropertyValueChanged(ScenePropertyValue);

private slots:
    void onTriggered(bool);
};

#endif // PROPERTYBAR_H
