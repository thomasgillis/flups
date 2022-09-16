#!/bin/bash -l
#---------------------------------------------------------------------------------------
#               Case dependent info
#---------------------------------------------------------------------------------------
## Version: 
# All to all                 = a2a 
# Non blocking               = nb 
# MPI_datatypes              = isr
# First version all to all   = dprec_a2a 
# First version non blocking = dprec_nb
export CODE_VERSION='nb isr a2a'

## Kernels : 
# CHAT_2 = 0, /**< @brief quadrature in zero, order 2, Chatelain et al. (2010) */
# LGF_2  = 1, /**< @brief Lattice Green's function, order 2, Gillis et al. (2018)*/
# HEJ_2  = 2, /**< @brief regularized, order 2, Hejlesen et al. (2015)*/
# HEJ_4  = 3, /**< @brief regularized, order 4, Hejlesen et al. (2015)*/
# HEJ_6  = 4, /**< @brief regularized, order 6, Hejlesen et al. (2015)*/
# HEJ_8  = 5, /**< @brief regularized, order 8, Hejlesen et al. (2015)*/
# HEJ_10 = 6, /**< @brief regularized, order 10, Hejlesen et al. (2015)*/
# HEJ_0  = 7, /**< @brief Fourier cutoff, spectral-like, Hejlesen et al. (2019)*/
#export CODE_KERNEL='0 1 2 3 4 5 6 7 8'
export CODE_KERNEL='0'

## Center ; 
# Node-centred = 0 
# Cell-centred = 1
export CODE_CENTER='0'
export NPCPUS=96

## Compile the librairies  
export H3LPR_CXXFLAGS='-g -O3 -march=native -fopenmp -DNDEBUG -flto'
export H3LPR_LDFLAGS='-fopenmp -rdynamic -ldl -flto'

export FLUPS_AR='gcc-ar'
export FLUPS_CCFLAGS='-g -O3 -std=c99 -march=native -DNDEBUG -flto'
export FLUPS_CXXFLAGS='-g -O3 -std=c++17 -march=native -DNDEBUG -flto'
export FLUPS_LDFLAGS='-fopenmp -flto'
export FLUPS_OPTS=''

# ..................................................................
## Compile bash options
export COMPILE_NNODE=1
export COMPILE_NTASK=4
export COMPILE_TIME='00:10:00'

# ..................................................................
## Kernel bash options
export KERNEL_TIME='02:30:00'

# ..................................................................
#export CLUSTER='vega'
export CLUSTER='meluxina'

export LAUNCH_COMMAND='mpirun'

# ..................................................................
#---------------------------------------------------------------------------------------
#               MELUXINA
#---------------------------------------------------------------------------------------
if [[ ${CLUSTER} == "meluxina" ]]; then
    #export BASE_SCRATCHDIR=/project/scratch/p200053
    export BASE_SCRATCHDIR=/project/scratch/p200067
    ## .................................
    ## NEEDED dir
    export HOME_FLUPS=${HOME}/flups/
    export HOME_H3LPR=${HOME}/h3lpr/

    export FFTW_DIR=${EBROOTFFTW}
    export HDF5_DIR=${EBROOTHDF5}

    ## .................................
    ## MPI information
    #export MPI_VERSION=4.1.4
    export MPI_VERSION=4.1.3
    export MPICC='mpicc'
    export MPICXX='mpic++'
    export NPROC_NODES=128

    ## BASH OPTIONS -- GENERAL
    export PARTITION='cpu'
    #export ACCOUNT='p200053'
    export ACCOUNT='p200067'

    ## BASH OPTIONS -- Compilation job 
    export COMPILE_CLUSTER_SPEC='--qos=short --mem=491520'

    ## .................................
    ## BASH OPTIONS -- kernel job 
    export KERNEL_CLUSTER_SPEC='--qos=default --mem=491520'

    # export UCX_TLS=self,shm,rc,dc
    # export OMPI_MCA_pml=ucx
    # export OMPI_MCA_osc=ucx
    # export UCX_UD_TX_QUEUE_LEN=4096

fi


#---------------------------------------------------------------------------------------
#               VEGA
#---------------------------------------------------------------------------------------
if [[ ${CLUSTER} == "vega" ]]; then
    export BASE_SCRATCHDIR=/ceph/hpc/data/b2203-024-users/
    ## .................................
    ## NEEDED dir
    export HOME_FLUPS=${HOME}/flups/
    export HOME_H3LPR=${HOME}/h3lpr/

    export FFTW_DIR=${EBROOTFFTW}
    export HDF5_DIR=${EBROOTHDF5}

    ## .................................
    ## MPI information
    export MPI_VERSION=4.1.3
    export MPICC='mpicc'
    export MPICXX='mpic++'
    export NPROC_NODES=128

    ## BASH OPTIONS -- GENERAL
    export PARTITION='cpu'
    export ACCOUNT='b2203-024-users'

    ## BASH OPTIONS -- Compilation job 
    export COMPILE_CLUSTER_SPEC=''

    ## .................................
    ## BASH OPTIONS -- kernel job 
    export KERNEL_CLUSTER_SPEC=''

    ## .................................
    # export UCX_TLS=ud,sm
    export UCX_TLS=self,shm,rc,dc
    export OMPI_MCA_pml=ucx
    export OMPI_MCA_osc=ucx

fi

#---------------------------------------------------------------------------------------
#               LUMI
#---------------------------------------------------------------------------------------
if [[ ${CLUSTER} == "lumi" ]]; then
    export BASE_SCRATCHDIR=/scratch/project_465000098/
    ## .................................
    ## NEEDED dir
    export HOME_FLUPS=${HOME}/flups/
    export HOME_H3LPR=${HOME}/h3lpr/

    export FFTW_DIR=${FFTW_DIR}
    export HDF5_DIR=${HDF5_DIR}

    ## .................................
    ## MPI information
    export MPI_VERSION=8.1.17
    export MPICC='cc'
    export MPICXX='CC'
    export NPROC_NODES=128

    ## BASH OPTIONS -- GENERAL
    export PARTITION='standard'
    export ACCOUNT='project_465000098'

    ## BASH OPTIONS -- Compilation job 
    export COMPILE_CLUSTER_SPEC=''

    ## .................................
    ## BASH OPTIONS -- kernel job 
    export KERNEL_CLUSTER_SPEC=''

    ## .................................
    export LCOMMAND='srun'
fi

# Performance 
# source ${HOME_FLUPS}/samples/validation/run/benchmark_workflow_weak.sh
# source ${HOME_FLUPS}/samples/validation/run/benchmark_workflow_strong.sh

# Validation 
# source ${HOME_FLUPS}/samples/validation/run/benchmark_workflow_validation.sh

# Exploration
# source ${HOME_FLUPS}/samples/validation/run/benchmark_workflow_order_list.sh
# source ${HOME_FLUPS}/samples/validation/run/benchmark_workflow_rolling_rank.sh
# source ${HOME_FLUPS}/samples/validation/run/benchmark_workflow_send_batch.sh
# source ${HOME_FLUPS}/samples/validation/run/benchmark_workflow_mpi_alloc.sh
source ${HOME_FLUPS}/samples/validation/run/benchmark_workflow_xplore_strong.sh
