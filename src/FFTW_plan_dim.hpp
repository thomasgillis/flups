/**
 * @file FFTW_plan_dim.hpp
 * @author Thomas Gillis and Denis-Gabriel Caprace
 * @copyright Copyright © UCLouvain 2020
 *
 * FLUPS is a Fourier-based Library of Unbounded Poisson Solvers.
 *
 * Copyright <2020> <Université catholique de Louvain (UCLouvain), Belgique>
 *
 * List of the contributors to the development of FLUPS, Description and complete License: see LICENSE and NOTICE files.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef FFTW_PLAN_DIM_HPP
#define FFTW_PLAN_DIM_HPP

#include <array>
#include <tuple>

#include "Topology.hpp"
#include "defines.hpp"
#include "fftw3.h"

/**
 * @brief A FFTW plan in one dimension
 *
 */
class FFTW_plan_dim {
   public:
    /**
     * @brief PlanType is the type of plan considered and is computed as the sum of both BoundaryType variables
     *
     * The integer value associated gives is the priority of processing.
     * We first have to do the real to real transforms, then the padded real to real (mix direction = unbounded + boundary condition),
     * then the periodic (DFT) directions and finally the padded periodic boundary condition.
     * This order is chosen in order to reduce the computational cost.
     *
     * If a multi-dimension FFT is asked, one plan is created for each dimension as it may be different.
     * If the plans are the same, we keep the first plan issued
     */
    enum PlanType {
        SYMSYM = 2, /**< type real 2 real (DCT / DST) : EE (0) , EO/OE (1) , OO (2) */
        MIXUNB = 5, /**< type unbounded and a symetry condition: UE/EU (4) , UO/OU (5) */
        PERPER = 6, /**< type periodic - periodic: PERPER (6) */
        UNBUNB = 8, /**< type fully unbounded UU (8) */
        EMPTY  = 18 /**< type empty, i.e. this direction is not used */
    };

    /**
     * @brief Type of real plan, this will drive the correction step in the execute function
     * - CORRECTION_NONE: no correction is needed
     * - CORRECTION_DCT: the correction of a DCT is needed (while going forward, put 0 in the flip-flop mode)
     * - CORRECTION_DST: the correction of a DST is needed (forward: shift the modes FORWARD and put 0, backward: shift the mode backward)
     *
     */
    enum PlanCorrectionType {
        CORRECTION_NONE = 0,
        CORRECTION_DCT  = 1,
        CORRECTION_DST  = 2,
        CORRECTION_NDST = 3
    };

   protected:
    const int  lda_;     /**<@brief the dimension of the solver */
    const bool isGreen_; /**< @brief boolean is true if this plan is for a Green's function */
    const int  dimID_;   /**< @brief the dimension of the plan in the field reference */
    const int  sign_;    /**< @brief FFT_FORWARD (-1) or FFT_BACKWARD(+1) */

    bool isr2c_       = false; /**< @brief is this plan the one that changes to complex?*/
    bool isSpectral_  = false; /**< @brief indicate if the Green's function has to be done spectrally (leading to a helmolz problem) */
    int  fftw_stride_ = 0;     /**<@brief the memory space between two ffts */
    int  howmany_     = 0;     /**<@brief the number of FFT's to do */

    int    n_out_      = 1;   /**< @brief the number of element coming out of the transform*/
    int    fieldstart_ = 0;   /**< @brief the starting index for the field copy in the direction of the plan*/
    double symstart_   = 0.0; /**< @brief the first index to be copied for the symmetry done on the Green's function, set to 0 if no symmetry is needed*/
    double volfact_    = 1.0; /**< @brief volume factor*/
    double normfact_   = 1.0; /**< @brief factor you need to multiply to get the transform on the right scaling*/
    double kfact_      = 0.0; /**< @brief multiplication factor to have the correct k numbers*/
    double koffset_    = 0.0; /**< @brief additive factor to have the correct k numbers*/

    int* n_in_      = NULL; /**< @brief the number of element in the transform, i. e. given to fftw calls*/
    int* fftwstart_ = NULL; /**< @brief the starting index for the field to be given to FFTW functions*/

    PlanType            type_;                    /**< @brief type of this plan, see #PlanType*/
    BoundaryType*       bc_[2]    = {NULL, NULL}; /**< @brief boundary condition for the ith component [0][i]=LEFT/MIN - [1][i]=RIGHT/MAX*/
    PlanCorrectionType* corrtype_ = NULL;         /**< @brief correction type of this plan, see #PlanCorrectionType*/
    bool*               imult_    = NULL;         /**< @brief boolean indicating that we have to multiply by (-i) in forward and (i) in backward*/
    fftw_r2r_kind*      kind_     = NULL;         /**< @brief kind of transfrom to perform (used by r2r and mix plan only)*/
    fftw_plan*          plan_     = NULL;         /**< @brief the array of FFTW plan*/

   public:
    FFTW_plan_dim(const int lda, const int dimID, const double h[3], const double L[3], BoundaryType* mybc[2], const int sign, const bool isGreen);
    virtual ~FFTW_plan_dim();  // Virtual, following http://www.gotw.ca/publications/mill18.htm

    void init(const int size[3], const bool isComplex);

    void allocate_plan(const Topology* topo, double* data);
    void correct_plan(const Topology*, double* data);
    void execute_plan(const Topology* topo, double* data) const;

    /**
     * @name Getters - return the value
     *
     */
    /**@{ */
    inline bool   isSpectral() const { return isSpectral_; }
    inline bool   isr2c() const { return isr2c_; }
    inline bool   imult(const int lia) const { return imult_[lia]; }
    inline bool   isr2c_doneByFFT() const { return isr2c_ && (!isSpectral_); }
    inline int    dimID() const { return dimID_; }
    inline int    type() const { return type_; }
    inline double symstart() const { return symstart_; }
    inline double normfact() const { return normfact_; }
    inline double volfact() const { return volfact_; }
    inline double kfact() const { return kfact_; }
    inline double koffset() const { return koffset_; }
    inline void   get_outsize(int* size) const { size[dimID_] = n_out_; };
    inline void   get_fieldstart(int* start) const { start[dimID_] = fieldstart_; };
    inline void   get_isNowComplex(bool* isComplex) const { (*isComplex) = (*isComplex) || isr2c_; };
    /**@} */

    void disp();

   protected:
    void check_dataAlign_(const Topology* topo, double* data) const;

    /**
     * @name Plan allocation
     */
    /**@{ */
    void allocate_plan_real_(const Topology* topo, double* data);
    void allocate_plan_complex_(const Topology* topo, double* data);
    /**@} */

    /**
     * @name Initialization
     */
    /**@{ */
    virtual void        init_real2real_(const int size[3], bool isComplex)    = 0;
    virtual void        init_mixunbounded_(const int size[3], bool isComplex) = 0;
    virtual void        init_periodic_(const int size[3], bool isComplex)     = 0;
    virtual void        init_unbounded_(const int size[3], bool isComplex)    = 0;
    virtual void        init_empty_(const int size[3], bool isComplex)        = 0;
    virtual std::string disp_data_center() const                              = 0;
    /**@} */
};

void sort_priority(std::array<std::tuple<int, int>, 3>* priority);
void sort_plans(FFTW_plan_dim* plan[3]);
int  bc_to_types(const BoundaryType* bc[2]);

#endif