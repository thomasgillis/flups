/**
 * @file vtube.cpp
 * @author Thomas Gillis and Denis-Gabriel Caprace
 * @copyright Copyright © UCLouvain 2019
 * 
 * FLUPS is a Fourier-based Library of Unbounded Poisson Solvers.
 * 
 * Copyright (C) <2019> <Universite catholique de Louvain (UCLouvain), Belgique>
 * 
 * List of the contributors to the development of FLUPS, Description and complete License: see LICENSE file.
 * 
 * This program (FLUPS) is free software: 
 * you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING file).  If not, 
 * see <http://www.gnu.org/licenses/>.
 * 
 */

#include "vtube.hpp"
#include "omp.h"


using namespace std;

void vtube(const DomainDescr myCase, const FLUPS_GreenType typeGreen, const int nSolve) {
    int rank, comm_size;
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &comm_size);

    const int lda =3;

    const int *   nglob  = myCase.nglob;
    const int *   nproc  = myCase.nproc;
    const double *L      = myCase.L;

    const double h[3] = {L[0] / nglob[0], L[1] / nglob[1], L[2] / nglob[2]};

    FLUPS_BoundaryType* mybc[3][2];
    for(int id=0; id<3; id++){
        for(int is=0; is<2; is++){
            mybc[id][is] = (FLUPS_BoundaryType*) flups_malloc(sizeof(int)*lda);
        }
    }

        for (int id = 0; id < 3; id++) {
            for (int is = 0; is < 2; is++) {
                for (int lia = 0; lia < 3; lia++) {
                    mybc[id][is][lia] = myCase.mybcv[id][is][lia];
                }
            }
        }

    // create a real topology
    FLUPS_Topology *topo = flups_topo_new(0, 3, nglob, nproc, false, NULL, FLUPS_ALIGNMENT, comm);

    //-------------------------------------------------------------------------
    /** - Initialize the solver */
    //-------------------------------------------------------------------------
    std::string     name = "tube_" + std::to_string((int)(nglob[0] / L[0])) + "_nrank" + std::to_string(comm_size) + "_nthread" + std::to_string(omp_get_max_threads());
    FLUPS_Profiler *prof = flups_profiler_new_n(name.c_str());
    FLUPS_Solver *  mysolver;
    mysolver = flups_init_timed(topo, mybc, h, L,1, prof);

    flups_set_greenType(mysolver, typeGreen);
    flups_setup(mysolver, true);

    // update the comm and the rank
    comm = flups_topo_get_comm(topo);
    MPI_Comm_rank(comm, &rank);

    //-------------------------------------------------------------------------
    /** - allocate rhs and solution */
    //-------------------------------------------------------------------------
    double *rhs   = (double *)flups_malloc(sizeof(double) * flups_topo_get_memsize(topo));
    double *sol   = (double *)flups_malloc(sizeof(double) * flups_topo_get_memsize(topo));
    double *field = (double *)flups_malloc(sizeof(double) * flups_topo_get_memsize(topo));
    std::memset(rhs, 0, sizeof(double) * flups_topo_get_memsize(topo));
    std::memset(sol, 0, sizeof(double) * flups_topo_get_memsize(topo));
    std::memset(field, 0, sizeof(double) * flups_topo_get_memsize(topo));

    //-------------------------------------------------------------------------
    /** - fill the rhs and the solution */
    //-------------------------------------------------------------------------

    int istart[3];
    flups_topo_get_istartGlob(topo, istart);

    {
        const double sigma = 0.05;
        const int ax0     = flups_topo_get_axis(topo);
        const int ax1     = (ax0 + 1) % 3;
        const int ax2     = (ax0 + 2) % 3;
        const int nmem[3] = {flups_topo_get_nmem(topo, 0), flups_topo_get_nmem(topo, 1), flups_topo_get_nmem(topo, 2)};
        
        double* rhs0 = rhs + flups_locID(ax0, 0, 0, 0, 0, ax0, nmem, 1);
        double* rhs1 = rhs + flups_locID(ax0, 0, 0, 0, 1, ax0, nmem, 1);
        double* rhs2 = rhs + flups_locID(ax0, 0, 0, 0, 2, ax0, nmem, 1);
        double* sol0 = sol + flups_locID(ax0, 0, 0, 0, 0, ax0, nmem, 1);
        double* sol1 = sol + flups_locID(ax0, 0, 0, 0, 1, ax0, nmem, 1);
        double* sol2 = sol + flups_locID(ax0, 0, 0, 0, 2, ax0, nmem, 1);

            for (int i2 = 0; i2 < flups_topo_get_nloc(topo, ax2); i2++) {
                for (int i1 = 0; i1 < flups_topo_get_nloc(topo, ax1); i1++) {
                    for (int i0 = 0; i0 < flups_topo_get_nloc(topo, ax0); i0++) {
                        const size_t id   = flups_locID(ax0, i0, i1, i2, 0, ax0, nmem, 1);
                        const double pos[3] = {(istart[ax0] + i0 + 0.5) * h[ax0],
                                               (istart[ax1] + i1 + 0.5) * h[ax1],
                                               (istart[ax2] + i2 + 0.5) * h[ax2]};
                        //---------------------------------------------------------------
                        // main tube
                        {
                            const double x     = pos[0] - (myCase.xcntr * L[0]);
                            const double y     = pos[1] - (myCase.ycntr * L[1]);
                            const double theta = std::atan2(y, x);  // get the angle in the x-y plane
                            const double r     = sqrt(x * x + y * y);
                            const double rho   = r / sigma;
                            const double vel   = 1.0 / (c_2pi * r) * (1.0 - exp(-rho * rho * 0.5));
                            const double vort  = 1.0 / (c_2pi * sigma * sigma) * exp(-rho * rho * 0.5);

                            rhs0[id] = 0.0;
                            rhs1[id] = 0.0;
                            rhs2[id] = -vort;
                            sol0[id] = -sin(theta) * vel;
                            sol1[id] = +cos(theta) * vel;
                            sol2[id] = 0.0;
                        }

                        //---------------------------------------------------------------
                        // x sym tube
                        {
                            const double x     = pos[0] + (myCase.xcntr * L[0]);
                            const double y     = pos[1] - (myCase.ycntr * L[1]);
                            const double theta = std::atan2(y, x);
                            const double r     = sqrt(x * x + y * y);
                            const double rho   = r / sigma;
                            const double vel   = 1.0 / (c_2pi * r) * (1.0 - exp(-rho * rho * 0.5));
                            const double vort  = 1.0 / (c_2pi * sigma * sigma) * exp(-rho * rho * 0.5);

                            rhs0[id] += 0.0;
                            rhs1[id] += 0.0;
                            rhs2[id] += -myCase.xsign * vort;
                            sol0[id] += -sin(theta) * myCase.xsign * vel;
                            sol1[id] += +cos(theta) * myCase.xsign * vel;
                            sol2[id] += 0.0;
                        }

                        //---------------------------------------------------------------
                        // y sym tube
                        {
                            const double x     = pos[0] - (myCase.xcntr * L[0]);
                            const double y     = pos[1] + (myCase.ycntr * L[1]);
                            const double theta = std::atan2(y, x);
                            const double r     = sqrt(x * x + y * y);
                            const double rho   = r / sigma;
                            const double vel   = 1.0 / (c_2pi * r) * (1.0 - exp(-rho * rho * 0.5));
                            const double vort  = 1.0 / (c_2pi * sigma * sigma) * exp(-rho * rho * 0.5);

                            rhs0[id] += 0.0;
                            rhs1[id] += 0.0;
                            rhs2[id] += -myCase.ysign * vort;
                            sol0[id] += -sin(theta) * myCase.ysign * vel;
                            sol1[id] += +cos(theta) * myCase.ysign * vel;
                            sol2[id] += 0.0;
                        }
                    }
                }
            }
    }

#ifdef DUMP_DBG
    char msg[512];
    // write the source term and the solution
    sprintf(msg, "rhs_%d%d%d%d%d%d_%dx%dx%d", mybc[0][0][0], mybc[0][1][0], mybc[1][0][0], mybc[1][1][0], mybc[2][0][0], mybc[2][1][0], nglob[0], nglob[1], nglob[2]);
    flups_hdf5_dump(topo, msg, rhs);
    sprintf(msg, "anal");
    flups_hdf5_dump(topo, msg, sol);
#endif

    //-------------------------------------------------------------------------
    /** - solve the equations */
    //-------------------------------------------------------------------------
    for (int is = 0; is < nSolve; is++) {
        flups_solve(mysolver, field, rhs, ROT);
    }

#ifdef PROF
    flups_profiler_disp(prof, "solve");
#endif
    flups_profiler_free(prof);

#ifdef DUMP_DBG
    // write the source term and the solution
    sprintf(msg, "sol_%d%d%d%d%d%d_%dx%dx%d", mybc[0][0][0], mybc[0][1][0], mybc[1][0][0], mybc[1][1][0], mybc[2][0][0], mybc[2][1][0], nglob[0], nglob[1], nglob[2]);
    flups_hdf5_dump(topo, msg, field);
#endif

    //-------------------------------------------------------------------------
    /** - compute the error */
    //-------------------------------------------------------------------------
    double *lerr2 = (double *)malloc(lda * sizeof(double));
    double *lerri = (double *)malloc(lda * sizeof(double));
    double *err2  = (double *)malloc(lda * sizeof(double));
    double *erri  = (double *)malloc(lda * sizeof(double));

    std::memset(lerr2, 0, sizeof(double) * lda);
    std::memset(lerri, 0, sizeof(double) * lda);
    std::memset(err2, 0, sizeof(double) * lda);
    std::memset(erri, 0, sizeof(double) * lda);

    //determine the volume associated to a mesh
    double vol = 1.0;
    for (int id = 0; id < 3; id++) {
        if (mybc[id][0][0] != NONE && mybc[id][1][0] != NONE) {
            vol *= h[id];
        }
    }

    {
        const int ax0     = flups_topo_get_axis(topo);
        const int ax1     = (ax0 + 1) % 3;
        const int ax2     = (ax0 + 2) % 3;
        const int nmem[3] = {flups_topo_get_nmem(topo, 0), flups_topo_get_nmem(topo, 1), flups_topo_get_nmem(topo, 2)};
        for (int lia = 0; lia < lda; lia++) {
            for (int i2 = 0; i2 < flups_topo_get_nloc(topo, ax2); i2++) {
                for (int i1 = 0; i1 < flups_topo_get_nloc(topo, ax1); i1++) {
                    for (int i0 = 0; i0 < flups_topo_get_nloc(topo, ax0); i0++) {
                        const size_t id  = flups_locID(ax0, i0, i1, i2, lia, ax0, nmem, 1);
                        const double err = sol[id] - field[id];

                        lerri[lia] = max(lerri[lia], fabs(err));
                        lerr2[lia] += (err * err) * vol;
                    }
                }
            }
        }
    }
    MPI_Allreduce(lerr2, err2, lda, MPI_DOUBLE, MPI_SUM, comm);
    MPI_Allreduce(lerri, erri, lda, MPI_DOUBLE, MPI_MAX, comm);

    for (int i = 0; i < lda; i++) {
        err2[i] = sqrt(err2[i]);
    }

    char   filename[512];
    string folder = "./data";

    sprintf(filename, "%s/%s_%d%d%d%d%d%d_typeGreen=%d.txt", folder.c_str(), __func__, mybc[0][0][0], mybc[0][1][0], mybc[1][0][0], mybc[1][1][0], mybc[2][0][0], mybc[2][1][0], typeGreen);

    if (rank == 0) {
        struct stat st = {0};
        if (stat(folder.c_str(), &st) == -1) {
            mkdir(folder.c_str(), 0770);
        }

        FILE *myfile = fopen(filename, "a+");
        if (myfile != NULL) {
            fprintf(myfile, "%d ", nglob[0]);
            for (int i = 0; i < lda; i++) {
                fprintf(myfile, "%12.12e %12.12e ", err2[i], erri[i]);
            }
            fprintf(myfile, "\n");

            fclose(myfile);
        } else {
            printf("unable to open file %s ! Here is what I would have written:", filename);
            printf("%d ", nglob[0]);
            for (int i = 0; i < lda; i++) {
                printf("%12.12e %12.12e ", err2[i], erri[i]);
            }
            printf("\n");
        }
    }

    free(lerr2);
    free(lerri);
    free(err2);
    free(erri);

    flups_free(sol);
    flups_free(rhs);
    flups_free(field);
    flups_cleanup(mysolver);
    flups_topo_free(topo);

    for (int id = 0; id < 3; id++) {
        for (int is = 0; is < 2; is++) {
            flups_free(mybc[id][is]);
        }
    }
}
