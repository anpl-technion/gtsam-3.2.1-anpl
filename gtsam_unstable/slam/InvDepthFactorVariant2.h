
/**
 * @file InvDepthFactorVariant2.h
 * @brief Inverse Depth Factor based on Civera09tro, Montiel06rss.
 * Landmarks are parameterized as (theta,phi,rho) with the reference point
 * created at landmark construction and then never updated (i.e. the point
 * [x,y,z] is treated as fixed and not part of the optimization). The factor
 * involves a single pose and a landmark.
 * @author Chris Beall, Stephen Williams
 */

#pragma once

#include <gtsam/nonlinear/NonlinearFactor.h>
#include <gtsam/geometry/PinholeCamera.h>
#include <gtsam/geometry/Cal3_S2.h>
#include <gtsam/geometry/Pose3.h>
#include <gtsam/geometry/Point2.h>
#include <gtsam/base/LieVector.h>
#include <gtsam/base/numericalDerivative.h>

namespace gtsam {

/**
 * Binary factor representing a visual measurement using an inverse-depth parameterization
 */
class InvDepthFactorVariant2: public NoiseModelFactor2<Pose3, LieVector> {
protected:

  // Keep a copy of measurement and calibration for I/O
  Point2 measured_;        ///< 2D measurement
  Cal3_S2::shared_ptr K_;  ///< shared pointer to calibration object
  Point3 referencePoint_;  ///< the reference point/origin for this landmark

public:

  /// shorthand for base class type
  typedef NoiseModelFactor2<Pose3, LieVector> Base;

  /// shorthand for this class
  typedef InvDepthFactorVariant2 This;

  /// shorthand for a smart pointer to a factor
  typedef boost::shared_ptr<This> shared_ptr;

  /// Default constructor
  InvDepthFactorVariant2() : K_(new Cal3_S2(444, 555, 666, 777, 888)) {}

  /**
   * Constructor
   * @param poseKey is the index of the camera pose
   * @param pointKey is the index of the landmark
   * @param measured is the 2 dimensional location of point in image (the measurement)
   * @param K shared pointer to the constant calibration
   * @param model is the standard deviation
   */
  InvDepthFactorVariant2(const Key poseKey, const Key landmarkKey,
      const Point2& measured, const Cal3_S2::shared_ptr& K, const Point3 referencePoint,
      const SharedNoiseModel& model) :
        Base(model, poseKey, landmarkKey), measured_(measured), K_(K), referencePoint_(referencePoint) {}

  /** Virtual destructor */
  virtual ~InvDepthFactorVariant2() {}

  /**
   * print
   * @param s optional string naming the factor
   * @param keyFormatter optional formatter useful for printing Symbols
   */
  void print(const std::string& s = "InvDepthFactorVariant2",
      const gtsam::KeyFormatter& keyFormatter = gtsam::DefaultKeyFormatter) const {
    Base::print(s, keyFormatter);
    measured_.print(s + ".z");
  }

  /// equals
  virtual bool equals(const gtsam::NonlinearFactor& p, double tol = 1e-9) const {
    const This *e = dynamic_cast<const This*>(&p);
    return e
        && Base::equals(p, tol)
        && this->measured_.equals(e->measured_, tol)
        && this->K_->equals(*e->K_, tol)
        && this->referencePoint_.equals(e->referencePoint_, tol);
  }

  Vector inverseDepthError(const Pose3& pose, const LieVector& landmark) const {
    try {
      // Calculate the 3D coordinates of the landmark in the world frame
      double theta = landmark(0), phi = landmark(1), rho = landmark(2);
      Point3 world_P_landmark = referencePoint_ + Point3(cos(theta)*cos(phi)/rho, sin(theta)*cos(phi)/rho, sin(phi)/rho);
      // Project landmark into Pose2
      PinholeCamera<Cal3_S2> camera(pose, *K_);
      gtsam::Point2 reprojectionError(camera.project(world_P_landmark) - measured_);
      return reprojectionError.vector();
    } catch( CheiralityException& e) {
      std::cout << e.what()
          << ": Inverse Depth Landmark [" << DefaultKeyFormatter(this->key2()) << "]"
          << " moved behind camera [" << DefaultKeyFormatter(this->key1()) <<"]"
          << std::endl;
      return gtsam::ones(2) * 2.0 * K_->fx();
    }
    return (gtsam::Vector(1) << 0.0);
  }

  /// Evaluate error h(x)-z and optionally derivatives
  Vector evaluateError(const Pose3& pose, const LieVector& landmark,
      boost::optional<gtsam::Matrix&> H1=boost::none,
      boost::optional<gtsam::Matrix&> H2=boost::none) const {

    if(H1) {
      (*H1) = numericalDerivative11<Pose3>(boost::bind(&InvDepthFactorVariant2::inverseDepthError, this, _1, landmark), pose);
    }
    if(H2) {
      (*H2) = numericalDerivative11<LieVector>(boost::bind(&InvDepthFactorVariant2::inverseDepthError, this, pose, _1), landmark);
    }

    return inverseDepthError(pose, landmark);
  }

  /** return the measurement */
  const gtsam::Point2& imagePoint() const {
    return measured_;
  }

  /** return the calibration object */
  const Cal3_S2::shared_ptr calibration() const {
    return K_;
  }

  /** return the calibration object */
  const Point3& referencePoint() const {
    return referencePoint_;
  }

private:

  /// Serialization function
  friend class boost::serialization::access;
  template<class ARCHIVE>
  void serialize(ARCHIVE & ar, const unsigned int version) {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Base);
    ar & BOOST_SERIALIZATION_NVP(measured_);
    ar & BOOST_SERIALIZATION_NVP(K_);
    ar & BOOST_SERIALIZATION_NVP(referencePoint_);
  }
};

} // \ namespace gtsam
