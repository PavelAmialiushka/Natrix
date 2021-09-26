#ifndef MANIPULATORTEXTLINE_H
#define MANIPULATORTEXTLINE_H

#include "manipulatorTools.h"

namespace geometry
{

class TextLine : public ManipulatorTool
{
    Q_OBJECT

    point3d         startPoint;
    PObjectToSelect objectToConnect;

public:
    TextLine(Manipulator *m, PObjectToSelect object, point3d point);
    static PManipulatorTool create(Manipulator *m, PObjectToSelect object, point3d point);

public:
    void do_prepare();
    void do_drag(point2d, PNeighbourhood, bool);
    void do_drop(point2d, PNeighbourhood);

    QString helperText() const;
};

} // namespace geometry
#endif // MANIPULATORTEXTLINE_H
