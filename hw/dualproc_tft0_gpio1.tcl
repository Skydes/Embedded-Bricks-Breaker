#######################################################################################
#
# Title:       Embedded Bricks-Breaker Project
# File:        dualproc_tft0_gpio1.tcl
# Date:        2017-04-13
# Author:      Paul-Edouard Sarlin
# Description: tcl script to build the block diagram for a dual-processor system with
#              the TFT assigned to uB 0 and the GPIO assigned to uB1.
#
#######################################################################################

create_bd_design "design_dual_tf0_gpio1"

#-- Processors

# Create PS
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 processing_system7_0
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR" apply_board_preset "1" Master "Disable" Slave "Disable" }  [get_bd_cells processing_system7_0]
startgroup
set_property -dict [list CONFIG.PCW_FPGA1_PERIPHERAL_FREQMHZ {25.000000} CONFIG.PCW_USE_M_AXI_GP1 {1} CONFIG.PCW_USE_S_AXI_HP0 {1} CONFIG.PCW_EN_CLK1_PORT {1} CONFIG.PCW_QSPI_PERIPHERAL_ENABLE {0} CONFIG.PCW_ENET0_PERIPHERAL_ENABLE {0}] [get_bd_cells processing_system7_0]
endgroup
startgroup
set_property -dict [list CONFIG.PCW_USE_M_AXI_GP0 {0} CONFIG.PCW_USE_M_AXI_GP1 {0} CONFIG.PCW_USE_S_AXI_GP0 {1} CONFIG.PCW_USE_S_AXI_GP1 {1}] [get_bd_cells processing_system7_0]
endgroup
startgroup
set_property -dict [list CONFIG.PCW_S_AXI_HP0_DATA_WIDTH {32}] [get_bd_cells processing_system7_0]
endgroup
startgroup
set_property -dict [list CONFIG.PCW_USE_EXPANDED_IOP {1}] [get_bd_cells processing_system7_0]
endgroup

# Create uB0
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:microblaze:9.6 microblaze_0
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:microblaze -config {local_mem "128KB" ecc "None" cache "None" debug_module "Debug Only" axi_periph "Enabled" axi_intc "1" clk "/processing_system7_0/FCLK_CLK0 (100 MHz)" }  [get_bd_cells microblaze_0]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins processing_system7_0/S_AXI_GP0]

# Create uB1
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:microblaze:9.6 microblaze_1
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:microblaze -config {local_mem "128KB" ecc "None" cache "None" debug_module "Debug Only" axi_periph "Enabled" axi_intc "1" clk "/processing_system7_0/FCLK_CLK0 (100 MHz)" }  [get_bd_cells microblaze_1]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_1 (Periph)" Clk "Auto" }  [get_bd_intf_pins processing_system7_0/S_AXI_GP1]


#-- Timers

# Setup timer for uB0
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 axi_timer_0
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins axi_timer_0/S_AXI]
connect_bd_net [get_bd_pins axi_timer_0/interrupt] [get_bd_pins microblaze_0_xlconcat/In0]

# Setup timer for uB1
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 axi_timer_1
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_1 (Periph)" Clk "Auto" }  [get_bd_intf_pins axi_timer_1/S_AXI]
#regenerate_bd_layout
connect_bd_net [get_bd_pins axi_timer_1/interrupt] [get_bd_pins microblaze_1_xlconcat/In0]


#-- TFT

# Create TFT
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_tft:2.0 axi_tft_0
endgroup
startgroup
set_property -dict [list CONFIG.C_TFT_INTERFACE {0} CONFIG.C_DEFAULT_TFT_BASE_ADDR {0x10000000} CONFIG.C_M_AXI_DATA_WIDTH {32} CONFIG.C_MAX_BURST_LEN {16}] [get_bd_cells axi_tft_0]
endgroup
startgroup
set_property -dict [list CONFIG.C_M_AXI_DATA_WIDTH {32} CONFIG.C_MAX_BURST_LEN {16}] [get_bd_cells axi_tft_0]
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/axi_tft_0/M_AXI_MM" Clk "Auto" }  [get_bd_intf_pins processing_system7_0/S_AXI_HP0]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins axi_tft_0/S_AXI_MM]
startgroup
set_property range 256M [get_bd_addr_segs {axi_tft_0/Video_data/SEG_processing_system7_0_HP0_DDR_LOWOCM}]
set_property offset 0x10000000 [get_bd_addr_segs {axi_tft_0/Video_data/SEG_processing_system7_0_HP0_DDR_LOWOCM}]
endgroup

# Create concat R for TFT
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0
endgroup
startgroup
set_property -dict [list CONFIG.NUM_PORTS {1} CONFIG.IN0_WIDTH {4}] [get_bd_cells xlconcat_0]
endgroup
set_property location {2 839 968} [get_bd_cells xlconcat_0]
set_property location {2 913 961} [get_bd_cells xlconcat_0]
set_property location {2.5 955 952} [get_bd_cells xlconcat_0]
set_property name xlconcat_tft_r [get_bd_cells xlconcat_0]
connect_bd_net [get_bd_pins axi_tft_0/tft_vga_r] [get_bd_pins xlconcat_tft_r/In0]
startgroup
create_bd_port -dir O -from 3 -to 0 tft_vga_r
connect_bd_net [get_bd_pins /xlconcat_tft_r/dout] [get_bd_ports tft_vga_r]
endgroup

# Create concat G for TFT
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0
endgroup
set_property name xlconcat_tft_g [get_bd_cells xlconcat_0]
startgroup
set_property -dict [list CONFIG.NUM_PORTS {1} CONFIG.IN0_WIDTH {4}] [get_bd_cells xlconcat_tft_g]
endgroup
connect_bd_net [get_bd_pins axi_tft_0/tft_vga_g] [get_bd_pins xlconcat_tft_g/In0]
startgroup
create_bd_port -dir O -from 3 -to 0 tft_vga_g
connect_bd_net [get_bd_pins /xlconcat_tft_g/dout] [get_bd_ports tft_vga_g]
endgroup

# Create concat B for TFT
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0
endgroup
set_property name xlconcat_tft_b [get_bd_cells xlconcat_0]
startgroup
set_property -dict [list CONFIG.NUM_PORTS {1} CONFIG.IN0_WIDTH {4}] [get_bd_cells xlconcat_tft_b]
endgroup
connect_bd_net [get_bd_pins axi_tft_0/tft_vga_b] [get_bd_pins xlconcat_tft_b/In0]
startgroup
create_bd_port -dir O -from 3 -to 0 tft_vga_b
connect_bd_net [get_bd_pins /xlconcat_tft_b/dout] [get_bd_ports tft_vga_b]
endgroup

# Finsih setup TFT interface
startgroup
create_bd_port -dir O tft_hsync
connect_bd_net [get_bd_pins /axi_tft_0/tft_hsync] [get_bd_ports tft_hsync]
endgroup
startgroup
create_bd_port -dir O tft_vsync
connect_bd_net [get_bd_pins /axi_tft_0/tft_vsync] [get_bd_ports tft_vsync]
endgroup
connect_bd_net [get_bd_pins processing_system7_0/FCLK_CLK1] [get_bd_pins axi_tft_0/sys_tft_clk]
regenerate_bd_layout


#-- IPC

# Setup hardware mutex 0,1,2
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:mutex:2.1 mutex_0
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins mutex_0/S0_AXI]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_1 (Periph)" Clk "Auto" }  [get_bd_intf_pins mutex_0/S1_AXI]
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:mutex:2.1 mutex_1
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins mutex_1/S0_AXI]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_1 (Periph)" Clk "Auto" }  [get_bd_intf_pins mutex_1/S1_AXI]
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:mutex:2.1 mutex_2
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins mutex_2/S0_AXI]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_1 (Periph)" Clk "Auto" }  [get_bd_intf_pins mutex_2/S1_AXI]

# Setup Mailbox
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:mailbox:2.1 mailbox_0
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" Clk "Auto" }  [get_bd_intf_pins mailbox_0/S0_AXI]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_1 (Periph)" Clk "Auto" }  [get_bd_intf_pins mailbox_0/S1_AXI]


#-- GPIO

# Create GPIO 0
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_0
apply_board_connection -board_interface "btns_5bits" -ip_intf "axi_gpio_0/GPIO" -diagram "design_dual_tf0_gpio1" 
endgroup
startgroup
apply_board_connection -board_interface "leds_8bits" -ip_intf "/axi_gpio_0/GPIO2" -diagram "design_dual_tf0_gpio1" 
endgroup
startgroup
set_property -dict [list CONFIG.C_INTERRUPT_PRESENT {1}] [get_bd_cells axi_gpio_0]
endgroup

# Create GPIO 1
startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_1
apply_board_connection -board_interface "sws_8bits" -ip_intf "axi_gpio_1/GPIO" -diagram "design_dual_tf0_gpio1" 
endgroup
startgroup
set_property -dict [list CONFIG.C_INTERRUPT_PRESENT {1}] [get_bd_cells axi_gpio_1]
endgroup

# Connect GPIOs to uB1
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_1 (Periph)" Clk "Auto" }  [get_bd_intf_pins axi_gpio_0/S_AXI]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_1 (Periph)" Clk "Auto" }  [get_bd_intf_pins axi_gpio_1/S_AXI]

#-- Interrupts

# Setup size of concatenation
startgroup
set_property -dict [list CONFIG.NUM_PORTS {2}] [get_bd_cells microblaze_0_xlconcat]
endgroup
startgroup
set_property -dict [list CONFIG.NUM_PORTS {4}] [get_bd_cells microblaze_1_xlconcat]
endgroup

# GPIO interrupts
connect_bd_net [get_bd_pins axi_gpio_0/ip2intc_irpt] [get_bd_pins microblaze_1_xlconcat/In1]
connect_bd_net [get_bd_pins axi_gpio_1/ip2intc_irpt] [get_bd_pins microblaze_1_xlconcat/In2]

# Mailbox interrupts
connect_bd_net [get_bd_pins mailbox_0/Interrupt_0] [get_bd_pins microblaze_0_xlconcat/In1]
connect_bd_net [get_bd_pins mailbox_0/Interrupt_1] [get_bd_pins microblaze_1_xlconcat/In3]


regenerate_bd_layout
assign_bd_address
save_bd_design
