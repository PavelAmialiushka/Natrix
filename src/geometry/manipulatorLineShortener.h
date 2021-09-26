#ifndef MANIPULATORLINESHORTENER_H
#define MANIPULATORLINESHORTENER_H

#include "manipulatorTools.h"

namespace geometry
{

struct MoveData;

class LineShortener : public ManipulatorTool
{
    Q_OBJECT

    bool    dragMode_;
    point3d dragStartPoint_;
    PObject dragObject_;

    struct Parameters;

public:
    LineShortener(Manipulator *m);

    static PManipulatorTool create(Manipulator *m);

    void makeLineShorter(Parameters &param, bool &ok);

public:
    void    do_click(point2d, PNeighbourhood);
    void    do_move(point2d, PNeighbourhood);
    void    do_drag(point2d, PNeighbourhood, bool started);
    void    do_drop(point2d, PNeighbourhood);
    void    dragFinish(point2d, PNeighbourhood, bool isdrop);
    QString helperText() const;
};

} // namespace geometry

#endif // MANIPULATORLINESHORTENER_H
