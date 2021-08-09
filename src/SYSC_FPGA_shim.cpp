#include "SYSC_FPGA_shim.hpp"
using namespace std;


SYSC_FPGA_hndl::SYSC_FPGA_hndl(int memStartOft) : FPGA_hndl()
{
    m_remAddrOfst = memStartOft;
}


SYSC_FPGA_hndl::~SYSC_FPGA_hndl()
{

}


int SYSC_FPGA_hndl::hardware_init()
{
    if((m_socket = client_connect()) == -1)
    {
        return -1;
    }
    msgHeader_t hdr;
    hdr.msgType = CONNECT_HARDWARE;
    hdr.pyld = false;	
    hdr.length = 0;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent CONNECT_HARDWARE" << endl;
    return 0;
}


int SYSC_FPGA_hndl::software_init(soft_init_param* param)
{
    if((m_socket = client_connect()) == -1)
    {
        return -1;
    }
    msgHeader_t hdr;
    hdr.msgType = CONNECT_SOFTWARE;
	hdr.pyld = false;
    hdr.length = 0;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent CONNECT_SOFTWARE" << endl;
    return 0;
}


uint64_t SYSC_FPGA_hndl::allocate(Accel_Payload* pyld, uint64_t size)
{
    uint64_t aligned_sz = ALGN_PYLD_SZ(size, DMA_BUFFER_ALIGNMENT);
    if(my_aligned_malloc((void**)&pyld->m_buffer, DMA_BUFFER_ALIGNMENT, aligned_sz))
    {
        printf("[SYSC_FPGA_SHIM]: Failed to allocate 0x%X B for DMA buffer.", size);
        return -1;
    }
    printf("[SYSC_FPGA_SHIM]: Allocated %llud bytes buffer space on Remote Memory\n", aligned_sz);
    pyld->m_size            = aligned_sz;
    pyld->m_remAddress      = (uint64_t)m_remAddrOfst;
    m_remAddrOfst           += aligned_sz;
    return                  (uint64_t)pyld->m_buffer;
}


void SYSC_FPGA_hndl::deallocate(Accel_Payload* pyld)
{
    my_aligned_free(pyld->m_buffer);
    pyld->m_buffer          = NULL;
    pyld->m_size            = 0;
    pyld->m_remAddress      = -1;
}


uint64_t SYSC_FPGA_hndl::wait_sysC_FPGAconfig()
{
	// Wait for ACCL_SYSC_FPGA_BGN_CFG
    msgHeader_t hdr;
	hdr.pyld = false;
	hdr.length = 0;
    wait_message(m_socket, &hdr, nullptr, ACCL_SYSC_FPGA_BGN_CFG);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_SYSC_FPGA_BGN_CFG" << endl;
	// Wait for ACCL_CFG_PYLD(s)
    hdr.msgType = ACCL_CFG_PYLD;
	hdr.pyld = true;
	int totalBytes = hdr.length;
    int remBytes = hdr.length;
    int bufIdx = 0;
    uint8_t* buf = new uint8_t[hdr.length];
    do
    {
        int rb = wait_message(m_socket, &hdr, &buf[bufIdx], ACCL_CFG_PYLD);
        bufIdx += rb;
        remBytes -= rb;
		cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_CFG_PYLD - " << bufIdx << "/" << totalBytes << " bytes" << endl;
    } while(remBytes > 0);
    // Wait for ACCL_SYSC_FPGA_END_CFG
	hdr.pyld = false;
	hdr.length = 0;
    wait_message(m_socket, &hdr, nullptr, ACCL_SYSC_FPGA_END_CFG);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_SYSC_FPGA_END_CFG" << endl;
    return (uint64_t)buf;    
}


int SYSC_FPGA_hndl::wr_sysC_FPGAconfig(Accel_Payload* pyld)
{
	// Send ACCL_SYSC_FPGA_BGN_CFG
	msgHeader_t hdr;
	hdr.msgType = ACCL_SYSC_FPGA_BGN_CFG;
	hdr.pyld = false;
	hdr.length = pyld->m_size;
	send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_SYSC_FPGA_BGN_CFG" << endl;
	// Send ACCL_CFG_PYLD(s)
    hdr.msgType = ACCL_CFG_PYLD;
	hdr.pyld = true;
	int totalBytes = hdr.length;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        int rb = send_message(m_socket, &hdr, &pyld_ptr[pyldIdx]);
        pyldIdx += rb;
        remBytes -= rb;
		cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_CFG_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
    } while(remBytes > 0);
	// Send ACCL_SYSC_FPGA_END_CFG
	hdr.msgType = ACCL_SYSC_FPGA_END_CFG;
	hdr.pyld = false;
	hdr.length = 0;
	send_message(m_socket, &hdr, nullptr);
	cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_SYSC_FPGA_END_CFG" << endl;
	return 0;    
}

uint64_t SYSC_FPGA_hndl::waitConfig()
{
	// Wait for ACCL_BGN_CFG
    msgHeader_t hdr;
	hdr.pyld = false;
	hdr.length = 0;
    wait_message(m_socket, &hdr, nullptr, ACCL_BGN_CFG);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_BGN_CFG" << endl;
	// Wait for ACCL_CFG_PYLD(s)
    hdr.msgType = ACCL_CFG_PYLD;
	hdr.pyld = true;
	int totalBytes = hdr.length;
    int remBytes = hdr.length;
    int bufIdx = 0;
    uint8_t* buf = new uint8_t[hdr.length];
    do
    {
        int rb = wait_message(m_socket, &hdr, &buf[bufIdx], ACCL_CFG_PYLD);
        bufIdx += rb;
        remBytes -= rb;
		cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_CFG_PYLD - " << bufIdx << "/" << totalBytes << " bytes" << endl;
    } while(remBytes > 0);
    // Wait for ACCL_END_CFG
	hdr.pyld = false;
	hdr.length = 0;
    wait_message(m_socket, &hdr, nullptr, ACCL_END_CFG);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_END_CFG" << endl;
    return (uint64_t)buf;
}


int SYSC_FPGA_hndl::wrConfig(Accel_Payload* pyld)
{
	// Send ACCL_BGN_CFG
	msgHeader_t hdr;
	hdr.msgType = ACCL_BGN_CFG;
	hdr.pyld = false;
	hdr.length = pyld->m_size;
	send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_BGN_CFG" << endl;
	// Send ACCL_CFG_PYLD(s)
    hdr.msgType = ACCL_CFG_PYLD;
	hdr.pyld = true;
	int totalBytes = hdr.length;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        int rb = send_message(m_socket, &hdr, &pyld_ptr[pyldIdx]);
        pyldIdx += rb;
        remBytes -= rb;
		cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_CFG_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
    } while(remBytes > 0);
	// Send ACCL_END_CFG
	hdr.msgType = ACCL_END_CFG;
	hdr.pyld = false;
	hdr.length = 0;
	send_message(m_socket, &hdr, nullptr);
	cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_END_CFG" << endl;
	return 0;
}


int SYSC_FPGA_hndl::rdConfig(Accel_Payload* pyld)
{
    return 0;
}


void SYSC_FPGA_hndl::waitParam(uint64_t& addr, int& size)
{
	// Wait for ACCL_BGN_PARAM
    msgHeader_t hdr;
	hdr.pyld = false;
	hdr.length = 0;
    wait_message(m_socket, &hdr, nullptr, ACCL_BGN_PARAM);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_BGN_PARAM" << endl;
    if(hdr.length == 0)
    {
		// Wait for ACCL_END_PARAM
		hdr.pyld = false;
		hdr.length = 0;
		wait_message(m_socket, &hdr, nullptr, ACCL_END_PARAM);
		cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_END_PARAM" << endl;
		addr = -1;
		size = 0;
        return;
    }
	// Wait for ACCL_PARAM_PYLD(s)
    hdr.msgType = ACCL_PARAM_PYLD;
	int totalBytes = hdr.length;
    int remBytes = hdr.length;
    int bufIdx = 0;
    uint8_t* buf = new uint8_t[hdr.length];
    hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        int rb = wait_message(m_socket, &hdr, &buf[bufIdx], ACCL_PARAM_PYLD);
        bufIdx += rb;
        remBytes -= rb;
		cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_PARAM_PYLD - " << bufIdx << "/" << totalBytes << " bytes" << endl;
    } while(remBytes > 0);
	// Wait for ACCL_END_PARAM
    hdr.pyld = false;
	hdr.length = 0;
    wait_message(m_socket, &hdr, nullptr, ACCL_END_PARAM);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_END_PARAM" << endl;
    addr = (uint64_t)buf;
    size = hdr.length;
}


int SYSC_FPGA_hndl::wrParam(Accel_Payload* pyld)
{
	// Send ACCL_BGN_PARAM
    msgHeader_t hdr;
    hdr.msgType = ACCL_BGN_PARAM;
    hdr.pyld = false;
    hdr.length = pyld->m_size;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_BGN_PARAM" << endl;
    if(pyld->m_size == 0)
    {
		// Send ACCL_END_PARAM
		hdr.msgType = ACCL_END_PARAM;
		hdr.pyld = false;
		hdr.length = 0;
		send_message(m_socket, &hdr, nullptr);
		cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_END_PARAM" << endl;
        return 0;
    }
	// Send ACCL_PARAM_PYLD(s)
    hdr.msgType = ACCL_PARAM_PYLD;
	int totalBytes = hdr.length;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
    hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        int rb = send_message(m_socket, &hdr, &pyld_ptr[pyldIdx]);
        pyldIdx += rb;
        remBytes -= rb;
		cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_PARAM_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
    } while(remBytes > 0);
	// Send ACCL_END_PARAM
	hdr.msgType = ACCL_END_PARAM;
    hdr.pyld = false;
	hdr.length = 0;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_END_PARAM" << endl;
    return 0;
}


int SYSC_FPGA_hndl::waitStart()
{
    msgHeader_t hdr;
    hdr.length = 0;
    hdr.pyld = false;
    wait_message(m_socket, &hdr, nullptr, ACCL_START);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_START" << endl;
    return 0;
}


int SYSC_FPGA_hndl::sendStart()
{
    msgHeader_t hdr;
    hdr.msgType = ACCL_START;
    hdr.length = 0;
    hdr.pyld = false;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_START" << endl;
    return 0;
}


int SYSC_FPGA_hndl::waitComplete()
{
    msgHeader_t hdr;
    hdr.length = 0;
    hdr.pyld = false;
    wait_message(m_socket, &hdr, nullptr, ACCL_FINISHED);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACCL_FINISHED" << endl;
    return 0;
}


int SYSC_FPGA_hndl::sendComplete()
{
    msgHeader_t hdr;
    hdr.msgType = ACCL_FINISHED;
    hdr.length = 0;
    hdr.pyld = false;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACCL_FINISHED" << endl;
    return 0;
}


int SYSC_FPGA_hndl::getOutput(Accel_Payload* pyld)
{
	// Send ACCL_BGN_OUTPUT
    msgHeader_t hdr;
    hdr.msgType = ACCL_BGN_OUTPUT;
	hdr.pyld = false;
    hdr.length = 0;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_BGN_OUTPUT" << endl;
	// Wait for ACCL_OUTPUT_PYLD(s)
	int totalBytes = pyld->m_size;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
    hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        int rb = wait_message(m_socket, &hdr, &pyld_ptr[pyldIdx], ACCL_OUTPUT_PYLD);
        cout << "[SYSC_FPGA_SHIM]: Software recvd ACCL_OUTPUT_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
        pyldIdx += rb;
        remBytes -= rb;
    } while(remBytes > 0);
	// Wait for ACCL_END_OUTPUT
    hdr.pyld = false;	
    hdr.length = 0;
    wait_message(m_socket, &hdr, nullptr, ACCL_END_OUTPUT);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACCL_END_OUTPUT" << endl;
    return 0;
}


int SYSC_FPGA_hndl::sendOutput(Accel_Payload* pyld)
{
	// Send ACCL_BGN_OUTPUT
    msgHeader_t hdr;
    hdr.pyld = false;	
    hdr.length = 0;
    wait_message(m_socket, &hdr, nullptr, ACCL_BGN_OUTPUT);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_BGN_OUTPUT" << endl;
	// Wait for ACCL_OUTPUT_PYLD(s)
	hdr.msgType = ACCL_OUTPUT_PYLD;
	int totalBytes = pyld->m_size;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
    hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        int rb = send_message(m_socket, &hdr, &pyld_ptr[pyldIdx]);
        cout << "[SYSC_FPGA_SHIM]: Hardware sent ACCL_OUTPUT_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
        pyldIdx += rb;
        remBytes -= rb;
    } while(remBytes > 0);
	// Send ACCL_END_OUTPUT
    hdr.msgType = ACCL_END_OUTPUT;
    hdr.pyld = false;	
    hdr.length = 0;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACCL_END_OUTPUT" << endl;
    return 0;
}
