#pragma once


#include "FPGA_shim.hpp"
#include "syscNetProto.hpp"
#include <cmath>


#define DMA_BUFFER_ALIGNMENT                        64
#define AXI_BUFFER_ALIGNMENT                        64
#define my_aligned_malloc(pp, alignment, size)      posix_memalign(pp, alignment, size)
#define my_aligned_free(p)                          free(p)
#define ALGN_PYLD_SZ(sz, algn)                      ((uint64_t)(ceil((float)sz / (float)algn) * (float)algn))


class SYSC_FPGA_shim_pyld : public Accel_Payload
{
    public:
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			constructor for systemC remote device payload
         *      \return         returns pointer to Accelerator payload object
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        SYSC_FPGA_shim_pyld() { }


        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			destructor for systemC remote device payload
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        ~SYSC_FPGA_shim_pyld() { }
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			serializes payload to sent ot remote device
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
		void serialize() { }


        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			deserializes payload from remote device
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        void deserialize() { }
};


class SYSC_FPGA_hndl : public FPGA_hndl
{
    public:
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			constructor for systemC remote device payload
         *      \param[in]      memStartOft offset to start address in remote memory space
         *      \return         returns pointer to SystemC FPGA handle object
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        SYSC_FPGA_hndl(uint64_t memStartOft = 0);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			destructor for SystemC FPGA handle object
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------         
        ~SYSC_FPGA_hndl();
        

        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			initializes remote hardware for processing
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int hardware_init();
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			initializes software shim on host device for processing
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int software_init(soft_init_param* param);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			allocates host and remote memory space for remote device payload
         *		\param[in]	    pyld object holding metadata of payload
         *		\param[in]	    size size of payload
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        uint64_t allocate(Accel_Payload* pyld, uint64_t size);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			deallocates host and remote memory space for remote device payload
         *		\param[in]	    pyld object holding metadata of payload
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        void deallocate(Accel_Payload* pyld);
        
        
        
        uint64_t wait_sysC_FPGAconfig();        
        int wr_sysC_FPGAconfig(Accel_Payload* pyld);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			remote rountine used to wait for remote device configuration (Simulation Only)
         *		\return			address to payload on success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        uint64_t waitConfig();
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			writes remote device configuration data to remote device
         *		\param[in]	    pyld object holding metadata of payload
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int wrConfig(Accel_Payload* pyld);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			reads remote device configuration data from remote device
         *		\param[in]	    pyld object holding metadata of payload
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int rdConfig(Accel_Payload* pyld);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			remote rountine used to wait for remote device paramater data (Simulation Only)
         *      \param[in,out]  addr address to place payload information
         *      \param[in]      size size of payload
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        void waitParam(uint64_t& addr, int& size);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			writes payload holding remote device paramater data to remote device
         *		\param[in]	    pyld object holding metadata of payload
         *      \return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int wrParam(Accel_Payload* pyld);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			remote rountine used to wait for host signal for remote device to start processing (Simulation Only)
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int waitStart();
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			routine for host devie to signal remote device to start processing
         *		\param[in]	    pyld object holding information to start remote device
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int sendStart(Accel_Payload* pyld);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			routine used by host device to wait for signal from remote device that processing is complete
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int waitComplete();
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			routine used by remote device signal host device that processing is complete (Simulation Only)
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int sendComplete();
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			routine used by host device to receive output data from host device
         *		\param[in,out]	pyld object holding information to hold remote device output
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------       
        int getOutput(Accel_Payload* pyld);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			routine used by remote device to send output data to host device (Simulation Only)
         *		\param[in,out]	pyld object holding information to hold remote device output
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int sendOutput(Accel_Payload* pyld);
        
        
        // ----------------------------------------------------------------------------------------------------------------------------------------------
        /**
         *		\brief			host routine for reseting remote memory address space distribution
         *		\return			0 success, 1 failure
         */
        // ----------------------------------------------------------------------------------------------------------------------------------------------        
        int resetMemSpace();
        
        
        int m_socket; /*<! socket for remote to host communication */
};
