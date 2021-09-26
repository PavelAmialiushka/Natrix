#ifndef SCENEPROCEDURES_H
#define SCENEPROCEDURES_H

#include "command.h"
#include "scene.h"

#include <QMultiMap>
#include <tuple>

namespace geometry
{
struct MoveData;

bool areObjectsConnected(PObject, PObject);
bool areObjectsTouching(Scene *, PObject, PObject);

PNode connectedNode(Scene const *, PNode);

PNode secondNode(PNode);
PNode thirdNode(PNode);
PNode forthNode(PNode);

double toDegrees(point2d d);

bool canMergeLines(PObject, PObject);

PNode          commonNode(PObject, PObject);
QList<PObject> neighbours(Scene const *scene, PObject object, bool keepEmpty = false);

void mostDistantPoints(PObject, PObject, point3d &a, point3d &b);

point3d getMarkLandingDirection(PObject object);

PMarker takeStickyMarker(Scene *scene, TextInfo const &info, QList<PMarker> markers);

Command cmdTouchObject(Scene *scene, PObject Object);

Command cmdAttachObject(Scene *scene, PObject object, PObject selectionSample = PObject());
Command cmdDetachObject(Scene *scene, PObject object, bool removeMarkers = false);

Command cmdReplaceObject(Scene *   scene,
                         PObject   first,
                         PObject   second,
                         MoveData *moveData         = 0,
                         bool      reconnectMarkers = true);
Command cmdReplaceObject1to2(Scene *   scene,
                             PObject   first,
                             PObject   second1,
                             PObject   second2,
                             MoveData *moveData         = 0,
                             bool      reconnectMarkers = true);
Command cmdReplaceObject2to1(Scene *   scene,
                             PObject   first1,
                             PObject   first2,
                             PObject   second,
                             MoveData *moveData         = 0,
                             bool      reconnectMarkers = true);
Command cmdReplaceElement(Scene *scene, PElement element, ElementInfo opt, MoveData *moveData = 0);

Command cmdPlaceLabelAndMarker(Scene *scene, int type, PObject, point3d);
Command cmdAttachMarker(Scene *scene, PMarker marker);
Command cmdDetachMarker(Scene *scene, PMarker marker);

Command cmdTryToReconnectMarker(Scene *scene, PMarker marker, QSet<PObject> objects);

Command cmdReconnectMarkers(Scene *scene, PObject orig1, PObject dest1, MoveData *moveData = 0);
Command cmdReconnectMarkers(Scene *   scene,
                            PObject   orig1,
                            PObject   orig2,
                            PObject   dest1,
                            PObject   dest2,
                            MoveData *moveData = 0);

Command cmdAttachLabel(Scene *scene, PLabel label);
Command cmdDetachLabel(Scene *scene, PLabel label, bool checkMarker = true);
Command cmdReplaceLabel(Scene *scene,
                        PLabel label1,
                        PLabel label2,
                        bool   checkMarker = true,
                        bool   dropMarker  = false);

Command cmdAttachLabelToObject(Scene * scene,
                               PLabel  label,
                               PLabel  newBy,
                               PObject object,
                               point3d point);

Command cmdModify(Scene *scene);

struct EraseData
{
    QSet<PObject> toDelete; // просто удаляемые объекты
    QSet<PObject> toAdd;    // созданы новые объекты
    QSet<PObject> toSave;   // удалены, но должны быть оставлены
    QSet<PObject> toTouch;  // объекты, которые нужно перерисовать

    QSet<PObject> toReplace; // удалены, но заменены на другой элемент
    QMultiMap<PObject, PObject> replaceMap;

    void addToReplace(PObject old, PObject newbie);
    void addToReplace(QList<PObject> olds, PObject newbie);
    void addToReplace(QSet<PObject> olds, PObject newbie);

    QSet<PLabel> labelsToDelete; // метки для удаления

    // удаляемые маркеры
    QList<PMarker> toEraseMarkers;     // вместе с объектами
    QList<PMarker> toReconnectMarkers; // вместо элемента вставлена линия
    QMap<PMarker, PMarker> markerReplaceMap; // карта замены маркеров
};

void    eraseObjectsToSelect(Scene *scene, EraseData &, QSet<PObjectToSelect>);
Command cmdEraseObjectsToSelect(Scene *scene, QSet<PObjectToSelect>);

Command cmdMatchStyles(Scene *                   scene,
                       QSet<PObjectToSelect>     selObjects,
                       QList<ScenePropertyValue> values);

std::tuple<point3d, point3d> breakOrShortLineBy(point3d p1,
                                                point3d p2,
                                                point3d start,
                                                double  size,
                                                bool    acceptSinglePart,
                                                bool *  ok = 0);

std::tuple<point3d, point3d> breakLineBy(point3d p1,
                                         point3d p2,
                                         point3d start,
                                         double  size,
                                         bool *  ok = 0);

void pushTextGrips(Scene *scene, QList<PMarker> list);

} // namespace geometry

#endif // SCENEPROCEDURES_H
