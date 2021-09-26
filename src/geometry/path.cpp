#include "path.h"

#include "sceneProcedures.h"

#include <QList>
#include <QMultiMap>
#include <QSet>

namespace geometry
{

typedef QSet<PNode>  Set;
typedef QList<PNode> Route;
typedef QList<Route> Routes;

bool Path::alwaysTrue(NodePathInfo const &)
{
    return false;
}

bool Path::alwaysFalse(NodePathInfo const &)
{
    return false;
}

struct Path::impl
{
    Scene *scene;

    PNode origin;

    QSet<PNode>         visited;
    QList<QList<PNode>> routes;

    Routes makePathSimple(PNode          node,
                          Route &        route,
                          Set &          set,
                          Path::PathPrep prepAccept,
                          Path::PathPrep prepFound,
                          bool           keepDeadEnds);
};

Path::Path(Scene *scene)
    : pimpl(new impl)
{
    pimpl->scene = scene;
}

Path::~Path()
{
}

Routes Path::routes() const
{
    return pimpl->routes;
}

/* Из начальной ноды строит все возможные пути объхода
 * Для каждого пути вызываются предикаты
 * prepAccept - true - поиск в данном направлении продолжается
 *              false - данное направление отменяется
 * prepFound - true - поиск в данном направлении успешно завершается
 *             false продолжается поиск
 *
 * Возвращается объект Path содержащий список путей
 */

Path Path::makePathSimple(Scene *        scene,
                          PNode          node,
                          Path::PathPrep prepAccept,
                          Path::PathPrep prepFound,
                          bool           keepDeadEnds)
{
    Path p          = Path(scene);
    p.pimpl->origin = node;

    Set   visited;
    Route basic_route;
    p.pimpl->routes << p.pimpl->makePathSimple(
        node, basic_route, visited, prepAccept, prepFound, keepDeadEnds);
    return p;
}

/* Обходит контур вызывая для каждого нода функцию prepAccept
 * если функция возвращает false то происходит немедленное прекращение
 * поиска и возврат из процедуры.
 *
 * Лучше всего использовать для полного обхода контура
 */

void Path::walkContour(Scene *scene, PNode node, PathPrep prepAccept)
{
    Route        stub;
    QSet<PNode>  visited;
    QList<PNode> toVisit;
    while(1)
    {
        // собираем соседей
        QList<PNode> neighbours;

        // анализ в том же объекте
        foreach(PNode bro, node->object()->nodes())
            if(node != bro)
                neighbours << bro;

        // переходим на соседний объект
        PNode bro = connectedNode(scene, node);
        if(bro)
            neighbours << bro;

        // соседей, у которых были и у которых не были
        QList<PNode> doneNeighbours = (neighbours.toSet() & visited).toList();
        neighbours                  = (neighbours.toSet() - visited).toList();

        // проводим поиск и выбор нодов, подходящих по предикату
        NodePathInfo info{stub, node, doneNeighbours, neighbours};
        if(!prepAccept(info))
            return;

        visited << node;

        if(neighbours.size() >= 1)
        {
            // если один вариант пути, то просто продолжаем здесь
            node = neighbours.takeFirst();
            toVisit << neighbours;

            continue;
        }
        else
        {
            // не найдено допустимых путей
            toVisit = (toVisit.toSet() - visited).toList();
            if(toVisit.empty())
                return;

            node = toVisit.takeFirst();
            continue;
        }
    }
}

Routes Path::impl::makePathSimple(PNode          node,
                                  Route &        route,
                                  Set &          visited,
                                  Path::PathPrep prepAccept,
                                  Path::PathPrep prepFound,
                                  bool           keepDeadEnds)
{
    while(1)
    {
        // собираем соседей
        QList<PNode> neighbours;

        // анализ в том же объекте
        foreach(PNode bro, node->object()->nodes())
            if(node != bro && bro->jointType() == 0)
                neighbours << bro;

        // переходим на соседний объект
        PNode bro = connectedNode(scene, node);
        if(bro)
            neighbours << bro;

        // соседей, у которых были и не были
        QList<PNode> doneNeighbours = (neighbours.toSet() & visited).toList();
        neighbours                  = (neighbours.toSet() - visited).toList();

        // проводим поиск и выбор нодов, подходящих по предикату
        NodePathInfo info{route, node, doneNeighbours, neighbours};
        if(!prepAccept(info))
            return Routes();

        bool found = prepFound(info);
        route << node;
        visited << node;

        if(found)
        {
            Routes result;
            result << route;
            return result;
        }

        if(neighbours.size() == 1)
        {
            // если один вариант пути, то просто продолжаем здесь
            node = neighbours.takeFirst();
            continue;
        }
        else if(neighbours.empty())
        {
            // не найдено допустимых путей
            return Routes();
        }
        else
        {
            // найдено несколько путей
            Routes result;
            foreach(PNode node, neighbours)
            {
                if(!visited.contains(node))
                {
                    Route route_copy = route;
                    result << makePathSimple(
                        node, route_copy, visited, prepAccept, prepFound, keepDeadEnds);
                }
            }
            return result;
        }
    }
}

} // namespace geometry
