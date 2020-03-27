#pragma once


#include "FPGA_shim.hpp"
#include "MyNetProto.hpp"


class SYSC_FPGA_hndl : public FPGA_hndl
{
    public:
        SYSC_FPGA_hndl();
        ~SYSC_FPGA_hndl();
        int hardware_init();
        int software_init();
        uint64_t waitConfig();
        int setConfig(Accel_Payload* pyld);
        uint64_t waitParam();
        int setParam(Accel_Payload* pyld);
        int waitStart();
        int sendStart();
        int waitComplete();
        int sendComplete();
        int getOutput(Accel_Payload* pyld);
        
        int m_socket;
};
