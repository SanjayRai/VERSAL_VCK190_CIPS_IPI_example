// Hardware test debug only code :  Sanjay Rai
//
//
#include<stdio.h>
#include<math.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <cmath>
#include "pcie_memio.h" 
#include "srai_accel_utils.h" 
#define ZERO_f 1.0e-4
#define ITERATION_SZ 1
//#define ITERATION_SZ 4096
//#define ITERATION_SZ 8192
//#define ITERATION_SZ 16384
//#define ITERATION_SZ 32768
#define ONE_GIG (1024UL*1024UL*1024UL)

#define TEST_DATA_SIZE 1024

//#define USE_UDMABUF


#define HW_Kernel_frequency 250000000.0


using namespace std;


int check_buf(unsigned char* buf, unsigned int size)
{
    int i;
    int error_count = 0;
    for(i = 0; i < size; i++) {
        buf[i] = (char)0xA5;
    }
    for(i = 0; i < size; i++) {
        if (buf[i] != (char)0xA5) {
         error_count++;
        }
    }
    return error_count;
}


int main(int argc, char** argv) {

  int            fd;
  char  attr[1024];
  unsigned int   udma_buf_size;
  uint32_t   buf_size;
  uint32_t   result_buffer_offset;
  uint64_t phys_addr;
  uint64_t phys_addr_upper;
  unsigned long  debug_vma = 0;
  unsigned long  sync_mode = 2;
  uint32_t dbg_counter = 0;
  uint32_t test_data_byte_count = 0;

  unsigned long int itrn = 0;
  int compute_itn_count;
  uint32_t TCR0;
  volatile uint32_t TCSR0;
  uint32_t itn_count = 0;
  time_t t;
  srand((unsigned) time(&t));
  double high_res_elapsed_time = 0.0f;
  double high_res_elapsed_time_HW = 0.0f;
  double high_res_elapsed_time_SW = 0.0f;
  chrono::high_resolution_clock::time_point start_t;
  chrono::high_resolution_clock::time_point stop_t;
  chrono::duration<double> elapsed_hi_res;


  int   fd_kmem;
  void* kmem_buf;
  unsigned char * kmem_buf_ptr;

  SysMon_temp_struct sys_temprature;
  deviceDNA_struct deviceDNA;
  bool RESULT_SUCESSFULL;
  kernel_execution_metric_struct kernel_execution_metric; 

  uint64_t cal_itn_count = 0;

// Compile for SRAI custom HLS accelerator platform 
    uint32_t PCI_BAR_base_VF0;
    stringstream PCI_BAR_VF0_base_str;

    if (argc != 2) {
        printf("usage: %s fpga_bin_file \n", argv[0]);
        return -1;
    }

    PCI_BAR_VF0_base_str << hex <<  argv[1]; 

    PCI_BAR_VF0_base_str >> PCI_BAR_base_VF0;

    
    cout << "VF0 Base Addr = 0x"  << hex << PCI_BAR_base_VF0 << endl;

    cout << "Initializing FPGA\n";
    fpga_xdma_dev_mem *fpga_ptr_vf0 = new fpga_xdma_dev_mem;
    fpga_ptr_vf0->fpga_xdma_dev_mem_init((4*1024*1024), PCI_BAR_base_VF0);


#ifdef USE_UDMABUF
    // __ udmabuf kernal mode driver needs to be installed 
    if ((fd  = open("/sys/class/udmabuf/udmabuf0/phys_addr", O_RDONLY)) != -1) {
      read(fd, attr, 1024);
      sscanf(attr, "%llx", &phys_addr);
      close(fd);
    }

    if ((fd  = open("/sys/class/udmabuf/udmabuf0/size"     , O_RDONLY)) != -1) {
      read(fd, attr, 1024);
      sscanf(attr, "%d", &udma_buf_size);
      close(fd);
    }

    if ((fd  = open("/sys/class/udmabuf/udmabuf0/sync_mode", O_WRONLY)) != -1) {
      sprintf(attr, "%d", sync_mode);
      write(fd, attr, strlen(attr));
      close(fd);
    }

    if ((fd  = open("/sys/class/udmabuf/udmabuf0/debug_vma", O_WRONLY)) != -1) {
      sprintf(attr, "%d", debug_vma);
      write(fd, attr, strlen(attr));
      close(fd);
    }

    //check_buf_test((64*1024), 0, 0);
    if ((fd  = open("/sys/class/udmabuf/udmabuf0/sync_mode", O_WRONLY)) != -1) {
      sprintf(attr, "%d", sync_mode);
      write(fd, attr, strlen(attr));
      close(fd);
    }

    buf_size = udma_buf_size;
    result_buffer_offset = (buf_size/2);
    //buf_size = 512*1024;
    test_data_byte_count = (TEST_DATA_SIZE > (buf_size/2)) ? (buf_size/2) : TEST_DATA_SIZE; 

    cout << "Test data set size = " << dec << test_data_byte_count << endl;

    printf("UDMA_BUF phys_addr=0x%llx\n", phys_addr);
    printf("size=%d\n", udma_buf_size);
    printf("DMA size=%d\n", buf_size);
    printf("sync_mode=%d, O_SYNC=%d, ", sync_mode, (O_SYNC)?1:0);

    if ((fd_kmem  = open("/dev/udmabuf0", O_RDWR | O_SYNC)) != -1) {
      kmem_buf = mmap(NULL, buf_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
      kmem_buf_ptr = (unsigned char *)kmem_buf;
      printf ("SRAI__DBG :: UDMA_BUF Virtual Address = %llx\n", kmem_buf);
      //error_count = check_buf((unsigned char *)kmem_buf, size);
    }
    // __SRAI Initialize Kernel buffer (1MB worth)
    for (int i = 0; i < buf_size; i++) {
       // kmem_buf_ptr[i] = (unsigned char)((rand() % 256));
          if (i < (buf_size/2)) { 
              //kmem_buf_ptr[i] = (0x000000FF & i);
              kmem_buf_ptr[i] = i; 
          } else {
              kmem_buf_ptr[i] = (0xA5);
          }
    }
    // __ udmabuf kernal mode driver needs to be installed 
   printf("-------------------------------------------------------------\n\n\n");

#endif //USE_UDMABUF


    unsigned char *to_FPGA_data_buf = new unsigned char[ONE_GIG];
    unsigned char *from_FPGA_data_buf = new unsigned char[ONE_GIG];
    for (int i= 0; i < ONE_GIG; i ++) {
        to_FPGA_data_buf[i] = ((unsigned char)((rand() % 256)));
    }

    cout << dec << "Testing BRAM 8K Memory \n";
    fpga_test_AXIL_LITE_8KSCRATCHPAD_BRAM (fpga_ptr_vf0);

    int xfer_count = 0;
    int ERR_count = 0;
    for (int xdma_itr = 0; xdma_itr <= 1; xdma_itr++) {
    /* Initialize DDR4 Memory with Input Arguments*/
        start_t = chrono::high_resolution_clock::now();
        xfer_count = fpga_ptr_vf0->fpga_xDMA_write64((0x0000000000000000), (char *)(to_FPGA_data_buf), ONE_GIG);
        xfer_count = fpga_ptr_vf0->fpga_xDMA_read64((0x0000000000000000), (char *)(from_FPGA_data_buf), ONE_GIG);
        stop_t = chrono::high_resolution_clock::now();
        elapsed_hi_res = stop_t - start_t ;
        high_res_elapsed_time = elapsed_hi_res.count();
        double xDMA_round_Trip_thruput = (ONE_GIG/high_res_elapsed_time);
        cout << "xDMA Execution time =  " <<  high_res_elapsed_time << "s\n";
        cout << "xDMA THroughput =  " << xDMA_round_Trip_thruput  << " Bytes/s\n";
        cout << "xDMA efficency =  " <<  (xDMA_round_Trip_thruput/(4*ONE_GIG))*100.00 << "%\n";
        for (int i= 0; i < ONE_GIG; i++) {
            if(to_FPGA_data_buf[i] != from_FPGA_data_buf[i]) {
                ERR_count++;
            }
        } 
        //fpga_CDMA_XFER (fpga_ptr_vf0, 0x20100000000, 0x00000000, 2048);
        //cout << "XDMA iteration cont = " << xdma_itr << endl;
        //SRAI_dbg_wait("Slave_ Bridge CDMA");
        //fpga_CDMA_XFER (fpga_ptr_vf0, 0x20100000000, 0x8000000000, 2048);
        //xfer_count = fpga_ptr_vf0->fpga_xDMA_read64((0x0000000000000000), (char *)(from_FPGA_data_buf), 2048);
        //for (int i = 0; i < 512; i+=4) {
        //    //if(kmem_buf_ptr[i] != kmem_buf_ptr[i + result_buffer_offset]) {
        //    cout << "Data  : " << dec  << i << " : " << (uint32_t)kmem_buf_ptr[i] << " ;\n";
       // }
    }

    cout << "xDMA xfer Err Count : " << dec << ERR_count << endl;


#ifdef USE_UDMABUF
    phys_addr_upper = phys_addr + (uint64_t)(result_buffer_offset);

    uint32_t XFER_buf_size;

    //XFER_buf_size = 512;
    XFER_buf_size = buf_size;
    // Throughput testing
    //SRAI_dbg_wait("Bandwidth test From Host to Card");
// ---------------------------------------------------
    cout << "\n\nTesting Host to Card C0 Bandwidth .........\n";
    start_t = chrono::high_resolution_clock::now();
    for (itrn = 0; itrn < ITERATION_SZ; itrn++) {
        fpga_CDMA_XFER (fpga_ptr_vf0, phys_addr, (AXI_MM_DDR4_C0 + itrn*XFER_buf_size), XFER_buf_size );
    }
    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "Host Memory to Card DDR4 xfer  Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "Host Memory to Card DDR4 xfer  THroughput =  " <<  ((itrn*XFER_buf_size)/high_res_elapsed_time_HW) << " Bytes/s   with " << dec  << (itrn*XFER_buf_size) << " bytes\n";

    cout << "\n\nTesting Card C0 HOST Bandwidth .........\n";
    start_t = chrono::high_resolution_clock::now();
    for (itrn = 0; itrn < ITERATION_SZ; itrn++) {
        fpga_CDMA_XFER (fpga_ptr_vf0, (AXI_MM_DDR4_C0 + itrn*XFER_buf_size), phys_addr, XFER_buf_size );
    }
    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "Host Memory to Card DDR4 xfer  Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "Host Memory to Card DDR4 xfer  THroughput =  " <<  ((itrn*XFER_buf_size)/high_res_elapsed_time_HW) << " Bytes/s   with " << dec  << (itrn*XFER_buf_size) << " bytes\n";
// ---------------------------------------------------
    cout << "\n\nTesting Host to Card C1 Bandwidth .........\n";
    start_t = chrono::high_resolution_clock::now();
    for (itrn = 0; itrn < ITERATION_SZ; itrn++) {
        fpga_CDMA_XFER (fpga_ptr_vf0, phys_addr, (AXI_MM_DDR4_C1 + itrn*XFER_buf_size), XFER_buf_size );
    }
    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "Host Memory to Card DDR4 xfer  Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "Host Memory to Card DDR4 xfer  THroughput =  " <<  ((itrn*XFER_buf_size)/high_res_elapsed_time_HW) << " Bytes/s   with " << dec  << (itrn*XFER_buf_size) << " bytes\n";


    cout << "\n\nTesting Card C1 Host Bandwidth .........\n";
    start_t = chrono::high_resolution_clock::now();
    for (itrn = 0; itrn < ITERATION_SZ; itrn++) {
        fpga_CDMA_XFER (fpga_ptr_vf0, (AXI_MM_DDR4_C1 + itrn*XFER_buf_size), phys_addr, XFER_buf_size );
    }
    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "Host Memory to Card DDR4 xfer  Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "Host Memory to Card DDR4 xfer  THroughput =  " <<  ((itrn*XFER_buf_size)/high_res_elapsed_time_HW) << " Bytes/s   with " << dec  << (itrn*XFER_buf_size) << " bytes\n";
// ---------------------------------------------------
    cout << "\n\nTesting Host to Card C2 Bandwidth .........\n";
    start_t = chrono::high_resolution_clock::now();
    for (itrn = 0; itrn < ITERATION_SZ; itrn++) {
        fpga_CDMA_XFER (fpga_ptr_vf0, phys_addr, (AXI_MM_DDR4_C2 + itrn*XFER_buf_size), XFER_buf_size );
    }
    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "Host Memory to Card DDR4 xfer  Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "Host Memory to Card DDR4 xfer  THroughput =  " <<  ((itrn*XFER_buf_size)/high_res_elapsed_time_HW) << " Bytes/s   with " << dec  << (itrn*XFER_buf_size) << " bytes\n";


    cout << "\n\nTesting Card C2 Host Bandwidth .........\n";
    start_t = chrono::high_resolution_clock::now();
    for (itrn = 0; itrn < ITERATION_SZ; itrn++) {
        fpga_CDMA_XFER (fpga_ptr_vf0, (AXI_MM_DDR4_C2 + itrn*XFER_buf_size), phys_addr, XFER_buf_size );
    }
    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "Host Memory to Card DDR4 xfer  Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "Host Memory to Card DDR4 xfer  THroughput =  " <<  ((itrn*XFER_buf_size)/high_res_elapsed_time_HW) << " Bytes/s   with " << dec  << (itrn*XFER_buf_size) << " bytes\n";
// ---------------------------------------------------
    cout << "\n\nTesting Host to Card C3 Bandwidth .........\n";
    start_t = chrono::high_resolution_clock::now();
    for (itrn = 0; itrn < ITERATION_SZ; itrn++) {
        fpga_CDMA_XFER (fpga_ptr_vf0, phys_addr, (AXI_MM_DDR4_C3 + itrn*XFER_buf_size), XFER_buf_size );
    }
    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "Host Memory to Card DDR4 xfer  Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "Host Memory to Card DDR4 xfer  THroughput =  " <<  ((itrn*XFER_buf_size)/high_res_elapsed_time_HW) << " Bytes/s   with " << dec  << (itrn*XFER_buf_size) << " bytes\n";


    cout << "\n\nTesting Card C3 Host Bandwidth .........\n";
    start_t = chrono::high_resolution_clock::now();
    for (itrn = 0; itrn < ITERATION_SZ; itrn++) {
        fpga_CDMA_XFER (fpga_ptr_vf0, (AXI_MM_DDR4_C3 + itrn*XFER_buf_size), phys_addr, XFER_buf_size );
    }
    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "Host Memory to Card DDR4 xfer  Execution time =  " <<  high_res_elapsed_time_HW << "s\n";
    cout << "Host Memory to Card DDR4 xfer  THroughput =  " <<  ((itrn*XFER_buf_size)/high_res_elapsed_time_HW) << " Bytes/s   with " << dec  << (itrn*XFER_buf_size) << " bytes\n";
// ---------------------------------------------------


    //SRAI_dbg_wait("CDMA From_Host");
    // __SRAI Initialize Kernel buffer (1MB worth)
    for (int i = 0; i < buf_size; i++) {
        kmem_buf_ptr[i] = (unsigned char)((rand() % 256));
    }

    start_t = chrono::high_resolution_clock::now();
    cout << " Initiating CDMA XFER From host to DDR_C0 (shell DDR4)----------- " << endl;
    fpga_CDMA_XFER (fpga_ptr_vf0, phys_addr, AXI_MM_DDR4_C0,  test_data_byte_count);

    cout << " Initiating CDMA XFER From DDR_C0 to DDR_C1 (shell DDR4)----------- " << endl;
    fpga_CDMA_XFER (fpga_ptr_vf0, AXI_MM_DDR4_C0, AXI_MM_DDR4_C1,  test_data_byte_count);

    cout << " Initiating CDMA XFER From DDR_C1 to DDR_C2 (shell DDR4)----------- " << endl;
    fpga_CDMA_XFER (fpga_ptr_vf0, AXI_MM_DDR4_C1, AXI_MM_DDR4_C2, test_data_byte_count);

    cout << " Initiating CDMA XFER From DDR_C2 to DDR_C3 (shell DDR4)----------- " << endl;
    fpga_CDMA_XFER (fpga_ptr_vf0, AXI_MM_DDR4_C2, AXI_MM_DDR4_C3, test_data_byte_count);

    cout << " Initiating CDMA XFER From DDR_C3 to host (shell DDR4)----------- " << endl;
    fpga_CDMA_XFER (fpga_ptr_vf0, AXI_MM_DDR4_C3, phys_addr_upper, test_data_byte_count);


    stop_t = chrono::high_resolution_clock::now();
    elapsed_hi_res = stop_t - start_t ;
    high_res_elapsed_time = elapsed_hi_res.count();
    high_res_elapsed_time_HW = high_res_elapsed_time;
    cout << "CDMA HOST -> 4x DDR4 -> Host xfer time =  " <<  high_res_elapsed_time_HW << "s\n";
    
    unsigned int val_error = 0;
    for (int i = 0; i < test_data_byte_count;  i++) {
        if(kmem_buf_ptr[i] != kmem_buf_ptr[i + result_buffer_offset]) {
            //cout << " At index = " << i <<"  Expected  " << hex << (unsigned int)kmem_buf_ptr[i] << " But Got  " << (unsigned int)kmem_buf_ptr[i + buf_size] << endl;
            val_error++;
        }
    }

    cout << "Kernel buffer validation : No. of Errors = " << dec << val_error << endl;
    cout << "------------------------------------------------------------------------\n";

    // ------------ Clean -----------------------
    close(fd_kmem);
#endif //USE_UDMABUF

    fpga_clean(fpga_ptr_vf0);
    cout << "*************     Test finished ****************************************************************************\n"; 
    return 0;
}


