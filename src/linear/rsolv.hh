//----------------------------------*-C++-*----------------------------------//
/*!
 * \file   linear/rsolv.hh
 * \author Kent Budge
 * \date   Tue Aug 10 13:01:02 2004
 * \brief  Solve an upper triangular system of equations
 * \note   Copyright (C) 2016-2017 Los Alamos National Security, LLC.
 *         All rights reserved.
 */
//---------------------------------------------------------------------------//
// $Id$
//---------------------------------------------------------------------------//

#ifndef linear_rsolv_hh
#define linear_rsolv_hh

namespace rtt_linear {

//! Solve an upper triangular system of equations.
template <typename RandomContainer>
void rsolv(const RandomContainer &R, const unsigned n, RandomContainer &b);

} // end namespace rtt_linear

#include "rsolv.i.hh"

#endif // linear_rsolv_hh

//---------------------------------------------------------------------------//
// end of linear/rsolv.hh
//---------------------------------------------------------------------------//
