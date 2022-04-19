# Created : 9:31:38, Tue Jun 21, 2016 : Sanjay Rai

source ../device_type.tcl
create_project -force project_X project_X -part [DEVICE_TYPE] 
set_property board_part [BOARD_TYPE] [current_project]

proc customize_CPM_BD {} {
    add_files -fileset sources_1 -norecurse {
    ../IP/CPM_bd/CPM_bd.bd
    }
}

proc create_CPM_BD {} {
    source ./CPM_bd_tcl.tcl
    update_compile_order -fileset sources_1
    validate_bd_design -force
    generate_target all [get_files  ../IP/CPM_bd/CPM_bd.bd]
}

