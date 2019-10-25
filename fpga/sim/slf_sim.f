

# This is the simulation root module.
SLF_SIM.v

# This is to simulate Xilinx loading.
#$(XILINX_VER)/glbl.v

# The device-under-test lives here
+libdir+../ver

# SIMBUS support files are here
+libdir+$(SIMBUS_VER)

# Xilinx libraries
#+libdir+$(XILINX_VER)/unisims
