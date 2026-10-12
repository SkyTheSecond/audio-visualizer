#include <gtest/gtest.h>

#include <shazam/shazam.hpp>

TEST(Smoke, VersionIsNotEmpty) { EXPECT_FALSE(shazam::version().empty()); }
