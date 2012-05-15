/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file small.cpp
 * @brief UGM (undirected graphical model) examples: small
 * @author Frank Dellaert
 *
 * See http://www.di.ens.fr/~mschmidt/Software/UGM/small.html
 */

#include <gtsam/discrete/DiscreteFactorGraph.h>
#include <gtsam/discrete/DiscreteSequentialSolver.h>

using namespace std;
using namespace gtsam;

int main(int argc, char** argv) {

	// We will assume 2-state variables, where, to conform to the "small" example
	// we have 0 == "right answer" and 1 == "wrong answer"
	size_t nrStates = 2;

	// define variables
	DiscreteKey Cathy(1, nrStates), Heather(2, nrStates), Mark(3, nrStates),
			Allison(4, nrStates);

	// create graph
	DiscreteFactorGraph graph;

	// add node potentials
	graph.add(Cathy,   "1 3");
	graph.add(Heather, "9 1");
	graph.add(Mark,    "1 3");
	graph.add(Allison, "9 1");

	// add edge potentials
	graph.add(Cathy & Heather, "2 1 1 2");
	graph.add(Heather & Mark,  "2 1 1 2");
	graph.add(Mark & Allison,  "2 1 1 2");

	// Print the UGM distribution
	cout << "\nUGM distribution:" << endl;
	for (size_t a = 0; a < nrStates; a++)
		for (size_t m = 0; m < nrStates; m++)
			for (size_t h = 0; h < nrStates; h++)
				for (size_t c = 0; c < nrStates; c++) {
					DiscreteFactor::Values values;
					values[1] = c;
					values[2] = h;
					values[3] = m;
					values[4] = a;
					double prodPot = graph(values);
					cout << c << " " << h << " " << m << " " << a << " :\t"
							<< prodPot << "\t" << prodPot/3790 << endl;
				}

	// "Decoding", i.e., configuration with largest value
	// We use sequential variable elimination
	DiscreteSequentialSolver solver(graph);
	DiscreteFactor::sharedValues optimalDecoding = solver.optimize();
	optimalDecoding->print("\noptimalDecoding");

	return 0;
}
