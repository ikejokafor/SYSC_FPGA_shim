#pragma once


#include "FPGA_shim.hpp"
#include "syscNetProto.hpp"
#include <cmath>


#define DMA_BUFFER_ALIGNMENT                        64
#define my_aligned_malloc(pp, alignment, size)      posix_memalign(pp, alignment, size)
#define my_aligned_free(p)                          free(p)
#define ALGN_PYLD_SZ(sz, algn)                      ((uint64_t)(ceil((float)sz / (float)algn) * (float)algn))


class SYSC_FPGA_shim_pyld : public Accel_Payload
{
    public:
        SYSC_FPGA_shim_pyld() { }
        ~SYSC_FPGA_shim_pyld() { }
		void serialize() { }
        void deserialize() { }
};


class SYSC_FPGA_hndl : public FPGA_hndl
{
    public:
        SYSC_FPGA_hndl(int memStartOft = 0);
        ~SYSC_FPGA_hndl();
        int hardware_init();
        int software_init(soft_init_param* param);
        uint64_t allocate(Accel_Payload* pyld, uint64_t size);
        void deallocate(Accel_Payload* pyld);
        uint64_t wait_sysC_FPGAconfig();        
        int wr_sysC_FPGAconfig(Accel_Payload* pyld);
        uint64_t waitConfig();
        int wrConfig(Accel_Payload* pyld);
        int rdConfig(Accel_Payload* pyld);
        void waitParam(uint64_t& addr, int& size);
        int wrParam(Accel_Payload* pyld);
        int waitStart();
        int sendStart();
        int waitComplete();
        int sendComplete();
        int getOutput(Accel_Payload* pyld);
        int sendOutput(Accel_Payload* pyld);

        int m_socket;
};
