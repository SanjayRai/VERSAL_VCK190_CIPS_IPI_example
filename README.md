# VERSAL_VCK190_CIPS_IPI_example
VERSAL_VCK190_CIPS_IPI_example build script


The exmaple build  Versal  CIPS  example (CPM-PCIE xDMA, NoC, DDR, MMIO-BRAM) with TCL Scripts.

* THe Design was initial created in Vivado IPI  and converted to TCL script.

Reuirements:
1. Vivado 2021.2

Build instructions:

To Build the  design from scripts (linux command) :
    1. cd  vivado_project
    2. ./build_all

To  view and analyze an implemented  design in Vivado GUI :
    1. cd vivado_project
    2. vivado project_X/project_X.xpr

To view or re-customize the IPI generated IP in Vivado GUI :
    1. cd IP
    2. vivado -source vivado_project.tcl
    3. customize_CPM_BD

To  Build just the IP in IPI  from script
    1. cd IP
    2. vivado -source vivado_project.tcl
    3. create_CPM_BD


To run the design  in Hardware :
Requirements :  x86 host  with Linux OS, VCK190 dev kit, Xilinx xDMA driver and Laptop installed  with Vivado or Vivado Lab tools

1. Xilinx xDMA driver: https://github.com/Xilinx/dma_ip_drivers
    a. Download and build driver on the x86 Linux host.
2. Insert the VCK190 into the x86 host systems PCIe Slot (up to PCIe Gen4 x8).
3. Download the vck190_tester_top.pdi and vck190_tester_top.ltx to the card using Vivado HW Manager (Laptop connected to VCK190 via USB cable)
4. Warm restart  (ie, sudo reboot) to reenumerate the VCK190
5. issue lspci command to ensure card is enumerated and xDMA driver is attached to the PCIe  BDF of the card.
6. cd HOST_code/BUILD_DIR/Make_flow
7. make srai_hw
8. ./passthru.hw <BAR0 Address of VCK190>  (use test_srai as  referance)
9.  Vivado HW manager ILA/CHipScope can be used to View AXI transactions  with the PCIe DMA as well as  MMIO transactions.
10. make srai_hw_dbg can also be use to make Debug target for  use with gdb or ddd 
