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
	hdr.length = 0;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Hardware sent CONNECT_HARDWARE" << endl;
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
	hdr.length = 0;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Software sent CONNECT_SOFTWARE" << endl;
	return 0;
}


uint64_t SYSC_FPGA_hndl::allocate(Accel_Payload* pyld, int size)
{
    uint64_t aligned_sz = ALGN_PYLD_SZ(size, DMA_BUFFER_ALIGNMENT);
    if(my_aligned_malloc((void**)&pyld->m_buffer, DMA_BUFFER_ALIGNMENT, aligned_sz))
    {
        printf("Failed to allocate 0x%X B for DMA buffer.", size);
        return -1;
    }
    printf("Allocoated %ud bytes buffer space on Remote Memory\n", aligned_sz);
    pyld->m_size            = aligned_sz;
    pyld->m_remAddress      = (uint64_t)m_remAddrOfst;
    m_remAddrOfst           += aligned_sz;
    return                  (uint64_t)pyld->m_buffer;
}


void SYSC_FPGA_hndl::deallocate(Accel_Payload* pyld)
{
    my_aligned_free(pyld->m_buffer);
    pyld->m_buffer          = NULL;
    pyld->m_size            = -1;
    pyld->m_remAddress      = -1;
}


uint64_t SYSC_FPGA_hndl::waitConfig()
{
	msgHeader_t hdr;
	hdr.msgType = NULL_MSG;
	hdr.pyld = false;
	wait_message(m_socket, &hdr, nullptr, ACCEL_BGN_CFG);
	cout << "Hardware recvd ACCEL_SET_CONFIG" << endl;
    hdr.msgType = ACCEL_CFG_PYLD;
    int remBytes = hdr.length;
    int bufIdx = 0;
    uint8_t* buf = new uint8_t[hdr.length];
	hdr.pyld = true;
    do
    {
        hdr.msgType = NULL_MSG;
        int rb = wait_message(m_socket, &hdr, &buf[bufIdx], ACCEL_CFG_PYLD);
        cout << "Hardware recvd ACCEL_CFG_PYLD" << endl;
        bufIdx += rb;
        remBytes -= rb;
    } while (remBytes > 0);
	hdr.pyld = false;
	wait_message(m_socket, &hdr, nullptr, ACCEL_END_CFG);
	cout << "Hardware recvd ACCEL_END_CFG" << endl;
    return (uint64_t)buf;
}


int SYSC_FPGA_hndl::wrConfig(Accel_Payload* pyld)
{
	msgHeader_t hdr;
	hdr.msgType = ACCEL_BGN_CFG;
	hdr.length = pyld->m_size;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
    cout << "Software sent ACCEL_BGN_CFG" << endl;
    hdr.msgType = ACCEL_CFG_PYLD;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
	hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        send_message(m_socket, &hdr, &pyld_ptr[pyldIdx]);
        cout << "Software sent ACCEL_CFG_PYLD" << endl;
        pyldIdx += hdr.length;
        remBytes -= hdr.length;
    } while (remBytes > 0);
	hdr.msgType = ACCEL_END_CFG;
	hdr.length = 0;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Software sent ACCEL_END_CFG" << endl;
	return 0;
}


int SYSC_FPGA_hndl::rdConfig(Accel_Payload* pyld)
{
    return 0;
}


uint64_t SYSC_FPGA_hndl::waitParam()
{
	msgHeader_t hdr;
	hdr.msgType = NULL_MSG;
	hdr.pyld = false;
	wait_message(m_socket, &hdr, nullptr, ACCEL_BGN_PARAM);
	cout << "Hardware recvd ACCEL_SET_PARAM" << endl;
    hdr.msgType = ACCEL_PARAM_PYLD;
    int remBytes = hdr.length;
    int bufIdx = 0;
    uint8_t* buf = new uint8_t[hdr.length];
	hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        hdr.msgType = NULL_MSG;
        int rb = wait_message(m_socket, &hdr, &buf[bufIdx], ACCEL_PARAM_PYLD);
        cout << "Hardware recvd ACCEL_PARAM_PYLD" << endl;
        bufIdx += rb;
        remBytes -= rb;
    } while (remBytes > 0);
	hdr.pyld = false;
	wait_message(m_socket, &hdr, nullptr, ACCEL_END_PARAM);
	cout << "Hardware recvd ACCEL_END_PARAM" << endl;
    return (uint64_t)buf;
}


int SYSC_FPGA_hndl::wrParam(Accel_Payload* pyld)
{
	msgHeader_t hdr;
	hdr.msgType = ACCEL_BGN_PARAM;
	hdr.length = pyld->m_size;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Software sent ACCEL_SET_PARAM" << endl;
    hdr.msgType = ACCEL_PARAM_PYLD;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
	hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        send_message(m_socket, &hdr, &pyld_ptr[pyldIdx]);
        cout << "Software sent ACCEL_PARAM_PYLD" << endl;
        pyldIdx += hdr.length;
        remBytes -= hdr.length;
    } while (remBytes > 0);
	hdr.msgType = ACCEL_END_PARAM;
	hdr.length = 0;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Software sent ACCEL_END_PARAM" << endl;
	return 0;
}


int SYSC_FPGA_hndl::waitStart()
{
	msgHeader_t hdr;
	hdr.msgType =  NULL_MSG;
	hdr.length = 0;
	hdr.pyld = false;
	wait_message(m_socket, &hdr, nullptr, ACCEL_START);
	cout << "Hardware recvd ACCEL_START" << endl;
	return 0;
}


int SYSC_FPGA_hndl::sendStart()
{
	msgHeader_t hdr;
	hdr.msgType = ACCEL_START;
	hdr.length = 0;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Software sent ACCEL_START" << endl;
	return 0;
}


int SYSC_FPGA_hndl::waitComplete()
{
	msgHeader_t hdr;
	hdr.msgType = NULL_MSG;
	hdr.length = 0;
	hdr.pyld = false;
	wait_message(m_socket, &hdr, nullptr, ACCEL_FINISHED);
	cout << "Software recvd ACCEL_FINISHED" << endl;
	return 0;
}


int SYSC_FPGA_hndl::sendComplete()
{
	msgHeader_t hdr;
	hdr.msgType = ACCEL_FINISHED;
	hdr.length = 0;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Hardware sent ACCEL_FINISHED" << endl;
	return 0;
}


int SYSC_FPGA_hndl::getOutput(Accel_Payload* pyld)
{
	msgHeader_t hdr;
	hdr.msgType = ACCEL_BGN_OUTPUT;
	hdr.length = 0;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Software sent ACCEL_BGN_OUTPUT" << endl;

	hdr.msgType = NULL_MSG;
	hdr.length = pyld->m_size;
	hdr.pyld = true;
	wait_message(m_socket, &hdr, (uint8_t*)pyld->m_buffer, ACCEL_OUTPUT_PYLD);
	cout << "Software recvd ACCEL_OUTPUT_PYLD" << endl;

	hdr.msgType = NULL_MSG;
	hdr.length = 0;
	hdr.pyld = false;
	wait_message(m_socket, &hdr, nullptr, ACCEL_END_OUTPUT);
	cout << "Software recvd ACCEL_END_OUTPUT" << endl;
	return 0;
}


int SYSC_FPGA_hndl::sendOutput(Accel_Payload* pyld)
{
	msgHeader_t hdr;
	hdr.msgType = NULL_MSG;
	hdr.length = 0;
	hdr.pyld = false;
	wait_message(m_socket, &hdr, nullptr, ACCEL_BGN_OUTPUT);
	cout << "Hardware recvd ACCEL_BGN_OUTPUT" << endl;

	hdr.msgType = ACCEL_OUTPUT_PYLD;
	hdr.length = pyld->m_size;
	hdr.pyld = true;
	send_message(m_socket, &hdr, (uint8_t*)pyld->m_buffer);
	cout << "Hardware sent ACCEL_OUTPUT_PYLD" << endl;

	hdr.msgType = ACCEL_END_OUTPUT;
	hdr.length = 0;
	hdr.pyld = false;
	send_message(m_socket, &hdr, nullptr);
	cout << "Hardware sent ACCEL_END_OUTPUT" << endl;
	return 0;
}



