/**
 * @file testVelocityConstraint3
 * @author Duy-Nguyen Ta
 */

#include <CppUnitLite/TestHarness.h>

#include <gtsam/inference/Symbol.h>
#include <gtsam_unstable/dynamics/VelocityConstraint3.h>

/* ************************************************************************* */
using namespace gtsam;

namespace {

  const double tol=1e-5;
  const double dt = 1.0;

  LieScalar origin,
    x1(1.0), x2(2.0), v(1.0);

}

/* ************************************************************************* */
TEST( testVelocityConstraint3, evaluateError) {

  using namespace gtsam::symbol_shorthand;

  // hard constraints don't need a noise model
  VelocityConstraint3 constraint(X(1), X(2), V(1), dt);

  // verify error function
  EXPECT(assert_equal(zero(1), constraint.evaluateError(x1, x2, v), tol));
}


/* ************************************************************************* */
int main() { TestResult tr; return TestRegistry::runAllTests(tr); }
/* ************************************************************************* */
