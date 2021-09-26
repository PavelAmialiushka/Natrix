#ifndef MANIPULATORSTARTLABEL_H
#define MANIPULATORSTARTLABEL_H

#include "manipulatorTools.h"

namespace geometry
{

class LabelStart : public ManipulatorTool
{
public:
    Q_OBJECT

    QString elementType_;

public:
    LabelStart(Manipulator *m, QString elementType);
    static PManipulatorTool create(Manipulator *m, QString elementType);

public:
    void do_toggleMode();
    void do_click(point2d, PNeighbourhood);
    void do_drag(point2d pt, PNeighbourhood, bool started = 0);

    QString helperText() const;
};

} // namespace geometry

#endif // MANIPULATORSTARTLABEL_H
