#include "SYSC_FPGA_shim.hpp"
using namespace std;


SYSC_FPGA_hndl::SYSC_FPGA_hndl(int memStartOft) : FPGA_hndl()
{
    m_remAddrOfst = memStartOft;
}


SYSC_FPGA_hndl::~SYSC_FPGA_hndl()
{
#ifdef MODEL_TECH
    return;
#endif
    close(m_socket);
}


int SYSC_FPGA_hndl::hardware_init()
{
#ifdef MODEL_TECH
    return 0;
#endif
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
#ifdef MODEL_TECH
    return 0;
#endif
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
    uint64_t AXI_aligned_sz = ALGN_PYLD_SZ(size, AXI_BUFFER_ALIGNMENT);
    if(my_aligned_malloc((void**)&pyld->m_buffer, AXI_BUFFER_ALIGNMENT, AXI_aligned_sz))
    {
        printf("[SYSC_FPGA_SHIM]: Failed to allocate 0x%X B for DMA buffer.", size);
        return -1;
    }
    printf("[SYSC_FPGA_SHIM]: Allocated %llud bytes buffer space on Remote Memory\n", AXI_aligned_sz);
    pyld->m_size            = AXI_aligned_sz;
    pyld->m_remAddress      = (uint64_t)m_remAddrOfst;
    m_remAddrOfst           += AXI_aligned_sz;
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
    cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACCL_SYSC_FPGA_BGN_CFG" << endl;    
    wait_message(m_socket, &hdr, nullptr, ACCL_SYSC_FPGA_BGN_CFG);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_SYSC_FPGA_BGN_CFG" << endl;
    int totalBytes = hdr.length;    
    // Send ACK
    hdr.pyld = false;
    hdr.length = 0;
    hdr.msgType = ACCL_ACK;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACK" << endl;
    // Wait for ACCL_PYLD(s)
    hdr.msgType = ACCL_PYLD;
    hdr.pyld = true;
    int remBytes = totalBytes;
    int bufIdx = 0;
    uint8_t* buf = new uint8_t[totalBytes];
    do
    {
        cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACCL_PYLD" << endl;
        int rb = wait_message(m_socket, &hdr, &buf[bufIdx], ACCL_PYLD);
        bufIdx += rb;
        remBytes -= rb;
        cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_PYLD - " << bufIdx << "/" << totalBytes << " bytes" << endl;
        // Send ACK
        hdr.pyld = false;
        hdr.length = 0;
        hdr.msgType = ACCL_ACK;
        send_message(m_socket, &hdr, nullptr);
        cout << "[SYSC_FPGA_SHIM]: Hardware sent ACK" << endl;
    } while(remBytes > 0);
    // Wait for ACCL_SYSC_FPGA_END_CFG
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACCL_SYSC_FPGA_END_CFG" << endl;    
    wait_message(m_socket, &hdr, nullptr, ACCL_SYSC_FPGA_END_CFG);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_SYSC_FPGA_END_CFG" << endl;
    // Send ACK
    hdr.pyld = false;
    hdr.length = 0;
    hdr.msgType = ACCL_ACK;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACK" << endl;
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
    // Wait ACK
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Software waiting for ACK" << endl;    
    wait_message(m_socket, &hdr, nullptr, ACCL_ACK);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACK" << endl;
    // Send ACCL_PYLD(s)
    hdr.msgType = ACCL_PYLD;
    hdr.pyld = true;
    int totalBytes = pyld->m_size;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        int rb = send_message(m_socket, &hdr, &pyld_ptr[pyldIdx]);
        pyldIdx += rb;
        remBytes -= rb;
        cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
        // Wait ACK
        hdr.pyld = false;
        hdr.length = 0;
        cout << "[SYSC_FPGA_SHIM]: Software waiting for ACK" << endl;
        wait_message(m_socket, &hdr, nullptr, ACCL_ACK);
        cout << "[SYSC_FPGA_SHIM]: Software recvd ACK" << endl;
    } while(remBytes > 0);
    // Send ACCL_SYSC_FPGA_END_CFG
    hdr.msgType = ACCL_SYSC_FPGA_END_CFG;
    hdr.pyld = false;
    hdr.length = 0;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_SYSC_FPGA_END_CFG" << endl;
    // Wait ACK
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Software waiting for ACK" << endl;   
    wait_message(m_socket, &hdr, nullptr, ACCL_ACK);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACK" << endl;
    return 0;    
}

uint64_t SYSC_FPGA_hndl::waitConfig()
{
    // Wait for ACCL_BGN_CFG
    msgHeader_t hdr;
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACCL_BGN_CFG" << endl;
    wait_message(m_socket, &hdr, nullptr, ACCL_BGN_CFG);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_BGN_CFG" << endl;
    int totalBytes = hdr.length;
    // Send ACK
    hdr.pyld = false;
    hdr.length = 0;
    hdr.msgType = ACCL_ACK;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACK" << endl;
    // Wait for ACCL_PYLD(s)
    hdr.msgType = ACCL_PYLD;
    hdr.pyld = true;
    int remBytes = totalBytes;
    int bufIdx = 0;
    uint8_t* buf = new uint8_t[totalBytes];
    do
    {
        cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACCL_PYLD" << endl;
        int rb = wait_message(m_socket, &hdr, &buf[bufIdx], ACCL_PYLD);
        bufIdx += rb;
        remBytes -= rb;
        cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_PYLD - " << bufIdx << "/" << totalBytes << " bytes" << endl;
        // Send ACK
        hdr.pyld = false;
        hdr.length = 0;
        hdr.msgType = ACCL_ACK;
        send_message(m_socket, &hdr, nullptr);
        cout << "[SYSC_FPGA_SHIM]: Hardware sent ACK" << endl;
    } while(remBytes > 0);
    // Wait for ACCL_END_CFG
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACCL_END_CFG" << endl;
    wait_message(m_socket, &hdr, nullptr, ACCL_END_CFG);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_END_CFG" << endl;
    // Send ACK
    hdr.pyld = false;
    hdr.length = 0;
    hdr.msgType = ACCL_ACK;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACK" << endl;    
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
    // Wait ACK
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Software waiting for ACK" << endl;
    wait_message(m_socket, &hdr, nullptr, ACCL_ACK);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACK" << endl;    
    // Send ACCL_PYLD(s)
    hdr.msgType = ACCL_PYLD;
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
        cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
         // Wait ACK
        hdr.pyld = false;
        hdr.length = 0;
        cout << "[SYSC_FPGA_SHIM]: Software waiting for ACK" << endl;        
        wait_message(m_socket, &hdr, nullptr, ACCL_ACK);
        cout << "[SYSC_FPGA_SHIM]: Software recvd ACK" << endl;       
    } while(remBytes > 0);
    // Send ACCL_END_CFG
    hdr.msgType = ACCL_END_CFG;
    hdr.pyld = false;
    hdr.length = 0;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_END_CFG" << endl;
    // Wait ACK
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Software waiting for ACK" << endl;    
    wait_message(m_socket, &hdr, nullptr, ACCL_ACK);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACK" << endl;  
    return 0;
}


int SYSC_FPGA_hndl::rdConfig(Accel_Payload* pyld)
{
    return 0;
}


void SYSC_FPGA_hndl::waitParam(uint64_t& addr, int& size)
{
    cout << "FIXME" << endl;
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
    // Wait for ACCL_PYLD(s)
    hdr.msgType = ACCL_PYLD;
    int totalBytes = hdr.length;
    int remBytes = hdr.length;
    int bufIdx = 0;
    uint8_t* buf = new uint8_t[hdr.length];
    hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        int rb = wait_message(m_socket, &hdr, &buf[bufIdx], ACCL_PYLD);
        bufIdx += rb;
        remBytes -= rb;
        cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_PYLD - " << bufIdx << "/" << totalBytes << " bytes" << endl;
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
    cout << "FIXME" << endl;
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
    // Send ACCL_PYLD(s)
    hdr.msgType = ACCL_PYLD;
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
        cout << "[SYSC_FPGA_SHIM]: Software sent ACCL_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
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
    // Send ACK
    hdr.pyld = false;
    hdr.length = 0;
    hdr.msgType = ACCL_ACK;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACK" << endl;    
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
    // Wait ACK
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Software waiting for ACK" << endl;
    wait_message(m_socket, &hdr, nullptr, ACCL_ACK);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACK" << endl;    
    return 0;
}


int SYSC_FPGA_hndl::waitComplete()
{
    msgHeader_t hdr;
    hdr.length = 0;
    hdr.pyld = false;
    cout << "[SYSC_FPGA_SHIM]: Software waiting for ACCL_COMPLETE" << endl;
    wait_message(m_socket, &hdr, nullptr, ACCL_COMPLETE);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACCL_COMPLETE" << endl;
    return 0;
}


int SYSC_FPGA_hndl::sendComplete()
{
    msgHeader_t hdr;
    hdr.msgType = ACCL_COMPLETE;
    hdr.length = 0;
    hdr.pyld = false;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACCL_COMPLETE" << endl;
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
    // Wait ACK
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Software waiting for ACK" << endl; 
    wait_message(m_socket, &hdr, nullptr, ACCL_ACK);
    cout << "[SYSC_FPGA_SHIM]: Software recvd for ACK" << endl;    
    // Wait for ACCL_PYLD(s)
    int totalBytes = pyld->m_size;
    int remBytes = pyld->m_size;
    int pyldIdx = 0;
    uint8_t* pyld_ptr = (uint8_t*)pyld->m_buffer;
    hdr.pyld = true;
    do
    {
        hdr.length = min(remBytes, MAX_BLK_SZ);
        cout << "[SYSC_FPGA_SHIM]: Software waiting for ACCL_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
        int rb = wait_message(m_socket, &hdr, &pyld_ptr[pyldIdx], ACCL_PYLD);
        pyldIdx += rb;
        remBytes -= rb;       
        cout << "[SYSC_FPGA_SHIM]: Software recvd ACCL_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;
        // Send ACK
        hdr.pyld = false;
        hdr.length = 0;
        hdr.msgType = SOFT_ACK;
        send_message(m_socket, &hdr, nullptr);
        cout << "[SYSC_FPGA_SHIM]: Software sent ACK" << endl;           
    } while(remBytes > 0);
    // Wait for ACCL_END_OUTPUT
    hdr.pyld = false;    
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Software waiting for ACCL_END_OUTPUT" << endl;
    wait_message(m_socket, &hdr, nullptr, ACCL_END_OUTPUT);
    cout << "[SYSC_FPGA_SHIM]: Software recvd ACCL_END_OUTPUT" << endl;
    // Send ACK
    hdr.pyld = false;
    hdr.length = 0;
    hdr.msgType = SOFT_ACK;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Software sent ACK" << endl;      
    return 0;
}


int SYSC_FPGA_hndl::sendOutput(Accel_Payload* pyld)
{
    // Send ACCL_BGN_OUTPUT
    msgHeader_t hdr;
    hdr.pyld = false;    
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACCL_BGN_OUTPUT" << endl;
    wait_message(m_socket, &hdr, nullptr, ACCL_BGN_OUTPUT);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACCL_BGN_OUTPUT" << endl;
    // Send ACK
    hdr.pyld = false;
    hdr.length = 0;
    hdr.msgType = ACCL_ACK;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACK" << endl;    
    // Wait for ACCL_PYLD(s)
    hdr.msgType = ACCL_PYLD;
    int totalBytes = pyld->m_size;
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
        cout << "[SYSC_FPGA_SHIM]: Hardware sent ACCL_PYLD - " << pyldIdx << "/" << totalBytes << " bytes" << endl;        
        // Wait ACK
        hdr.pyld = false;
        hdr.length = 0;
        cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACK" << endl;
        wait_message(m_socket, &hdr, nullptr, SOFT_ACK);
        cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACK" << endl;
    } while(remBytes > 0);
    // Send ACCL_END_OUTPUT
    hdr.msgType = ACCL_END_OUTPUT;
    hdr.pyld = false;    
    hdr.length = 0;
    send_message(m_socket, &hdr, nullptr);
    cout << "[SYSC_FPGA_SHIM]: Hardware sent ACCL_END_OUTPUT" << endl;
    // Wait ACK
    hdr.pyld = false;
    hdr.length = 0;
    cout << "[SYSC_FPGA_SHIM]: Hardware waiting for ACK" << endl;
    wait_message(m_socket, &hdr, nullptr, SOFT_ACK);
    cout << "[SYSC_FPGA_SHIM]: Hardware recvd ACK" << endl;    
    return 0;
}
