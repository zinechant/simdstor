# Script for UMC013 by Vincent Chan, IPEL, HKUST
# Version : 5-Nov-2013
# Changed to UMC065 by Yasu Lu, IPEL, HKUST
# Version : Jun-2015
# Revised by Feng Chen, IPEL, HKUST
# Version : Apr-2018

# time_unit : "1 ns" ;
# capacitive_load_unit : "1 pF" ;
# voltage_unit : "1 V" ;
# current_unit : "1 mA" ;
# leakage_power_unit : "1 pW" ;
# pulling_resistance_unit : "1 kohm" ;
# dynamic energy unit: "1 pJ" ;
# bus naming style : "%s[%d] (default)" ;

# if using "uk65lscllmvbbl_120c25_tc"
# default process: 1.00
# temperature: 25.00
# voltage: 1.20

# Set top module / design name
set DesignName crossbar

#---- Start to analyze, elaborate and uniquify ----#
analyze -format sverilog -library synthesis $DesignName.sv
elaborate $DesignName -library synthesis
uniquify

#---- All constraints are listed below ----#
set clk_period 0.5
# Create a real clock if clock port is found
if {[sizeof_collection [get_ports clk]] > 0} {
  set clk_name clk
  create_clock -period $clk_period $clk_name
}
# Create a virtual clock if clock port is not found
if {[sizeof_collection [get_ports clk]] == 0} {
  set clk_name vclk
  create_clock -period $clk_period -name $clk_name
}

#---- Set up clock property ----#
# Set clock latency, transition and uncertainty
set_clock_latency 0.0 $clk_name
set_clock_transition 0.01 $clk_name
set_clock_uncertainty -setup 0.1 $clk_name
set_clock_uncertainty -hold 0.1 $clk_name
# Prevent Design compiler from adding buffers to the clock network
# set_dont_touch_network $clk_name
# Hold time violations of clock are fixed in encounter
# set_fix_hold $clk_name

#---- Input transition and delay, and output delay and fanout ----#
# set_input_transition 0.1 \
#   [remove_from_collection [all_inputs] [get_ports clk]]
set_input_delay 0.1 -max -clock $clk_name \
  [remove_from_collection [all_inputs] [get_ports clk]]
set_output_delay 0.1 -max -clock $clk_name [all_output]
set_fanout_load 2.0 [all_outputs]

#---- Output load and assumed input load ----#
set_load -pin_load 0.1 [all_outputs]
# Get the cell from symbol library for driving the input of the digital circuits
# set_driving_cell -lib_cell INVM1W -no_design_rule \
#   [remove_from_collection [all_inputs] [get_ports clk]]

#---- Maximum delay, capacitance, transition, fanout and area constraint ----#
# set_max_delay [expr $clk_period * 0.9] -from [get_ports rst] -to [all_outputs]
set_max_capacitance 1.0 $DesignName
set_max_transition 0.1 $DesignName
set_max_fanout 20.0 $DesignName
# set_max_area 0.0

# Remove assign statements in verilog netlist
set_fix_multiple_port_nets -all -buffer_constants

# Group paths into four categories: IN2REG, REG2OUT, REG2REG and IN2OUT
group_path -name IN2REG -from [remove_from_collection [all_inputs] [get_ports clk]] \
  -to [all_registers -data_pins]
group_path -name REG2OUT -from [all_registers -clock_pins] -to [all_outputs]
group_path -name REG2REG -from [all_registers -clock_pins] -to [all_registers -data_pins]
group_path -name IN2OUT -from [remove_from_collection [all_inputs] [get_ports clk]] \
  -to [all_outputs]

#---- Make necessary directories for reports ----#
file mkdir reports

#---- Check design and dump report ----#
check_design -summary -nosplit
check_design -nosplit > reports/check_design.rpt

#---- Design compilation and optimization ----#
#compile_ultra -no_autoungroup -exact_map -no_boundary_optimization -top
compile_ultra
# compile -exact_map -map_effort high -area_effort high
optimize_registers
optimize_netlist -area
change_names -rules verilog -hierarchy -verbose

#---- Saving .v .sdc .sdf files ----#
# Synopsys internal database format .ddc file is skipped
#write -format ddc -hierarchy -output $DesignName.ddc
write -format verilog -hierarchy -output ${DesignName}_syn.v
write_sdc -nosplit $DesignName.sdc
write_sdf $DesignName.sdf

#---- Report compilation results ----#
report_constraint -all_violators
report_timing -max_paths 2 -transition_time -nets -attributes -capacitance -nosplit
report_constraint -nosplit > reports/constraint.rpt
report_timing -max_paths 4 -transition_time -nets -attributes -capacitance -nosplit > reports/timing.rpt
report_qor > reports/qor.rpt
report_area -nosplit -physical -hierarchy > reports/area.rpt
report_power -nosplit -verbose > reports/power.rpt
