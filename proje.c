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
	int* SEG0 = 0xFF200020;
	int* SEG1 = 0xFF200030;
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
  temp = temp & 0b000000;
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
  if (flag==0) hexdisp(123456); //success
  else hexdisp(666);            //error
  //Read the response
  volatile unsigned int response = *RESP0; //the voltage that the card supports.
  hexdisp(response);
}

void setClocks(){

}

int main(void){
  //TODO:Confirm voltage setting
  //Power on
  powerOn();
  //TODO Wait a few seconds for power ramp up.
  //Disable Interrrupts
  disableInterrupts();
  //TODO: Discover the card stack according to the card type. For discovery, you must restrict the clock frequency
  // to 400 kHz in accordance with SD/MMC/CE-ATA standards. For more information, refer to
  // Enumerated Card Stack.
  discoverCardStack();
  //TODO:Set the clock source assignments. Set the card frequency
  //using the clkdiv and clksrc registers of the
  //controller. For more information, refer to Clock Setup
  setClocks();




  *BYTCNT = 32; //Read 32 bytes
  *BLKSIZ = 8;  //8 bytes per block
  return 0;
}
