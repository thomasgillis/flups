/**
 * @file SwitchTopoX_nb.hpp
 * @copyright Copyright (c) Université catholique de Louvain (UCLouvain), Belgique 
 *      See LICENSE file in top-level directory
*/
#ifndef SRC_SWITCHTOPOX_PART_HPP_
#define SRC_SWITCHTOPOX_PART_HPP_

#include "SwitchTopoX.hpp"

class SwitchTopoX_part : public SwitchTopoX {
    int*     completed_id_ = nullptr;        //!< array used by the Wait/Test in the non-blocking comms
    int*     recv_order_   = nullptr;        //!< array used by the Wait/Test in the non-blocking comms
    int* i2o_send_order_ = nullptr;
    int* o2i_send_order_ = nullptr;

    MPI_Request* i2o_send_rqst_ = NULL;  //!< MPI send requests
    MPI_Request* i2o_recv_rqst_ = NULL;  //!< MPI recv requests
    MPI_Request* o2i_send_rqst_ = NULL;  //!< MPI send requests
    MPI_Request* o2i_recv_rqst_ = NULL;  //!< MPI recv requests

   public:
    explicit SwitchTopoX_part(const Topology* topo_in, const Topology* topo_out, const int shift[3], H3LPR::Profiler* prof);
    ~SwitchTopoX_part();

    virtual bool need_send_buf()const override{return true;};
    virtual bool need_recv_buf()const override{return true;};


    virtual void setup_buffers(opt_double_ptr sendData, opt_double_ptr recvData) override;
    virtual void execute(opt_double_ptr data, const int sign) const override;
    virtual void disp() const override;
};

#endif
