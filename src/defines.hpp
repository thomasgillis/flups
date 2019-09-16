/**
 * @file defines.hpp
 * @author Thomas Gillis
 * @brief 
 * @version
 * @date 2019-07-16
 * 
 * @copyright Copyright © UCLouvain 2019
 * 
 */

#ifndef DEFINES_HPP
#define DEFINES_HPP

#include <cassert>
#include <cmath>
#include <iostream>

#include <execinfo.h>
#include "fftw3.h"
#include "mpi.h"

#define GREEN_DIM 3

// #define DUMP_H5
#undef DUMP_H5

#define FFTW_FLAG FFTW_MEASURE

//=============================================================================
// LOCATORS
//=============================================================================
#define LOCATION ("in " + std::string(__FUNCTION__) + " from  " + std::string(__FILE__) + " at line " + std::to_string(__LINE__) )
#define LOC      ("in " + std::string(__FUNCTION__) )

//=============================================================================
// WARNINGS
//=============================================================================
static inline void FLUPS_WARNING(std::string a, std::string loc) {
    char tmp[512];
    sprintf(tmp, a.c_str());
    char msg_error[1024];
    sprintf(msg_error, "[FLUPS - WARNING] %s - %s\n", tmp, loc);
    printf(msg_error);
    fflush(stdout);
};
template<typename T1>
static inline void FLUPS_WARNING(std::string a, T1 b, std::string loc) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b);
    char msg_error[1024];
    sprintf(msg_error, "[FLUPS - WARNING] %s - %s\n", tmp, loc);
    printf(msg_error);
    fflush(stdout);
};
template<typename T1,typename T2>
static inline void FLUPS_WARNING(std::string a, T1 b, T2 c, std::string loc) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c);
    char msg_error[1024];
    sprintf(msg_error, "[FLUPS - WARNING] %s - %s\n", tmp, loc);
    printf(msg_error);
    fflush(stdout);
};
template<typename T1,typename T2,typename T3>
static inline void FLUPS_WARNING(std::string a, T1 b, T2 c, T3 d, std::string loc) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c, d);
    char msg_error[1024];
    sprintf(msg_error, "[FLUPS - WARNING] %s - %s\n", tmp, loc);
    printf(msg_error);
    fflush(stdout);
};
template<typename T1,typename T2,typename T3,typename T4>
static inline void FLUPS_WARNING(std::string a, T1 b, T2 c, T3 d, T4 e, std::string loc) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c, d, e);
    char msg_error[1024];
    sprintf(msg_error, "[FLUPS - WARNING] %s - %s\n", tmp, loc);
    printf(msg_error);
    fflush(stdout);
};
template<typename T1,typename T2,typename T3,typename T4,typename T5>
static inline void FLUPS_WARNING(std::string a, T1 b, T2 c, T3 d, T4 e, T5 f, std::string loc) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c, d, e, f);
    char msg_error[1024];
    sprintf(msg_error, "[FLUPS - WARNING] %s - %s\n", tmp, loc);
    printf(msg_error);
    fflush(stdout);
};
template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static inline void FLUPS_WARNING(std::string a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g, std::string loc) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c, d, e, f, g);
    char msg_error[1024];
    sprintf(msg_error, "[FLUPS - WARNING] %s - %s\n", tmp, loc);
    printf(msg_error);
    fflush(stdout);
};

//=============================================================================
// LOG / FLUPS_INFO
//=============================================================================
#ifdef VERBOSE
static inline void FLUPS_INFO_DISP(std::string a) {
    char msg[1024];
    sprintf(msg, "[FLUPS] %s\n", a.c_str());
    printf(msg);
    fflush(stdout);
}
static inline void FLUPS_INFO(std::string a) {
    char tmp[512];
    sprintf(tmp, a.c_str());
    FLUPS_INFO_DISP(tmp);
};
template <typename T1>
static inline void FLUPS_INFO(std::string a, T1 b) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b);
    FLUPS_INFO_DISP(tmp);
};
template <typename T1,typename T2>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c);
    FLUPS_INFO_DISP(tmp);
};
template <typename T1,typename T2,typename T3>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c, T3 d) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c, d);
    FLUPS_INFO_DISP(tmp);
};
template <typename T1,typename T2,typename T3,typename T4>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c, T3 d, T4 e) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c, d, e);
    FLUPS_INFO_DISP(tmp);
};
template <typename T1,typename T2,typename T3,typename T4,typename T5>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c, T3 d, T4 e, T5 f) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c, d, e, f);
    FLUPS_INFO_DISP(tmp);
};
template <typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g) {
    char tmp[512];
    sprintf(tmp, a.c_str(), b, c, d, e, f, g);
    FLUPS_INFO_DISP(tmp);
};
#define BEGIN_FUNC {FLUPS_INFO(">>> entering %s from %s at line %d", __func__, __FILE__, __LINE__);};
#else
static inline void FLUPS_INFO_DISP(std::string a) {
    (void(0));
}
static inline void FLUPS_INFO(std::string a) {
    (void(0));
};
template <typename T1>
static inline void FLUPS_INFO(std::string a, T1 b) {
    ((void)0);
};
template <typename T1,typename T2>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c) {
    ((void)0);
};
template <typename T1,typename T2,typename T3>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c, T3 d) {
    ((void)0);
};
template <typename T1,typename T2,typename T3,typename T4>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c, T3 d, T4 e) {
    ((void)0);
};
template <typename T1,typename T2,typename T3,typename T4,typename T5>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c, T3 d, T4 e, T5 f) {
    ((void)0);
};
template <typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static inline void FLUPS_INFO(std::string a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g) {
    ((void)0);
};
// static inline void BEGIN_FUNC() {
//     ((void)0);
// };
#define BEGIN_FUNC { ((void)0);};
#endif

//=============================================================================
// ERRORS AND ASSERTS
//=============================================================================
static inline void FLUPS_ERROR(std::string a, std::string loc) {
    char msg_error[1024];
    sprintf(msg_error, "[FLUPS - ERROR] %s - %s\n", a.c_str(), loc.c_str());
    printf(msg_error);
    fprintf(stderr,msg_error);
    fflush(stderr);
    fflush(stdout);
    MPI_Abort(MPI_COMM_WORLD,0);
};

#ifndef NDEBUG
static inline void FLUPS_CHECK(bool a, std::string b, std::string loc) {
    if (!(a)) {
        FLUPS_ERROR(b,loc);
    }
};
template<typename T1>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, std::string loc) {
    if (!(a)) {
        char msg_chk[1024];
        sprintf(msg_chk, b.c_str(), c);
        FLUPS_ERROR(msg_chk,loc);
    }
};
template<typename T1,typename T2>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, std::string loc) {
    if (!(a)) {
        char msg_chk[1024];
        sprintf(msg_chk, b.c_str(), c, d);
        FLUPS_ERROR(msg_chk,loc);
    }
};
template<typename T1,typename T2,typename T3>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, T3 e, std::string loc) {
    if (!(a)) {
        char msg_chk[1024];
        sprintf(msg_chk, b.c_str(), c, d, e);
        FLUPS_ERROR(msg_chk,loc);
    }
};
template<typename T1,typename T2,typename T3,typename T4>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, T3 e, T4 f, std::string loc) {
    if (!(a)) {
        char msg_chk[1024];
        sprintf(msg_chk, b.c_str(), c, d, e, f);
        FLUPS_ERROR(msg_chk,loc);
    }
};
template<typename T1,typename T2,typename T3,typename T4,typename T5>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, T3 e, T4 f, T5 g, std::string loc) {
    if (!(a)) {
        char msg_chk[1024];
        sprintf(msg_chk, b.c_str(), c, d, e, f, g);
        FLUPS_ERROR(msg_chk,loc);
    }
};
template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, T3 e, T4 f, T5 g, T6 h, std::string loc) {
    if (!(a)) {
        char msg_chk[1024];
        sprintf(msg_chk, b.c_str(), c, d, e, f, g, h);
        FLUPS_ERROR(msg_chk,loc);
    }
};
#else
static inline void FLUPS_CHECK(bool a, std::string b, std::string loc) {
    ((void)0);
};
template<typename T1>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, std::string loc) {
    (void(0));
};
template<typename T1,typename T2>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, std::string loc) {
    (void(0));
};
template<typename T1,typename T2,typename T3>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, T3 e, std::string loc) {
    (void(0));
};
template<typename T1,typename T2,typename T3,typename T4>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, T3 e, T4 f, std::string loc) {
    (void(0));
};
template<typename T1,typename T2,typename T3,typename T4,typename T5>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, T3 e, T4 f, T5 g, std::string loc) {
    (void(0));
};
template<typename T1,typename T2,typename T3,typename T4,typename T5,typename T6>
static inline void FLUPS_CHECK(bool a, std::string b, T1 c, T2 d, T3 e, T4 f, T5 g, T6 h, std::string loc) {
    (void(0));
};
#endif

    //=============================================================================
    // CONSTANTS AND OTHERS
    //=============================================================================

#define GAMMA 0.5772156649015328606

#define FLUPS_FORWARD -1  // = FFTW_FORWARD
#define FLUPS_BACKWARD 1  // = FFTW_BACKWARD

#define FLUPS_ALIGNMENT 32

template <typename T>
static inline bool FLUPS_ISALIGNED(T a) {
    return ((uintptr_t)(const void*)a) % FLUPS_ALIGNMENT == 0;
}

typedef int* __restrict __attribute__((aligned(FLUPS_ALIGNMENT))) opt_int_ptr;
typedef double* __restrict __attribute__((aligned(FLUPS_ALIGNMENT))) opt_double_ptr;
typedef fftw_complex* __restrict __attribute__((aligned(FLUPS_ALIGNMENT))) opt_complex_ptr;

namespace FLUPS {
/**
     * @brief the boundary condition can be EVEN, ODD, PERiodic or UNBounded
     * 
     */
enum BoundaryType {
    EVEN = 0, /**< EVEN boundary condition = zero flux  */
    ODD  = 1, /**< ODD boundary condition = zero value */
    PER  = 3, /**< PERiodic boundary conditions */
    UNB  = 4  /**< UNBounded boundary condition */
};

/**
     * @brief The type of Green's function used for the Poisson solver
     * 
     */
enum GreenType {
    CHAT_2 = 0, /**< @brief quadrature in zero, order 2, Chatelain et al. (2010) */
    LGF_2  = 1,  /**< @brief Lattice Green's function, order 2, Gillis et al. (2018)*/
    HEJ_2  = 2,  /**< @brief regularized in zero, order 2, Hejlesen et al. (2015)*/
    HEJ_4  = 3,  /**< @brief regularized in zero, order 4, Hejlesen et al. (2015)*/
    HEJ_6  = 4,  /**< @brief regularized in zero, order 6, Hejlesen et al. (2015)*/
};

/**
     * @brief Type of Poisson equation solved
     * 
     */
enum SolverType {
    SRHS, /**<@brief scalar \f$ \nabla^2 f = rhs \f$ */
    VRHS, /**<@brief vectorial \f$ \nabla^2 f = rhs \f$ */
    ROT,  /**<@brief vectorial \f$ \nabla^2 f = \nabla \times rhs \f$ */
    DIV   /**<@brief scalar \f$ \nabla^2 f = \nabla \cdot rhs \f$ */
};

class Solver;
class FFTW_plan_dim;
class Topology;
class SwitchTopo;
}  // namespace FLUPS

static const double c_1opi     = 1.0 / (1.0 * M_PI);
static const double c_1o2pi    = 1.0 / (2.0 * M_PI);
static const double c_1o4pi    = 1.0 / (4.0 * M_PI);
static const double c_1osqrtpi = 1.0 / sqrt(M_PI);
static const double c_1o2      = 1.0 / 2.0;
static const double c_1o4      = 1.0 / 4.0;
static const double c_7o4      = 7. / 4.;
static const double c_19o8     = 19. / 8;
static const double c_2o3      = 2. / 3;
static const double c_1o24     = 1. / 24;

static const double c_1osqrt2 = 1.0 / M_SQRT2;

static const double c_2pi = 2.0 * M_PI;

#endif