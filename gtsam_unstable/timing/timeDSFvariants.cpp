/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    timeDSFvariants.cpp
 * @brief   Time different implementations of DSF
 * @author  Frank Dellaert
 * @date    Oct 26, 2013
 */

#include <gtsam/base/DSFVector.h>
#include <gtsam_unstable/base/DSF.h>
#include <gtsam_unstable/base/DSFMap.h>

#include <boost/random.hpp>
#include <boost/timer.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/std/vector.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <utility>

using namespace std;
using namespace gtsam;
using namespace boost::assign;
using boost::timer;
using boost::format;

int main(int argc, char* argv[]) {

  // Create CSV file for results
  ofstream os("dsf-timing.csv");
  os << "images,points,matches,Base,Map,BTree" << endl;

  // loop over number of images
  vector<size_t> ms;
  ms += 10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000;
  BOOST_FOREACH(size_t m,ms) {
    // We use volatile here to make these appear to the optimizing compiler as
    // if their values are only known at run-time.
    volatile size_t n = 500; // number of points per image
    volatile size_t N = m * n; // number of points per image

    volatile double fm = 0.1; // fraction of image pairs matched
    volatile size_t np = fm * m * m / 2; // number of image pairs
    volatile double fpm = 0.5; // fraction of points matched
    volatile size_t nm = fpm * n * np; // number of matches

    cout << format("\nTesting with %1% images, %2% points, %3% matches\n") % m % N % nm;
    cout << "Generating " << nm << " matches" << endl;
    boost::variate_generator<boost::mt19937, boost::uniform_int<size_t> > rn(
        boost::mt19937(), boost::uniform_int<size_t>(0, N - 1));
    typedef pair<size_t, size_t> Match;
    vector<Match> matches;
    matches.reserve(nm);
    for (size_t k = 0; k < nm; k++)
      matches.push_back(Match(rn(), rn()));

    os << format("%1%,%2%,%3%,") % m % N % nm;

    {
      // DSFBase version
      timer tim;
      DSFBase dsf(N); // Allow for N keys
      BOOST_FOREACH(const Match& m, matches)
        dsf.merge(m.first, m.second);
      os << tim.elapsed() << ",";
      cout << format("DSFBase: %1% s") % tim.elapsed() << endl;
    }

    {
      // DSFMap version
      timer tim;
      DSFMap<size_t> dsf;
      BOOST_FOREACH(const Match& m, matches)
        dsf.merge(m.first, m.second);
      os << tim.elapsed() << endl;
      cout << format("DSFMap: %1% s") % tim.elapsed() << endl;
    }

    if (false) {
      // DSF version, functional
      timer tim;
      DSF<size_t> dsf;
      for (size_t j = 0; j < N; j++)
        dsf = dsf.makeSet(j);
      BOOST_FOREACH(const Match& m, matches)
        dsf = dsf.makeUnion(m.first, m.second);
      os << tim.elapsed() << endl;
      cout << format("DSF functional: %1% s") % tim.elapsed() << endl;
    }

    if (false) {
      // DSF version, in place - always slower - use functional !
      timer tim;
      DSF<size_t> dsf;
      for (size_t j = 0; j < N; j++)
        dsf.makeSetInPlace(j);
      BOOST_FOREACH(const Match& m, matches)
        dsf.makeUnionInPlace(m.first, m.second);
      os << tim.elapsed() << ",";
      cout << format("DSF in-place: %1% s") % tim.elapsed() << endl;
    }

  }

  return 0;

}
