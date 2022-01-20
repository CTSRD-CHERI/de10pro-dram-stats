#!/usr/bin/env python3
##############################################################################
# Analyse DRAM EEPROM data
##############################################################################
# Copyright (c) Simon Moore, January 2022

import json
import sys

def extractbits(w,upper,lower):
    return (w>>lower) & ((1<<(upper-lower+1))-1)

def decode_ddr4(ddr):
    if(ddr[2]==12):
        print("Found DDR4 memory format")
    else:
        print("Not a DDR4 format: code=%d" % (ddr[2]))
        return

    print("Number of bytes in EEPROM = %d and %d have been downloaded" %
          (extractbits(ddr[0x000],3,0)*128, len(ddr)))
    print("SPD revision hex = 0x%02x" % (ddr[0x001]))
    udimm = {0x02: "UDIMM",
             0x03: "SO-UDIMM",
             0x06: "mini-UDIMM",
             0x09: "72b SO-UDIMM",
             0x0c: "16b SO-UDIMM",
             0x0d: "32b SO-UDIMM"}
    if(udimm[ddr[3]]):
        print("UDIMM format: ", udimm[ddr[3]]);
    else:
        print("Unknown UDIMM format")

    die_count = extractbits(ddr[0x006], 2, 0)+1
    asymetric = extractbits(ddr[0x00c], 6, 6)
    page_ranks = extractbits(ddr[0x00c], 5, 3)+1
    sdram_device_width = 4<<extractbits(ddr[0x00c], 2, 0)
    print("chip organization:")
    print("   die count = %d" % (die_count))
    print("   asymetric" if (asymetric==1) else "   symetric")
    print("   page ranks = %d" % (page_ranks))
    print("   width (per chip) = %d" % (sdram_device_width))
    module_bus_width = 8<<extractbits(ddr[0x00d], 2, 0)
    module_ecc_width = 8*extractbits(ddr[0x00d], 4, 3)
    print("Module bus width = %d" % (module_bus_width))
    print("Module ecc width = %d" % (module_ecc_width))
    sdram_minimum_cycle_time_mtb_units = ddr[0x012]
    sdram_minimum_cycle_time_ns = 0.125 * sdram_minimum_cycle_time_mtb_units
    ddr_speed = 2000/sdram_minimum_cycle_time_ns
    print("Speed grade = DDR4-%d" % (ddr_speed))
    b = extractbits(ddr[0x004],3,0)
    if(b<=7):
        density_per_chip_Mb = 256<<b
    else:
        print("Unknown density")
        density_per_chip_Mb = 0
    print("Density per chip = %d Mb = %dGb" % (density_per_chip_Mb, density_per_chip_Mb//1024))
    capacity_MB = density_per_chip_Mb * (module_bus_width/sdram_device_width) // 8
    print("Total capacity = %d MB = %dGB" % (capacity_MB, capacity_MB//1024))
    

def parse_stdin():
    jsonin=""
    for line in sys.stdin:
        if("JSON DUMP END" in line):
            break;
        if(len(jsonin)>0):
            jsonin = jsonin + line
        if("JSON DUMP START" in line):
            jsonin=" "
    print(jsonin)
    d = json.loads(jsonin)
    ddrB = d["DDR4_B"]
    decode_ddr4(ddrB)
    
if __name__=='__main__':
    parse_stdin()

