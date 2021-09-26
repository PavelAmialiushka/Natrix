#ifndef MOVEPROCEDURESDATA_H
#define MOVEPROCEDURESDATA_H

#include "makeSmart.h"
#include "object.h"

#include "moveProcedures.h"

#include <QList>
#include <QSet>

namespace geometry
{

MAKESMARTS(Vertex);
using PVertex = QSharedPointer<Vertex>;
MAKESMARTS(Edge);
using PEdge = QSharedPointer<Edge>;
MAKESMARTS(MetaEdge);
using PMetaEdge = QSharedPointer<MetaEdge>;
MAKESMARTS(MetaVertex);
using PMetaVertex = QSharedPointer<MetaVertex>;

// группа элементов, связанных друг с другом последовательно.
struct Edge
{
    PVertex start;
    PVertex finish;

    // то, что лежит внутри грани
    PObject object;
};

// группа элементов, которые перемещаются
// не изменяя своего размера
struct Vertex
{
    QString name;

    QSet<PNode>  nodes; // содержание вершины
    QList<PEdge> edges; // грани к другим вершинам
};

struct VertexData
{
    point3d delta; // смещение узла
    bool    fixed; // можно ли двигать

    QList<point3d> deltas;
    point3d        initDelta;
    point3d        prevDelta;

    VertexData(point3d x = 0)
        : fixed(false)
        , delta(x)
    {
    }
};

struct MetaVertexData
{
    bool    hasActualDelta = false;
    point3d actualDelta;

    void setActualDelta(point3d d)
    {
        hasActualDelta = true;
        actualDelta    = d;
    }
};

///////////////////////////////////////////////////////////////////////////////

struct MetaVertex
{
    QString          name;
    PVertex          subvertex;
    QList<PMetaEdge> edges;

    MetaVertex(PVertex v)
        : subvertex(v)
        , name(v->name)
    {
    }
};

struct MetaEdge
{
    PMetaVertex    start;       // [0]
    PMetaVertex    finish;      // [last]
    QList<PEdge>   subedges;    // [1,3,...last-1]
    QList<PVertex> subvertexes; // [2,4,...last-2]

    QString name;
    void    makeName();

    PMetaVertex otherVertex(PMetaVertex m) const
    {
        return start == m ? finish : start;
    }
};

///////////////////////////////////////////////////////////////////////////////
struct MoveGraph
{
    // список узлов
    QList<PVertex> vertexes;

    // список граней
    QList<PEdge> edges;

    // кеши
    QHash<PNode, PVertex> vertexOf;

    //    QHash<PNode, NodeMovement> nodeMovementCache;
    //    QHash<PObject, ObjectResize> objectResizeCache;
    //    QHash<PObject, LineRestriction> lineRestrictionCache;

    QHash<PVertex, VertexData> vertexData;

    MoveParameters params;

    QList<PMetaVertex>                 metaVertexes;
    QHash<PVertex, PMetaVertex>        metaVertexOf;
    QHash<PMetaVertex, MetaVertexData> metaVertexData;

public:
    // создание графа
    static MoveGraph create(Scene *scene, MoveParameters param, bool &ok);
    PEdge            createEdge(PObject object, PVertex vertex, PVertex otherV);
    void             joinVertexes(PVertex vertex, PVertex alter);

    // поиск решения
    bool solve_MoveNode_simple(PNode startNode, point3d startDelta);

    ////////////////////////
    void constructMetaGraph();
    bool solve_MoveNode_advanced();

public:
    struct StretchMetaEdgeResult
    {
        QMap<PVertex, VertexData> vertexData;
        double                    k;
        double                    length;
    };

private: // работа с метаграфом
    void walkMetaEdge(PMetaEdge me,
                      function<void(PVertex a, PEdge edge, PVertex b)>,
                      bool lastVertex = false);

    bool strechMetaEdge(PMetaEdge me, point3d startStretch, StretchMetaEdgeResult &result);

    ////////////////////////
    void selectMasterAndSlaves(QSet<PMetaVertex> &slaveVs,
                               QSet<PMetaVertex> &citizenVs,
                               QSet<PMetaVertex> &masterVs);
    bool moveMasterVertexes(QSet<PMetaVertex> &masterVs, QSet<PMetaEdge> &drawnEdges);
    bool moveCitizenVertexes(QSet<PMetaVertex> citizenVs,
                             QSet<PMetaVertex> slaveVs,
                             QSet<PMetaEdge> & drawnEdges);
    bool moveSlaveVertexes(QSet<PMetaEdge> &drawnEdges);

private:
    ////////////////////////
    QStringList toStringList() const;

private:
    void convertToSubVertex(PVertex vertex);
    void addSubEdge(PEdge edge, PEdge right, bool flipRight);
};

} // namespace geometry

#endif // MOVEPROCEDURESDATA_H
