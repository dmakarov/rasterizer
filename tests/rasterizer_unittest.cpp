/**
   \file rasterizer_unittest.cpp

   Created by Dmitri Makarov on 16-06-01.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
*/

#include "rasterizer.h"
#include "gtest/gtest.h"

TEST(RGB8, DefaultConstructor) {
  const RGB8 black;
  EXPECT_EQ(0.f, black.get_red());
  EXPECT_EQ(0.f, black.get_green());
  EXPECT_EQ(0.f, black.get_blue());
}

TEST(RGB8, ConstructFromValue) {
  const RGB8 white{0x00ffffff};
  EXPECT_EQ(1.f, white.get_red());
  EXPECT_EQ(1.f, white.get_green());
  EXPECT_EQ(1.f, white.get_blue());
}

TEST(Rasterizer, RenderToFile) {
  Rasterizer r{400, 400};
  std::vector<std::string> args{"1", "1", "../examples/sample1.obs", "image"};
  r.render_to_file(args);
}

TEST(Rasterizer, Rasterize) {
  Rasterizer r{500, 500};
  r.rasterize(1, false, 1, false, 1, "");
  (void) r.getPixelsAsRGB();
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
