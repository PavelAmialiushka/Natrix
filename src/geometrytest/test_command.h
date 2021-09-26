#include "geometry/command.h"

#include "gtest/gtest.h"

using namespace geometry;

TEST(commands, simple)
{
    int     a = 0;
    Command cmd;
    cmd << [&]() { a = 1; };
    cmd >> [&]() { a = 2; };

    cmd.doit();
    EXPECT_EQ(a, 1);

    cmd.undo();
    EXPECT_EQ(a, 2);
}
