/**
 * @file SimWall2D.cpp
 * @author Alex Cunningham
 */

#include <boost/foreach.hpp>

#include <gtsam/geometry/Pose2.h>
#include <gtsam_unstable/geometry/SimWall2D.h>

namespace gtsam {

using namespace std;

/* ************************************************************************* */
void SimWall2D::print(const std::string& s) const {
  std::cout << "SimWall2D " << s << ":" << std::endl;
  a_.print("  a");
  b_.print("  b");
}

/* ************************************************************************* */
bool SimWall2D::equals(const SimWall2D& other, double tol) const {
  return a_.equals(other.a_, tol) && b_.equals(other.b_, tol);
}

/* ************************************************************************* */
bool SimWall2D::intersects(const SimWall2D& B, boost::optional<Point2&> pt) const {
  const bool debug = false;

  const SimWall2D& A = *this;

  // translate so A.a is at the origin, then rotate so that A.b is along x axis
  Pose2 transform(Rot2::relativeBearing(A.b() - A.a()), A.a());

  // normalized points, Aa at origin, Ab at (length, 0.0)
  double len = A.length();
  if (debug) cout << "len: " << len << endl;
  Point2 Ba = transform.transform_to(B.a()),
       Bb = transform.transform_to(B.b());
  if (debug) Ba.print("Ba");
  if (debug) Bb.print("Bb");

  // check sides of line
  if (Ba.y() * Bb.y() > 0.0 ||
     (Ba.x() * Bb.x() > 0.0 && Ba.x() < 0.0) ||
     (Ba.x() > len && Bb.x() > len)) {
    if (debug) cout << "Failed first check" << endl;
    return false;
  }

  // check conditions for exactly on the same line
  if (Ba.y() == 0.0 && Ba.x() > 0.0 && Ba.x() < len) {
    if (pt) *pt = transform.transform_from(Ba);
    if (debug) cout << "Ba on the line" << endl;
    return true;
  } else if (Bb.y() == 0.0 && Bb.x() > 0.0 && Bb.x() < len) {
    if (pt) *pt = transform.transform_from(Bb);
    if (debug) cout << "Bb on the line" << endl;
    return true;
  }

  // handle vertical case to avoid calculating slope
  if (fabs(Ba.x() - Bb.x()) < 1e-5) {
    if (debug) cout << "vertical line" << endl;
    if (Ba.x() < len && Ba.x() > 0.0) {
      if (pt) *pt = transform.transform_from(Point2(Ba.x(), 0.0));
      if (debug) cout << "  within range" << endl;
      return true;
    } else {
      if (debug) cout << "   not within range" << endl;
      return false;
    }
  }

  // find lower point by y
  Point2 low, high;
  if (Ba.y() > Bb.y()) {
    high = Ba;
    low = Bb;
  } else {
    high = Bb;
    low = Ba;
  }
  if (debug) high.print("high");
  if (debug) low.print("low");

  // find x-intercept
  double slope = (high.y() - low.y())/(high.x() - low.x());
  if (debug) cout << "slope " << slope << endl;
  double xint = (low.x() < high.x()) ? low.x() + fabs(low.y())/slope : high.x() - fabs(high.y())/slope;
  if (debug) cout << "xintercept " << xint << endl;
  if (xint > 0.0 && xint < len) {
    if (pt) *pt = transform.transform_from(Point2(xint, 0.0));
    return true;
  } else {
    if (debug) cout << "xintercept out of range" << endl;
    return false;
  }
}

/* ************************************************************************* */
Point2 SimWall2D::midpoint() const {
  Point2 vec = b_ - a_;
  return a_ + vec * 0.5 * vec.norm();
}

/* ************************************************************************* */
Point2 SimWall2D::norm() const {
  Point2 vec = b_ - a_;
  return Point2(vec.y(), -vec.x());
}

/* ************************************************************************* */
Rot2 SimWall2D::reflection(const Point2& init, const Point2& intersection) const {
  // translate to put the intersection at the origin and the wall along the x axis
  Rot2 wallAngle = Rot2::relativeBearing(b_ - a_);
  Pose2 transform(wallAngle, intersection);
  Point2 t_init = transform.transform_to(init);
  Point2 t_goal(-t_init.x(), t_init.y());
  return Rot2::relativeBearing(wallAngle.rotate(t_goal));
}

/* ***************************************************************** */
std::pair<Pose2, bool> moveWithBounce(const Pose2& cur_pose, double step_size,
    const std::vector<SimWall2D> walls, Sampler& angle_drift,
    Sampler& reflect_noise, const Rot2& bias) {

  // calculate angle to change by
  Rot2 dtheta = Rot2::fromAngle(angle_drift.sample()(0) + bias.theta());
  Pose2 test_pose = cur_pose.retract((Vector(3) << step_size, 0.0, Rot2::Logmap(dtheta)(0)));

  // create a segment to use for intersection checking
  // find the closest intersection
  SimWall2D traj(test_pose.t(), cur_pose.t());
  bool collision = false;  Point2 intersection(1e+10, 1e+10);
  SimWall2D closest_wall;
  BOOST_FOREACH(const SimWall2D& wall, walls) {
    Point2 cur_intersection;
    if (wall.intersects(traj,cur_intersection)) {
      collision = true;
      if (cur_pose.t().distance(cur_intersection) < cur_pose.t().distance(intersection)) {
        intersection = cur_intersection;
        closest_wall = wall;
      }
    }
  }

  // reflect off of wall with some noise
  Pose2 pose(test_pose);
  if (collision) {

    // make sure norm is on the robot's side
    Point2 norm = closest_wall.norm();
    norm = norm / norm.norm();

    // Simple check to make sure norm is on side closest robot
    if (cur_pose.t().distance(intersection + norm) > cur_pose.t().distance(intersection - norm))
      norm = norm.inverse();

    // using the reflection
    const double inside_bias = 0.05;
    pose = Pose2(closest_wall.reflection(cur_pose.t(), intersection), intersection + inside_bias * norm);

    // perturb the rotation for better exploration
    pose = pose.retract(reflect_noise.sample());
  }

  return make_pair(pose, collision);
}
/* ***************************************************************** */

} // \namespace gtsam
