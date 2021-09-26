#include "gtest/gtest.h"

#include "geometry/manipulator.h"
#include "geometry/scene.h"

#include <QDir>
#include <QFile>
#include <QStringList>

using namespace geometry;

class sceneManipulatorTests : public testing::Test
{
protected:
    QSharedPointer<Scene> scene;
    Manipulator *         man;

    virtual void SetUp() override
    {
        // орто
        scene.reset(new Scene(3));
        man = scene->manipulator();
    }
    virtual void TearDown() override
    {
    }
};

int count_(QStringList list, QString templ)
{
    int count = 0;
    foreach(QString s, list)
        if(s.contains(templ, Qt::CaseInsensitive))
            ++count;
    return count;
}

// TEST_F(sceneManipulatorTests, lines)
//{
//    QString cmds =
//            "line 0,0 100,0 100,200 0,200 ctrl 0,0 .";
//    man->executeCommand(cmds);
//    QStringList dmp = scene->dumpContents();
//    ASSERT_EQ(count_(dmp, "Line"), 4);
//    ASSERT_EQ(count_(dmp, "BendJoiner"), 4);
//    ASSERT_EQ(dmp.size(), 8);
//}
