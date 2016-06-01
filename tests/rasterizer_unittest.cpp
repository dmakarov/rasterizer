//
//  rasterizer_unittest.cpp
//  rasterizer
//
//  Created by Dmitri Makarov on 16-06-01.
//  Copyright © 2016 Dmitri Makarov. All rights reserved.
//

#include "rasterizer.h"
#include "gtest/gtest.h"

TEST(RGB8, DefaultConstructor) {
  const RGB8 black;
  EXPECT_EQ(0u, black.get_red());
  EXPECT_EQ(0u, black.get_green());
  EXPECT_EQ(0u, black.get_blue());
}

TEST(RGB8, ConstructFromValue) {
  const RGB8 white{0x00ffffff};
  EXPECT_EQ(0xffu, white.get_red());
  EXPECT_EQ(0xffu, white.get_green());
  EXPECT_EQ(0xffu, white.get_blue());
}

TEST(Rasterizer, RenderToFile) {
  Rasterizer r{400, 400};
  std::vector<std::string> args{"1", "1", "/Users/dmakarov/work/rasterizer/examples/sample1.obs", "image"};
  r.render_to_file(args);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
