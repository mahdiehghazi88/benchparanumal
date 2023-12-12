/*

The MIT License (MIT)

Copyright (c) 2017-2022 Tim Warburton, Noel Chalmers, Jesse Chan, Ali Karakus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef LINEARSOLVER_HPP
#define LINEARSOLVER_HPP

#include "core.hpp"
#include "platform.hpp"
#include "solver.hpp"

namespace libp {

//virtual base linear solver class
class linearSolver_t {
public:
  platform_t platform;
  comm_t comm;

  dlong N;
  dlong Nhalo;

  linearSolver_t(platform_t& _platform, dlong _N, dlong _Nhalo):
    platform(_platform), comm(platform.comm),
    N(_N), Nhalo(_Nhalo) {}

  virtual int Solve(solver_t& solver,
                    deviceMemory<dfloat> o_x,
                    deviceMemory<dfloat> o_rhs,
                    const dfloat tol,
                    const int MAXIT,
                    const int verbose)=0;
};

//Conjugate Gradient
class cg: public linearSolver_t {
private:
  deviceMemory<dfloat> o_p, o_Ap;

  deviceMemory<dfloat> o_tmprdotr;
  pinnedMemory<dfloat> h_tmprdotr;

  int cgBlockSize;

  kernel_t updateCGKernel;

  dfloat UpdateCG(const dfloat alpha,
                  deviceMemory<dfloat> o_x,
                  deviceMemory<dfloat> o_r);

public:
  cg(platform_t& _platform, dlong _N, dlong _Nhalo);

  int Solve(solver_t& solver,
            deviceMemory<dfloat> o_x,
            deviceMemory<dfloat> o_rhs,
            const dfloat tol,
            const int MAXIT,
            const int verbose);
};

} //namespace libp

#endif
