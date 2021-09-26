#include "weldProcedures.h"

#include "joiners.h"
#include "line.h"
#include "object.h"
#include "scene.h"

#include "sceneProcedures.h"

namespace geometry
{

QMultiMap<PObject, WeldInfo> object2welds(Scene *scene, QList<PObject> objects)
{
    QMultiMap<PObject, WeldInfo> weldList;
    QSet<PObject>                done;
    while(objects.size())
    {
        PObject obj = objects.takeFirst();
        if(done.contains(obj))
            continue;
        else
            done.insert(obj);

        if(auto line = obj.dynamicCast<Line>())
        {
            foreach(auto x, neighbours(scene, line))
                if(x.dynamicCast<Joiner>())
                    objects << x;
        }
        else if(auto weld = obj.dynamicCast<WeldJoiner>())
        {
            WeldInfo info{weld, 0, weld->globalPoint(0)};
            info.localPoint = info.globalPoint >> *scene->worldSystem();
            weldList.insert(weld, info);
        }
        else if(auto bend = obj.dynamicCast<BendJoiner>())
        {
            // размер любого отвода можно менять
            for(int index = 2; index-- > 0;)
            {
                WeldInfo info{bend, index, bend->weldPoint(index)};
                info.localPoint = info.globalPoint >> *scene->worldSystem();
                weldList.insert(bend, info);
            }
        }
        else if(auto tee = obj.dynamicCast<TeeJoiner>())
        {
            for(int index = 3; index-- > 0;)
            {
                WeldInfo info{tee, index, tee->weldPoint(index)};
                info.localPoint = info.globalPoint >> *scene->worldSystem();
                weldList.insert(bend, info);
            }
        }
    }
    return weldList;
}

} // namespace geometry
