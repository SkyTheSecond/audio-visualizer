#include <gtest/gtest.h>

#include <audioviz/audioviz.hpp>

TEST(Smoke, VersionIsNotEmpty) { EXPECT_FALSE(audioviz::version().empty()); }
