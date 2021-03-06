/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 *  @file  TransformBtwRobotsUnaryFactorEM.h
 *  @brief Unary factor for determining transformation between given trajectories of two robots
 *  @author Vadim Indelman
 **/
#pragma once

#include <ostream>

#include <gtsam/base/Testable.h>
#include <gtsam/base/Lie.h>
#include <gtsam/nonlinear/NonlinearFactor.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>
#include <gtsam/nonlinear/Marginals.h>
#include <gtsam/linear/GaussianFactor.h>
#include <gtsam/slam/BetweenFactor.h>

namespace gtsam {

  /**
   * A class for a measurement predicted by "between(config[key1],config[key2])"
   * @tparam VALUE the Value type
   * @addtogroup SLAM
   */
  template<class VALUE>
  class TransformBtwRobotsUnaryFactorEM: public NonlinearFactor {

  public:

    typedef VALUE T;

  private:

    typedef TransformBtwRobotsUnaryFactorEM<VALUE> This;
    typedef gtsam::NonlinearFactor Base;

    gtsam::Key key_;

    VALUE measured_; /** The measurement */

    gtsam::Values valA_; // given values for robot A map\trajectory
    gtsam::Values valB_; // given values for robot B map\trajectory
    gtsam::Key keyA_;    // key of robot A to which the measurement refers
    gtsam::Key keyB_;    // key of robot B to which the measurement refers

    // TODO: create function to update valA_ and valB_

    SharedGaussian model_inlier_;
    SharedGaussian model_outlier_;

    double prior_inlier_;
    double prior_outlier_;

    bool flag_bump_up_near_zero_probs_;
    mutable bool start_with_M_step_;

    /** concept check by type */
    GTSAM_CONCEPT_LIE_TYPE(T)
    GTSAM_CONCEPT_TESTABLE_TYPE(T)

  public:

    // shorthand for a smart pointer to a factor
    typedef typename boost::shared_ptr<TransformBtwRobotsUnaryFactorEM> shared_ptr;

    /** default constructor - only use for serialization */
    TransformBtwRobotsUnaryFactorEM() {}

    /** Constructor */
    TransformBtwRobotsUnaryFactorEM(Key key, const VALUE& measured, Key keyA, Key keyB,
        const gtsam::Values& valA, const gtsam::Values& valB,
        const SharedGaussian& model_inlier, const SharedGaussian& model_outlier,
        const double prior_inlier, const double prior_outlier,
        const bool flag_bump_up_near_zero_probs = false,
        const bool start_with_M_step = false) :
          Base(cref_list_of<1>(key)), key_(key), measured_(measured), keyA_(keyA), keyB_(keyB),
          model_inlier_(model_inlier), model_outlier_(model_outlier),
          prior_inlier_(prior_inlier), prior_outlier_(prior_outlier), flag_bump_up_near_zero_probs_(flag_bump_up_near_zero_probs),
          start_with_M_step_(false){

      setValAValB(valA, valB);

    }

    virtual ~TransformBtwRobotsUnaryFactorEM() {}


    /** Clone */
    virtual gtsam::NonlinearFactor::shared_ptr clone() const { return boost::make_shared<This>(*this); }


    /** implement functions needed for Testable */

    /** print */
    virtual void print(const std::string& s, const KeyFormatter& keyFormatter = DefaultKeyFormatter) const {
      std::cout << s << "TransformBtwRobotsUnaryFactorEM("
          << keyFormatter(key_) << ")\n";
      std::cout << "MR between factor keys: "
          << keyFormatter(keyA_) << ","
          << keyFormatter(keyB_) << "\n";
      measured_.print("  measured: ");
      model_inlier_->print("  noise model inlier: ");
      model_outlier_->print("  noise model outlier: ");
      std::cout << "(prior_inlier, prior_outlier_) = ("
                << prior_inlier_ << ","
                << prior_outlier_ << ")\n";
      //      Base::print(s, keyFormatter);
    }

    /** equals */
    virtual bool equals(const NonlinearFactor& f, double tol=1e-9) const {
      const This *t =  dynamic_cast<const This*> (&f);

      if(t && Base::equals(f))
        return key_ == t->key_ && measured_.equals(t->measured_) &&
            //            model_inlier_->equals(t->model_inlier_ ) && // TODO: fix here
            //            model_outlier_->equals(t->model_outlier_ ) &&
            prior_outlier_ == t->prior_outlier_ && prior_inlier_ == t->prior_inlier_;
      else
        return false;
    }

    /** implement functions needed to derive from Factor */

    /* ************************************************************************* */
    void setValAValB(const gtsam::Values& valA, const gtsam::Values& valB){
      if ( (!valA.exists(keyA_)) && (!valB.exists(keyA_)) && (!valA.exists(keyB_)) && (!valB.exists(keyB_)) )
        throw("something is wrong!");

      // TODO: make sure the two keys belong to different robots

      if (valA.exists(keyA_)){
        valA_ = valA;
        valB_ = valB;
      }
      else {
        valA_ = valB;
        valB_ = valA;
      }
    }

    /* ************************************************************************* */
    virtual double error(const gtsam::Values& x) const {
      return whitenedError(x).squaredNorm();
    }

    /* ************************************************************************* */
    /**
     * Linearize a non-linearFactorN to get a gtsam::GaussianFactor,
     * \f$ Ax-b \approx h(x+\delta x)-z = h(x) + A \delta x - z \f$
     * Hence \f$ b = z - h(x) = - \mathtt{error\_vector}(x) \f$
     */
    /* This version of linearize recalculates the noise model each time */
    virtual boost::shared_ptr<gtsam::GaussianFactor> linearize(const gtsam::Values& x) const {
      // Only linearize if the factor is active
      if (!this->active(x))
        return boost::shared_ptr<gtsam::JacobianFactor>();

      //std::cout<<"About to linearize"<<std::endl;
      gtsam::Matrix A1;
      std::vector<gtsam::Matrix> A(this->size());
      gtsam::Vector b = -whitenedError(x, A);
      A1 = A[0];

      return gtsam::GaussianFactor::shared_ptr(
          new gtsam::JacobianFactor(key_, A1, b, gtsam::noiseModel::Unit::Create(b.size())));
    }


    /* ************************************************************************* */
    gtsam::Vector whitenedError(const gtsam::Values& x,
        boost::optional<std::vector<gtsam::Matrix>&> H = boost::none) const {

      bool debug = true;

      Matrix H_compose, H_between1, H_dummy;

      T orgA_T_currA = valA_.at<T>(keyA_);
      T orgB_T_currB = valB_.at<T>(keyB_);

      T orgA_T_orgB = x.at<T>(key_);

      T orgA_T_currB = orgA_T_orgB.compose(orgB_T_currB, H_compose, H_dummy);

      T currA_T_currB_pred = orgA_T_currA.between(orgA_T_currB, H_dummy, H_between1);

      T currA_T_currB_msr  = measured_;

      Vector err = currA_T_currB_msr.localCoordinates(currA_T_currB_pred);

      // Calculate indicator probabilities (inlier and outlier)
      Vector p_inlier_outlier = calcIndicatorProb(x, err);
      double p_inlier  = p_inlier_outlier[0];
      double p_outlier = p_inlier_outlier[1];

      if (start_with_M_step_){
        start_with_M_step_ = false;

        p_inlier  = 0.5;
        p_outlier = 0.5;
      }

      Vector err_wh_inlier  = model_inlier_->whiten(err);
      Vector err_wh_outlier = model_outlier_->whiten(err);

      Matrix invCov_inlier  = model_inlier_->R().transpose() * model_inlier_->R();
      Matrix invCov_outlier = model_outlier_->R().transpose() * model_outlier_->R();

      Vector err_wh_eq;
      err_wh_eq.resize(err_wh_inlier.rows()*2);
      err_wh_eq << sqrt(p_inlier) * err_wh_inlier.array() , sqrt(p_outlier) * err_wh_outlier.array();

      Matrix H_unwh = H_compose * H_between1;

      if (H){

        Matrix H_inlier  = sqrt(p_inlier)*model_inlier_->Whiten(H_unwh);
        Matrix H_outlier = sqrt(p_outlier)*model_outlier_->Whiten(H_unwh);
        Matrix H_aug = gtsam::stack(2, &H_inlier, &H_outlier);

        (*H)[0].resize(H_aug.rows(),H_aug.cols());
        (*H)[0] = H_aug;
      }

      if (debug){
        //        std::cout<<"H_compose - rows, cols, : "<<H_compose.rows()<<", "<< H_compose.cols()<<std::endl;
        //        std::cout<<"H_between1 - rows, cols, : "<<H_between1.rows()<<", "<< H_between1.cols()<<std::endl;
        //        std::cout<<"H_unwh - rows, cols, : "<<H_unwh.rows()<<", "<< H_unwh.cols()<<std::endl;
        //        std::cout<<"H_unwh: "<<std:endl<<H_unwh[0]

      }


      return err_wh_eq;
    }

    /* ************************************************************************* */
    gtsam::Vector calcIndicatorProb(const gtsam::Values& x) const {

      Vector err =  unwhitenedError(x);

      return this->calcIndicatorProb(x, err);
    }

    /* ************************************************************************* */
    gtsam::Vector calcIndicatorProb(const gtsam::Values& x, const gtsam::Vector& err) const {

      // Calculate indicator probabilities (inlier and outlier)
      Vector err_wh_inlier  = model_inlier_->whiten(err);
      Vector err_wh_outlier = model_outlier_->whiten(err);

      Matrix invCov_inlier  = model_inlier_->R().transpose() * model_inlier_->R();
      Matrix invCov_outlier = model_outlier_->R().transpose() * model_outlier_->R();

      double p_inlier  = prior_inlier_ * sqrt(invCov_inlier.norm()) * exp( -0.5 * err_wh_inlier.dot(err_wh_inlier) );
      double p_outlier = prior_outlier_ * sqrt(invCov_outlier.norm()) * exp( -0.5 * err_wh_outlier.dot(err_wh_outlier) );

      double sumP = p_inlier + p_outlier;
      p_inlier  /= sumP;
      p_outlier /= sumP;

      if (flag_bump_up_near_zero_probs_){
        // Bump up near-zero probabilities (as in linerFlow.h)
        double minP = 0.05; // == 0.1 / 2 indicator variables
        if (p_inlier < minP || p_outlier < minP){
          if (p_inlier < minP)
            p_inlier = minP;
          if (p_outlier < minP)
            p_outlier = minP;
          sumP = p_inlier + p_outlier;
          p_inlier  /= sumP;
          p_outlier /= sumP;
        }
      }

      return (Vector(2) << p_inlier, p_outlier);
    }

    /* ************************************************************************* */
    gtsam::Vector unwhitenedError(const gtsam::Values& x) const {

      T orgA_T_currA = valA_.at<T>(keyA_);
      T orgB_T_currB = valB_.at<T>(keyB_);

      T orgA_T_orgB = x.at<T>(key_);

      T orgA_T_currB = orgA_T_orgB.compose(orgB_T_currB);

      T currA_T_currB_pred = orgA_T_currA.between(orgA_T_currB);

      T currA_T_currB_msr  = measured_;

      return currA_T_currB_msr.localCoordinates(currA_T_currB_pred);
    }

    /* ************************************************************************* */
    SharedGaussian get_model_inlier() const {
      return model_inlier_;
    }

    /* ************************************************************************* */
    SharedGaussian get_model_outlier() const {
      return model_outlier_;
    }

    /* ************************************************************************* */
    Matrix get_model_inlier_cov() const {
      return (model_inlier_->R().transpose()*model_inlier_->R()).inverse();
    }

    /* ************************************************************************* */
    Matrix get_model_outlier_cov() const {
      return (model_outlier_->R().transpose()*model_outlier_->R()).inverse();
    }

    /* ************************************************************************* */
    void updateNoiseModels(const gtsam::Values& values, const gtsam::Marginals& marginals) {
      /* given marginals version, don't need to marginal multiple times if update a lot */

      std::vector<gtsam::Key> Keys;
      Keys.push_back(keyA_);
      Keys.push_back(keyB_);
      JointMarginal joint_marginal12 = marginals.jointMarginalCovariance(Keys);
      Matrix cov1 = joint_marginal12(keyA_, keyA_);
      Matrix cov2 = joint_marginal12(keyB_, keyB_);
      Matrix cov12 = joint_marginal12(keyA_, keyB_);

      updateNoiseModels_givenCovs(values, cov1, cov2, cov12);
    }

    /* ************************************************************************* */
    void updateNoiseModels(const gtsam::Values& values, const gtsam::NonlinearFactorGraph& graph){
      /* Update model_inlier_ and model_outlier_ to account for uncertainty in robot trajectories
       * (note these are given in the E step, where indicator probabilities are calculated).
       *
       * Principle: R += [H1 H2] * joint_cov12 * [H1 H2]', where H1, H2 are Jacobians of the
       * unwhitened error w.r.t. states, and R is the measurement covariance (inlier or outlier modes).
       *
       * TODO: improve efficiency (info form)
       */

       // get joint covariance of the involved states

       Marginals marginals(graph, values, Marginals::QR);

       this->updateNoiseModels(values, marginals);
    }

    /* ************************************************************************* */
    void updateNoiseModels_givenCovs(const gtsam::Values& values, const Matrix& cov1, const Matrix& cov2, const Matrix& cov12){
      /* Update model_inlier_ and model_outlier_ to account for uncertainty in robot trajectories
       * (note these are given in the E step, where indicator probabilities are calculated).
       *
       * Principle: R += [H1 H2] * joint_cov12 * [H1 H2]', where H1, H2 are Jacobians of the
       * unwhitened error w.r.t. states, and R is the measurement covariance (inlier or outlier modes).
       *
       * TODO: improve efficiency (info form)
       */

      const T& p1 = values.at<T>(keyA_);
      const T& p2 = values.at<T>(keyB_);

      Matrix H1, H2;
      T hx = p1.between(p2, H1, H2); // h(x)

      Matrix H;
      H.resize(H1.rows(), H1.rows()+H2.rows());
      H << H1, H2; // H = [H1 H2]

      Matrix joint_cov;
      joint_cov.resize(cov1.rows()+cov2.rows(), cov1.cols()+cov2.cols());
      joint_cov << cov1, cov12,
          cov12.transpose(), cov2;

      Matrix cov_state = H*joint_cov*H.transpose();

      //       model_inlier_->print("before:");

      // update inlier and outlier noise models
      Matrix covRinlier = (model_inlier_->R().transpose()*model_inlier_->R()).inverse();
      model_inlier_ = gtsam::noiseModel::Gaussian::Covariance(covRinlier + cov_state);

      Matrix covRoutlier = (model_outlier_->R().transpose()*model_outlier_->R()).inverse();
      model_outlier_ = gtsam::noiseModel::Gaussian::Covariance(covRoutlier + cov_state);

      //       model_inlier_->print("after:");
      //       std::cout<<"covRinlier + cov_state: "<<covRinlier + cov_state<<std::endl;
    }


    /* ************************************************************************* */

    /** number of variables attached to this factor */
    std::size_t size() const {
      return 1;
    }

    virtual size_t dim() const {
      return model_inlier_->R().rows() + model_inlier_->R().cols();
    }

  private:

    /** Serialization function */
    friend class boost::serialization::access;
    template<class ARCHIVE>
    void serialize(ARCHIVE & ar, const unsigned int version) {
      ar & boost::serialization::make_nvp("NonlinearFactor",
          boost::serialization::base_object<Base>(*this));
      //ar & BOOST_SERIALIZATION_NVP(measured_);
    }
  }; // \class TransformBtwRobotsUnaryFactorEM

} /// namespace gtsam
