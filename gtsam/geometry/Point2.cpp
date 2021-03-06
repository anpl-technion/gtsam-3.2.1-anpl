/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    Point2.cpp
 * @brief   2D Point
 * @author  Frank Dellaert
 */

#include <gtsam/geometry/Point2.h>
#include <gtsam/base/Lie-inl.h>
#include <boost/foreach.hpp>
#include <cmath>

using namespace std;

namespace gtsam {

/** Explicit instantiation of base class to export members */
INSTANTIATE_LIE(Point2);

static const Matrix oneOne = (Matrix(1, 2) <<  1.0, 1.0);

/* ************************************************************************* */
void Point2::print(const string& s) const {
  cout << s << *this << endl;
}

void Point2::print(std::ostream& os, const string& s) const {
  os << s << *this << endl;
}

/* ************************************************************************* */
bool Point2::equals(const Point2& q, double tol) const {
  return (fabs(x_ - q.x()) < tol && fabs(y_ - q.y()) < tol);
}

/* ************************************************************************* */
double Point2::norm(boost::optional<Matrix&> H) const {
  double r = sqrt(x_ * x_ + y_ * y_);
  if (H) {
    Matrix D_result_d;
    if (fabs(r) > 1e-10)
      D_result_d = (Matrix(1, 2) <<  x_ / r, y_ / r);
    else
      D_result_d = oneOne; // TODO: really infinity, why 1 here??
    *H = D_result_d;
  }
  return r;
}

/* ************************************************************************* */
static const Matrix I2 = eye(2);
double Point2::distance(const Point2& point, boost::optional<Matrix&> H1,
    boost::optional<Matrix&> H2) const {
  Point2 d = point - *this;
  if (H1 || H2) {
    Matrix H;
    double r = d.norm(H);
    if (H1) *H1 = -H;
    if (H2) *H2 =  H;
    return r;
  } else
    return d.norm();
}

/*
 * Calculate f and h, respectively the parallel and perpendicular distance of
 * the intersections of two circles along and from the line connecting the centers.
 * Both are dimensionless fractions of the distance d between the circle centers.
 * If the circles do not intersect or they are identical, returns boost::none.
 * If one solution (touching circles, as determined by tol), h will be exactly zero.
 * h is a good measure for how accurate the intersection will be, as when circles touch
 * or nearly touch, the intersection is ill-defined with noisy radius measurements.
 * @param R_d : R/d, ratio of radius of first circle to distance between centers
 * @param r_d : r/d, ratio of radius of second circle to distance between centers
 * @param tol: absolute tolerance below which we consider touching circles
 */
/* ************************************************************************* */
// Math inspired by http://paulbourke.net/geometry/circlesphere/
boost::optional<Point2> Point2::CircleCircleIntersection(double R_d, double r_d,
    double tol) {

  double R2_d2 = R_d*R_d; // Yes, RD-D2 !
  double f = 0.5 + 0.5*(R2_d2 - r_d*r_d);
  double h2 = R2_d2 - f*f; // just right triangle rule

  // h^2<0 is equivalent to (d > (R + r) || d < (R - r))
  // Hence, there are only solutions if >=0
  if (h2<-tol) return boost::none; // allow *slightly* negative
  else if (h2<tol) return Point2(f,0.0); // one solution
  else return Point2(f,sqrt(h2)); // two solutions
}

/* ************************************************************************* */
list<Point2> Point2::CircleCircleIntersection(Point2 c1, Point2 c2,
    boost::optional<Point2> fh) {

  list<Point2> solutions;
  // If fh==boost::none, there are no solutions, i.e., d > (R + r) || d < (R - r)
  if (fh) {
    // vector between circle centers
    Point2 c12 = c2-c1;

    // Determine p2, the point where the line through the circle
    // intersection points crosses the line between the circle centers.
    Point2 p2 = c1 + fh->x() * c12;

    // If h == 0, the circles are touching, so just return one point
    if (fh->y()==0.0)
      solutions.push_back(p2);
    else {
      // determine the offsets of the intersection points from p
      Point2 offset = fh->y() * Point2(-c12.y(), c12.x());

      // Determine the absolute intersection points.
      solutions.push_back(p2 + offset);
      solutions.push_back(p2 - offset);
    }
  }
  return solutions;
}

/* ************************************************************************* */
list<Point2> Point2::CircleCircleIntersection(Point2 c1, double r1, Point2 c2,
    double r2, double tol) {

  // distance between circle centers.
  double d = c1.dist(c2);

  // centers coincide, either no solution or infinite number of solutions.
  if (d<1e-9) return list<Point2>();

  // Calculate f and h given normalized radii
  double _d = 1.0/d, R_d = r1*_d, r_d=r2*_d;
  boost::optional<Point2> fh = CircleCircleIntersection(R_d,r_d);

  // Call version that takes fh
  return CircleCircleIntersection(c1, c2, fh);
}

/* ************************************************************************* */
ostream &operator<<(ostream &os, const Point2& p) {
  os << '(' << p.x() << ", " << p.y() << ')';
  return os;
}

} // namespace gtsam
