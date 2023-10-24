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

#include "bp.hpp"

int main(int argc, char **argv){

  // start up MPI
  comm_t::Init(argc, argv);

  { /*Scope so everything is destructed before MPI_Finalize */
    comm_t comm(comm_t::world().Dup());

    bpSettings_t settings(argc, argv, comm);
    if (settings.compareSetting("VERBOSE", "TRUE"))
      settings.report();

    // set up platform
    platform_t platform(settings);

    // set up mesh
    mesh_t mesh(platform, settings, comm);

    // set up bp solver
    bp_t bp(platform, settings, mesh);

    // run
    if (settings.compareSetting("KERNEL TUNING", "TRUE")) {
      bp.RunTuning();
    } else if (settings.compareSetting("KERNEL TEST", "TRUE")) {
      bp.RunBK();
    } else {
      bp.RunBP();
    }
  }

  // close down MPI
  comm_t::Finalize();
  return LIBP_SUCCESS;
}
