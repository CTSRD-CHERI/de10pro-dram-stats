# Copyright (c) 2022 Simon W. Moore
# All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause
# 
# This software was developed at the University of Cambridge Computer
# Laboratory (Department of Computer Science and Technology).
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.


PROJECT     = DE10_Pro.qpf
QSYS        = niosv_smb_reader.qsys
VERILOG_SRC = DE10_Pro.v			\
	      $(wildcard Fan/*.v)

.PHONY: help
help:
	@echo "Build steps:"
	@echo "0. Clean up: make clean"
	@echo "1. Generate the FPGA image (also does step 1): make fpga_image"
	@echo "2. Compile the software (also generates BSP): make build_software"
	@echo "3. Program FPGA: make program_fpga"
	@echo "4. Run the software: make run_software"
	@echo "5. Run the software and analyse results: make analysis"
	@echo "Alternatively, you can use 'make all' to complete steps 1 and 2"

.PHONY: all
all: fpga_image build_software


#-----------------------------------------------------------------------------
# build the FPGA image
#  - also generates IP required by DE10_Pro.qsf
.PHONY: fpga_image
fpga_image: output_files/DE10_Pro.sof

output_files/DE10_Pro.sof: $(VERILOG_SRC) generate_ip
	quartus_sh --flow compile $(PROJECT)

#-----------------------------------------------------------------------------
# dumb generation of IP
# - just generate it all, don't attempt to determine if it needs to be done
.PHONY: generate_ip
generate_ip:
	qsys-generate --quartus-project=$(PROJECT) --synthesis niosv_smb_reader.qsys
	quartus_ipgenerate --generate_project_ip_files -synthesis=verilog $(PROJECT) --clear_ip_generation_dirs

#-----------------------------------------------------------------------------
# program the FPGA
.PHONY: program_fpga
program_fpga:
	quartus_pgm -c 1 -m JTAG -o p\;output_files/DE10_Pro.sof@1
#	../py/flashDE10 output_files/DE10_Pro.sof


#-----------------------------------------------------------------------------
# software build

.PHONY: build_software
build_software: software/bsp/settings.bsp software/app/build/Makefile
	make -C software/app/build

software/bsp/settings.bsp: $(PROJECT) $(QSYS)
	niosv-bsp -c --type=hal --quartus-project=$(PROJECT) --qsys=$(QSYS) software/bsp/settings.bsp

software/app/CMakeList.txt: software/bsp/settings.bsp
	niosv-app --bsp-dir=./software/bsp --app-dir=software/app --srcs=software/app --elfname=app.elf
software/app/build/Makefile: software/app/CMakeList.txt
	cmake -S software/app -G "Unix Makefiles" -B software/app/build

.PHONY: run_software
run_software: build_software
	niosv-download -g ./software/app/build/app.elf
	juart-terminal

.PHONY: analysis
analysis: build_software
	niosv-download -g ./software/app/build/app.elf
	juart-terminal | ../analyse/analyse.py

#-----------------------------------------------------------------------------
# clean up
clean:
	rm -rf output_files tmp-clearbox qdb synth_dumps
	quartus_ipgenerate --clear_ip_generation_dirs $(PROJECT)
	make -C software/app/build clean
	rm -rf software/bsp software/app/build
