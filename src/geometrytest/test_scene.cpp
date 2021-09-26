#include "gtest/gtest.h"

#include "geometry/manipulator.h"
#include "geometry/scene.h"

#include <QDir>
#include <QFile>
#include <QStringList>

using namespace geometry;

class scenePersistanceTests : public testing::Test
{
protected:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

TEST_F(scenePersistanceTests, persistence)
{
    QDir path = QDir::current();

    int counter = 5;
    while(!path.isRoot() && counter-- > 0)
    {
        if(!path.exists("regression"))
            path.cdUp();
        else
            break;
    }

    path.cd("regression");
    ASSERT_TRUE(path.exists());

    foreach(QFileInfo fileInfo, path.entryInfoList())
    {
        if(fileInfo.isFile() && !fileInfo.filePath().contains("."))
        {
            try
            {
                QString result = Manipulator::executeFile(fileInfo.absoluteFilePath());
                ASSERT_TRUE(result.size() == 0) << result.toStdString();
            }
            catch(...)
            {
                throw;
            }
        }
    }
}
