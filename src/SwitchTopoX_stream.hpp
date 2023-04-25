/**
 * @file SwitchTopoX_stream.hpp
 * @copyright Copyright (c) Université catholique de Louvain (UCLouvain), Belgique 
 *      See LICENSE file in top-level directory
*/
#ifndef SRC_SWITCHTOPOX_STREAM_HPP_
#define SRC_SWITCHTOPOX_STREAM_HPP_

#include "SwitchTopoX.hpp"

#ifdef MPICH
#define M_FLUPS_STREAM 1
#else
#define M_FLUPS_STREAM 0
#endif

class SwitchTopoX_stream : public SwitchTopoX {
    int* i2o_send_order_ = nullptr;  //!< order in which to perform the send for input to output
    int* o2i_send_order_ = nullptr;  //!< order in which to perform the send for output to input
    int* completed_id_   = nullptr;  //!< array used by the Wait/Test in the non-blocking comms
    int* recv_order_     = nullptr;  //!< array used by the Wait/Test in the non-blocking comms

#if (M_FLUPS_STREAM)
    int n_streams_ ;
    MPIX_Stream* streams_ = nullptr;  //!< streams to be used for the communications
#endif
    int  n_comm_;
    MPI_Comm*    comm_ = nullptr;     //!< communicator associated to each threads

    MPI_Comm shared_comm_ = MPI_COMM_NULL;  //<! communicators with ranks on the same node

    MPI_Request* send_rqst_;  //<! storage for send requests
    MPI_Request* recv_rqst_;  //<! storage for recv requests

   public:
    explicit SwitchTopoX_stream(const Topology* topo_in, const Topology* topo_out, const int shift[3], H3LPR::Profiler* prof);
    ~SwitchTopoX_stream();

    virtual bool need_send_buf() const override { return false; };
    virtual bool need_recv_buf() const override { return true; };

    /**
     * @brief streams allow us to overlap communications with computations
     */
    virtual bool overlap_comm() { return true; }

    virtual void setup_buffers(opt_double_ptr sendData, opt_double_ptr recvData) override;
    virtual void execute(opt_double_ptr data, const int sign) const override;
    virtual void disp() const override;
};
#endif
