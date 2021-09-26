#ifndef MANIPULATORELEMENTSTART_H
#define MANIPULATORELEMENTSTART_H

#include "manipulator.h"

namespace geometry
{

class StartElement : public ManipulatorTool
{
    Q_OBJECT
    ElementInfo sample_;

public:
    StartElement(Manipulator *m, ElementInfo);
    StartElement(Manipulator *m, QString elementType);

public: //
    void    do_click(point2d, PNeighbourhood);
    QString helperText() const;

    void do_changeRotation(int d);
    void do_changeSize(int sizeFactor, bool absolute);
    void do_toggleMode();

    void do_takeToolProperty(SceneProperties &);
    bool do_updateToolProperties(ScenePropertyValue);
};

} // namespace geometry

#endif // MANIPULATORELEMENTSTART_H
