#ifndef MANIPULATORLINEBREAKER_H
#define MANIPULATORLINEBREAKER_H

#include "manipulatorTools.h"
#include "neighbourhood.h"

#include <QSet>

namespace geometry
{

class LineBreaker : public ManipulatorTool
{
    Q_OBJECT
public:
    LineBreaker(Manipulator *m);

    static PManipulatorTool create(Manipulator *m);

public:
    void    do_connect(point2d, PNeighbourhood);
    void    do_click(point2d, PNeighbourhood);
    QString helperText() const;
};

} // namespace geometry

#endif // MANIPULATORLINEBREAKER_H
