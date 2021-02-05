#include "proje.h"

char sevenseg(short number){
  if      (number==0) return 0b0111111;
  else if (number==1) return 0b0000110;
  else if (number==2) return 0b1011011;
  else if (number==3) return 0b1001111;
  else if (number==4) return 0b1100110;
  else if (number==5) return 0b1101101;
  else if (number==6) return 0b1111101;
  else if (number==7) return 0b0000111;
  else if (number==8) return 0b1111111;
  else if (number==9) return 0b1101111;
  else return 0b1111110;
}

void hexDisp(volatile unsigned int digits){
	int* SEG0 = (int*)0xFF200020;
	int* SEG1 = (int*)0xFF200030;
	short dummy;
	dummy=digits%10;
	char HEX0 = sevenseg(dummy);
	dummy = (digits/10) %10 ;
	char HEX1 = sevenseg(dummy);
	dummy = (digits/100) %10;
	char HEX2 = sevenseg(dummy);
	dummy = (digits/1000) %10;
	char HEX3 = sevenseg(dummy);
	dummy = (digits/10000) %10;
	char HEX4 = sevenseg(dummy);
	dummy = (digits/100000) %10;
	char HEX5 = sevenseg(dummy);

	*SEG0 = (HEX3 << 24) | (HEX2 << 16) | (HEX1 << 8) | (HEX0);
	*SEG1 = (HEX5 << 8)| HEX4;
}

void powerOn(){
  *PWREN = 0b1;
}

void disableInterrupts(){
  *INTMASK = 0;
  *RINTSTS = 0XFFFFFFFF;
  unsigned int temp;
  temp = *CTRL;
  temp = temp & 0xFFFFFFEF;   //set int_enable to 0
  *CTRL = temp;
}

void send_command(unsigned int command, unsigned int argument){
  *CMDARG = argument;  //Send with argument 'argument'
  unsigned int temp = *CMD;
  //send_initialization = 0, bit 15
  temp = temp & 0xFFFF7FFF;
  //cmd = command
  temp = temp & 0xFFFFFFC0;
  temp +=command;
  //start_cmd  = 1
  temp = temp | 0x80000000;
  *CMD = temp;
  volatile int flag = 1;
  while(flag){
    flag = *CMD;
    flag = flag & 0x80000000;
  }//Then command is accepted.
  //TODO:Check if there is a hardware lock error?
  //Wait for command execution to complete.
  flag = 0;
  while(flag != 0b100){//?????
    flag = *RINTSTS;
    flag = flag & 0b100;
  }
  *RINTSTS = 0b100; //clear the command done.
  //Check if bar, rcrc or re is one.
  //These are bit8 bit6 bit1
  flag = *RINTSTS;
  flag = flag & 0b101000010;
  if (flag==0) hexDisp(123456); //success
  else hexDisp(666);            //error
  return;
}

void discoverCardStack(){
  *CTYPE = 0;
  //TODO:Make sure clock <= 400khz
  //Send an SD/SDIO IO_SEND_OP_COND (CMD5) command with argument 0 to the card.
  //write to CMD
  //write to CMDARG
  *CMDARG = 0;  //Send command 5 SD/SDIO IO_SEND_OP_COND with argument 0
  unsigned int temp = *CMD;
  //send_initialization = 1
  temp = temp | 0b1000000000000000;
  //cmd = 5
  temp = temp & 0xFFFFFFC0;
  temp +=5;
  //start_cmd  = 1
  temp = temp | 0x80000000;
  *CMD = temp;
  volatile int flag = 1;
  while(flag){
    flag = *CMD;
    flag = flag & 0x80000000;
  }//Then command is accepted.
  //TODO:Check if there is a hardware lock error?
  //Wait for command execution to complete.
  flag = 0;
  while(flag != 0b100){//?????
    flag = *RINTSTS;
    flag = flag & 0b100;
  }
  *RINTSTS = 0b100; //clear the command done.
  //Check if bar, rcrc or re is one.
  //These are bit8 bit6 bit1
  flag = *RINTSTS;
  flag = flag & 0b101000010;
  if (flag==0) hexDisp(123456); //success
  else hexDisp(666);            //error
  //Read the response. This should be the supported voltage.
  volatile unsigned int response = *RESP0; //the voltage that the card supports.
  hexDisp(response);


  //send the same command with the voltage as the argument
  //write to CMD
  //write to CMDARG
  send_command(5,response);
  //Read the response. Check bit 27.
  response = *RESP0; // Bit 27 must be 0 ?
  if (response & 0x08000000 == 0){ //Then SDIO card.
    //TODO: set clock to 400khz
    //SD card??
    // GO_IDLE_STATE CMD0
    send_command(0, 0);
    // SEND_IF_COND (CMD8)
    send_command(8, 0);
    // SD_SEND_IF_COND (ACMD41)
    send_command(41, 0);
    // ALL_SEND_CID (CMD2)
    send_command(2, 0);
    // SEND_RELATIVE_ADDR (CMD3)
    send_command(3, 0);
  }
  hexDisp(response);
}

void setClocks(){
  //verify card is not in use.
  //check data_busy bit9 of the STATUS register.
  unsigned int temp = *STATUS;
  temp = temp & 0x00000200;
  // reset cclk_enable of CLKENA to 0
  *CLKENA = 0;
  // reset CLKSRC register to 0
  *CLKSRC = 0;
  // set update_clk_regs_only(bit21) and wait_prvdata_complete(bit13) and start_cmd(bit31) to 1 in CMD
  temp = *CMD;
  temp = temp | 0x80202000;
  *CMD = temp;
  //wait until start_cmd(31) and update_clk_regs_only(21) bits change to zero
  volatile int flag = 1;
  while(flag){
    flag = *CMD;
    flag = flag & 0x80200000;
  }//Then command is done executing.
  //Check for hardware lock??
  // Reset the sdmmc_clk_enable bit8 to 0 in the enable register of the
  // clock manager peripheral PLL group (perpllgrp).
  temp = *EN;
  temp = temp & 0b111011111111;
  *EN = temp;
  //In CTRL register of the SDMMC group, set drvsel bits[2:0] and smplse1 bits[5:3]
  temp = *SYSTEM_SDMMC_CTRL;
  temp = 0;
  *SYSTEM_SDMMC_CTRL = temp;
  // Set the sdmmc_clk_enable bit8 to 1 in the enable register of the
  // clock manager peripheral PLL group (perpllgrp).
  temp = *EN;
  temp = temp | 0b000100000000;
  *EN = temp;
  // Set the clkdiv register of the controller to the correct divider value for the required clock frequency
  // Clock divider is 2^n
  // TODO:Default clock is ??
  // If 100 mhz, 100mhz/400khz=250. log2(250) = 7,96
  *CLKDIV = 8;
  //set cclk_enable bit of CLKENA to 1
  *CLKENA = 1;
  return;
}

void setTimeout(){
  //set response timeout, with max data timeout
  *TMOUT = 0xFFFFFF40;
}

void setDebounce(){
  //TODO: determine this value
  //*DEBNCE = 0xFFFFFFFF;
}

void setWatermark(){
  // tx_wmark is 512, rx_wmark is 511
  unsigned int temp = 512 + (511 << 16);
  *FIFOTH = temp;
}

void wait(unsigned int num){
  while(num>0){
    --num;
  }
}

int main(void){
  //TODO:Confirm voltage setting
  //Power on
  powerOn();
  //Wait for power ramp up
  hexDisp(0);
  int i;
  for(i=0; i<1e6; i++){
    wait(0xFFFFFFFF); //1s?
  }
  hexDisp(1);
  //Disable Interrrupts
  disableInterrupts();
  //TODO: Discover the card stack according to the card type. For discovery, you must restrict the clock frequency
  // to 400 kHz in accordance with SD/MMC/CE-ATA standards. For more information, refer to
  setClocks();
  // Enumerated Card Stack.
  discoverCardStack();
  //TODO:Set the clock source assignments. Set the card frequency
  //using the clkdiv and clksrc registers of the
  //controller. For more information, refer to Clock Setup
  // TODO 25Mhz instead of 400khz
  setClocks();
  //set response timeout
  setTimeout();
  //set debounce counter
  setDebounce();
  //set watermark
  setWatermark();

  //Artik kartin kullanima hazir olmasi lazim.


  *BYTCNT = 32; //Read 32 bytes
  *BLKSIZ = 8;  //8 bytes per block
  return 0;
}
