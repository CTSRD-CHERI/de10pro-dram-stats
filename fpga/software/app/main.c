#include <stdio.h>
#include <altera_avalon_i2c.h>
#include <system.h>

void print_code(ALT_AVALON_I2C_STATUS_CODE code) {
  switch(code)
    {
    case ALT_AVALON_I2C_TRUE:         printf("Status code = TRUE\n");           break;
    case ALT_AVALON_I2C_SUCCESS:      printf("Status code = SUCCESS\n");        break;
    case ALT_AVALON_I2C_ERROR:        printf("Status code = ERROR\n");          break;
    case ALT_AVALON_I2C_TIMEOUT:      printf("Status code = TIMEOUT\n");        break;
    case ALT_AVALON_I2C_BAD_ARG:      printf("Status code = BAD_ARG\n");        break;
    case ALT_AVALON_I2C_RANGE:        printf("Status code = RANGE\n");          break;
    case ALT_AVALON_I2C_NACK_ERR:     printf("Status code = NACK error\n");     break;
    case ALT_AVALON_I2C_ARB_LOST_ERR: printf("Status code = ARB_LOST error\n"); break;
    case ALT_AVALON_I2C_BUSY:         printf("Status code = BUSY\n");           break;
    default:                          printf("Unknown status code = %d\n", (int) code);
    }
}


void decode_ddr4(alt_u8 *rxbuf) {
  // DDR4 coding information available from JEDEC:
  // https://www.jedec.org/system/files/docs/4_01_02_AnnexL-6R30.pdf
  // Note that the wikipedia page is one of date when I checked 19 Jan 2021:
  // https://en.wikipedia.org/wiki/Serial_presence_detect
  // This is helpful:
  // https://www.simmtester.com/News/PublicationArticle/184
  printf("SPD revision = 0x%02x (expecting 0x11 or 0x12)\n", rxbuf[0x01]);
  int sizeMB = 256<<(((int) rxbuf[0x04]) & 0x0f);
  printf("Capacity per die = %d MB = %d GB\n", sizeMB, sizeMB/1024);
  printf("Number of banks = %d\n", 4<<((rxbuf[0x4]>>4) & 0x03));
  printf("Bank groups = %d\n", 2*((rxbuf[0x4]>>6) & 0x03));
  printf("Row address bits = %d \tColumn address bits = %d\n",
	 ((rxbuf[0x05]>>3)&0x7)+12, 
	 (rxbuf[0x05]&0x7)+9); 
  switch(rxbuf[0x03]) {
  case 0x02: printf("UDIMM scheme: UDIMM\n"); break;
  case 0x03: printf("UDIMM scheme: SO-DIMM\n"); break;
  case 0x06: printf("UDIMM scheme: mini-UDIMM\n"); break;
  case 0x09: printf("UDIMM scheme: 72b SO-UDIMM\n"); break;
  case 0x0c: printf("UDIMM scheme: 16b SO-UDIMM\n"); break;
  case 0x0d: printf("UDIMM scheme: 32b SO-UDIMM\n"); break;
  default: printf("Unknown UDIMM scheme: 0x%02x\n", rxbuf[0x03]);
  }
  printf("Die count = %d\n", ((rxbuf[0x06]>>4) & 0x07)+1);
  int j = rxbuf[0x0c];
  int ranks = ((j>>3)&0x07)+1;
  int width = 4<<(j&0x07);
  int asymetric = (j>>6) & 1;
  printf("Module organization:");
  if(asymetric==1)
    printf("Asymentric");
  else
    printf("Symetric");
  printf(", Num package ranks per DIMM = %d, SDRAM device width = %d\n",
	 ranks, width);
  int jep = (int) rxbuf[0x75] | ((int) rxbuf[0x76])<<8;
  printf("JEP-106 module manufacturer ID = 0x%04x = %d\n",jep,jep);
}


int download_eeprom(alt_u8 *rxbuf, const int buf_read_len, char *device_name) {
  ALT_AVALON_I2C_STATUS_CODE status;
  // start address to access on I2C device (big endian)
  alt_u8 addr_buf[2];
  addr_buf[0] = 0x00;   addr_buf[1] = 0x00;
  ALT_AVALON_I2C_DEV_t* i2c_dev = alt_avalon_i2c_open(I2C_B_NAME);
  if(i2c_dev == NULL) {
    printf("Failed to open I2C device %s\n", device_name);
    return 1;
  }
  
  // set the address of the target device: 0x50
  alt_avalon_i2c_master_target_set(i2c_dev,0x50);
  // transmit start address of EEPROM and read results
  status = alt_avalon_i2c_master_tx_rx(i2c_dev, addr_buf, 2, rxbuf, buf_read_len, ALT_AVALON_I2C_NO_INTERRUPTS);
  if(status==ALT_AVALON_I2C_SUCCESS)
    return 0;
  else {
     print_code(status);
     return 1;
  }
}


int download_display_eeprom(char *device_name, char *dram_channel_name) {
  const int buf_len = 0x200;
  const int buf_read_len = 384;
  alt_u8 rxbuf[buf_len];
  alt_u8 rxbuf_check[buf_len];
  for(int j=0; j<buf_len; j++) {
    rxbuf[j] = 0xaa;
    rxbuf_check[j] = 0xbb;
  }

  if(download_eeprom(rxbuf, buf_read_len, I2C_B_NAME) ||
     download_eeprom(rxbuf_check, buf_read_len, I2C_B_NAME)) {
    printf("Failed to download EEPROM from %s\n", device_name);
    return 1;
  }
  for(int j=0; j<buf_read_len; j++)
    if(rxbuf[j]!=rxbuf_check[j]) {
      printf("Difference in EEPROM download at address 0x%03x: 0x%02x != 0x%02x\n",
	     j, (int) rxbuf[j], (int) rxbuf_check[j]);
      return 1;
    }
  printf("\"%s\": [", dram_channel_name);
  for(int j=0; j<buf_read_len; j++) {
    printf("%3d", (int) rxbuf[j]);
    if((j+1)<buf_read_len)
      printf(", ");
    if((j%8)==7)
      printf("\n             ");
  }
  printf("]\n");
  return 0;
}


int main() {
  printf("--------JSON DUMP START--------\n");
  printf("{ ");
  if(download_display_eeprom(I2C_B_NAME, "DDR4_B"))
    printf("\nFAILED\n");
  printf(" }\n\n");
  printf("--------JSON DUMP END--------\n");
  printf("\004\n"); // sent ctl-D to exit terminal
  return 0;
}

