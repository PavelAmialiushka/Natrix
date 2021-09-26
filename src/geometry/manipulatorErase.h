#ifndef MANIPULATORERASE_H
#define MANIPULATORERASE_H

#include "manipulatorTools.h"
#include "neighbourhood.h"

#include <QSet>

namespace geometry
{

class EraseObjects : public ManipulatorTool
{
    Q_OBJECT
    QSet<PObjectToSelect> selObjects_;
    point2d               lt_, rb_;

public:
    EraseObjects(Manipulator *m);

    static PManipulatorTool create(Manipulator *m);

private:
    void makeSelection(PNeighbourhood nei);

public:
    void do_prepare();
    void do_click(point2d, PNeighbourhood);
    void do_drag(point2d, PNeighbourhood, bool started);
    void do_drop(point2d, PNeighbourhood);

    void    do_commit();
    QString helperText() const;
};

} // namespace geometry

#endif // MANIPULATORERASE_H
