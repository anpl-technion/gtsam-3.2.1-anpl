/**
 * @file testStereoPoint2.cpp
 *
 * @brief Tests for the StereoPoint2 class
 *
 * @date Nov 4, 2011
 * @author Alex Cunningham
 */

#include <CppUnitLite/TestHarness.h>

#include <gtsam/base/Testable.h>
#include <gtsam/geometry/StereoPoint2.h>

using namespace gtsam;

GTSAM_CONCEPT_TESTABLE_INST(StereoPoint2)
GTSAM_CONCEPT_LIE_INST(StereoPoint2)

/* ************************************************************************* */
TEST(testStereoPoint2, test) {

}

/* ************************************************************************* */
int main() { TestResult tr; return TestRegistry::runAllTests(tr); }
/* ************************************************************************* */