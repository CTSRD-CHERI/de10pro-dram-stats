#!/bin/bash
##############################################################################
# Downloads a prebuilt FPGA image containing a NIOSV core and runs 
# a small app that reads the EEPROM data from the SODIMM memory modules
# that is analysed by a Python script
#
# Note that this has been tested for the Stratix 10 SX with a single FPGA
# on the JTAG chain.  If the jtag chain isn't found, try running jtagconfig
# first.

if [[ ! -e prebuilt/DE10_Pro.sof ]]; then
    echo "Extracting SOF file"
    bunzip2 -k prebuilt/DE10_Pro.sof.bz2
fi
quartus_pgm -c 1 -m JTAG -o p\;prebuilt/DE10_Pro.sof@1
niosv-download -g prebuilt/app.elf
juart-terminal | analyse/analyse.py
