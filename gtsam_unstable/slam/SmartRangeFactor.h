/**
 * @file SmartRangeFactor.h
 *
 * @brief A smart factor for range-only SLAM that does initialization and marginalization
 * 
 * @date Sep 10, 2012
 * @author Alex Cunningham
 */

#pragma once

#include <gtsam_unstable/base/dllexport.h>
#include <gtsam/nonlinear/NonlinearFactor.h>
#include <gtsam/inference/Key.h>
#include <gtsam/geometry/Pose2.h>
#include <gtsam/base/timing.h>
#include <boost/foreach.hpp>
#include <map>

namespace gtsam {

/**
 * Smart factor for range SLAM
 * @addtogroup SLAM
 */
class SmartRangeFactor: public NoiseModelFactor {
protected:

  struct Circle2 {
    Circle2(const Point2& p, double r) :
        center(p), radius(r) {
    }
    Point2 center;
    double radius;
  };

  typedef SmartRangeFactor This;

  std::vector<double> measurements_; ///< Range measurements
  double variance_; ///< variance on noise

public:

  /** Default constructor: don't use directly */
  SmartRangeFactor() {
  }

  /**
   * Constructor
   * @param s standard deviation of range measurement noise
   */
  SmartRangeFactor(double s) :
      NoiseModelFactor(noiseModel::Isotropic::Sigma(1, s)), variance_(s * s) {
  }

  virtual ~SmartRangeFactor() {
  }

  /// Add a range measurement to a pose with given key.
  void addRange(Key key, double measuredRange) {
    keys_.push_back(key);
    measurements_.push_back(measuredRange);
    size_t n = keys_.size();
    // Since we add the errors, the noise variance adds
    noiseModel_ = noiseModel::Isotropic::Variance(1, n * variance_);
  }

  // Testable

  /** print */
  virtual void print(const std::string& s = "",
      const KeyFormatter& keyFormatter = DefaultKeyFormatter) const {
    std::cout << s << "SmartRangeFactor with " << size() << " measurements\n";
    NoiseModelFactor::print(s);
  }

  /** Check if two factors are equal */
  virtual bool equals(const NonlinearFactor& f, double tol = 1e-9) const {
    return false;
  }

  // factor interface

  /**
   * Triangulate a point from at least three pose-range pairs
   * Checks for best pair that includes first point
   */
  Point2 triangulate(const Values& x) const {
    //gttic_(triangulate);
    // create n circles corresponding to measured range around each pose
    std::list<Circle2> circles;
    size_t n = size();
    for (size_t j = 0; j < n; j++) {
      const Pose2& pose = x.at<Pose2>(keys_[j]);
      circles.push_back(Circle2(pose.translation(), measurements_[j]));
    }

    Circle2 circle1 = circles.front();
    boost::optional<Point2> best_fh;
    boost::optional<Circle2> best_circle;

    // loop over all circles
    BOOST_FOREACH(const Circle2& it, circles) {
      // distance between circle centers.
      double d = circle1.center.dist(it.center);
      if (d < 1e-9)
        continue; // skip circles that are in the same location
      // Find f and h, the intersection points in normalized circles
      boost::optional<Point2> fh = Point2::CircleCircleIntersection(
          circle1.radius / d, it.radius / d);
      // Check if this pair is better by checking h = fh->y()
      // if h is large, the intersections are well defined.
      if (fh && (!best_fh || fh->y() > best_fh->y())) {
        best_fh = fh;
        best_circle = it;
      }
    }

    // use best fh to find actual intersection points
    std::list<Point2> intersections = Point2::CircleCircleIntersection(
        circle1.center, best_circle->center, best_fh);

    // pick winner based on other measurements
    double error1 = 0, error2 = 0;
    Point2 p1 = intersections.front(), p2 = intersections.back();
    BOOST_FOREACH(const Circle2& it, circles) {
      error1 += it.center.dist(p1);
      error2 += it.center.dist(p2);
    }
    return (error1 < error2) ? p1 : p2;
    //gttoc_(triangulate);
  }

  /**
   * Error function *without* the NoiseModel, \f$ z-h(x) \f$.
   */
  virtual Vector unwhitenedError(const Values& x,
      boost::optional<std::vector<Matrix>&> H = boost::none) const {
    size_t n = size();
    if (n < 3) {
      if (H)
        // set Jacobians to zero for n<3
        for (size_t j = 0; j < n; j++)
          (*H)[j] = zeros(3, 1);
      return zero(1);
    } else {
      Vector error = zero(1);

      // triangulate to get the optimized point
      // TODO: Should we have a (better?) variant that does this in relative coordinates ?
      Point2 optimizedPoint = triangulate(x);

      // TODO: triangulation should be followed by an optimization given poses
      // now evaluate the errors between predicted and measured range
      for (size_t j = 0; j < n; j++) {
        const Pose2& pose = x.at<Pose2>(keys_[j]);
        if (H)
          // also calculate 1*3 derivative for each of the n poses
          error[0] += pose.range(optimizedPoint, (*H)[j]) - measurements_[j];
        else
          error[0] += pose.range(optimizedPoint) - measurements_[j];
      }
      return error;
    }
  }

  /// @return a deep copy of this factor
  virtual gtsam::NonlinearFactor::shared_ptr clone() const {
    return boost::static_pointer_cast<gtsam::NonlinearFactor>(
        gtsam::NonlinearFactor::shared_ptr(new This(*this)));
  }

};

} // \namespace gtsam

