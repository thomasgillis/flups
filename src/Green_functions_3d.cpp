/**
 * @file Green_functions_3d.cpp
 * @author Denis-Gabriel Caprace, Thomas Gillis
 * @brief 
 * @version
 * @date 2019-07-22
 * 
 * @copyright Copyright © UCLouvain 2019
 * 
 * -------------------------------
 * **Symmetry computation:**
 * 
 * We have to take the symmetry around symstart. e.g. in X direction: `symstart[0] - (ix - symstart[0]) = 2 symstart[0] - ix`
 * 
 * In some cases when we have an R2C transform, it ask for 2 additional doubles.
 * The value is meaningless but we would like to avoid segfault and nan's.
 * To do so, we use 2 tricks:
 * - The `abs` is used to stay on the positivie side and hence avoid negative memory access
 * - The `max` is used to prevent the computation of the value in 0, which is never used in the symmetry.
 * 
 * As an example, the final formula is then ( in the X direction):
 * `max( abs(2 symstart[0] - ix) , 1)`
 * 
 */

#include "Green_functions_3d.hpp"



/**
 * @brief generic type for Green kernel, takes a table of parameters that can be used depending on the kernel
 * 
 */
typedef double (*GreenKernel)(const void* );

//notice that these function will likely not be inlined as we have a pointer to them...
static inline double _hej_2(const void* params) {
    double r   = ((double*)params) [0];
    double eps = ((double*)params) [1];
    return c_1o4pi / r * (erf(r / eps * c_1osqrt2));
}
static inline double _hej_4(const void* params) {
    double r   = ((double*)params) [0];
    double eps = ((double*)params) [1];
    double rho = r / eps;
    return c_1o4pi / r * (c_1osqrt2 * c_1osqrtpi * (rho)*exp(-rho * rho * .5 ) + erf(rho * c_1osqrt2));
}
static inline double _hej_6(const void* params) {
    double r   = ((double*)params) [0];
    double eps = ((double*)params) [1];
    double rho = r / eps;
    return c_1o4pi / r * (c_1osqrt2 * c_1osqrtpi * (c_7o4 * rho - c_1o4 * pow(rho, 3)) * exp(-rho * rho * .5 ) + erf(rho * c_1osqrt2));
}
static inline double _chat_2(const void* params) {
    double r   = ((double*)params) [0];
    return c_1o4pi / r ;
}

/**
 * @brief Compute the Green function for 3dirunbounded
 * 
 * @param topo the topology associated to the Green's function
 * @param hfact the h multiplication factors
 * @param symstart the symmetry plan asked
 * @param green the Green function array
 * @param typeGreen the type of Green function to be 
 * @param alpha the smoothing parameter (only used for HEJ kernels)
 * 
 */
void cmpt_Green_3D_3dirunbounded_0dirspectral(const Topology *topo, const double hfact[3], const int symstart[3], double *green, GreenType typeGreen, const double alpha){
    BEGIN_FUNC

    UP_CHECK0(!(topo->isComplex()),"Green topology cannot been complex");

    // assert that the green spacing is not 0.0 everywhere
    UP_CHECK0(hfact[0] != 0.0, "grid spacing cannot be 0");
    UP_CHECK0(hfact[1] != 0.0, "grid spacing cannot be 0");
    UP_CHECK0(hfact[2] != 0.0, "grid spacing cannot be 0");

    int ax0 = topo->axis();
    int ax1 = (ax0 + 1) % 3;
    int ax2 = (ax0 + 2) % 3;

    const double eps     = alpha * hfact[0];
    
    double      G0;  //value of G in 0
    GreenKernel G;

    switch (typeGreen) {
        case HEJ_2:
            G  = &_hej_2;
            G0 =       M_SQRT2 / (4.0 * eps * sqrt(M_PI * M_PI * M_PI));
            break;
        case HEJ_4:
            G  = &_hej_4;
            G0 = 3.0 * M_SQRT2 / (8.0 * eps * sqrt(M_PI * M_PI * M_PI));
            break;
        case HEJ_6:
            G  = &_hej_6;
            G0 = 15.0 * M_SQRT2 / (32.0 * eps * sqrt(M_PI * M_PI * M_PI));
            break;
        case CHAT_2:
            G  = &_chat_2;
            G0 = .5 * pow(1.5 * c_1o2pi * hfact[0] * hfact[1] * hfact[2], 2. / 3.);
            break;
        case LGF_2:
            UP_ERROR("Lattice Green Function not implemented yet.");
            //please add the parameters you need to params
            break;
        default:
            UP_ERROR("Green Function type unknow.");
    }

    int istart[3];
    get_istart_glob(istart,topo);

    for (int i2 = 0; i2 < topo->nloc(ax2); i2++) {
        for (int i1 = 0; i1 < topo->nloc(ax1); i1++) {
            for (int i0 = 0; i0 < topo->nloc(ax0); i0++) {
                // exact indexes in global indexing
                const int ie0 = (istart[ax0] + i0);
                const int ie1 = (istart[ax1] + i1);
                const int ie2 = (istart[ax2] + i2);

                // symmetrize indexes
                const int is0 = (symstart[ax0] == 0 || ie0 <= symstart[ax0]) ? ie0 : std::max(abs(2 * (int)symstart[ax0] - ie0), 1);
                const int is1 = (symstart[ax1] == 0 || ie1 <= symstart[ax1]) ? ie1 : std::max(abs(2 * (int)symstart[ax1] - ie1), 1);
                const int is2 = (symstart[ax2] == 0 || ie2 <= symstart[ax2]) ? ie2 : std::max(abs(2 * (int)symstart[ax2] - ie2), 1);

                // symmetrized position
                const double x0 = (is0)*hfact[ax0];
                const double x1 = (is1)*hfact[ax1];
                const double x2 = (is2)*hfact[ax2];

                // green function value
                const double r2 = x0 * x0 + x1 * x1 + x2 * x2;
                const double r  = sqrt(r2);
                const size_t id = i0 + topo->nloc(ax0) * (i1 + i2 * topo->nloc(ax1));

                const double tmp[2] = {r, eps};
                green[id]           = -G(tmp);
            }
        }
    }
    // reset the value in 0.0
    if (istart[ax0] == 0 && istart[ax1] == 0 && istart[ax2] == 0) {
        green[0] = -G0;
    }
}

/**
 * @brief 
 * 
 * @param topo must be the topo in which ax0 is the spectral dir
 * @param hfact 
 * @param kfact 
 * @param symstart 
 * @param green 
 * @param typeGreen 
 * @param alpha 
 */
void cmpt_Green_3D_2dirunbounded_1dirspectral(const Topology *topo, const double hfact[3], const double kfact[3], const int symstart[3], double *green, GreenType typeGreen, const double alpha){

    
    const int ax0 = topo->axis();  
    const int ax1 = (ax0 + 1) % 3; 
    const int ax2 = (ax0 + 2) % 3; 

    // assert that the green spacing and dk is not 0.0 - this is also a way to check that ax0 will be spectral, and the others are still to be transformed
    UP_CHECK0(kfact[ax0] != 0.0, "dk[0] cannot be 0"); 
    UP_CHECK0(hfact[ax1] != 0.0, "grid spacing[1] cannot be 0");
    UP_CHECK0(hfact[ax2] != 0.0, "grid spacing[2] cannot be 0");
    
    UP_CHECK0(hfact[ax0] == 0.0, "grid spacing[0] cannot be 0");
    UP_CHECK0(kfact[ax1] == 0.0, "dk[1] cannot be 0");
    UP_CHECK0(kfact[ax2] == 0.0, "dk[2] cannot be 0");

    // @Todo For Helmolz, we need Green to be complex 
    // UP_CHECK0(topo->isComplex(), "I can't fill a non complex topo with a complex green function.");
    // opt_double_ptr mygreen = green; //casting of the Green function to be able to access real and complex part

    // const double eps     = alpha * hfact[0];
    
    // double      G0 = 0.0;  //value of G in k=0. By convention, we here chose that the mode 0 is killed by the Poisson solver.
    // GreenKernel G;

    switch (typeGreen) {
        case HEJ_2:
            UP_ERROR("HEJ kernels not implemented in 2dirunbounded 1dirspectral.");
            // see [Spietz:2018]
            break;
        case HEJ_4:
            UP_ERROR("HEJ kernels not implemented in 2dirunbounded 1dirspectral.");
            break;
        case HEJ_6:
            UP_ERROR("HEJ kernels not implemented in 2dirunbounded 1dirspectral.");
            break;
        case CHAT_2:
            INFOLOG("CHAT_2 is the only kernel available in full spectral for now... so we hardcoded the kernel in the loop.\n");
            break;
        case LGF_2:
            UP_ERROR("Lattice Green Function not implemented yet.");
            break;
        default:
            UP_ERROR("Green Function type unknow.");
    }

    int istart[3];
    get_istart_glob(istart,topo);


    //Note: i0 (ax0) is the spectral axis. As data is aligned in pencils along this direction, nloc(ax0) = nglob(ax0)
    for (int i2 = 0; i2 < topo->nloc(ax2); i2++) {
        for (int i1 = 0; i1 < topo->nloc(ax1); i1++) {
            //local indexes start
            size_t id = localindex_ao(0, i1, i2, topo);

            const int ie1 = (istart[ax1] + i1);
            const int ie2 = (istart[ax2] + i2);

            //symmetry for unbounded direction:
            const int is1 = (symstart[ax1] == 0 || ie1 <= symstart[ax1]) ? ie1 : std::max(abs(2 * (int)symstart[ax1] - ie1), 1);    
            const int is2 = (symstart[ax2] == 0 || ie2 <= symstart[ax2]) ? ie2 : std::max(abs(2 * (int)symstart[ax2] - ie2), 1);    

            //(symmetrized) position
            const double x1 = (is1)*hfact[ax1];
            const double x2 = (is2)*hfact[ax2];
            const double r  = sqrt(x1 * x1 + x2 * x2);

            // green function value
            green[id] = c_1o2pi * log(r); //caution: mistake in [Chatelain2010]

            for (int i0 = 1; i0 < topo->nloc(ax0); i0++) {

                // global indexes
                const int ie0 = (istart[ax0] + i0);

                // symmetrize indexes for spectral direction
                const int is0 = (symstart[ax0] == 0 || ie0 <= symstart[ax0]) ? ie0 : std::min(-2 * (int)symstart[ax0] + ie0, -1);

                // (symmetrized) wave number
                const double k0 = (is0)*kfact[ax0];

                // green function value
                // const double tmp[2] = {r,eps};
                // const double ooksqr = 1 / ksqr;

                //Implementation note: if you want to do Helmolz, you need Hankel functions (3rd order Bessel) which are not implemented in stdC. Consider the use of boost lib.
                //notice that bessel_k has been introduced in c++17
                green[id + i0] = -c_1o2pi * std::cyl_bessel_k(0.0, abs(k0) * r);  //G( tmp );
            }
        }
    }
    // reset the value in x=y=0.0
    if (istart[ax1] == 0 && istart[ax2] == 0) {

            const double r_eq2D = c_1osqrtpi * sqrt(hfact[ax1]*hfact[ax2]);

            // green[0] = -2.0 * log(1 + sqrt(2)) * c_1opiE3o2 / r_eq2D;
            green[0] = .25 * c_1o2pi * (M_PI - 6.0 + 2. * log(.5 * M_PI * r_eq2D));  //caution: mistake in [Chatelain2010]

            for (int i0 = 1; i0 < topo->nloc(ax0); i0++) {

                // global indexes
                const int ie0 = (istart[ax0] + i0);

                //symmetry for spectral direction:
                const int is0 = (symstart[ax0] == 0 || ie0 <= symstart[ax0]) ? ie0 : std::max(abs(2 * (int)symstart[ax0] - ie0), 1);
                
                // (symmetrized) wave number
                const double k0 = (is0)*kfact[ax0];
                
                //Implementation note: if you want to do Helmolz, you need Hankel functions (3rd order Bessel) which are not implemented in stdC. Consider the use of boost lib.
                //notice that bessel_k has been introduced in c++17
                green[i0] = -(1.0 - k0 * r_eq2D * std::cyl_bessel_k(1.0, k0 * r_eq2D)) * c_1opi / ((k0 * r_eq2D) * (k0 * r_eq2D));
            }
    }
}

void cmpt_Green_3D_1dirunbounded_2dirspectral(const Topology *topo, const double hfact[3], const double kfact[3], const int symstart[3], double *green, GreenType typeGreen, const double alpha){
    
    const int ax0 = topo->axis();  
    const int ax1 = (ax0 + 1) % 3; 
    const int ax2 = (ax0 + 2) % 3; 

    printf("kfact - hfact : %lf,%lf,%lf - %lf,%lf,%lf\n",kfact[ax0],kfact[ax1],kfact[ax2],hfact[ax0],hfact[ax1],hfact[ax2]);

    // assert that the green spacing and dk is not 0.0 - this is also a way to check that ax0 will be spectral, and the others are still to be transformed
    UP_CHECK0(kfact[ax0] == 0.0, "dk[0] must be 0"); 
    UP_CHECK0(kfact[ax1] != 0.0, "dk[1] cannot be 0");
    UP_CHECK0(kfact[ax2] != 0.0, "dk[2] cannot be 0");
    
    UP_CHECK0(hfact[ax0] != 0.0, "grid spacing[0] cannot be 0");
    UP_CHECK0(hfact[ax1] == 0.0, "grid spacing[1] must be 0");
    UP_CHECK0(hfact[ax2] == 0.0, "grid spacing[2] must be 0");

    // @Todo For Helmolz, we need Green to be complex 
    // UP_CHECK0(topo->isComplex(), "I can't fill a non complex topo with a complex green function.");
    // opt_double_ptr mygreen = green; //casting of the Green function to be able to access real and complex part

    // const double eps     = alpha * hfact[0];
    
    // double      G0 = 0.0;  //value of G in k=0. By convention, we here chose that the mode 0 is killed by the Poisson solver.
    // GreenKernel G;

    switch (typeGreen) {
        case HEJ_2:
            UP_ERROR("HEJ kernels not implemented in 2dirunbounded 1dirspectral.");
            // see [Spietz:2018]
            break;
        case HEJ_4:
            UP_ERROR("HEJ kernels not implemented in 2dirunbounded 1dirspectral.");
            break;
        case HEJ_6:
            UP_ERROR("HEJ kernels not implemented in 2dirunbounded 1dirspectral.");
            break;
        case CHAT_2:
            INFOLOG("CHAT_2 is the only kernel available in full spectral for now... so we hardcoded the kernel in the loop.\n");
            break;
        case LGF_2:
            UP_ERROR("Lattice Green Function not implemented yet.");
            break;
        default:
            UP_ERROR("Green Function type unknow.");
    }

    int istart[3];
    get_istart_glob(istart,topo);


    //Note: i0 (ax0) is the only spatial (i.e. non spectral) axis
    for (int i2 = 0; i2 < topo->nloc(ax2); i2++) {
        for (int i1 = 0; i1 < topo->nloc(ax1); i1++) {
            //local indexes start
            size_t id = localindex_ao(0, i1, i2, topo);

            const int ie1 = (istart[ax1] + i1);
            const int ie2 = (istart[ax2] + i2);

            //symmetry for spectral direction:
            const int is1 = (symstart[ax1] == 0 || ie1 <= symstart[ax1]) ? ie1 : std::min(-2 * (int)symstart[ax1] + ie1, -1);
            const int is2 = (symstart[ax2] == 0 || ie2 <= symstart[ax2]) ? ie2 : std::min(-2 * (int)symstart[ax2] + ie2, -1);
                    
            // (symmetrized) wave number
            const double k1 = (is1)*kfact[ax1];
            const double k2 = (is2)*kfact[ax2];
            const double k  = sqrt(k1 * k1 + k2 * k2);

            for (int i0 = 0; i0 < topo->nloc(ax0); i0++) {

                // global indexes
                const int ie0 = (istart[ax0] + i0);

                // symmetrize for unbounded direction
                const int is0 = (symstart[ax0] == 0 || ie0 <= symstart[ax0]) ? ie0 : std::max(abs(2 * (int)symstart[ax0] - ie0), 1);

                // (symmetrized) position
                const double x0 = (is0)*hfact[ax0];

                // green function value
                green[id + i0] = -.5 * exp(-k * x0) / k;  
            }
        }
    }
    // reset the value in k1=k2=0.0
    if (istart[ax1] == 0 && istart[ax2] == 0) {

            for (int i0 = 0; i0 < topo->nloc(ax0); i0++) {

                // global indexes
                const int ie0 = (istart[ax0] + i0);

                //symmetry for unbounded direction:
                const int is0 = (symstart[ax0] == 0 || ie0 <= symstart[ax0]) ? ie0 : std::max(abs(2 * (int)symstart[ax0] - ie0), 1);
                
                // (symmetrized) position
                const double x0 = (is0)*hfact[ax0];

                // green function value
                green[i0] = .5 * abs(x0);
            }
    }
}

/**
 * @brief Compute the Green function for 3dirspectral
 * 
 * __Note on performance__: obviously, the Green function in full spectral
 * is \f$\-frac{1}{k^2}\f$ (at least for CHAT_2). We could perform that operation directly in the
 * loop of `dothemagic`. We here choose to still precompute and store it.
 * We burn more memory, but we should fasten `dothemagic` as we replace a
 * (expensive) evaluation of \f$\frac{1}{k^2}\f$ by a memory access.
 * 
 * @param topo 
 * @param kfact 
 * @param symstart 
 * @param green 
 * @param typeGreen 
 * @param alpha 
 */
void cmpt_Green_3D_0dirunbounded_3dirspectral(const Topology *topo, const double kfact[3], const int symstart[3], double *green, GreenType typeGreen, const double alpha){
    BEGIN_FUNC

    // assert that the green spacing is not 0.0 everywhere
    UP_CHECK0(kfact[0] != 0.0, "dk cannot be 0");
    UP_CHECK0(kfact[1] != 0.0, "dk cannot be 0");
    UP_CHECK0(kfact[2] != 0.0, "dk cannot be 0");

    opt_double_ptr mygreen = green; //casting of the Green function to be able to access real and complex part

    const int ax0 = topo->axis();
    const int ax1 = (ax0 + 1) % 3;
    const int ax2 = (ax0 + 2) % 3;

    // const double eps     = alpha * hfact[0];
    
    double      G0 = 0.0;  //value of G in k=0. By convention, we here chose that the mode 0 is killed by the Poisson solver.
    GreenKernel G;

    switch (typeGreen) {
        case HEJ_2:
            UP_ERROR("HEJ kernels not implemented in full spectral.");
            // To do this, you need to have the FT of the regularization kernel and divide it by k^2
            break;
        case HEJ_4:
            UP_ERROR("HEJ kernels not implemented in full spectral.");
            break;
        case HEJ_6:
            UP_ERROR("HEJ kernels not implemented in full spectral.");
            break;
        case CHAT_2:
            INFOLOG("CHAT_2 is the only kernel available in full spectral for now... so we hardcoded the kernel in the loop.");
            break;
        case LGF_2:
            UP_ERROR("Lattice Green Function not implemented yet.");
            break;
        default:
            UP_ERROR("Green Function type unknow.");
    }

    int istart[3];
    get_istart_glob(istart,topo);

    if (topo->nf() == 2) { //i.e. the topo is complex
        for (int i2 = 0; i2 < topo->nloc(ax2); i2++) {
            for (int i1 = 0; i1 < topo->nloc(ax1); i1++) {
                //local indexes start
                size_t id = localindex_ao(0, i1, i2, topo);

                for (int i0 = 0; i0 < topo->nloc(ax0); i0++) {
                    // global indexes
                    const int ie0 = (istart[ax0] + i0);
                    const int ie1 = (istart[ax1] + i1);
                    const int ie2 = (istart[ax2] + i2);

                    // symmetrize indexes if required (theoretically never for 3dirspectral)
                    const int is0 = (symstart[ax0] == 0 || ie0 <= symstart[ax0]) ? ie0 : std::min(-2 * (int)symstart[ax0] + ie0, -1);
                    const int is1 = (symstart[ax1] == 0 || ie1 <= symstart[ax1]) ? ie1 : std::min(-2 * (int)symstart[ax1] + ie1, -1);
                    const int is2 = (symstart[ax2] == 0 || ie2 <= symstart[ax2]) ? ie2 : std::min(-2 * (int)symstart[ax2] + ie2, -1);
                    // Caution: not the same kind of symmetry as for unbounded! Here, indices restart from -symstart toward 0

                    // (symmetrized) wave number
                    const double k0 = (is0)*kfact[ax0];
                    const double k1 = (is1)*kfact[ax1];
                    const double k2 = (is2)*kfact[ax2];

                    // green function value
                    const double ksqr = k0 * k0 + k1 * k1 + k2 * k2;
                    const double k    = sqrt(ksqr);

                    // const double tmp[2] = {r,eps};
                    const double ooksqr = 1 / ksqr;
                    
                    mygreen[id + 0]     = -ooksqr;  //G( tmp );
                    mygreen[id + 1]     = 0.0;  //G( tmp );

                    id += 2;
                }
            }
        }
        // reset the value in 0.0
        if (istart[ax0] == 0 && istart[ax1] == 0 && istart[ax2] == 0) {
            mygreen[0] = -G0;
            mygreen[1] = 0.0;
        }
    } else { //this happens when all BCs are symmetry conditions
        for (int i2 = 0; i2 < topo->nloc(ax2); i2++) {
            for (int i1 = 0; i1 < topo->nloc(ax1); i1++) {
                for (int i0 = 0; i0 < topo->nloc(ax0); i0++) {
                    // global indexes
                    const int ie0 = (istart[ax0] + i0);
                    const int ie1 = (istart[ax1] + i1);
                    const int ie2 = (istart[ax2] + i2);

                    // symmetrize indexes if required (theoretically never for 3dirspectral)
                    const int is0 = (symstart[ax0] == 0 || ie0 <= symstart[ax0]) ? ie0 : std::min(-2 * (int)symstart[ax0] + ie0, -1);
                    const int is1 = (symstart[ax1] == 0 || ie1 <= symstart[ax1]) ? ie1 : std::min(-2 * (int)symstart[ax1] + ie1, -1);
                    const int is2 = (symstart[ax2] == 0 || ie2 <= symstart[ax2]) ? ie2 : std::min(-2 * (int)symstart[ax2] + ie2, -1);
                    // Caution: not the same kind of symmetry as for unbounded! Here, indices restart from -symstart toward 0

                    // (symmetrized) wave number
                    const double k0 = (is0)*kfact[ax0];
                    const double k1 = (is1)*kfact[ax1];
                    const double k2 = (is2)*kfact[ax2];

                    // green function value
                    const double ksqr = k0 * k0 + k1 * k1 + k2 * k2;
                    const double k    = sqrt(ksqr);
                    const size_t id = i0 + topo->nloc(ax0) * (i1 + i2 * topo->nloc(ax1));

                    // const double tmp[2] = {r,eps};
                    const double ooksqr = 1 / ksqr;

                    mygreen[id]     = -ooksqr;  //G( tmp );
                }
            }
        }
        // reset the value in 0.0
        if (istart[ax0] == 0 && istart[ax1] == 0 && istart[ax2] == 0) {
            mygreen[0] = -G0;
        }
    }
}