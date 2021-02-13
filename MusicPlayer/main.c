//Standard Libraries
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
//Altera Libraries


#include "alt_sdmmc.h"
#include "fs/fat_filelib.h"
//Global Definitions
#define SLIDERS_PTR ((volatile unsigned int*) 0xFF200040)
#define LED_PTR ((volatile unsigned int*) 0xFF200000)
#define FIFOTH ((volatile unsigned int*) 0xFF70404C)
#define SEG0 ((volatile unsigned int*) 0xFF200020)
#define SONG_NUM 7
#define MAX_FILE_SIZE 55877136

//Interrupt Variables

extern volatile int songchanging;
extern volatile int press;
extern volatile int volume;
extern volatile int songnumber;


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
void hexDisp(volatile int ind){

	ind=ind/48000;

	unsigned int sec=ind%60;
	unsigned int min=ind/60;
	char HEX0 = sevenseg(sec%10);
	char HEX1 = sevenseg(sec/10);
	char HEX2 = sevenseg(min%10);
	*SEG0 = (HEX2 << 16) | (HEX1 << 8) | (HEX0);

}
void showLED(unsigned int value){// As bit values.
  int* led_ptr = (int *) 0xFF200000;			//led port address, 8bit unsigned
  *led_ptr = value;
}



void set_A9_IRQ_stack (void){
  int stack, mode;
  stack = 0xFFFFFFFF-7;
  // top of A9 on-chip memory, aligned to 8 bytes
  /* change processor to IRQ mode with interrupts disabled */
  mode = 0b11010010;
  asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
  /* set banked stack pointer */
  asm("mov sp, %[ps]" : : [ps] "r" (stack));
  /* go back to SVC mode before executing subroutine return! */
  mode = 0b11010011;
  asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
}
void config_GIC (void){
  /* configure the HPS timer interrupt */
  *((int *) 0xFFFED8C4) = 0x01000000;
  *((int *) 0xFFFED118) = 0x00000080;
  /* configure the FPGA interval timer and KEYs interrupts */
  *((int *) 0xFFFED848) = 0x00000101;
  *((int *) 0xFFFED108) = 0x00000300;
  // Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all priorities
  *((int *) 0xFFFEC104) = 0xFFFF;
  // Set CPU Interface Control Register (ICCICR). Enable signaling of interrupts
  *((int *) 0xFFFEC100) = 1;  // enable = 1
  // Configure the Distributor Control Register (ICDDCR) to send pending interrupts to CPUs
  *((int *) 0xFFFED000) = 1;  // enable = 1

}
void config_interval_timer (void){
  volatile int * interval_timer_ptr = (int *) 0xFF202000; // interal timer base address
  /* set the interval timer period for scrolling the HEX displays */
  //unsigned int counter = 10000000;  // .1 sec
  //int counter = 5000000;  // 1/(100 MHz) ×(5000000) = 50 msec

  //Default counter=0.125ms
  *(interval_timer_ptr) = 0b10;
  /*
  *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
  *(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF; */
  /* start interval timer, enable its interrupts */
  *(interval_timer_ptr + 1) = 0x7;  // STOP = 0, START = 1, CONT = 1, ITO = 1
}
void config_keys (void){
	volatile int *KEY_ptr=(int*)0xFF200050;
	*(KEY_ptr+2) =0xF;
}
void enable_A9_interrupts (void){
  int status = 0b01010011;
  asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}
void incrementLED(){
int val = *LED_PTR;
if (val==0b100000000) val=1;
else val=val<<1;
*LED_PTR = val;
}


void interval_timer_ISR (void){
  volatile int * interval_timer_ptr = (int *) 0xFF202000; // Altera timer address
  //*(interval_timer_ptr) = 0b10;  // clear the interrupt
  *(interval_timer_ptr) = 0;  // clear the interrupt
  if(songchanging) incrementLED();
  return;
}
void keys_ISR(){
	int* button_addr = (int*) 0xFF200050;
	press=*(button_addr+3);
	*(button_addr+3)=0xFF;
	if (press & 0b1){//Volume +
			if (volume <750) volume+=75;
		}
		else if(press & 0b10){//Volume -
			if (volume > 0) volume-=75;
		}
		else if(press & 0b100){ //next song
			if(songnumber<SONG_NUM-1){
				songnumber++;
				songchanging=1;
			}
			else{
				songnumber = 0;
				songchanging=1;
			}
		}
		else if(press & 0b1000){ //prev song
			if(songnumber > 0){
				songnumber--;
				songchanging=1;
				}
			else{
				songnumber = SONG_NUM-1;
				songchanging=1;
			}
		}
		press=0;
}

/* Define the IRQ exception handler */
void __attribute__ ((interrupt)) __cs3_isr_irq (void){
  // Read the ICCIAR from the processor interface
  int int_ID = *((int *) 0xFFFEC10C);
  if (int_ID == 72)    // check if interrupt is from the Altera timer
	interval_timer_ISR ();
  if (int_ID == 73)
	keys_ISR ();
  // Write to the End of Interrupt Register (ICCEOIR)
  *((int *) 0xFFFEC110) = int_ID;
  return;
}
// Define the remaining exception handlers */
void __attribute__ ((interrupt)) __cs3_isr_undef (void){
while (1);
}
void __attribute__ ((interrupt)) __cs3_isr_swi (void){
while (1);
}
void __attribute__ ((interrupt)) __cs3_isr_pabort (void){
while (1);
}
void __attribute__ ((interrupt)) __cs3_isr_dabort (void){
while (1);
}
void __attribute__ ((interrupt)) __cs3_isr_fiq (void){
while (1);
}




ALT_SDMMC_CARD_INFO_t Card_Info;

int read(uint32_t sector, uint8_t *buffer, uint32_t sector_count)
{
  //Should normally be done in a for loop. Remove *sector_count and put into loop.
  //For defragmented files, this is much faster...
	alt_sdmmc_read(&Card_Info, buffer, (void*)(sector*FAT_SECTOR_SIZE), FAT_SECTOR_SIZE*sector_count);
	return 1;
}

void initilize(void)
{
	//Interrupt Initilizations
	set_A9_IRQ_stack ();  // initialize the stack pointer for IRQ mode
	config_GIC ();        // configure the general interrupt controller
	config_interval_timer (); // configure Altera interval timer to generate interrupts
	config_keys();			  // configure keys to generate interrupts
	enable_A9_interrupts ();
	*LED_PTR=1;
	alt_sdmmc_init();
	alt_sdmmc_card_pwr_on();
	alt_sdmmc_card_identify(&Card_Info); // Card_Info.card_type == ALT_SDMMC_CARD_TYPE_SDHC
	alt_sdmmc_card_bus_width_set(&Card_Info, ALT_SDMMC_BUS_WIDTH_4);
	//Set tx and rx watermark levels
	*FIFOTH = ((0x200 << 16) + (0x200));
	alt_sdmmc_card_speed_set(&Card_Info, 2 * Card_Info.xfer_speed);
	fl_init();
	// Attach media access functions to library
	fl_attach_media(read, NULL);
}

//Required by alt_sdmmc.h
int print_debug(const char* fmt, ...){
	return 127;
}
void changesong(short* musicptr,int songnumber,unsigned int* buffer_index,unsigned int* size){

	*buffer_index=0;
	FL_FILE* file;

	if(songnumber==0){
		file = fl_fopen("/stir.bin", "rb");
	}
	else if(songnumber==1){
		file = fl_fopen("/master.bin", "rb");
	}
	else if(songnumber==2){
		file = fl_fopen("/jazz.bin", "rb");
	}
	else if(songnumber==3){
		file = fl_fopen("/unforgiven.bin", "rb");
	}
	else if(songnumber==4){
		file = fl_fopen("/ciao.bin", "rb");
	}
	else if(songnumber==5){
		file = fl_fopen("/mesafe.bin", "rb");
	}
	else if(songnumber==6){
		file = fl_fopen("/klasik.bin", "rb");
	}


	fl_fseek(file, 0, SEEK_END); // seek to end of file
	*size = fl_ftell(file); // get current file pointer
	fl_fseek(file, 0, SEEK_SET);
	fl_fread(musicptr, 2, ((*size)/2), file);
	fl_fclose(file);
	songchanging=0;
}



/* These global variables are written by interrupt service routines; we have to declare these as volatile
* to avoid the compiler caching their values in registers */
volatile int press = 0;
volatile int songchanging = 1;
volatile int songnumber = 0;
volatile int volume = 375;

/********************************************************************************
* Main program
********************************************************************************/
void main(void)
{
	initilize();

	//Allocating Space in ram to write songs
	short* musicptr = (short *) malloc(MAX_FILE_SIZE);
	unsigned int size=0;
	unsigned int buffer_index = 0;

	changesong(musicptr,songnumber,&buffer_index,&size);

	volatile int * audio_ptr = (int *) 0xFF203040; 	// audio port address = 0xFF203040
	volatile int fifospace;
	fifospace = *(audio_ptr + 1);    //One register to rule them all. SUPER DUPER important.
	// read the audio port fifospace register, 0xFF203040 [WSLC WSRC RALC RARC]
	while(1){
		fifospace = *(audio_ptr + 1);
		if ( (fifospace & 0xFF000000) > 0x20000000) // check WSLC, for more than %25 writable 0x 0011 1111
		{
				/* insert data until the audio-out FIFO is full, or the data is depleted. */
			while ( (fifospace & 0xFF000000)  && (buffer_index < size) )
			{
				int * addr = (int *) 0xFF203040;
				addr = addr +2;
				int temp = (musicptr[buffer_index] * volume);

				*addr = temp ; //Leftdata
				addr = addr+1;
				*addr =  temp ; //Rightdata
				++buffer_index;						//Increment buffer index
				fifospace = *(audio_ptr + 1); // read the audio port fifospace register

			}
		}
		//Writes the song duration
		hexDisp(buffer_index);

		volatile unsigned int sliders = *SLIDERS_PTR;
		//Pause Condition
		while(sliders){
			 sliders = *SLIDERS_PTR;
		};
		//Display time
		//Change song if key pressed
		if (songchanging){
		changesong(musicptr,songnumber,&buffer_index,&size);
		}
		//Change song if song ended
		if(buffer_index >= (size/2)) {
			songchanging=1;
			if(songnumber<SONG_NUM-1){
				songnumber++;
				changesong(musicptr,songnumber,&buffer_index,&size);
			}
			else{
				songnumber = 0;
				changesong(musicptr,songnumber,&buffer_index,&size);
			}
		}
	}
	free(musicptr);
	fl_shutdown();
	return;
}
