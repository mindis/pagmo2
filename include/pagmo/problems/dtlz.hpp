/* Copyright 2017 PaGMO development team

This file is part of the PaGMO library.

The PaGMO library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The PaGMO library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the PaGMO library.  If not,
see https://www.gnu.org/licenses/. */

#ifndef PAGMO_PROBLEM_DTLZ_HPP
#define PAGMO_PROBLEM_DTLZ_HPP

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

#include "../detail/constants.hpp"
#include "../exceptions.hpp"
#include "../io.hpp"
#include "../population.hpp"
#include "../problem.hpp"
#include "../types.hpp"

namespace pagmo
{

/// DTLZ problem test suite
/**
 * This widespread test suite was conceived for multiobjective problems with scalable fitness dimensions
 * and takes its name from its authors Deb, Thiele, Laumanns and Zitzler
 *
 * All problems in this test suite are box-constrained continuous n-dimensional multi-objective problems, scalable in
 * fitness dimension. The dimension of the decision space is \f$ k + fdim - 1 \f$, whereas fdim is the number of
 * objectives and k a paramter. Properties of the decision space and the Pareto-front of each problems are as follow:
 *
 * DTLZ1:
 *
 * The optimal pareto front lies on a linear hyperplane \f$ \sum_{m=1}^M f_m = 0.5 \f$ .
 *
 * DTLZ2:
 *
 * The search space is continous, unimodal and the problem is not deceptive.
 *
 * DTLZ3:
 *
 * The search space is continous, unimodal and the problem is not deceptive.
 * It is supposed to be harder to converge towards the optimal pareto front than DTLZ2
 *
 * DTLZ4:
 *
 * The search space contains a dense area of solutions next to the \f$ f_M / f_1\f$ plane.
 *
 * DTLZ5:
 *
 * This problem will test an MOEA's ability to converge to a cruve and will also allow an easier way to visually
 * demonstrate (just by plotting f_M with any other objective function) the performance of an MOEA. Since there is a
 * natural bias for solutions close to this Pareto-optimal curve, this problem may be easy for an algorithmn to solve.
 * Because of its simplicity its recommended to use a higher number of objectives \f$ M \in [5, 10]\f$.
 *
 * DTLZ6:
 *
 * A more difficult version of the DTLZ5 problem: the non-linear distance function g makes it harder to convergence
 * against the pareto optimal curve.
 *
 * DTLZ7:
 *
 * This problem has disconnected Pareto-optimal regions in the search space.
 *
 * @see K. Deb, L. Thiele, M. Laumanns, E. Zitzler, Scalable test problems for evolutionary multiobjective optimization
 */

class dtlz
{
public:
    /** Constructor
    *
    * Will construct a problem of the DTLZ test-suite.
    *
    * @param prob_id problem id
    * @param dim the problem dimension (size of the decision vector)
    * @param fdim number of objectives
    * @param alpha controls density of solutions (used only by DTLZ4)
    *
    * @throw std::invalid_argument if the prob_id is not in [1 .. 7], if fdim is less than 2 or if fdim or dim_param are
    * larger than an implementation defiend value
    *
    */
    dtlz(unsigned int prob_id = 1u, vector_double::size_type dim = 7u, vector_double::size_type fdim = 3u,
         unsigned int alpha = 100u)
        : m_prob_id(prob_id), m_alpha(alpha), m_dim(dim), m_fdim(fdim)
    {
        if (prob_id == 0u || prob_id > 7u) {
            pagmo_throw(std::invalid_argument, "DTLZ test suite contains seven (prob_id = [1 ... 7]) problems, prob_id="
                                                   + std::to_string(prob_id) + " was detected");
        }
        if (fdim < 2u) {
            pagmo_throw(std::invalid_argument, "DTLZ test problem have a minimum of 2 objectives: fdim="
                                                   + std::to_string(fdim) + " was detected");
        }
        // We conservatively limit these dimensions to avoid checking overflows later
        if (fdim > std::numeric_limits<decltype(fdim)>::max() / 3u) {
            pagmo_throw(std::invalid_argument, "The number of objectives is too large");
        }
        if (dim > std::numeric_limits<decltype(dim)>::max() / 3u) {
            pagmo_throw(std::invalid_argument, "The problem dimension is too large");
        }
        if (dim <= fdim) {
            pagmo_throw(std::invalid_argument, "The problem dimension has to be larger than the number of objectives.");
        }
    }
    /// Fitness computation
    /**
     * Computes the fitness for this UDP
     *
     * @param x the decision vector.
     *
     * @return the fitness of \p x.
     */
    vector_double fitness(const vector_double &x) const
    {
        vector_double retval;
        switch (m_prob_id) {
            case 1:
                retval = f1_objfun_impl(x);
                break;
            case 2:
            case 3:
                retval = f23_objfun_impl(x);
                break;
            case 4:
                retval = f4_objfun_impl(x);
                break;
            case 5:
            case 6:
                retval = f56_objfun_impl(x);
                break;
            case 7:
                retval = f7_objfun_impl(x);
                break;
        }
        return retval;
    }
    /// Number of objectives
    /**
     * One of the optional methods of any user-defined problem (UDP).
     * It returns the number of objectives.
     *
     * @return the number of objectives
     */
    vector_double::size_type get_nobj() const
    {
        return m_fdim;
    }
    /// Box-bounds
    /**
     * One of the optional methods of any user-defined problem (UDP).
     * It returns the box-bounds for this UDP.
     *
     * @return the lower and upper bounds for each of the decision vector components
     */
    std::pair<vector_double, vector_double> get_bounds() const
    {
        std::pair<vector_double, vector_double> retval{vector_double(m_dim, 0.), vector_double(m_dim, 1.)};
        return retval;
    }
    /// Distance from the Pareto front (of a population)
    /**
     * Will return the average across the entire population of the convergence metric
     *
     * @param[in] pop population to be assigned a pareto distance
     *
     * @see problem::base_unc_mo::p_distance virtual method.
     */
    double p_distance(const pagmo::population &pop) const
    {
        double c = 0.0;
        for (decltype(pop.size()) i = 0u; i < pop.size(); ++i) {
            c += p_distance(pop.get_x()[i]);
        }

        return c / pop.size();
    }
    /// Distance from the Pareto front
    /**
     * Convergence metric for a given decision_vector (0 = on the optimal front)
     *
     * Introduced by Martens and Izzo, this metric is able
     * to measure "a distance" of any point from the pareto front of any ZDT
     * problem analytically without the need to precompute the front.
     *
     * @param x input decision vector
     * @return the p_distance
     *
     * @see Märtens, Marcus, and Dario Izzo. "The asynchronous island model
     * and NSGA-II: study of a new migration operator and its performance."
     * Proceedings of the 15th annual conference on Genetic and evolutionary computation. ACM, 2013.
     */
    double p_distance(const vector_double &x) const
    {
        if (x.size() != m_dim) {
            pagmo_throw(std::invalid_argument, "The size of the decision vector should be " + std::to_string(m_dim)
                                                   + " while " + std::to_string(x.size()) + " was detected");
        }
        return convergence_metric(x);
    }
    /// Problem name
    /**
     * One of the optional methods of any user-defined problem (UDP).
     *
     * @return a string containing the problem name
     */
    std::string get_name() const
    {
        return "DTLZ" + std::to_string(m_prob_id);
    }
    /// Object serialization
    /**
     * This method will save/load \p this into the archive \p ar.
     *
     * @param ar target archive.
     *
     * @throws unspecified any exception thrown by the serialization of the UDP and of primitive types.
     */
    template <typename Archive>
    void serialize(Archive &ar)
    {
        ar(m_prob_id, m_dim, m_fdim, m_alpha);
    }

private:
    /// Convergence metric for a dv (0 = converged to the optimal front)
    double g_func(const vector_double &x) const
    {
        switch (m_prob_id) { // We start with the 6-7 cases as for absurd reasons behind my comprehension this is
                             // way more efficient
            case 6:
                return g6_func(x);
            case 7:
                return g7_func(x);
            case 1:
            case 3:
                return g13_func(x);
            case 2:
            case 4:
            case 5:
                return g245_func(x);
        }
        return -1;
    }
    /// Implementations of the different g-functions used
    double g13_func(const vector_double &x) const
    {
        double y = 0.;
        for (decltype(x.size()) i = 0u; i < x.size(); ++i) {
            y += std::pow(x[i] - 0.5, 2) - std::cos(20. * detail::pi() * (x[i] - 0.5));
        }
        return 100. * (y + static_cast<double>(x.size()));
    }

    double g245_func(const vector_double &x) const
    {
        double y = 0.;
        for (decltype(x.size()) i = 0u; i < x.size(); ++i) {
            y += std::pow(x[i] - 0.5, 2);
        }
        return y;
    }

    double g6_func(const vector_double &x) const
    {
        double y = 0.0;
        for (decltype(x.size()) i = 0u; i < x.size(); ++i) {
            y += std::pow(x[i], 0.1);
        }
        return y;
    }

    double g7_func(const vector_double &x) const
    {
        // NOTE: the original g-function should return 1 + (9.0 / x.size()) * y but we drop the 1
        // to have the minimum at 0.0 so we can use the p_distance implementation in base_dtlz
        // to have the p_distance converging towards 0.0 rather then towards 1.0
        double y = 0.;
        for (decltype(x.size()) i = 0u; i < x.size(); ++i) {
            y += x[i];
        }
        return (9. / static_cast<double>(x.size())) * y;
    }
    /// Implementation of the distribution function h
    double h7_func(const vector_double &f, const double g) const
    {
        // NOTE: we intentionally ignore the last element of the vector to make things easier
        double y = 0.;

        for (decltype(f.size()) i = 0u; i < f.size() - 1; ++i) {
            y += (f[i] / (1.0 + g)) * (1.0 + std::sin(3 * detail::pi() * f[i]));
        }
        return m_fdim - y;
    }
    /// Implementation of the objective functions.
    /* The chomosome: x_1, x_2, ........, x_M-1, x_M, .........., x_M+k
     *											 [------- Vector x_M -------]
     *               x[0], x[1], ... ,x[fdim-2], x[fdim-1], ... , x[fdim+k-1] */
    vector_double f1_objfun_impl(const vector_double &x) const
    {
        vector_double f(m_fdim);
        // computing distance-function
        vector_double x_M;

        for (decltype(x.size()) i = f.size() - 1u; i < x.size(); ++i) {
            x_M.push_back(x[i]);
        }

        double g = g_func(x_M);

        // computing shape-functions
        f[0] = 0.5 * (1. + g);

        for (decltype(f.size()) i = 0u; i < f.size() - 1u; ++i) {
            f[0] *= x[i];
        }

        for (decltype(f.size()) i = 1u; i < f.size() - 1; ++i) {
            f[i] = 0.5 * (1.0 + g);
            for (decltype(f.size()) j = 0u; j < f.size() - (i + 1); ++j) {
                f[i] *= x[j];
            }
            f[i] *= 1. - x[f.size() - (i + 1u)];
        }

        f[f.size() - 1u] = 0.5 * (1. - x[0]) * (1. + g);
        return f;
    }

    vector_double f23_objfun_impl(const vector_double &x) const
    {
        vector_double f(m_fdim);
        // computing distance-function
        vector_double x_M;
        for (decltype(x.size()) i = f.size() - 1u; i < x.size(); ++i) {
            x_M.push_back(x[i]);
        }

        auto g = g_func(x_M);

        // computing shape-functions
        f[0] = (1. + g);
        for (decltype(f.size()) i = 0u; i < f.size() - 1u; ++i) {
            f[0] *= cos(x[i] * detail::pi_half());
        }

        for (decltype(f.size()) i = 1u; i < f.size() - 1u; ++i) {
            f[i] = (1. + g);
            for (decltype(f.size()) j = 0u; j < f.size() - (i + 1u); ++j) {
                f[i] *= cos(x[j] * detail::pi_half());
            }
            f[i] *= std::sin(x[f.size() - (i + 1u)] * detail::pi_half());
        }

        f[f.size() - 1u] = (1. + g) * std::sin(x[0] * detail::pi_half());
        return f;
    }

    vector_double f4_objfun_impl(const vector_double &x) const
    {
        vector_double f(m_fdim);
        // computing distance-function
        vector_double x_M;
        for (decltype(x.size()) i = f.size() - 1; i < x.size(); ++i) {
            x_M.push_back(x[i]);
        }

        auto g = g_func(x_M);

        // computing shape-functions
        f[0] = (1. + g);
        for (decltype(f.size()) i = 0u; i < f.size() - 1u; ++i) {
            f[0] *= cos(std::pow(x[i], m_alpha) * detail::pi_half());
        }

        for (decltype(f.size()) i = 1u; i < f.size() - 1u; ++i) {
            f[i] = (1.0 + g);
            for (decltype(f.size()) j = 0u; j < f.size() - (i + 1u); ++j) {
                f[i] *= cos(std::pow(x[j], m_alpha) * detail::pi_half());
            }
            f[i] *= std::sin(std::pow(x[f.size() - (i + 1u)], m_alpha) * detail::pi_half());
        }

        f[f.size() - 1u] = (1. + g) * std::sin(std::pow(x[0], m_alpha) * detail::pi_half());
        return f;
    }

    vector_double f56_objfun_impl(const vector_double &x) const
    {
        vector_double f(m_fdim);
        // computing distance-function
        vector_double x_M;

        for (decltype(x.size()) i = f.size() - 1u; i < x.size(); ++i) {
            x_M.push_back(x[i]);
        }

        auto g = g_func(x_M);

        // computing meta-variables
        vector_double theta(f.size(), 0.);
        double t;

        theta[0] = x[0];
        t = 1. / (2. * (1. + g));

        for (decltype(f.size()) i = 1u; i < f.size(); ++i) {
            theta[i] = t + ((g * x[i]) / (1.0 + g));
        }

        // computing shape-functions
        f[0] = (1. + g);
        for (decltype(f.size()) i = 0u; i < f.size() - 1u; ++i) {
            f[0] *= cos(theta[i] * detail::pi_half());
        }

        for (decltype(f.size()) i = 1u; i < f.size() - 1u; ++i) {
            f[i] = (1. + g);
            for (decltype(f.size()) j = 0u; j < f.size() - (i + 1u); ++j) {
                f[i] *= cos(theta[j] * detail::pi_half());
            }
            f[i] *= std::sin(theta[f.size() - (i + 1u)] * detail::pi_half());
        }

        f[f.size() - 1u] = (1. + g) * std::sin(theta[0] * detail::pi_half());
        return f;
    }

    vector_double f7_objfun_impl(const vector_double &x) const
    {
        vector_double f(m_fdim);
        // computing distance-function
        vector_double x_M;
        double g;

        for (decltype(x.size()) i = f.size() - 1u; i < x.size(); ++i) {
            x_M.push_back(x[i]);
        }

        g = 1. + g_func(x_M); // +1.0 according to the original definition of the g-function for DTLZ7

        // computing shape-functions
        for (decltype(f.size()) i = 0u; i < f.size() - 1u; ++i) {
            f[i] = x[i];
        }

        f[f.size() - 1u] = (1. + g) * h7_func(f, g);
        return f;
    }
    /// Gives a convergence metric for the population (0 = converged to the optimal front)
    double convergence_metric(const vector_double &x) const
    {
        double c = 0.;
        vector_double x_M;
        for (decltype(x.size()) j = m_fdim - 1u; j < x.size(); ++j) {
            x_M.push_back(x[j]);
        }
        c += g_func(x_M);
        return c;
    }

    // Problem dimensions
    unsigned int m_prob_id;
    // used only for DTLZ4
    unsigned int m_alpha;
    // dimension parameter
    vector_double::size_type m_dim;
    // number of objectives
    vector_double::size_type m_fdim;
};
}

PAGMO_REGISTER_PROBLEM(pagmo::dtlz)

#endif
