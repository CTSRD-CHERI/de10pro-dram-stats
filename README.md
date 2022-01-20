# DE10pro (Stratix 10 SX) DRAM Statistics

## Introduction

The objective of this project is to read the Serial Presence Detect
(SPD) EEPROM on the DDR4 channels in order to identify the SODIMMs
installed (data width, capacity, timing parameters, etc.).  The format
of the data in this EEPROM is determined by the [JEDEC
standard](https://jedec.org/) that requires registration to access.  I
pulled the following document _SPD Annex L: Serial Presence Detect
(SPD) for DDR4 SDRAM Modules, Release 6_.  There is also a [Wikipedia
SPD page](https://en.wikipedia.org/wiki/Serial_presence_detect) that
outlines what is in the standard, but the official standard document
contains much more detail.

## Accessing the SPD EEPROM

The SPD EEPROM is accessed over a [System Management Bus
(SMB)](https://en.wikipedia.org/wiki/System_Management_Bus), which is
a derivative of [I2C](https://en.wikipedia.org/wiki/I2C).  I used
Intel's I2C master core in Platform Designer to connect to the SMB.
This has two input signals for serial data and clock suffixed
`_sda_in` and `_scl_in`, and two output signals also for serial data
and clock suffixed `_sda_oe` and `_scl_oe`.  These need to be
connected to the SMB bidirectional DDR4 pins using tristate buffers.
For example:
```
   wire i2c_sda_oe, i2c_scl_oe;
   assign DDR4B_SCL = i2c_scl_oe ? 1'b0 : 1'bz;
   assign DDR4B_SDA = i2c_sda_oe ? 1'b0 : 1'bz;

   niostest qsys0
     (
      .clk_clk           (clk_100),
      .i2c_serial_sda_in (DDR4B_SDA),
      .i2c_serial_scl_in (DDR4B_SCL),
      .i2c_serial_sda_oe (i2c_sda_oe),
      .i2c_serial_scl_oe (i2c_scl_oe),
      .reset_reset       (!reset_n_100)
      );
```


## Prerequisites
* Built using Quartus 21.3pro
* Uses the NIOSV softcore, so the RISC-V toolchain from Intel needed to be installed (it is not installed as part of the Quartus 21.3pro install).
  * See the _Installing NIOSV toolchain_ section below.
* Designed for the Terasic DE10Pro board containing a Stratix 10 SX part 1SX280HU2F50E1VG

## Notes on building

* To build the FPGA image in fpga/:
  ```bash
  cd fpga
  make clean
  make fpga_image
  ```

* To build the software for the NIOSV softcore that will read the DRAM information:
  ```bash
  make build_software
  ```

* To program the FPGA:
  ```bash
  make program_fpga
  ```
  Note that if this fails to find the JTAG chain, try `jtagconfig` first.

* To download and run the software:
  ```bash
  make run_software
  ```

### Installing NIOSV toolchain

#### Documentation
* Based on [Intel Documentation](https://www.intel.cn/content/dam/www/programmable/us/en/pdfs/literature/ug/ug20345.pdf)

#### Install RISC-V cross compiler from xpack
* Project page: <https://github.com/xpack-dev-tools/riscv-none-embed-gcc-xpack>
* get npm if you don't have it installed:
  ```bash
  sudo apt install npm
  ```
* get xpm:
  ```bash
  sudo npm install --global xpm@latest
  ```
* get xpack risc-v tools:
  ```bash
  xpm install --global @xpack-dev-tools/riscv-none-embed-gcc@latest
  ```
  This installed them into ~/.local/xPacks...
* Add path to the tools.  The version I pulled was 10.2.0-1.2.1, so I needed to add the following to my path:
  ```bash
  ~/.local/xPacks/@xpack-dev-tools/riscv-none-embed-gcc/10.2.0-1.2.1/.content/bin/
  ```

#### Example use of tools
* Create board support package (BSP) using the simple hardware abstraction layer (HAL):
  ```bash
  niosv-bsp -c --type=hal --quartus-project=DE10_Pro.qpf --qsys=niostest.qsys software/bsp/niostest_sw.bsp
  ```
* Copy example app:
  ```bash
  mkdir sofware/app
  cp $ECAD/intelFPGA_pro/current/niosv/examples/software/hello_world/hello_world.c software/app/
  ```
* Generate Cmakefile:
  ```bash
  niosv-app --bsp-dir=software/bsp --app-dir=software/app --srcs=software/app --elfname=app.elf
  cmake -S software/app -G "Unix Makefiles" -B software/app/build
  ```
* Build app:
  ```bash
  make -C software/app/build
  ```
* Download FPGA image:
  ```bash
  quartus_pgm -c 1 -m JTAG -o p\;output_files/DE10_Pro.sof@1
  ```
* Download software to NIOSV core and get it to run (`-g` option):
  ```bash
  niosv-download -g ./software/app/build/app.elf
  ```
* Open JTAG UART terminal
  ```bash
  juart-terminal
  ```
  