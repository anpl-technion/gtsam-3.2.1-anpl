/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file Simulated2DPointPrior.h
 * @brief Re-created on Feb 22, 2010 for compatibility with MATLAB
 * @author Frank Dellaert
 */

#pragma once

#include <gtsam/slam/simulated2D.h>
#include <gtsam/slam/Simulated2DValues.h>

namespace gtsam {

	/** Create a prior on a landmark Point2 with key 'l1' etc... */
	typedef simulated2D::GenericPrior<Simulated2DValues, simulated2D::PointKey> Simulated2DPointPrior;

}
