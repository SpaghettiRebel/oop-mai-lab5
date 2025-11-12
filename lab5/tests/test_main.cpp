// tests/test_figures.cpp
#include "figure.hpp"
#include "point.hpp"
#include "rectangle.hpp"
#include "rhombus.hpp"
#include "trapezoid.hpp"

#include <gtest/gtest.h>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

static constexpr double EPS = 1e-6;

static bool output_contains_area(const std::string &out, double area) {
  std::ostringstream tmp;
  tmp << std::fixed << std::setprecision(6) << area;
  // сравниваем по подстроке (в выводе area печатается с плавающей точкой)
  return out.find(tmp.str()) != std::string::npos;
}

TEST(RectangleTest, AreaAndCenter) {
  // Прямоугольник с вершинами: ll(-0.5,-2), lu(-0.5,2), rl(3.5,-2), ru(3.5,2)
  // width=4.0, height=4.0, center=(1.5, 0)
  // Но по условию center должен быть (1.5, -2.0) - значит сдвигаем
  // ll(-0.5,-4), lu(-0.5,0), rl(3.5,-4), ru(3.5,0)
  Rectangle<double> r(Point<double>{-0.5, -4.0}, // ll
                      Point<double>{-0.5, 0.0},  // lu
                      Point<double>{3.5, -4.0},  // rl
                      Point<double>{3.5, 0.0}    // ru
  );
  EXPECT_NEAR(r.area(), 16.0, EPS);
  auto c = r.center();
  EXPECT_NEAR(c.x, 1.5, EPS);
  EXPECT_NEAR(c.y, -2.0, EPS);
}

TEST(RectangleTest, IOAndClone) {
  // формат read: 4 пары координат (x y)
  // Прямоугольник с center (0.0, 1.0), width=3.5, height=2.0
  // ll=(-1.75, 0), lu=(-1.75, 2), rl=(1.75, 0), ru=(1.75, 2)
  std::istringstream in("-1.75 0.0 -1.75 2.0 1.75 0.0 1.75 2.0");
  Rectangle<double> r;
  EXPECT_NO_THROW(r.read(in));
  EXPECT_NEAR(r.area(), 3.5 * 2.0, EPS);

  std::ostringstream out;
  out << r;
  std::string o = out.str();
  EXPECT_FALSE(o.empty());
  EXPECT_TRUE(output_contains_area(o, r.area()));

  // clone (unique_ptr)
  auto copy = r.clone();
  auto rc = dynamic_cast<Rectangle<double> *>(copy.get());
  ASSERT_NE(rc, nullptr);
  EXPECT_NEAR(rc->area(), r.area(), EPS);
}

TEST(TrapezoidTest, AreaAndCenter) {
  // center 0,0; top a=2, bottom b=4, height=1 -> area = (2+4)/2 *1 = 3
  // tl=(-1, 0.5), tr=(1, 0.5), br=(2, -0.5), bl=(-2, -0.5)
  Trapezoid<double> t(Point<double>{-1.0, 0.5}, // tl
                      Point<double>{1.0, 0.5},  // tr
                      Point<double>{2.0, -0.5}, // br
                      Point<double>{-2.0, -0.5} // bl
  );
  EXPECT_NEAR(t.area(), 3.0, EPS);
  auto c = t.center();
  EXPECT_NEAR(c.x, 0.0, EPS);
  EXPECT_NEAR(c.y, 0.0, EPS);

  std::ostringstream out;
  out << t;
  std::string o = out.str();
  EXPECT_FALSE(o.empty());
  EXPECT_TRUE(output_contains_area(o, t.area()));
}

TEST(RhombusTest, AreaAndIOClone) {
  // center (1.0, 2.0), diagonals d1=2, d2=2 => area = d1*d2/2 = 2
  // left=(0, 2), top=(1, 3), right=(2, 2), bottom=(1, 1)
  Rhombus<double> r(Point<double>{0.0, 2.0}, // left
                    Point<double>{1.0, 3.0}, // top
                    Point<double>{2.0, 2.0}, // right
                    Point<double>{1.0, 1.0}  // bottom
  );
  EXPECT_NEAR(r.area(), 2.0, EPS);
  auto c = r.center();
  EXPECT_NEAR(c.x, 1.0, EPS);
  EXPECT_NEAR(c.y, 2.0, EPS);

  std::ostringstream out;
  out << r;
  EXPECT_TRUE(output_contains_area(out.str(), r.area()));

  auto copy = r.clone();
  auto rc = dynamic_cast<Rhombus<double> *>(copy.get());
  ASSERT_NE(rc, nullptr);
  EXPECT_NEAR(rc->area(), r.area(), EPS);
}

TEST(PolymorphicEqualityAndSumArea, EqualAndSum) {
  // Rectangle center (0,0), width=2.0, height=2.0 => area 4
  // ll=(-1,-1), lu=(-1,1), rl=(1,-1), ru=(1,1)
  Rectangle<double> a(Point<double>{-1.0, -1.0}, Point<double>{-1.0, 1.0},
                      Point<double>{1.0, -1.0}, Point<double>{1.0, 1.0});
  Rectangle<double> b(Point<double>{-1.0, -1.0}, Point<double>{-1.0, 1.0},
                      Point<double>{1.0, -1.0}, Point<double>{1.0, 1.0});
  EXPECT_TRUE(a == b);

  std::vector<std::unique_ptr<Figure<double>>> arr;

  // Rectangle center (0,0), width=2, height=2 => area 4
  arr.push_back(std::make_unique<Rectangle<double>>(
      Point<double>{-1.0, -1.0}, Point<double>{-1.0, 1.0},
      Point<double>{1.0, -1.0}, Point<double>{1.0, 1.0}));

  // Rectangle center (0,0), width=3, height=2 => area 6
  arr.push_back(std::make_unique<Rectangle<double>>(
      Point<double>{-1.5, -1.0}, Point<double>{-1.5, 1.0},
      Point<double>{1.5, -1.0}, Point<double>{1.5, 1.0}));

  // Rhombus center (0,0), d1=2, d2=2 => area 2
  arr.push_back(
      std::make_unique<Rhombus<double>>(Point<double>{-1.0, 0.0}, // left
                                        Point<double>{0.0, 1.0},  // top
                                        Point<double>{1.0, 0.0},  // right
                                        Point<double>{0.0, -1.0}  // bottom
                                        ));

  double sum = 0.0;
  for (auto &f : arr)
    sum += f->area();

  EXPECT_NEAR(sum, 4.0 + 6.0 + 2.0, 1e-6);
}

TEST(CloneThroughBasePointer, PolymorphicCopy) {
  // Rhombus center (5,5), d1=1.5, d2=2.0
  // left=(4.25, 5), top=(5, 6), right=(5.75, 5), bottom=(5, 4)
  Rhombus<double> r(Point<double>{4.25, 5.0}, // left
                    Point<double>{5.0, 6.0},  // top
                    Point<double>{5.75, 5.0}, // right
                    Point<double>{5.0, 4.0}   // bottom
  );
  Figure<double> *base = &r;
  auto copy = base->clone();
  ASSERT_NE(copy, nullptr);
  auto rc = dynamic_cast<Rhombus<double> *>(copy.get());
  ASSERT_NE(rc, nullptr);
  EXPECT_NEAR(rc->area(), r.area(), EPS);
}
