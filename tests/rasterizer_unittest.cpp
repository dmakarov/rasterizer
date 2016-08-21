/**
   \file rasterizer_unittest.cpp

   Created by Dmitri Makarov on 16-06-01.
   Copyright © 2016 Dmitri Makarov. All rights reserved.
*/

#include "scene.h"
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

TEST(Polygon, GetVertices) {
  Frame f;
  f.vertices.emplace_back(Point{1,2});
  f.vertices.emplace_back(Point{3,4});
  f.vertices.emplace_back(Point{5,6});
  f.number = 1;
  Polygon p;
  p.keyframes.emplace_back(f);
  f.vertices[0] = Point{7,8};
  f.vertices[1] = Point{9,10};
  f.vertices[2] = Point{11,12};
  f.number = 3;
  p.keyframes.emplace_back(f);
  std::vector<Point> v;
  p.getVertices(4.5f, v);
  EXPECT_EQ(v[0].x, 7);
  EXPECT_EQ(v[0].y, 8);
  EXPECT_EQ(v[1].x, 9);
  EXPECT_EQ(v[1].y, 10);
  EXPECT_EQ(v[2].x, 11);
  EXPECT_EQ(v[2].y, 12);
  v.clear();
  p.getVertices(2.0f, v);
  EXPECT_EQ(v[0].x, 4);
  EXPECT_EQ(v[0].y, 5);
  EXPECT_EQ(v[1].x, 6);
  EXPECT_EQ(v[1].y, 7);
  EXPECT_EQ(v[2].x, 8);
  EXPECT_EQ(v[2].y, 9);
}

TEST(Scene, RenderToFile) {
  Scene s;
  std::vector<std::string> args{"1", "1", "../examples/sample1.obs", "image"};
  s.renderToFile(args);
}

TEST(Rasterizer, Rasterize) {
  std::vector<std::shared_ptr<Polygon>> polygons;
  Rasterizer r{500, 500};
  r.run(polygons, 1, false, 1, false, 1, "");
  (void) r.getPixelsAsRGB();
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
