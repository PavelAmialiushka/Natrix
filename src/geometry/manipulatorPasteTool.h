#ifndef MANIPULATORPASTETOOL_H
#define MANIPULATORPASTETOOL_H

#include "manipulator.h"
#include "neighbourhood.h"

namespace geometry
{

class PasteTool : public ManipulatorTool
{
    Q_OBJECT

    QByteArray            data_;
    QSet<PObject>         objects_;
    QSet<PLabel>          labels_;
    QList<PMarker>        markers_;
    point2d               center_;
    bool                  correct_;
    int                   planeType_;
    QSharedPointer<Scene> otherScene_;

    int rotation_;

public:
    PasteTool(Manipulator *m, QByteArray data);
    static PManipulatorTool create(Manipulator *m, PManipulatorTool prev, QByteArray data);

    void    do_click(point2d, PNeighbourhood);
    void    do_prepare();
    void    do_changeRotation(int d);
    QString helperText() const;
};

} // namespace geometry
#endif // MANIPULATORPASTETOOL_H
