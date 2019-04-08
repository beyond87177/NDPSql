source board.tcl
source $connectaldir/scripts/connectal-synth-ip.tcl

connectal_synth_ip aurora_8b10b 11.1 aurora_8b10b_fmc1 [list CONFIG.C_AURORA_LANES {4} CONFIG.C_LANE_WIDTH {4} CONFIG.C_LINE_RATE {4.4} CONFIG.C_REFCLK_FREQUENCY {275} CONFIG.C_INIT_CLK {110.0} CONFIG.Interface_Mode {Streaming} CONFIG.C_GT_LOC_4 {4} CONFIG.C_GT_LOC_3 {3} CONFIG.C_GT_LOC_2 {2} CONFIG.C_START_QUAD {Quad_X0Y5} CONFIG.C_START_LANE {X0Y20} CONFIG.C_REFCLK_SOURCE {MGTREFCLK1 of Quad X0Y5} CONFIG.CHANNEL_ENABLE {X0Y20 X0Y21 X0Y22 X0Y23}]
#[list CONFIG.C_AURORA_LANES {4} CONFIG.C_LANE_WIDTH {4} CONFIG.C_LINE_RATE {4.4} CONFIG.C_REFCLK_FREQUENCY {275} CONFIG.Interface_Mode {Streaming} CONFIG.C_GT_LOC_24 {4} CONFIG.C_GT_LOC_23 {3} CONFIG.C_GT_LOC_22 {2} CONFIG.C_GT_LOC_21 {1} CONFIG.C_GT_LOC_1 {X}]

