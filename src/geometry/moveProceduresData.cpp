#include "moveProceduresData.h"

#include "global.h"
#include "line.h"

#include "joiner.h"
#include "path.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFile>

#include <Eigen/Dense>

#include "simplexCalculator.h"

namespace geometry
{

#define __DO_NOT_LOG_MATRIX_RESULT
#define __DO_NOT_STRETCHMOVE_LOG

MoveGraph MoveGraph::create(Scene *scene, MoveParameters params, bool &ok)
{
    ok = true;
    MoveGraph graph;
    graph.params = params;

    int  vertex_index = 0;
    auto index2name   = [](int index) -> QString {
        QString name;
        while(1)
        {
            int digit = index % 26;
            index     = index / 26;

            name = QChar('A' + digit) + name;
            if(index)
                continue;

            return name;
        };
    };

    auto visit = [&](NodePathInfo const &info) -> bool {
        if(!ok)
            return false;

        PNode   currentNode   = info.node;
        PObject currentObject = currentNode->object();

        // что из существующих нодов входит в текущий объект
        QList<PNode> doneNeighboursOfObject;
        // соседний нод в чужом объекте (может быть только один)
        PNode doneFarNeighbour, doneNearNeighbour;

        // анализируем соседние ноды
        foreach(PNode it, info.doneNeighbours)
        {
            if(it->object() == currentNode->object())
            {
                doneNeighboursOfObject << it;
                doneNearNeighbour = it;
            }
            else
                doneFarNeighbour = it;
        }

        bool theFirstNode = info.doneNeighbours.isEmpty();
        bool theLastNode  = info.nextNeighbours.isEmpty();

        bool shouldCreateEdge = currentObject.dynamicCast<Line>();

        shouldCreateEdge = shouldCreateEdge && doneNearNeighbour;

        PVertex vertex;
        if(theFirstNode)
        {
            // никаких соседей нет, создаём новый узел
            vertex.reset(new Vertex{index2name(vertex_index++)});
            graph.vertexes << vertex;
        }
        else if(shouldCreateEdge)
        { // этот объект можно растягивать (это линия)

            if(theLastNode)
            {
                // убеждаемся, что сцена не испорчена
                if(!doneFarNeighbour || !doneNearNeighbour)
                {
                    qDebug() << "tree creation failed";
                    ok = false;
                    return false;
                }

                // встречный узел уже добавлен
                vertex             = graph.vertexOf[doneFarNeighbour]; // узел джойнера
                PVertex far_vertex = graph.vertexOf[doneNearNeighbour]; // дальний узел линии

                // создаём новую грань
                graph.createEdge(currentNode->object(), vertex, far_vertex);
            }
            else
            {
                // узел к которому будет прилепляться джойнер
                vertex.reset(new Vertex{index2name(vertex_index++)});
                graph.vertexes << vertex;

                if(!doneNearNeighbour)
                {
                    qDebug() << "tree creation failed";
                    ok = false;
                    return false;
                }
                PVertex far_vertex = graph.vertexOf[doneNearNeighbour]; // дальний узел линии

                // создаём новую грань
                graph.createEdge(currentNode->object(), vertex, far_vertex);
            }
        }
        else // всё остальное, просто добавляем к узлу
        {
            // проверяем, что не возникло лишних узлов
            vertex = graph.vertexOf[info.doneNeighbours.first()];
            foreach(PNode n, info.doneNeighbours)
            {
                PVertex alter = graph.vertexOf[n];
                if(alter != vertex)
                    graph.joinVertexes(vertex, alter);
            }
        }

        // добавляем ноду к вершине
        vertex->nodes.insert(currentNode);
        graph.vertexOf[currentNode] = vertex;

        if(params.objectRules.value(currentObject).rule == ObjectRestrictionType::Fixed)
            graph.vertexData[vertex].fixed = true;

        return true;
    };

    //
    //  создаём полный граф
    //
    PNode start = params.nodeMoves.keys().first();
    Path::walkContour(scene, start, visit);

    return graph;
}

void MoveGraph::constructMetaGraph()
{
    if(vertexes.isEmpty())
        return;

    QList<PVertex> toVisit;
    foreach(PNode n, params.nodeMoves.keys())
    {
        PVertex v = vertexOf[n];
        toVisit << v;
    }

    QSet<PVertex>              doneV;
    QSet<PEdge>                doneE;
    QMap<PVertex, PMetaVertex> v2mv;

    // отмечаем вершины, которые двигаются
    // они будут маркерами остановки
    QSet<PVertex> activeVertexes;
    foreach(PNode n, params.nodeMoves.keys())
        activeVertexes << vertexOf[n];

    std::function<void(PMetaVertex)>               checkEdges;
    std::function<void(PVertex v, PMetaEdge edge)> constructMetaEdge;

    // строит цепочку до следующей вершины
    constructMetaEdge = [&](PVertex from, PMetaEdge edge) -> void {
        PVertex next;
        for(;; from = next)
        {
            // движемся по этой грани
            PEdge e = edge->subedges.last();

            point3d s = e->object->globalPoint(1) - e->object->globalPoint(0);
            if(e->start == from)
                s = -s;
            Q_ASSERT(s);

            // следующая подвершина
            next = e->start == from ? e->finish : e->start;

            // если следующая вершина уже записана
            if(doneV.contains(next))
            {
                // находим метавершину
                PMetaVertex endPoint = v2mv[next];
                if(!endPoint)
                {
                    // это должна быть метавершина
                    Q_ASSERT(v2mv.count(next));

                    // ошибка, не знаю что делать
                    break;
                }

                // привязываемся к ней
                edge->finish = endPoint;
                endPoint->edges << edge;
                edge->makeName();

                // всё, стоп, формировать вершину не нужно
                return;
            }

            if(activeVertexes.contains(next))
                // активный узел, формируем вершину
                break;

            // продолжать не получиться, прерываемся
            if(next->edges.size() != 2)
                break;

            // если 2 грани, то пытаемся нарастить метагрань

            // подгрань, по которой будем выходить
            PEdge ne = next->edges[0] == e ? next->edges[1] : next->edges[0];

            ObjectRestriction r = params.objectRules[ne->object];
            if(r.rule == ObjectRestrictionType::Resize)
            {
                // эту грань нельзя использовать. прерываемся
                break;
            }

            // переходная вершина, превращаем следующую подвершину в метавершину
            edge->subvertexes << next;
            doneV << next;

            // удлиняем метагрань
            edge->subedges << ne;
            doneE << ne;
        }

        //////////////////////////////////////////////
        // не получиться продолжать. формируем веришну
        PMetaVertex endPoint{new MetaVertex{next}};
        metaVertexes << endPoint;
        metaVertexOf[next] = endPoint;

        // помечаем создание
        doneV << next;
        v2mv[next] = endPoint;

        // взаимные ссылки
        endPoint->edges << edge;
        edge->finish = endPoint;
        edge->makeName();

        // сканируем все окрестные пути
        checkEdges(endPoint);
    };

    // конструирует метаграни по всем направлениям
    checkEdges = [&](PMetaVertex mv) -> void {
        foreach(PEdge e, mv->subvertex->edges)
        {
            if(doneE.contains(e))
                continue;

            // создание новой метаграни
            PMetaEdge me{new MetaEdge{mv}};
            me->subedges << e;
            doneE << e;
            mv->edges << me;

            constructMetaEdge(mv->subvertex, me);
        }
    };

    while(toVisit.size())
    {
        PVertex sv = toVisit.takeFirst();
        if(doneV.contains(sv))
            continue;

        // создаём вершину
        PMetaVertex mv{new MetaVertex{sv}};
        metaVertexes << mv;
        metaVertexOf[sv] = mv;
        doneV << sv;
        v2mv[sv] = mv;

        checkEdges(mv);
    }

    // задаём начальные движения
    foreach(PNode n, params.nodeMoves.keys())
    {
        PVertex v = vertexOf[n];
        if(metaVertexOf.count(v))
        {
            auto mv       = metaVertexOf[v];
            auto movement = params.nodeMoves[n];
            metaVertexData[mv].setActualDelta(movement.delta);
        }
    }
}

static PEdge selectEdgeTo(PVertex left, PVertex right)
{
    foreach(PEdge a, left->edges)
        foreach(PEdge b, right->edges)
            if(a == b)
                return a;

    return PEdge();
}

PEdge MoveGraph::createEdge(PObject object, PVertex vertex, PVertex otherV)
{
    PEdge edge{new Edge{vertex, otherV, object}};
    if(vertexOf[object->nodeAt(0)] == otherV)
        qSwap(edge->start, edge->finish);

    edges << edge;
    vertex->edges << edge;
    otherV->edges << edge;

    return edge;
}

void MoveGraph::joinVertexes(PVertex vertex, PVertex alter)
{
    Q_ASSERT(vertex != alter);

    foreach(PEdge e, alter->edges)
    {
        if(e->start == alter)
            e->start = vertex;
        if(e->finish == alter)
            e->finish = vertex;
        vertex->edges << e;
    }

    vertex->nodes |= alter->nodes;
    foreach(PNode n, alter->nodes)
    {
        vertexOf[n] = vertex;
    }

    vertexData.remove(alter);
    vertexes.removeOne(alter);
}

// определяет какое количество движения может поглотить вершина
static point3d compensation(PEdge edge, PVertex fromVertex, point3d target, point3d delta0 = 0)
{
    // определяем ближнюю и дальнюю точки отрезка
    point3d closeEnd = edge->object->globalPoint(0);
    point3d farEnd   = edge->object->globalPoint(1);
    double  size     = (farEnd + delta0 - closeEnd).length();
    if(fromVertex == edge->finish)
        qSwap(closeEnd, farEnd);

    // заданное движение может быть скомпенсировано только в
    // параллельном направлении
    bool ok;
    target = target.project_to_direction(closeEnd - farEnd, &ok);

    // жесткая грань
    if(!ok)
        return 0;

    // не изменяем размеры никаких объектов, кроме линий
    if(!edge->object.dynamicCast<Line>())
        return 0;

    // если направление уменьшает отрезок
    // и размер отрезка недостаточен, чтобы компенсировать уменшение
    if((farEnd + delta0 - closeEnd).isCoaimed(target) && size - target.length() < lineMinimumSize)
    {
        // линия уменьшается до своего минимального значения
        return target.resized(size - lineMinimumSize);
    }

    // возможно компенсировать целиком (тангенциальную составляющую)
    return target;
}

static PVertex selectOtherVertex(PEdge edge, PVertex vertex)
{
    return edge->start == vertex ? edge->finish : edge->start;
}

//#define MOVE_LOG 1

bool MoveGraph::solve_MoveNode_simple(PNode startNode, point3d startDelta)
{
#ifdef MOVE_LOG
    QStringList log;
    log << "== Graph contents ==\n";
    log << toStringList();
#endif

    QList<PVertex> inConflict;
    QList<PVertex> toVisit;
    QSet<PVertex>  visited;

    PVertex start = vertexOf[startNode];
    Q_ASSERT(start);
    vertexData[start].initDelta = startDelta;
    vertexData[start].delta     = startDelta;

    // определяем возможное смещение элемента, с учетом
    // компенсации по данной грани
    auto getNextVertexMovement = [&](PVertex vertex, PVertex other) -> point3d {
        PEdge edge = selectEdgeTo(vertex, other);
        Q_ASSERT(edge);

        // на столько хотим сместиться
        point3d force = vertexData[vertex].delta - vertexData[other].delta;

        // столько движения может поглотить грань
        point3d resistance = compensation(edge, vertex, force);

        // значит следующий элемент смещается на вот эту величину
        return force - resistance;
    };

    // повторяем проходы по графу неопределенное количества раз до тех
    // пор, пока граф не придет в устойчивое положение
    for(int modified = 1, counter = 100; counter-- > 0 && modified;)
    {
        modified = 0;
#ifdef MOVE_LOG
        log << "next stage";
        log << "~~~~~~~~~~";
#endif

        // поиск элемента, в котором присутствует конфликт
        if(inConflict.empty())
        {
            foreach(PVertex vertex, vertexes)
            {
                foreach(PEdge edge, vertex->edges)
                {
                    PVertex other    = selectOtherVertex(edge, vertex);
                    point3d movement = getNextVertexMovement(vertex, other);
                    if(movement)
                    {
#ifdef MOVE_LOG
                        log << ("inConflict vertex: " + vertex->name);
#endif

                        // начинать надо с большей дельты
                        if(vertexData[vertex].delta.length() > vertexData[other].delta.length())
                            inConflict << vertex;
                        else
                            inConflict << other;
                    }
                }
            }
        }

        // решение найдено
        if(inConflict.empty())
            break;

        // начинаем обход графа с конфликтного элемента
        toVisit << inConflict.takeFirst();

        visited.clear();
        inConflict.clear();

#ifdef MOVE_LOG
        log << "lets go";
        log << "~~~~~~~";
#endif

        // пробегаем по всем вершинам
        while(toVisit.size())
        {
            // помечаем вершину как посещенную
            PVertex vertex = toVisit.takeFirst();
            visited << vertex;

#ifdef MOVE_LOG
            log << ("vertex " + vertex->name);
#endif

            // просматриваем соседей
            foreach(PEdge edge, vertex->edges)
            {
                // добавляем соседа в список для посещения
                PVertex otherV = selectOtherVertex(edge, vertex);

                bool isVisited = visited.contains(otherV);

                // анализируем движение соседа
                point3d movement = getNextVertexMovement(vertex, otherV);
#ifdef MOVE_LOG
                log << QString("  | %1 (%2) %3 (%4) - %5")
                           .arg(vertex->name)
                           .arg(vertexData[vertex].delta.toQString())
                           .arg(otherV->name)
                           .arg(vertexData[otherV].delta.toQString())
                           .arg(movement.toQString());
#endif

                // если нет движения то ничего не надо делать
                if(!movement)
                {
#ifdef MOVE_LOG
                    log << QString("  | %1 ok").arg(otherV->name);
#endif
                    continue;
                }
                // движение было, компенсируем
                else if(isVisited || vertexData[otherV].fixed || otherV == start)
                {
                    // движение есть но оно запрещено
                    inConflict << otherV;
                }
                else
                {
#ifdef MOVE_LOG
                    log << QString("  | %1 move by %2").arg(otherV->name).arg(movement.toQString());
#endif

                    // двигаем соседний узел
                    Q_ASSERT(otherV != start);
                    vertexData[otherV].delta += movement;

                    // отмечаем, что движение произошло
                    ++modified;

                    // анализируем дальше
                    toVisit << otherV;
                }
            }
        }
    }

#ifdef MOVE_LOG
    QFile file("d:\\moving-log.txt");
    file.open(QFile::Truncate | QFile::WriteOnly);
    file.write(log.join("\n").toLatin1());
#endif

    Q_ASSERT(vertexData[vertexOf[startNode]].delta == startDelta);

    return inConflict.empty();
}

void MoveGraph::walkMetaEdge(PMetaEdge                               me,
                             function<void(PVertex, PEdge, PVertex)> foo,
                             bool                                    lastVertex)
{
    PVertex a = me->start->subvertex;
    for(int index = 0; index < me->subedges.size(); ++index)
    {
        PEdge   e = me->subedges[index];
        PVertex b = index < me->subvertexes.size() ? me->subvertexes[index] : me->finish->subvertex;

        foo(a, e, b);
        a = b;
    }
    if(lastVertex)
        foo(a, PEdge(), PVertex());
}

namespace
{
struct simple_edge
{
    PLine   line;       // сам объект
    point3d l;          // радиус-вектор второй точки
    double  ax, ay, az; // проекции на оси
    double  k_x;        // коэффициент участия в изменении
    double  x_min;
};
} // namespace

bool MoveGraph::strechMetaEdge(PMetaEdge me, point3d rx, StretchMetaEdgeResult &result)
{
    if(!rx)
    {
        // ошибка: нулевое перещение не должно вообще сюда попадать
        Q_ASSERT(rx);
        return false;
    }

    double delta = rx.length();

    // два перпендикуляра к r
    point3d ry, rz, rxn = rx.normalized();
    if(!rx.isParallel(point3d::nz))
        ry = rxn.rotate3dAround(0, point3d::nz, M_PI_2);
    else
        ry = rxn.rotate3dAround(0, point3d::ny, M_PI_2);

    rz = rxn.rotate3dAround(0, ry, M_PI_2);
    Q_ASSERT(ry && rz);

    double             sx       = 0;
    double             sabsx    = 0;
    double             sy       = 0;
    double             sz       = 0;
    int                sx_count = 0;
    QList<simple_edge> ses;
    QStringList        debug_log;
    walkMetaEdge(me, [&](PVertex a, PEdge edge, PVertex /*b*/) {
        if(PLine line = edge->object.dynamicCast<Line>())
        {
            // определяем ближнюю и дальнюю точки отрезка
            point3d closeEnd = line->globalPoint(0);
            point3d farEnd   = line->globalPoint(1);
            if(a == edge->finish)
                qSwap(closeEnd, farEnd);

            simple_edge e;
            e.line = line;
            e.l    = farEnd - closeEnd;

            bool ok;
            e.ax = e.l.project_and_make_coordinate(rxn, &ok);
            if(fabs(e.ax) < 1e-9)
                e.ax = 0;

            // ограничение на минимальный размер грани
            e.x_min = lineMinimumSize / e.l.length();

            // количество актиынйх граней
            if(fabs(e.ax) > POINT_PREC)
                ++sx_count;

            e.ay = e.l.project_and_make_coordinate(ry, &ok);
            e.az = e.l.project_and_make_coordinate(rz, &ok);

            if(fabs(e.ay) < 1e-9)
                e.ay = 0;
            if(fabs(e.az) < 1e-9)
                e.az = 0;

            // если e.ax>0, то грань сжимается
            e.k_x = e.ax / e.l.length();
            sabsx += fabs(e.ax * e.k_x);
            sx += e.ax;
            sy += e.ay;
            sz += e.az;

            debug_log << QString("%1 (%2)").arg(e.l.length(), 0, 'f', 3).arg(e.ax, 0, 'f', 3);

            ses << e;
        }
    });

#ifndef __DO_NOT_LOG_MATRIX_RESULT
    qDebug() << "edge: " << debug_log.join(",");
#endif

    // грань не может изменять размер в этом направлении
    if(sabsx < POINT_PREC)
    {
#ifndef __DO_NOT_LOG_MATRIX_RESULT
        qDebug() << "sabsx == 0";
#endif
        return false;
    }

    using Eigen::ArrayXd;
    using Eigen::MatrixXd;
    using Eigen::VectorXd;

    int count        = ses.size();
    int lambda_count = 3;

    MatrixXd A(count + lambda_count, count + lambda_count);
    VectorXd B(count + lambda_count);
    A.setZero();
    B.setZero();

    // заполняем матрицу
    int la_index = count;

    // в среднем должен быть такой коэффициент сжатия
    //    double best_x =  1 - delta / sabsx / sx_count;

    // значение, которое нужно получить
    double A1       = sx - delta;
    B(la_index + 0) = A1;
    B(la_index + 1) = sy;
    B(la_index + 2) = sz;

    // заполняем матрицы
    for(int index = 0; index < count; ++index)
    {
        // матричная магия
        A(index, index) = 1;

        A(index, la_index + 0) = ses[index].ax / 2;
        A(index, la_index + 1) = ses[index].ay / 2;
        A(index, la_index + 2) = ses[index].az / 2;

        A(la_index + 0, index) = ses[index].ax;
        A(la_index + 1, index) = ses[index].ay;
        A(la_index + 2, index) = ses[index].az;

        // на сколько нужно измениться
        double local_delta = delta * ses[index].k_x / sabsx;
        double best_x      = 1 - local_delta;
        B(index)           = best_x;
    }

    // если ось z не задействована, удаляем соответствующие строки
    if((A.row(la_index + 2).array() == 0).all() && (A.col(la_index + 2).array() == 0).all())
    {
        A.conservativeResize(la_index + 2, la_index + 2);
        B.conservativeResize(la_index + 2);
    }

    VectorXd  X; // решение уравнения
    QSet<int> activated_indexes;
    bool      failure = false;

    while(1)
    {
        // X = A.colPivHouseholderQr().solve(B);
        X = A.completeOrthogonalDecomposition().solve(B);
        // проверка решения (нельзя решить)
        double p = (A * X - B).norm() / B.norm();
        if(p > 1)
            return false;

#ifndef __DO_NOT_LOG_MATRIX_RESULT
        ////////////////////////////////////////////////
        {
            int      w = A.cols();
            int      h = A.rows();
            MatrixXd T(h + 2, w + 1);
            T.setZero();
            T.block(0, 0, h, w) = A;
            T.block(0, w, h, 1) = B;
            T.block(h, 0, 1, w) = X.transpose();
            for(int index = 0; index < count; ++index)
                T(h + 1, index) = ses[index].x_min;

            Eigen::IOFormat   fmt(4, 0, ", ", "\n", "[", "]");
            std::stringstream s;
            s << MatrixXd(T).format(fmt) << std::endl;
            qDebug() << count;
            QString ss = QString::fromStdString(s.str());
            foreach(QString a, ss.split("\n"))
                qDebug() << a;
            qDebug() << "matrix precision=" << p * 100 << "%";
        }
        ////////////////////////////////////////////////
#endif
        // поиск линий, которые стали меньше минимума
        bool ok    = true;
        int  index = 0;
        for(; index < count; ++index)
        {
            if(X(index) < ses[index].x_min)
            {
                ok = false;
                break;
            }
        }

        // решение найдено
        if(ok)
            break;

        // ничего не выйдет
        if(activated_indexes.contains(index))
        {
#ifndef __DO_NOT_LOG_MATRIX_RESULT
            qDebug() << "cannot activate mu-koeff #" << index;
#endif
            failure = true;
            break;
            // return false;
        }
        else
            activated_indexes << index;

            // активируем очередной мю-коэффициент
#ifndef __DO_NOT_LOG_MATRIX_RESULT
        qDebug() << "activate mu-koeff #" << index;
#endif
        int w = A.rows();
        A.conservativeResize(w + 1, w + 1);
        A.row(w).setZero();
        A.col(w).setZero();
        A(w, index) = 1;
        A(index, w) = 1;

        B.conservativeResize(w + 1);
        B(w) = ses[index].x_min;

        // повторяем попытку вычисления
    }

    if(failure) // не удалось удержать коэффиенеты больше минимума
        return false;

    // определяем новые размеры элементов
    point3d current_delta = rx;

    int index = 0;
    // теперь претворяем в жизнь изменения
    walkMetaEdge(me, [&](PVertex a, PEdge edge, PVertex b) {
        // первая вершина сдвигается насколько положено
        if(index == 0)
            result.vertexData[a] = VertexData{current_delta};

        if(PLine line = edge->object.dynamicCast<Line>())
        {
            // новый размер линии
            double  x  = X(index);
            point3d l0 = ses[index].l;
            point3d l1 = l0 * x;
            point3d d  = l1 - l0;
            if(l1.length() < lineMinimumSize - POINT_PREC)
            {
                // нельзя так уменьшаться
                failure = true;
                return;
            }
            else if(l1.length() < lineMinimumSize)
            {
                // почти уложились в ограничение
                l1 = l1.resized(lineMinimumSize);
                d  = l1 - l0;
            }

            current_delta += d;
            ++index;
        }

        result.vertexData[b] = VertexData{current_delta};
    });

    if(failure)
        return false;

    if(!current_delta.empty())
    {
        // корректируем изменение
        point3d delta = 0;
        walkMetaEdge(me, [&](PVertex a, PEdge edge, PVertex b) {
            if(PLine line = edge->object.dynamicCast<Line>())
            {
                point3d d0 = result.vertexData[b].delta - result.vertexData[a].delta;
                point3d d  = compensation(edge, a, current_delta, d0);
                current_delta -= d;
                delta -= d;
            }
            result.vertexData[b].delta += delta;
        });
    }

    result.length = sabsx;
    result.k      = delta / sabsx;

    if(!current_delta.empty())
        return false;

    return true;
}

void MoveGraph::selectMasterAndSlaves(QSet<PMetaVertex> &slaveVs,
                                      QSet<PMetaVertex> &citizenVs,
                                      QSet<PMetaVertex> &masterVs)
{
    foreach(PMetaVertex mv, metaVertexes)
        if(metaVertexData[mv].hasActualDelta)
            masterVs << mv;
        else
            citizenVs << mv;

    // выявляем всех слуг
    while(1)
    {
        int newby = 0;
        foreach(PMetaVertex mv, citizenVs)
        {
            // сколько соседей не связано
            int non_slave_neighbours = mv->edges.size();
            foreach(PMetaEdge me, mv->edges)
                if(slaveVs.contains(me->otherVertex(mv)))
                    --non_slave_neighbours;

            if(non_slave_neighbours <= 1)
            {
                citizenVs.remove(mv);
                slaveVs << mv;
                ++newby;
            }
        }

        // если хотя бы один был выявлен, делаем ещё один проходs
        if(!newby)
            break;
    }

    QSet<PMetaVertex> pureCitizens;
    while(!citizenVs.isEmpty() && citizenVs.size() != pureCitizens.size())
    {
        QSet<PMetaVertex>  cluster;
        QList<PMetaVertex> toVisit;

        auto next = (citizenVs - pureCitizens).toList();
        toVisit << next.takeFirst();

        QSet<PMetaVertex> masters;
        while(!toVisit.isEmpty())
        {
            PMetaVertex mv = toVisit.takeFirst();
            if(cluster.contains(mv))
                continue;
            cluster << mv;

            foreach(PMetaEdge e, mv->edges)
            {
                PMetaVertex other = e->otherVertex(mv);
                if(pureCitizens.contains(other) || masterVs.contains(other))
                {
                    masters << other;
                    continue;
                }
                if(citizenVs.contains(other) && !cluster.contains(other))
                {
                    toVisit << other;
                }
            }
        }

        if(masters.size() > 1)
            pureCitizens |= cluster;
        else
        {
            slaveVs |= cluster;
            citizenVs -= cluster;
        }
    }
}

bool MoveGraph::moveMasterVertexes(QSet<PMetaVertex> &masterVs, QSet<PMetaEdge> &drawnEdges)
{
    QSet<PMetaEdge> mes;
    foreach(PMetaVertex mv, masterVs)
    {
        foreach(PMetaEdge me, mv->edges)
            if(masterVs.contains(me->otherVertex(mv)))
                mes << me;
    }

    foreach(PMetaEdge me, mes)
    {
        PMetaVertex start = me->start;
        PMetaVertex end   = me->finish;

        point3d startDelta = metaVertexData[start].actualDelta;
        point3d endDelta   = metaVertexData[end].actualDelta;

        point3d move    = endDelta;
        point3d stretch = startDelta - move;

        StretchMetaEdgeResult result;
        if(stretch)
        {
            bool ok = strechMetaEdge(me, stretch, result);
            if(!ok)
                return false;
        }

#ifndef __DO_NOT_STRETCHMOVE_LOG
        qDebug() << me->name;
#endif

        foreach(PVertex v, result.vertexData.keys())
        {
            point3d d           = result.vertexData[v].delta + move;
            vertexData[v].delta = d;

#ifndef __DO_NOT_STRETCHMOVE_LOG
            qDebug() << v->name << " -> " << d.toQString();
#endif
        }

        drawnEdges << me;
    }

    return true;
}

bool MoveGraph::moveSlaveVertexes(QSet<PMetaEdge> &drawnEdges)
{
    // собираем все метаграни
    QSet<PMetaEdge> edges;
    foreach(PMetaVertex mv, metaVertexes)
        edges |= mv->edges.toSet();
    edges -= drawnEdges;

    bool has_progress       = true;
    int  hangout_count_down = 50;
    while(edges.size() && hangout_count_down-- > 0 && has_progress)
    {
        has_progress = false;
        foreach(PMetaEdge me, edges)
        {
            bool is_s = metaVertexData[me->start].hasActualDelta;
            bool is_f = metaVertexData[me->finish].hasActualDelta;

            // пока ничего нельзя сделать
            if(!is_s && !is_f)
                continue;

            has_progress = true;

            PMetaVertex start  = me->start;
            PMetaVertex finish = me->finish;
            if(is_f)
                qSwap(start, finish);

            point3d d = metaVertexData[start].actualDelta;
            if(!is_f || !is_s)
                metaVertexData[finish].setActualDelta(d);

#ifndef __DO_NOT_STRETCHMOVE_LOG
            qDebug() << me->name;
#endif
            walkMetaEdge(
                me,
                [&](PVertex a, PEdge, PVertex b) {
                    vertexData[a].delta = d;

#ifndef __DO_NOT_STRETCHMOVE_LOG
                    qDebug() << a->name << " -> " << d.toQString();
#endif
                },
                true);

            edges.remove(me);
            drawnEdges << me;
        }
    }

    if(hangout_count_down < 0) // зациклились
    {
        qDebug() << "cannot move slave";
        return false;
    }
    return true;
}

namespace
{
struct StretchMetaEdgeResultEx : MoveGraph::StretchMetaEdgeResult
{
    point3d stretch;
    point3d move;
    bool    failed = 0;
    bool    zero   = 0;

    StretchMetaEdgeResultEx(point3d s = 0, point3d m = 0)
        : stretch(s)
        , move(m)
    {
    }
};

struct LocalVertexData
{
    point3d                                  current_delta;
    QList<PMetaEdge>                         edges;
    QMap<PMetaEdge, StretchMetaEdgeResultEx> edgeResults;
};

using Coords = QList<double>;

struct Context
{
    Coords focus(Coords *cs, int count)
    {
        Coords r;
        for(int index = 0; index < cs[0].size(); ++index)
        {
            double s = 0.0;
            for(int z = 0; z < count; ++z)
                s += cs[z][index];
            r << s / count;
        }

        return r;
    }

    Coords interpolate(double gamma, Coords const &a, Coords const &b)
    {
        Coords r;
        for(int index = 0; index < a.size(); ++index)
            r << (1 - gamma) * a[index] + gamma * b[index];

        return r;
    }

    double crude_distance(Coords const &a, Coords const &b)
    {
        double d = 0;
        for(int index = 0; index < a.size(); ++index)
        {
            double k = a[index] - b[index];
            d += k * k;
        }
        return sqrt(d);
    }
};

double frand(double min, double max)
{
    return (max - min) * ((double)rand() / (double)RAND_MAX) + min;
}

} // namespace

bool MoveGraph::moveCitizenVertexes(QSet<PMetaVertex> citizenVs,
                                    QSet<PMetaVertex> slaveVs,
                                    QSet<PMetaEdge> & drawnEdges)
{
    //    QElapsedTimer timer; timer.restart();

    // выбираем грани, которые необходимо двигать
    QList<PMetaVertex> localMasters;                  // список мастеров
    QList<PMetaVertex> citizens = citizenVs.toList(); // список граждан
    QSet<PMetaEdge>    citizenMes;                    // грани
    QMap<PMetaEdge, StretchMetaEdgeResultEx> edgeResults;
    QMap<PMetaVertex, LocalVertexData>       vlocal; // инфа по вершинам
    foreach(PMetaVertex mv, citizens)
    {
        foreach(PMetaEdge me, mv->edges)
        {
            PMetaVertex other = me->otherVertex(mv);
            if(slaveVs.contains(other))
                continue; // эту грань писать не нужно

            if(citizenMes.contains(me))
                continue; // эта грань уже записана

            // регистрируем грань
            if(!citizenVs.contains(other))
            {
                // второй элемент -- хозяин
                localMasters << other;
                vlocal[other].current_delta = metaVertexData[other].actualDelta;
            }
            else
            {
                vlocal[other].edges << me;
            }
            vlocal[mv].edges << me;
            citizenMes << me;
        }
    }

    bool         active    = false;
    const double INCORRECT = 1e3;
    // метод расчёта
    std::function<double(Coords)> sigma = [&](Coords k) -> double {
        foreach(PMetaVertex c, citizenVs)
        {
            int     index = 0;
            point3d d     = 0;
            foreach(PMetaVertex m, localMasters)
                d += metaVertexData[m].actualDelta * k[index++];
            vlocal[c].current_delta = d;
        }

        // метавершины, которые уже нельзя корректировать
        QSet<PMetaVertex> fixed = localMasters.toSet();
    restart:
        while(1)
        {
            double f = 0;
            foreach(PMetaEdge me, citizenMes)
            {
                PMetaVertex start = me->start;
                PMetaVertex end   = me->finish;

                point3d startDelta = vlocal[start].current_delta;
                point3d endDelta   = vlocal[end].current_delta;

                point3d move    = endDelta;
                point3d stretch = startDelta - move;

                StretchMetaEdgeResultEx result{stretch, move};
                if(!stretch)
                {
                    // нужно записать результаты, даже если перемещения не было
                    walkMetaEdge(
                        me,
                        [&](PVertex a, PEdge, PVertex) { result.vertexData[a].delta = 0; },
                        true);
                    edgeResults[me] = result;
                }
                else
                {
                    bool ok = strechMetaEdge(me, stretch, result);
                    if(ok)
                    {
                        if(result.length)
                        {
                            double n = result.k / result.length;
                            f += (n - 1) * (n - 1);
                        }
                        edgeResults[me] = result;
                    }
                    else
                    {
                        // если нельзя двигать, фиксируем одну из вершин, относительно второй
                        bool isS = fixed.contains(start);
                        bool isE = fixed.contains(end);
                        if(isS && isE)
                        {
                            return INCORRECT;
                        }
                        else
                        {
                            if(isS) // приравниваем вторую к первой
                                vlocal[end].current_delta = vlocal[start].current_delta;
                            else
                                vlocal[start].current_delta = vlocal[end].current_delta;
                            fixed << start;
                            fixed << end;

                            // пробуем ещё раз
                            goto restart;
                        }
                    }
                }
            }

            if(active)
            {
                foreach(PMetaEdge me, edgeResults.keys())
                {
#ifndef __DO_NOT_STRETCHMOVE_LOG
                    qDebug() << me->name;
#endif

                    StretchMetaEdgeResultEx result = edgeResults[me];
                    foreach(PVertex v, result.vertexData.keys())
                    {
                        point3d d           = result.vertexData[v].delta + result.move;
                        vertexData[v].delta = d;

#ifndef __DO_NOT_STRETCHMOVE_LOG
                        qDebug() << v->name << " -> " << d.toQString();
#endif
                    }
                    drawnEdges << me;
                }

                foreach(PMetaVertex c, citizenVs)
                {
                    point3d d = vlocal[c].current_delta;
                    metaVertexData[c].setActualDelta(d);
                }
            }

            return f;
        }
    };
    Context c;
    auto    calculator = MakeSimplexCalculator<Coords>(c, sigma);

    for(int count = 20; count-- > 0;)
    {
        Coords x;
        for(int index = 0; index < localMasters.size(); ++index)
        {
            x << frand(-1, +1);
        }
        sigma(x);
        calculator.addMeshPoint(x);
    }

    double f;
    Coords result;
    calculator.process(f, result);

    //    qDebug() << "sigma=" << f;
    //    qDebug() << "timer: " << timer.elapsed();

    if(f >= INCORRECT)
        return false;

    active = true;
    sigma(result);
    return true;
}

bool MoveGraph::solve_MoveNode_advanced()
{
    // подготовка к движению
    constructMetaGraph();

    QSet<PMetaVertex> slaveVs, // повторяющие соседей
        masterVs,              // определяющие
        citizenVs;             // промежуточные

    // выявляем мастеров
    selectMasterAndSlaves(slaveVs, citizenVs, masterVs);

#ifndef __DO_NOT_STRETCHMOVE_LOG
    qDebug() << "masters:";
#endif

    QSet<PMetaEdge> drawnEdges;
    if(!moveMasterVertexes(masterVs, drawnEdges))
        return false;

#ifndef __DO_NOT_STRETCHMOVE_LOG
    qDebug() << "citizens:";
#endif

    // выбираем все грани граждан, кроме граней связанных с рабами
    if(citizenVs.size())
        if(!moveCitizenVertexes(citizenVs, slaveVs, drawnEdges))
            return false;

#ifndef __DO_NOT_STRETCHMOVE_LOG
    qDebug() << "slaves:";
#endif

    // распроcтраняем на рабов
    if(!moveSlaveVertexes(drawnEdges))
        return false;

    return true;
}

void MetaEdge::makeName()
{
    double l = 0;
    foreach(PEdge e, subedges)
        l += e->object->globalPoint(0).distance(e->object->globalPoint(1));
    name = QString("%1-%2 %3").arg(start->name).arg(finish->name).arg(l, 0, 'f', 1);
}

} // namespace geometry
