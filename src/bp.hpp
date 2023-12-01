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

#ifndef BP_HPP
#define BP_HPP 1

#include "core.hpp"
#include "mesh.hpp"
#include "solver.hpp"
#include "linearSolver.hpp"

using namespace libp;

class bpSettings_t: public settings_t {
public:
  bpSettings_t(const int argc, char** argv, comm_t _comm);
  void report();
};

class bp_t: public solver_t {
public:
  mesh_t mesh;

  ogs::ogs_t ogs;
  ogs::halo_t gHalo;

  memory<dlong> GlobalToLocal;
  deviceMemory<dlong> o_GlobalToLocal;

  int problemNumber;
  int Nfields;
  dfloat lambda;

  deviceMemory<dfloat> o_AqL;

  kernel_t operatorKernel;
  kernel_t forcingKernel;

  bp_t() = default;
  bp_t(platform_t& _platform, settings_t& _settings,
        mesh_t& _mesh) {
    Setup(_platform, _settings, _mesh);
  }

  //setup
  void Setup(platform_t& _platform,
             settings_t& _settings,
             mesh_t& _mesh);

  void RunBP();
  void RunBK();
  void RunTuning();

  void LocalOperator(deviceMemory<dfloat>& o_q, deviceMemory<dfloat>& o_Aq);

  void Operator(deviceMemory<dfloat>& o_q, deviceMemory<dfloat>& o_Aq);
  void FEMMassOperator(deviceMemory<dfloat> &o_q, deviceMemory<dfloat> &o_Aq);
  void FEMHelmholtzOperator(deviceMemory<dfloat> &o_q, deviceMemory<dfloat> &o_Aq);
  void SEMHelmholtzOperator(deviceMemory<dfloat> &o_q, deviceMemory<dfloat> &o_Aq);

  std::string rhsFileName();
  std::string rhsKernelName();
  std::string AxFileName();
  std::string AxKernelName();

  void AxTuningParams(properties_t& kernelInfo);

  size_t AxBytesMoved();
  size_t AxFLOPs();
  size_t CGBytesMoved(int Niter);
  size_t CGFLOPs(int Niter);
};


#endif

