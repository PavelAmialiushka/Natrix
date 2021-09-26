#ifndef MANIPULATORSELECTOR_H
#define MANIPULATORSELECTOR_H

#include "manipulatorTools.h"
#include "neighbourhood.h"

#include <QSet>

namespace geometry
{

class Selector : public ManipulatorTool
{
    Q_OBJECT

    QSet<PObjectToSelect> selAddObjects;
    QSet<PObjectToSelect> selRemObjects;

    point2d lt, rb;

public:
    Selector(Manipulator *m);

    static PManipulatorTool create(Manipulator *m);

    void updateProperties(ScenePropertyValue v);

public:
    void    do_prepare();
    void    do_drag(point2d, PNeighbourhood, bool started);
    void    do_drop(point2d, PNeighbourhood);
    void    do_move(point2d start, PNeighbourhood nei);
    void    do_click(point2d, PNeighbourhood);
    void    do_doubleClick(point2d, PNeighbourhood);
    void    do_changeSize(int sizeFactor, bool absolute);
    void    do_commit();
    QString helperText() const;

private:
    void makeSelection(PNeighbourhood, bool combine = true);
};

} // namespace geometry

#endif // MANIPULATORSELECTOR_H
