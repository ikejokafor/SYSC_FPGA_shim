#pragma once


#include "FPGA_shim.hpp"
#include "myNetProto.hpp"
#include <cmath>


#define DMA_BUFFER_ALIGNMENT                        64
#define my_aligned_malloc(pp, alignment, size)      posix_memalign(pp, alignment, size)
#define my_aligned_free(p)                          free(p)
#define ALGN_PYLD_SZ(sz, algn)                      (ceil(sz / algn) * algn)


class SYSC_FPGA_hndl : public FPGA_hndl
{
    public:
        SYSC_FPGA_hndl(int memStartOft = 0);
        ~SYSC_FPGA_hndl();
        int hardware_init();
        int software_init();
        uint64_t allocate(Accel_Payload* pyld, int size);
        void deallocate(Accel_Payload* pyld);
        uint64_t waitConfig();
        int wrConfig(Accel_Payload* pyld);
        uint64_t waitParam();
        int wrParam(Accel_Payload* pyld);
        int waitStart();
        int sendStart();
        int waitComplete();
        int sendComplete();
        int getOutput(Accel_Payload* pyld);
        int sendOutput(Accel_Payload* pyld);

        int m_socket;
};
