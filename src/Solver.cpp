/**
 * @file Solver.cpp
 * @author Thomas Gillis
 * @brief 
 * @version
 * @date 2019-07-16
 * 
 * @copyright Copyright © UCLouvain 2019
 * 
 */

#include "Solver.hpp"

using namespace FLUPS;

/**
 * @brief Constructs a fftw Poisson solver, initilizes the plans and determines their order of execution
 * 
 * @param topo the input topology of the data (in physical space)
 * @param mybc the boundary conditions of the computational domain, the first index corresponds to the dimension, the second is left (0) or right (1) side
 * @param h the grid spacing
 * @param L the domain size
 */
Solver::Solver(const Topology *topo, const BoundaryType mybc[3][2], const double h[3], const double L[3]) {
    BEGIN_FUNC;
    //-------------------------------------------------------------------------
    /** - Create the timer */
    //-------------------------------------------------------------------------
    _prof = new Profiler("Solver");
    _prof->create("init");
    _prof->start("init");
    //-------------------------------------------------------------------------
    /** - For each dim, create the plans and sort them type */
    //-------------------------------------------------------------------------
    for (int id = 0; id < 3; id++)
        _hgrid[id] = h[id];

    for (int id = 0; id < 3; id++) {
        _plan_forward[id]  = new FFTW_plan_dim(id, h, L, mybc[id], FLUPS_FORWARD, false);
        _plan_backward[id] = new FFTW_plan_dim(id, h, L, mybc[id], FLUPS_BACKWARD, false);
        _plan_green[id]    = new FFTW_plan_dim(id, h, L, mybc[id], FLUPS_FORWARD, true);
    }

    _sort_plans(_plan_forward);
    _sort_plans(_plan_backward);
    _sort_plans(_plan_green);
    FLUPS_INFO("I will proceed with forward transforms in the following direction order: %d, %d, %d",_plan_forward[0]->dimID(),_plan_forward[1]->dimID(),_plan_forward[2]->dimID());

    //-------------------------------------------------------------------------
    /** - Initialise the plans and get the sizes */
    //-------------------------------------------------------------------------
    _init_plansAndTopos(topo, _topo_hat, _switchtopo, _plan_forward, false);
    _init_plansAndTopos(topo, NULL, NULL, _plan_backward, false);
    _init_plansAndTopos(topo, _topo_green, _switchtopo_green, _plan_green, true);

    //-------------------------------------------------------------------------
    /** - Get the factors #_normfact, #_volfact, #_shiftgreen and #_nbr_imult */
    //-------------------------------------------------------------------------
    _normfact  = 1.0;
    _volfact   = 1.0;
    _nbr_imult = 0;
    for (int ip = 0; ip < 3; ip++) {
        _normfact *= _plan_forward[ip]->normfact();
        _volfact *= _plan_forward[ip]->volfact();

        // _shiftgreen[_plan_forward[ip]->dimID()] = _plan_forward[ip]->shiftgreen();

        if (_plan_forward[ip]->imult())
            _nbr_imult++; //we multiply by i
        if (_plan_backward[ip]->imult())
            _nbr_imult--; //we devide by i
        if (_plan_green[ip]->imult())
            _nbr_imult++;
    }
    _prof->stop("init");
}

/**
 * @brief Sets up the Solver
 * 
 * After this function the parameter of the solver (size etc) cannot be changed anymore
 * 
 * -------------------------------------------
 * We do the following operations
 */
void Solver::setup() {
    _prof->start("init");
    //-------------------------------------------------------------------------
    /** - allocate the data for the field and Green */
    //-------------------------------------------------------------------------
    _allocate_data(_topo_hat, &_data);
    _allocate_data(_topo_green, &_green);

    //-------------------------------------------------------------------------
    /** - allocate the plans forward and backward for the field */
    //-------------------------------------------------------------------------
    _allocate_plans(_topo_hat, _plan_forward, _data);
    _allocate_plans(_topo_hat, _plan_backward, _data);

    //-------------------------------------------------------------------------
    /** - allocate the plan and comnpute the Green's function */
    //-------------------------------------------------------------------------
    _allocate_plans(_topo_green, _plan_green, _green);
    _cmptGreenFunction(_topo_green, _green, _plan_green);

    //-------------------------------------------------------------------------
    /** - delete the useless data for Green */
    //-------------------------------------------------------------------------
    _delete_switchtopos(_switchtopo_green);
    _prof->stop("init");
}

/**
 * @brief Destroy the fftw solver
 * 
 */
Solver::~Solver() {
    BEGIN_FUNC;
    // for Green
    if (_green != NULL) fftw_free(_green);
    _delete_plans(_plan_green);
    _delete_topologies(_topo_green);

    // for the field
    _delete_plans(_plan_forward);
    _delete_plans(_plan_backward);
    _delete_topologies(_topo_hat);
    _delete_switchtopos(_switchtopo);
    if (_data != NULL) fftw_free(_data);

    // the profiler
    if(_prof!=NULL) delete(_prof);

    //cleanup
    fftw_cleanup();
}
/**
 * @brief delete the FFTW_plan_dim stored in planmap
 * 
 * @param planmap 
 */
void Solver::_delete_plans(FFTW_plan_dim *planmap[3]) {
    BEGIN_FUNC;
    // deallocate the plans
    for (int ip = 0; ip < 3; ip++) {
        delete planmap[ip];
        planmap[ip] = NULL;
    }
}

/**
 * @brief delete the switchtopo objects
 * 
 * @param switchtopo 
 */
void Solver::_delete_switchtopos(SwitchTopo *switchtopo[3]) {
    BEGIN_FUNC;
    // deallocate the plans
    for (int ip = 0; ip < 3; ip++) {
        delete switchtopo[ip];
        switchtopo[ip] = NULL;
    }
}

/**
 * @brief delete the topologies
 * 
 * @param topo 
 */
void Solver::_delete_topologies(Topology *topo[3]) {
    BEGIN_FUNC;
    // deallocate the plans
    for (int ip = 0; ip < 3; ip++) {
        delete topo[ip];
        topo[ip] = NULL;
    }
}

/**
 * @brief smartly determines in which order the FFTs will be executed
 * 
 * @param plan the list of plan, which will be reordered
 */
void Solver::_sort_plans(FFTW_plan_dim *plan[3]) {
    BEGIN_FUNC;
    int id_min, val_min=INT_MAX;
    int priority[3];
    for (int id = 0; id < 3; id++) {
        priority[id] = plan[id]->type();
        if (priority[id] < val_min) {
            id_min = id;
            val_min = priority[id];
        }
    }
    if (id_min == 0) {
        if (priority[1] > priority[2]) {
            FFTW_plan_dim *temp_plan = plan[2];
            plan[2]                  = plan[1];
            plan[1]                  = temp_plan;
        }
    } else {
        // do the sort by hand...
        int temp_priority        = priority[id_min];
        FFTW_plan_dim *temp_plan = plan[id_min];
        plan[id_min]             = plan[0];
        plan[0]                  = temp_plan;
        priority[id_min]         = priority[0];
        priority[0]              = temp_priority;

        // printf("priority now = %d %d %d -> idim = %d",plan[0]->type(), plan[1]->type(),plan[2]->type());

        if (priority[1] > priority[2]) {
            FFTW_plan_dim *temp_plan = plan[2];
            plan[2]                  = plan[1];
            plan[1]                  = temp_plan;
        }
    }

    FLUPS_CHECK((plan[0]->type() <= plan[1]->type()) && (plan[1]->type() <= plan[2]->type()), "Wrong order in the plans: %d %d %d",plan[0]->type(),plan[1]->type(),plan[2]->type());
}

/**
 * @brief Initializes a set of 3 plans by doing a dry run through the plans
 * 
 * @param topo the starting topology
 * @param topomap the topology array to go through each dim ( may be NULL) it corresponds to the topology AFTER the plan
 * @param switchtopo the switchtopo array to switch between topologies (may be NULL, if so it is not computed)
 * @param planmap the plan that will be created
 * @param isGreen indicates if the plans are for Green
 */
void Solver::_init_plansAndTopos(const Topology *topo, Topology *topomap[3], SwitchTopo *switchtopo[3], FFTW_plan_dim *planmap[3], bool isGreen) {
    BEGIN_FUNC;

// @Todo: check that _plan_forward exists before doing _plan_green !

    //-------------------------------------------------------------------------
    /** - Store the current topology */
    //-------------------------------------------------------------------------
    const Topology *current_topo = topo;

    //-------------------------------------------------------------------------
    /** - Get the sizes to start with */
    //-------------------------------------------------------------------------
    // The size is initilized to that of the physical space. Then, with the 
    // dry run, it will grow/shrink in every dimension, and this will be used
    // as the size for the intermediate topos.
    // Eventually, the finial size of the data will be that of the largest 
    // topo.
    int size_tmp[3];
    for (int id = 0; id < 3; id++){
        size_tmp[id] = topo->nglob(id);
    }

    //-------------------------------------------------------------------------
    /** - creates the plans and the intermediate topologies (if not Green).
     *    This performs a dry run in order to determine the final amount of 
     *    memmory required. It also prepares switchtopo which allows to switch
     *    between two successive topologies.   */
    //-------------------------------------------------------------------------
    bool isComplex = false; //this refers to the "current state" of the data during dry run
    int  nproc[3];
    for (int ip = 0; ip < 3; ip++) {
        // initialize the plan (for Green only, using info from _plan_forward)
        planmap[ip]->init(size_tmp, isComplex);
        // update the size_tmp variable and get the complex information
        planmap[ip]->get_outsize(size_tmp);
        // virtually execute the plan and determine the output
        planmap[ip]->get_isNowComplex(&isComplex);
        // determines the fastest rotating index
        int dimID = planmap[ip]->dimID();

        // if we are Green and we have to ignore one mode based on the Green's function
        if(isGreen && planmap[ip]->ignoreMode() ){
            size_tmp[dimID] -= 1;
        }

        // we store a new topology BEFORE the plan is executed
        if (!isGreen && topomap != NULL && switchtopo != NULL) {
            // determines the proc repartition
            _pencil_nproc(dimID, nproc, topo->comm_size());
            // create the new topology corresponding to planmap[ip] in the output layout (size and isComplex)
            topomap[ip] = new Topology(dimID, size_tmp, nproc, isComplex);
            // determines fieldstart = the point where the old topo has to begin in the new one
            // There are cases (typically for MIXUNB) where the data after being switched starts with an offset in memory in the new topo.
            int fieldstart[3] = {0};
            planmap[ip]->get_fieldstart(fieldstart);
            // compute the Switch between the current topo (the one from which we come) and the new one (the one we just created).
            // if the topo was real before the plan and is now complex
            if (planmap[ip]->isr2c()) {
                topomap[ip]->switch2real();
                switchtopo[ip] = new SwitchTopo(current_topo, topomap[ip], fieldstart,_prof);
                topomap[ip]->switch2complex();
            } else {
                // create the switchtopoMPI to change topology
                switchtopo[ip] = new SwitchTopo(current_topo, topomap[ip], fieldstart,_prof);
            }
            // update the current topo to the new one
            current_topo = topomap[ip];

            current_topo->disp();
        }

        planmap[ip]->disp();
    }

    // -- at this point, size_tmp is the size that I need for the Green function in
    //    the last topo, and isComplex describes if the Green function in that topo is
    //    expressed in Complex or not.

    //-------------------------------------------------------------------------
    /** - For Green we need to compute the topologies using the full size of the domain.
     *    We proceed backward (from the last to the first topo), and we adapt the size
     *    in case of r2c, in order to obtain the correct size of Green in topo[0], which
     *    is the topo in which we fill the Green function.      */
    //-------------------------------------------------------------------------
    current_topo = NULL;
    // isComplex = false; //Change this for Helmolz: we will always need to fill Green in complex
    if (isGreen && topomap != NULL && switchtopo != NULL) {
        for (int ip = 2; ip >= 0; ip--) {
            
            // get the fastest rotating index
            int dimID = planmap[ip]->dimID();  // store the correspondance of the transposition
            // get the proc repartition
            _pencil_nproc(dimID, nproc, topo->comm_size());

            // if we had to forget one point for this plan, re-add it
            if(planmap[ip]->ignoreMode() ){
                size_tmp[dimID] += 1;
            }
            // create the new topology in the output layout (size and isComplex)
            topomap[ip] = new Topology(dimID, size_tmp, nproc, isComplex);
            //switchmap only to be done for topo0->topo1 and topo1->topo2
            if (ip < 2){
                // get the fieldstart = the point where the old topo has to begin in the new
                int fieldstart[3] = {0};
                // it shouldn't be different from 0 for the moment
                planmap[ip+1]->get_fieldstart(fieldstart);
                // the shift green is taken on the new topo to write to the current_topo
                const int shift = planmap[ip]->shiftgreen();
                if (!planmap[ip]->ignoreMode()) {
                    FLUPS_CHECK(shift == 0, "If no modes are ignored, you cannot ask for a shift!!");
                } else {
                    // if we aim at removing a point, we make sure to copy every mode except one
                    FLUPS_CHECK((topomap[ip]->nglob(dimID) - 1) == current_topo->nglob(dimID) - fieldstart[dimID], "You will copy too much node between the two topos (dimID = %d)", dimID);
                }

                // store the shift and do the mapping
                fieldstart[dimID] = -shift;
                // we do the link between topomap[ip] and the current_topo
                switchtopo[ip+1] = new SwitchTopo(topomap[ip], current_topo, fieldstart,_prof);
            }

            // Go to real data if the FFT is really done on green's array.
            // if not, keep it in complex
            if (planmap[ip]->isr2c_green() ){
                topomap[ip]->switch2real();
                size_tmp[dimID] *= 2; 
                isComplex = false;
            }
            // update the "current topo", which we need to define the switchtopo
            current_topo = topomap[ip];

            current_topo->disp();
        }
    }

    // Implementation Note:
    // If you want to do Helmoltz, you will always have to fill a complex Green function:
    // - we need to ignore all r2cs (bypass the condition on isr2c_green)
    // - as there will be only C2C transforms, the size obtained after the init of plans
    //   is already the correct size for Green.
    // -> we need to be able to do SYMSYM directions on a complex number... meaning that we
    //    will need to adapt the plan so that when it needs to do a "real2real" transform on
    //    a complex input, it actually does it separately on the real and imaginary part.
    // - if there are SYMSYM only, the last topo of fiels remains Real while I will have a 
    //   complex green function. Need to handle that in solve() ?

    //-------------------------------------------------------------------------
    /** - reset the topologies to real if needed, in order to prepare them for their execution  */
    //-------------------------------------------------------------------------
    for (int ip = 0; ip < 3; ip++) {
        if (!isGreen && planmap[ip]->isr2c() && topomap != NULL) {
            topomap[ip]->switch2real();
        }
    }
}

/**
 * @brief allocates the plans in planmap according to that computed during the dry run, see \ref _init_plansAndTopos
 * 
 * @param topo the map of topos that will be applied to data
 * @param planmap the list of plans that we need to allocate
 * @param data pointer to data (on which the FFTs will be applied in place)
 */
void Solver::_allocate_plans(const Topology *const topo[3], FFTW_plan_dim *planmap[3], double *data) {
    BEGIN_FUNC;

    for (int ip = 0; ip < 3; ip++) {
        // FLUPS_CHECK(!(planmap[ip]->isr2c() && topo[ip]->isComplex()), "The topology %d need to be reset to the state BEFORE the plan to have the correct sizes for allocation (isComplex=%d)", ip, topo[ip]->isComplex());
        planmap[ip]->allocate_plan(topo[ip], data);
    }
}

/**
 * @brief allocates memory depending on the requirements for the combination of topos in topo_hat
 * 
 * @param topo the map of successive topos that will be applied to data
 * @param data poiter to the pointer to data
 */
void Solver::_allocate_data(const Topology *const topo[3], double **data) {
    BEGIN_FUNC;
    //-------------------------------------------------------------------------
    /** - Sanity checks */
    //-------------------------------------------------------------------------
    FLUPS_CHECK((*data) == NULL, "Pointer has to be NULL for allocation");

    //-------------------------------------------------------------------------
    /** - Do the memory allocation */
    //-------------------------------------------------------------------------
    // the biggest size will be along the pencils
    size_t size_tot = 1;
    for (int id = 0; id < 3; id++)
        size_tot = std::max(topo[id]->locmemsize(), size_tot);

    FLUPS_INFO("Complex memory allocation, size = %ld", size_tot);
    (*data) = (double *)fftw_malloc(size_tot * sizeof(double));

    std::memset(*data,0, size_tot * sizeof(double));

    //-------------------------------------------------------------------------
    /** - Check memory alignement */
    //-------------------------------------------------------------------------
    FLUPS_CHECK(FLUPS_ISALIGNED(*data), "FFTW alignement not compatible with FLUPS_ALIGNMENT (=%d)", FLUPS_ALIGNMENT);
}

/**
 * @brief compute the Green's function
 * 
 * The Green function is always stored as a complex number (even if its complex part is 0).
 * This means that the all topos are turned to complex by this function (including the last one e.g.
 * for the case of a 3dirspectral).
 * 
 * @param topo the list of successive topos for the Green function
 * @param green ptr to the green function
 * @param planmap the list of successive maps to bring the Green function to full spectral
 * 
 * -----------------------------------
 * We do the following operations
 */
void Solver::_cmptGreenFunction(Topology *topo[3], double *green, FFTW_plan_dim *planmap[3]) {
    BEGIN_FUNC;

    //-------------------------------------------------------------------------
    /** - get the direction where we need to do spectral diff and count them */
    //-------------------------------------------------------------------------
    bool isSpectral[3] = {false};

    double hfact[3];    // multiply the index by this factor to obtain the position (1/2/3 corresponds to x/y/z )
    double kfact[3];    // multiply the index by this factor to obtain the wave number (1/2/3 corresponds to x/y/z )
    double koffset[3];  // add this to the index to obtain the wave number (1/2/3 corresponds to x/y/z )
    double symstart[3];
    double epsilon = _alphaGreen * _hgrid[0]; //the smoothing length scale of the HEJ kernels

    if ((_typeGreen == HEJ_2 || _typeGreen == HEJ_4 || _typeGreen == HEJ_6) && (_hgrid[0] != _hgrid[1] || _hgrid[1] != _hgrid[2])) {
        FLUPS_ERROR("You are trying to use a regularized kernel while not having dx=dy=dz.");
    }

    for (int ip = 0; ip < 3; ip++) {
        const int dimID = planmap[ip]->dimID();
        // get usefull datas
        isSpectral[dimID] = planmap[ip]->isSpectral();
        symstart[dimID]   = planmap[ip]->symstart();
        hfact[dimID]      = _hgrid[dimID];
        kfact[dimID]      = 0.0;
        koffset[dimID]    = 0.0;

        if (isSpectral[dimID]) {
            hfact[dimID]   = 0.0;
            kfact[dimID]   = planmap[ip]->kfact();
            koffset[dimID] = planmap[ip]->koffset();
        }
    }

    // count the number of spectral dimensions
    int nbr_spectral = 0;
    for (int id = 0; id < 3; id++)
        if (isSpectral[id])
            nbr_spectral++;

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //-------------------------------------------------------------------------
    /** - get the expression of Green in the full domain*/
    //-------------------------------------------------------------------------
    if (GREEN_DIM == 3) {
        if (nbr_spectral == 0) {
            FLUPS_INFO(">> using Green function type %d on 3 dir unbounded\n",_typeGreen);
            cmpt_Green_3D_3dirunbounded_0dirspectral(topo[0], hfact, symstart, green, _typeGreen, epsilon);
        } else if (nbr_spectral == 1) {
            FLUPS_INFO(">> using Green function of type %d on 2 dir unbounded - 1 dir spectral\n",_typeGreen);
            cmpt_Green_3D_2dirunbounded_1dirspectral(topo[0], hfact, kfact, koffset, symstart, green, _typeGreen, epsilon);
        } else if (nbr_spectral == 2) {
            FLUPS_INFO(">> using Green function of type %d on 1 dir unbounded - 2 dir spectral\n",_typeGreen);
            cmpt_Green_3D_1dirunbounded_2dirspectral(topo[0], hfact, kfact, koffset, symstart, green, _typeGreen, epsilon);
        } else if (nbr_spectral == 3) {
            FLUPS_INFO(">> using Green function of type %d on 3 dir spectral\n",_typeGreen);        
            cmpt_Green_3D_0dirunbounded_3dirspectral(topo[0], kfact, koffset, symstart, green, _typeGreen, epsilon);
        }
    }  else {
        FLUPS_ERROR("Sorry, the Green's function for 2D problems are not provided in this version.");
    }

    // dump the green func
    char msg[512];
    sprintf(msg, "green_%d%d%d_%dx%dx%d", planmap[0]->type(), planmap[1]->type(), planmap[2]->type(), topo[0]->nglob(0), topo[0]->nglob(1), topo[0]->nglob(2));
    hdf5_dump(topo[0], msg, green);

    //-------------------------------------------------------------------------
    /** - compute a symmetry and do the forward transform*/
    //-------------------------------------------------------------------------
    for (int ip = 0; ip < 3; ip++) {
        const int dimID = planmap[ip]->dimID();

        // go to the topology for the plan, if we are not already on it
        if (ip > 0) {
            _switchtopo_green[ip]->execute(green, FLUPS_FORWARD);
        }

        // execute the plan, if not already spectral
        if (!isSpectral[dimID]){
            _plan_green[ip]->execute_plan();
        }

        if (_plan_green[ip]->isr2c_green()) {
            topo[ip]->switch2complex();
        }
    }

    //-------------------------------------------------------------------------
    /** - scale the Green data using #_volfact */
    //-------------------------------------------------------------------------
    // - Explixitely destroying mode 0 ? no need to do that: we impose Green[0] is 0
    //   in full spectral.
    _scaleGreenFunction(topo[2], green, false);

    // complete the Green function in a very particular case
    if (GREEN_DIM == 3 && nbr_spectral == 1 && (_typeGreen==HEJ_2||_typeGreen==HEJ_4||_typeGreen==HEJ_6)) {
        int istart[3] = {0, 0, 0};
        int ishift[3] = {0, 0, 0};
        for (int ip = 0; ip < 3; ip++) {
            const int dimID = planmap[ip]->dimID();

            istart[dimID]  = isSpectral[dimID] ? 1 - _plan_green[2]->shiftgreen() : 0;  //avoid rewriting on the part of Green already computed (if there is a shiftgreen, we already skipped that part in the switchTopo)
            ishift[dimID]  = isSpectral[dimID] ? 0 : _plan_green[dimID]->shiftgreen();  //starting point for the computation of k (accounting for shiftGreen)
            kfact[dimID]   = planmap[ip]->kfact(); 
        }
        cmpt_Green_3D_0dirunbounded_3dirspectral(topo[2], kfact, koffset, symstart, green, _typeGreen, epsilon, istart, ishift);
    }
    hdf5_dump(topo[2], "green_h", green);
}

/**
 * @brief scales the Green's function given the #_volfact factor
 * 
 * @param topo the current topo
 * @param data the Green's function
 * @param killModeZero  specify if you want to kill what's in kx=ky=kz=0
 */
void Solver::_scaleGreenFunction(const Topology *topo, opt_double_ptr data, const bool killModeZero) {
    BEGIN_FUNC;
    // the symmetry is done along the fastest rotating index
    const int ax0 = topo->axis();
    const int ax1 = (ax0 + 1) % 3;
    const int ax2 = (ax0 + 2) % 3;

    for (int i2 = 0; i2 < topo->nloc(ax2); i2++) {
        for (int i1 = 0; i1 < topo->nloc(ax1); i1++) {
            size_t id = localindex_ao(0, i1, i2, topo);
            for (int i0 = 0; i0 < topo->nloc(ax0) * topo->nf(); i0++) {
                data[id + i0] = data[id + i0] * _volfact;
            }
        }
    }

    if (killModeZero) {
        int istart[3];
        get_istart_glob(istart, topo);
        if (istart[ax0] == 0 && istart[ax1] == 0 && istart[ax2] == 0) {
            for (int i0 = 0; i0 < topo->nf(); i0++) {
                data[i0] = 0.0;
            }
            FLUPS_INFO("Imposing Green's function mode 0 to be 0.");
        }
    }
}

/**
 * @brief Solve the Poisson equation
 * 
 * @param field 
 * @param rhs 
 * 
 * -----------------------------------------------
 * We perform the following operations:
 */
void Solver::solve(const Topology *topo, double *field, double *rhs, const SolverType type) {
    BEGIN_FUNC;
    //-------------------------------------------------------------------------
    /** - sanity checks */
    //-------------------------------------------------------------------------
    FLUPS_CHECK(field != NULL, "field is NULL");
    FLUPS_CHECK(rhs != NULL, "rhs is NULL");
    FLUPS_CHECK(FLUPS_ISALIGNED(field), "pointer no aligned to FLUPS_ALIGNMENT (=%d)", FLUPS_ALIGNMENT);
    FLUPS_CHECK(FLUPS_ISALIGNED(rhs), "pointer no aligned to FLUPS_ALIGNMENT (=%d)", FLUPS_ALIGNMENT);

    opt_double_ptr       myfield = field;
    opt_double_ptr       mydata  = _data;
    const opt_double_ptr myrhs   = rhs;

    _prof->create("solve_total");
    _prof->start("solve_total");

    //-------------------------------------------------------------------------
    /** - clean the data memory */
    //-------------------------------------------------------------------------
    // reset at the max size
    size_t size_tot = topo->locmemsize();
    for (int id = 0; id < 3; id++)
        size_tot = std::max(_topo_hat[id]->locmemsize(), size_tot);
    std::memset(mydata, 0, sizeof(double) * size_tot);

    //-------------------------------------------------------------------------
    /** - copy the rhs in the correct order */
    //-------------------------------------------------------------------------
    // FLUPS_INFO("------------------------------------------");
    // FLUPS_INFO("## memory information\n")
    // FLUPS_INFO("- size field   = %d %d %d", _size_field[0], _size_field[1], _size_field[2]);
    // FLUPS_INFO("- size hat     = %d %d %d", _size_hat[0], _size_hat[1], _size_hat[2]);
    // FLUPS_INFO("- dim order    = %d %d %d", _dimorder[0], _dimorder[1], _dimorder[2]);
    // FLUPS_INFO("- field start  = %d %d %d", _fieldstart[0], _fieldstart[1], _fieldstart[2]);
    // FLUPS_INFO("- dim multfact = %d %d %d", _dim_multfact[0], _dim_multfact[1], _dim_multfact[2]);
    // FLUPS_INFO("- offset       = %ld", _offset);
    // FLUPS_INFO("------------------------------------------");

    int ax0 = topo->axis();
    int ax1 = (ax0 + 1) % 3;
    int ax2 = (ax0 + 2) % 3;

    FLUPS_CHECK(!topo->isComplex(), "The RHS topology cannot be complex");

    _prof->create("solve_copy");
    _prof->start("solve_copy");
    for (int i2 = 0; i2 < topo->nloc(ax2); i2++) {
        for (int i1 = 0; i1 < topo->nloc(ax1); i1++) {
            // comnpute the index permutation
            size_t id= localindex_ao(0, i1, i2, topo);
            // the last direction is continuous in memory
            for (int i0 = 0; i0 < topo->nloc(ax0); i0++) { 
                mydata[id] = myrhs[id];
                id ++;
            }
        }
    }

    _prof->stop("solve_copy");

#ifdef DUMP_H5
    hdf5_dump(topo, "rhs", mydata);
#endif
    //-------------------------------------------------------------------------
    /** - go to Fourier */
    //-------------------------------------------------------------------------
    _prof->create("solve_fftw");
    _prof->create("solve_reorder");
    for (int ip = 0; ip < 3; ip++) {
        // go to the correct topo
        
        _prof->start("solve_reorder");
        _switchtopo[ip]->execute(mydata, FLUPS_FORWARD);
        _prof->stop("solve_reorder");
        // run the FFT
        _prof->start("solve_fftw");
        _plan_forward[ip]->execute_plan();
        _prof->stop("solve_fftw");
        // get if we are now complex
        if (_plan_forward[ip]->isr2c()) {
            _topo_hat[ip]->switch2complex();
        }
    }
#ifdef DUMP_H5
    hdf5_dump(_topo_hat[2], "rhs_h", mydata);
#endif
    //-------------------------------------------------------------------------
    /** - Perform the magic */
    //-------------------------------------------------------------------------
    
    _prof->create("solve_domagic");
    _prof->start("solve_domagic");
    if (type == SRHS) {
        if (!_topo_hat[2]->isComplex()) {
            //-> there is only the case of 3dirSYM in which we could stay real for the whole process
            if (_nbr_imult == 0)
                dothemagic_rhs_real();
            else
                FLUPS_CHECK(false, "the number of imult = %d is not supported", _nbr_imult);
        } else {
            if (_nbr_imult == 0)
                dothemagic_rhs_complex_nmult0();
            // else if(_nbr_imult == 1) dothemagic_rhs_complex_nmult1();
            // else if(_nbr_imult == 2) dothemagic_rhs_complex_nmult2();
            // else if(_nbr_imult == 3) dothemagic_rhs_complex_nmult3();
            else
                FLUPS_CHECK(false, "the number of imult = %d is not supported", _nbr_imult);
        }
    } else {
        FLUPS_CHECK(false, "type of solver %d not implemented", type);
    }

    _prof->stop("solve_domagic");
    // io if needed
    hdf5_dump(_topo_hat[2], "sol_h", mydata);

    //-------------------------------------------------------------------------
    /** - go back to reals */
    //-------------------------------------------------------------------------
    for (int ip = 2; ip >= 0; ip--) {
        _prof->start("solve_fftw");
        _plan_backward[ip]->execute_plan();
        _prof->stop("solve_fftw");
        // get if we are now complex
        if (_plan_forward[ip]->isr2c()) {
            _topo_hat[ip]->switch2real();
        }
        _prof->start("solve_reorder");
        _switchtopo[ip]->execute(mydata, FLUPS_BACKWARD);
        _prof->stop("solve_reorder");
    }

    //-------------------------------------------------------------------------
    /** - copy the solution in the field */
    //-------------------------------------------------------------------------
    _prof->start("solve_copy");
    for (int i2 = 0; i2 < topo->nloc(ax2); i2++) {
        for (int i1 = 0; i1 < topo->nloc(ax1); i1++) {
            // comnpute the index permutation
            size_t id= localindex_ao(0, i1, i2, topo);
            // the last direction is continuous in memory
            for (int i0 = 0; i0 < topo->nloc(ax0); i0++) { 
                myfield[id] = mydata[id];
                id ++;
            }
        }
    }
    _prof->stop("solve_copy");
    // io if needed
    hdf5_dump(topo, "sol", myfield);
    // stop the whole timer
    _prof->stop("solve_total");

    _prof->disp();
}

/**
 * @brief perform the convolution for real to real cases
 * 
 */
void Solver::dothemagic_rhs_real() {
    BEGIN_FUNC;

    FLUPS_CHECK(_topo_hat[2]->axis() == _topo_green[2]->axis(), "field and Green must have the same axis");
    FLUPS_CHECK(!_topo_hat[2]->isComplex() && !_topo_green[2]->isComplex(), "field and Green must be in real topos");

    opt_double_ptr       mydata  = _data;
    const opt_double_ptr mygreen = _green;

    const int ax0 = _topo_hat[2]->axis();
    const int ax1 = (ax0 + 1) % 3;
    const int ax2 = (ax0 + 2) % 3;
    
    const int shiftgreen = _plan_green[2]->shiftgreen(); 
    //we might still need to get the right part of the green function in odd cases, if this was not done befire by a switchtopo

    for (int i2 = 0; i2 < _topo_hat[2]->nloc(ax2); ++i2) {
        for (int i1 = 0; i1 < _topo_hat[2]->nloc(ax1); ++i1) {
            size_t id       = localindex_ao(0, i1, i2, _topo_hat[2]);
            size_t id_green = localindex_ao(shiftgreen, i1, i2, _topo_green[2]);

            for (int i0 = 0; i0 < _topo_hat[2]->nloc(ax0); ++i0) {
                mydata[id+i0] *= _normfact * mygreen[id_green+i0];
            }
        }
    }
}

/**
 * @brief Do the convolution between complex data and complex Green's function in spectral space
 * 
 */
void Solver::dothemagic_rhs_complex_nmult0() {
    BEGIN_FUNC;

    // printf("doing the dothemagic_rhs_complex_nmult0");

    FLUPS_CHECK(_topo_hat[2]->axis() == _topo_green[2]->axis(), "field and Green must have the same axis");

    opt_double_ptr       mydata  = _data;
    const opt_double_ptr mygreen = _green;

    const int ax0 = _topo_hat[2]->axis();
    const int ax1 = (ax0 + 1) % 3;
    const int ax2 = (ax0 + 2) % 3;

    const int shiftgreen = _plan_green[2]->shiftgreen(); 

    for (int i2 = 0; i2 < _topo_hat[2]->nloc(ax2); ++i2) {
        for (int i1 = 0; i1 < _topo_hat[2]->nloc(ax1); ++i1) {
            size_t id       = localindex_ao(0, i1, i2, _topo_hat[2]);
            size_t id_green = localindex_ao(shiftgreen, i1, i2, _topo_green[2]);

            for (int i0 = 0; i0 < _topo_hat[2]->nloc(ax0); ++i0) {
                const double a = mydata[id + 0];
                const double b = mydata[id + 1];
                const double c = mygreen[id_green + 0];
                const double d = mygreen[id_green + 1];

                // update the values
                mydata[id + 0] = _normfact * (a * c - b * d);
                mydata[id + 1] = _normfact * (a * d + b * c);

                id += 2;
                id_green += 2;
            }
        }
    }
}

/**
 * @brief Do the convolution between complex data and complex Green's function and multiply by (-i)
 * 
 */
void Solver::dothemagic_rhs_complex_nmult1() {
    BEGIN_FUNC;
    FLUPS_CHECK(false, "not implemented yet");
}

/**
 * @brief Do the convolution between complex data and complex Green's function and multiply by (-1)
 * 
 */
void Solver::dothemagic_rhs_complex_nmult2() {
    BEGIN_FUNC;
    FLUPS_CHECK(false, "not implemented yet");
}

/**
 * @brief Do the convolution between complex data and complex Green's function and multiply by (i)
 * 
 */
void Solver::dothemagic_rhs_complex_nmult3() {
    BEGIN_FUNC;
    FLUPS_CHECK(false, "not implemented yet");
}
