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

#include "bk1.hpp"
#include "timer.hpp"

void bk1_t::Run(){

  //create occa buffers
  dlong Nall = mesh.Np*mesh.Nelements*Nfields;
  dlong Ngather = ogs.Ngather;
  deviceMemory<dfloat> o_q  = platform.malloc<dfloat>(Ngather);
  deviceMemory<dfloat> o_Aq = platform.malloc<dfloat>(Nall);

  deviceMemory<dfloat> o_cubInterpT = platform.malloc<dfloat>(mesh.cubNp*mesh.cubNp);
  memory<dfloat> cubInterpT;
  cubInterpT.malloc(mesh.cubNp*mesh.cubNp);


  platform.linAlg().InitKernels({"axpy", "innerProd", "norm2", "set"});
  
  for(int n=0;n<mesh.cubNp;++n){
    for(int m=0;m<mesh.Np;++m){
      cubInterpT[m+n*mesh.Np] = mesh.cubInterp[n+m*mesh.cubNp];
    }
  }
  o_cubInterpT.copyFrom(cubInterpT);
  
  //populate x with a typical rhs (use Aq as temp storage)
  dfloat zero = 0.0;
  forcingKernel(mesh.Nelements,
                mesh.o_wJ,
                mesh.o_gllw,
                mesh.o_MM,
                mesh.o_x,
                mesh.o_y,
                mesh.o_z,
                zero,
                o_Aq);

  // gather rhs
  ogs.Gather(o_q, o_Aq, 1, ogs::Add, ogs::Trans);

  // int verbose = settings.compareSetting("VERBOSE", "TRUE") ? 1 : 0;

  int Ntests = 50;

  for(int n=0;n<5;++n){ //warmup
    operatorKernel(mesh.NlocalGatherElements,
                   mesh.o_localGatherElementList,
                   o_GlobalToLocal,
                   mesh.o_cubwJ,
                   mesh.o_cubInterp,
                   o_cubInterpT,
                   mesh.o_MM,
                   o_q,
                   o_Aq);
  }

  timePoint_t start = GlobalPlatformTime(platform);
  for(int n=0;n<Ntests;++n){
    operatorKernel(mesh.NlocalGatherElements,
                   mesh.o_localGatherElementList,
                   o_GlobalToLocal,
                   mesh.o_cubwJ,
                   mesh.o_cubInterp,
                   o_cubInterpT,
                   mesh.o_MM,
                   o_q,
                   o_Aq);
  }
  timePoint_t end = GlobalPlatformTime(platform);
  double elapsedTime = ElapsedTime(start,end)/Ntests;

  int Np = mesh.Np, cubNp = mesh.cubNp, Nq = mesh.Nq, cubNq = mesh.cubNq;

  hlong Ndofs = ogs.NgatherGlobal;

  bool affine = settings.compareSetting("AFFINE MESH", "TRUE");

  size_t Nbytes=0;
  if (affine) {
    Nbytes = Ndofs*sizeof(dfloat) //q
            + (sizeof(dfloat) // J
            + sizeof(dlong) // localGatherElementList
            + Np*Nfields*sizeof(dlong) // GlobalToLocal
            + Np*Nfields*sizeof(dfloat) /*Aq*/ )*mesh.NelementsGlobal;
  } else {
    Nbytes = Ndofs*sizeof(dfloat) //q
            + (cubNp*sizeof(dfloat) // JW
            + sizeof(dlong) // localGatherElementList
            + Np*Nfields*sizeof(dlong) // GlobalToLocal
            + Np*Nfields*sizeof(dfloat) /*Aq*/ )*mesh.NelementsGlobal;
  }

  size_t Nflops=0;
  if (affine) {
    switch (mesh.elementType) {
      case mesh_t::TRIANGLES:
      case mesh_t::TETRAHEDRA:
        Nflops =(  2*Np*Np
                   + 1*Np)*Nfields*mesh.NelementsGlobal;
        break;
      case mesh_t::QUADRILATERALS:
        Nflops =(  4*Nq*Nq*Nq
                   + 1*Nq*Nq)*Nfields*mesh.NelementsGlobal;
        break;
      case mesh_t::HEXAHEDRA:
        Nflops =(  6*Nq*Nq*Nq*Nq
                   + 1*Nq*Nq*Nq)*Nfields*mesh.NelementsGlobal;
        break;
    }
  } else {
    switch (mesh.elementType) {
      case mesh_t::TRIANGLES:
      case mesh_t::TETRAHEDRA:
        Nflops =(  4*cubNp*Np
                   + 1*cubNp)*Nfields*mesh.NelementsGlobal;
        break;
      case mesh_t::QUADRILATERALS:
        Nflops =(  4*cubNq*Nq*Nq
                   + 4*cubNq*cubNq*Nq
                   + 1*cubNq*cubNq)*Nfields*mesh.NelementsGlobal;
        break;
      case mesh_t::HEXAHEDRA:
        Nflops =(  4*cubNq*Nq*Nq*Nq
                   + 4*cubNq*cubNq*Nq*Nq
                   + 4*cubNq*cubNq*cubNq*Nq
                   + 1*cubNq*cubNq*cubNq)*Nfields*mesh.NelementsGlobal;
        break;
    }
  }

  if (mesh.rank==0){
    std::string suffix("Element=");
    switch (mesh.elementType) {
      case mesh_t::TRIANGLES:
        suffix += "Tri";
        break;
      case mesh_t::TETRAHEDRA:
        suffix += "Tet";
        break;
      case mesh_t::QUADRILATERALS:
        suffix += "Quad";
        break;
      case mesh_t::HEXAHEDRA:
        suffix += "Hex";
        break;
    }
    if (affine) suffix += ", Affine";

#if 0
    printf("BK1: N=%2d, DOFs=" hlongFormat ", elapsed=%4.4f, time per DOF=%1.2e, avg BW (GB/s)=%6.1f, avg GFLOPs=%6.1f, DOFs/ranks*time=%1.2e, %s \n",
           mesh.N,
           Ndofs,
           elapsedTime,
           elapsedTime/(Ndofs),
           Nbytes/(1.0e9 * elapsedTime),
           Nflops/(1.0e9 * elapsedTime),
           Ndofs/(mesh.size*elapsedTime),
           suffix.c_str());
#else

    // computue l2 norm of o_Aq
    dfloat checksum = platform.linAlg().norm2(Nall, o_Aq, Comm::World());
    
    printf("1, %02d, " hlongFormat ", %4.4f, %1.2e, %6.1f, %6.1f, %1.2e, %6.1f %%%% BK, N, DOFs, elapsed, time per DOF, avg BW (GB/s), avg GFLOPs, DOFs/ranks*time, checksum, %s \n",
           mesh.N,
           Ndofs,
           elapsedTime,
           elapsedTime/(Ndofs),
           Nbytes/(1.0e9 * elapsedTime),
           Nflops/(1.0e9 * elapsedTime),
           Ndofs/(mesh.size*elapsedTime),
           checksum,
           suffix.c_str());
#endif
  }
}
