#define frontBuffer ((volatile unsigned int *) 0xFF203020)
#define backBuffer ((volatile unsigned int *) 0xFF203024)
#define resolutionRegister ((volatile unsigned int *) 0xFF203028)
#define statusRegister ((volatile unsigned int *) 0xFF20302C)
#define next ((volatile unsigned int *) 0x00000500)
#define baseAddr ((volatile unsigned int *) 0x00000504)
/* Initialize the banked stack pointer register for IRQ mode */
void set_A9_IRQ_stack(void){
	int stack, mode;
	stack = 0x2FFFFFFF - 7; 
	mode = 0b11010010;
	asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
	/* set banked stack pointer */
	asm("mov sp, %[ps]" : : [ps] "r" (stack));
	/* go back to SVC mode before executing subroutine return! */
	mode = 0b11010011;
	asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
}

/* Configure the Generic Interrupt Controller (GIC) */
void config_GIC(void){
	
	/* configure the FPGA interval timer interrupt */
	*((int *) 0xFFFED848) = 0x00000001;
	*((int *) 0xFFFED108) = 0x00000100;
	// Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all priorities
	*((int *) 0xFFFEC104) = 0xFFFF;
	// Set CPU Interface Control Register (ICCICR). Enable signaling of interrupts
	*((int *) 0xFFFEC100) = 1; // enable = 1
	// Configure the Distributor Control Register (ICDDCR) to sendpending interrupts to CPUs
	*((int *) 0xFFFED000) = 1; // enable = 1
}

/* Turn on interrupts in the ARM processor */
void enable_A9_interrupts(void){
	int status = 0b01010011;
	asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}
/* Configure the Generic Interrupt Controller (GIC) */

void config_interval_timer(int time){
	volatile int * interval_timer_ptr = (int *) 0xFF202000; // interal timer base address
	/* set the interval timer period for scrolling the HEX displays */
	int counter = time; // 1/(100 MHz)£(5000000) = 50 msec
	*(interval_timer_ptr + 2) = (counter & 0xFFFF);
	*(interval_timer_ptr + 3) = (counter & 0xFFFF0000) >> 16;
	/* start interval timer, enable its interrupts */
	*(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1
}

void interval_timer_ISR( ){
	volatile int * interval_timer_ptr = (int *) 0xFF202000; // Altera timer address
	int pixel_ptr, y, x;
	*(interval_timer_ptr) = 0; // clear the interrupt
	
	
	
	if(*next==0){
		for (y = 0; y <240; y++){
			for (x = 0; x <320; x++){
			
				pixel_ptr = 0xC8000000 + (y<<10)+ (x<<1);
				*(short *)pixel_ptr = *(short *)(*baseAddr + (y<<10)+ (x<<1)); // set pixel color
				
			}
		}
		*next=1;
	}else if(*next==1){
		for (y = 0; y <240; y++){
			for (x = 0; x <320; x++){
			
				pixel_ptr = 0xC8040000 + (y<<10)+ (x<<1);
				*(short *)pixel_ptr = *(short *)(*baseAddr + (y<<10)+ (x<<1)); // set pixel color
				
				
			}
		}
		*next=0;
	}else{
		//pause
	}
	*frontBuffer=1;
	if(*baseAddr!=0x006C0000){
		*baseAddr += 0x00040000;
	}else{
		*baseAddr=0x000C0000;
	}
	

	return;
}
void __attribute__ ((interrupt)) __cs3_isr_irq (void){
	// Read the ICCIAR from the processor interface
	int int_ID = *((int *) 0xFFFEC10C);
	if (int_ID == 72) // check if interrupt is from the Altera timer
	interval_timer_ISR ();
	else
	while (1); // if unexpected, then stay here
	// Write to the End of Interrupt Register (ICCEOIR)
	*((int *) 0xFFFEC110) = int_ID;
	return;
}


void setFrame (unsigned int pixel_ptr, int z){
	
	int y, x;
	unsigned int temp= pixel_ptr;
	for (y = 0; y <240; y++){
		for (x = 0; x <320; x++){
			
			pixel_ptr = temp + (y<<10)+ (x<<1);
			if( x>=z && x < (z+12) && y< 140 && y>= 100){
				*(short *)pixel_ptr = 0xFFFF; // set pixel color
			}else{
				*(short *)pixel_ptr = 0;
			}
			
			
		}
	}
	return;
}


int main(void){
	//configure interrupts and timer
	set_A9_IRQ_stack (); // initialize the stack pointer for IRQ mode
	config_GIC (); // configure the general interrupt controller
	config_interval_timer (4000000); // configure Altera interval timer to generate interrupts, 1 sec
	
	
	*baseAddr = 0x00080000;
	*next =1;
	*backBuffer =0xC8040000;
	*resolutionRegister=0x00F00140;
	//resetting
	
	setFrame(0x000C0000,0);
	setFrame(0x00100000,12);
	setFrame(0x00140000,28);
	setFrame(0x00180000,40);
	setFrame(0x001C0000,52);
	
	setFrame(0x00200000,64);
	setFrame(0x00240000,76);
	setFrame(0x00280000,88);
	setFrame(0x002C0000,100);
	setFrame(0x00300000,112);
	
	setFrame(0x00340000,124);
	setFrame(0x00380000,136);
	setFrame(0x003C0000,148);
	setFrame(0x00400000,160);
	setFrame(0x00440000,172);
	
	setFrame(0x00480000,184);
	setFrame(0x004C0000,196);
	setFrame(0x00500000,208);
	setFrame(0x00540000,220);
	setFrame(0x00580000,232);
	
	setFrame(0x005C0000,244);
	setFrame(0x00600000,256);
	setFrame(0x00640000,268);
	setFrame(0x00680000,280);	
	setFrame(0x006C0000,292);
	
	
	
	
	enable_A9_interrupts (); // enable interrupts in the A9 processor
 return 0;
}


