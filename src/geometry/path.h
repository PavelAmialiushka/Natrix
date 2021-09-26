#ifndef PATH_H
#define PATH_H

#include "object.h"
#include "scene.h"

#include <QList>
#include <QSharedPointer>

#include <functional>
using std::function;

#ifdef _MSC_VER
#include <memory>
#else
#include <tr1/memory>
#endif

namespace geometry
{

typedef QSet<PNode>  Set;
typedef QList<PNode> Route;
typedef QList<Route> Routes;

struct NodePathInfo
{
    Route const &route;
    PNode        node;
    QList<PNode> doneNeighbours;
    QList<PNode> nextNeighbours;
};

class Path
{
    struct impl;
    std::shared_ptr<impl> pimpl;

public:
    explicit Path(Scene *);
    ~Path();

    PNode  origin() const;
    Routes routes() const;

public:
    static bool                                  alwaysTrue(NodePathInfo const &);
    static bool                                  alwaysFalse(NodePathInfo const &);
    typedef function<bool(NodePathInfo const &)> PathPrep;

    static Path makePathSimple(Scene *  scene,
                               PNode    node,
                               PathPrep prepAccept   = alwaysTrue,
                               PathPrep prepFound    = alwaysFalse,
                               bool     keepDeadEnds = false);

    static void walkContour(Scene *scene, PNode node, PathPrep prepAccept = alwaysTrue);
};

} // namespace geometry
#endif // PATH_H
