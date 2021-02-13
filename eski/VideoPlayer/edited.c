#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "alt_mmu.h"
#include "alt_cache.h"
#include "alt_sdmmc.h"
#include "alt_globaltmr.h"
#include "alt_clock_manager.h"
#include "socal/hps.h"

#include "fs/fat_filelib.h"

#define frontBuffer ((volatile unsigned int *) 0xFF203020)
#define backBuffer ((volatile unsigned int *) 0xFF203024)
#define resolutionRegister ((volatile unsigned int *) 0xFF203028)
#define statusRegister ((volatile unsigned int *) 0xFF20302C)

int * next;
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
		return; //pause
	}
	*frontBuffer=1;
	if(*baseAddr!=0x006C0000){
		*baseAddr += 0x00040000;
	}else{
		*baseAddr=0x000C0000;
	}
	

	return;
}
const uint32_t SEG_BCD[] =
{
	0x3F, // 0011 / 1111
	0x06, // 0000 / 0110
	0x5B, // 0101 / 1011
	0x4F, // 0100 / 1111
	0x66, // 0110 / 0110
	0x6D, // 0110 / 1101
	0x7D, // 0111 / 1101
	0x07, // 0000 / 0111
	0x7F, // 0111 / 1111
	0x6F  // 0110 / 1111
};

void ShowVal(const uint32_t val)
{
	volatile uint32_t* pDISP0 = (volatile uint32_t*)0xFF200020;
	volatile uint32_t* pDISP1 = (volatile uint32_t*)0xFF200030;

	uint32_t d5 = SEG_BCD[(val / 100000) % 10];
	uint32_t d4 = SEG_BCD[(val / 10000) % 10];
	uint32_t d3 = SEG_BCD[(val / 1000) % 10];
	uint32_t d2 = SEG_BCD[(val / 100) % 10];
	uint32_t d1 = SEG_BCD[(val / 10) % 10];
	uint32_t d0 = SEG_BCD[(val / 1) % 10];

	*pDISP0 = (d3 << 24) | (d2 << 16) | (d1 << 8) | d0;
	*pDISP1 = (d5 << 8) | d4;
}

int print_debug(const char *fmt, ...)
{
	volatile uint32_t* JTAG_UART_ptr = (volatile uint32_t*)0xFF201000;	// JTAG UART address
	
    int ret;
    char buf[512];
	char* p = &buf[0];
    va_list ap;

    va_start(ap, fmt);
    ret = vsprintf(buf, fmt, ap);
    va_end(ap);
	
    while((ret > 0) && *p)
	{
		if((*(JTAG_UART_ptr + 1)) & 0xFFFF0000)
		{
			*(JTAG_UART_ptr) = *p++;
		}
    }
	
    return ret;
}

ALT_SDMMC_CARD_INFO_t Card_Info;

void init_mmc(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    ALT_SDMMC_CARD_MISC_t card_misc_cfg;

    print_debug("MMC Initialization.\n");

	alt_globaltmr_int_is_enabled();
	alt_sdmmc_init();
	alt_sdmmc_card_pwr_on();

	alt_sdmmc_card_identify(&Card_Info); // Card_Info.card_type == ALT_SDMMC_CARD_TYPE_SDHC

	alt_sdmmc_card_bus_width_set(&Card_Info, ALT_SDMMC_BUS_WIDTH_4);

	alt_sdmmc_fifo_param_set((ALT_SDMMC_FIFO_NUM_ENTRIES >> 3) - 1, (ALT_SDMMC_FIFO_NUM_ENTRIES >> 3), ALT_SDMMC_MULT_TRANS_TXMSIZE1);
	alt_sdmmc_card_misc_get(&card_misc_cfg);    
    alt_sdmmc_dma_enable();

	alt_sdmmc_card_speed_set(&Card_Info, (Card_Info.high_speed ? 2 : 1) * Card_Info.xfer_speed);

	#define printMMC(x)	print_debug(#x " = %d\n", x)
	printMMC(Card_Info.card_type);
	printMMC(Card_Info.rca);
	printMMC(Card_Info.xfer_speed);
	printMMC(Card_Info.max_r_blkln);
	printMMC(Card_Info.max_w_blkln);
	printMMC(Card_Info.partial_r_allowed);
	printMMC(Card_Info.partial_w_allowed);
	printMMC(Card_Info.high_speed);
	printMMC(Card_Info.scr_sd_spec);
	printMMC(Card_Info.csd_ccc);
	printMMC(Card_Info.blk_number_high);
	printMMC(Card_Info.blk_number_low);
}


int media_init()
{
	init_mmc();
	return 1;
}

int media_read(uint32_t sector, uint8_t *buffer, uint32_t sector_count)
{
	for (uint32_t i=0; i < sector_count; i++)
	{
		alt_sdmmc_read(&Card_Info, buffer, (void*)(sector*FAT_SECTOR_SIZE), FAT_SECTOR_SIZE);

		sector++;
		buffer += FAT_SECTOR_SIZE;
	}

	return 1;
}

int media_write(uint32_t sector, uint8_t *buffer, uint32_t sector_count)
{
	for (uint32_t i=0; i < sector_count; i++)
	{
		alt_sdmmc_write(&Card_Info, (void*)(sector*FAT_SECTOR_SIZE), buffer, FAT_SECTOR_SIZE);

		sector++;
		buffer += FAT_SECTOR_SIZE;
	}

	return 1;
}

uint8_t data[300*1024*1024];

void usleep(uint64_t usecs_p)
{
	uint64_t	startTime = alt_globaltmr_get64();
	uint32_t	timerPrescaler = alt_globaltmr_prescaler_get() + 1;
	uint64_t	endTime;
	alt_freq_t	timerClkSrc;

	alt_clk_freq_get(ALT_CLK_MPU_PERIPH, &timerClkSrc);
	endTime = startTime + usecs_p * ((timerClkSrc / timerPrescaler) / 1000000);

	while (alt_globaltmr_get64() < endTime);
}


void TestClocks(void)
{
	#define printCLK(x)	alt_clk_freq_get(x, &freq); print_debug(#x " = %d\n", freq);

	alt_freq_t freq = 0;

	printCLK(ALT_CLK_IN_PIN_OSC1);
	printCLK(ALT_CLK_IN_PIN_OSC2);
	printCLK(ALT_CLK_F2H_PERIPH_REF);
	printCLK(ALT_CLK_F2H_SDRAM_REF);
	printCLK(ALT_CLK_IN_PIN_JTAG);
	printCLK(ALT_CLK_IN_PIN_ULPI0);
	printCLK(ALT_CLK_IN_PIN_ULPI1);
	printCLK(ALT_CLK_IN_PIN_EMAC0_RX);
	printCLK(ALT_CLK_IN_PIN_EMAC1_RX);
	printCLK(ALT_CLK_MAIN_PLL);
	printCLK(ALT_CLK_PERIPHERAL_PLL);
	printCLK(ALT_CLK_SDRAM_PLL);
	printCLK(ALT_CLK_OSC1);
	printCLK(ALT_CLK_MAIN_PLL_C0);
	printCLK(ALT_CLK_MAIN_PLL_C1);
	printCLK(ALT_CLK_MAIN_PLL_C2);
	printCLK(ALT_CLK_MAIN_PLL_C3);
	printCLK(ALT_CLK_MAIN_PLL_C4);
	printCLK(ALT_CLK_MAIN_PLL_C5);
	printCLK(ALT_CLK_MPU);
	printCLK(ALT_CLK_MPU_L2_RAM);
	printCLK(ALT_CLK_MPU_PERIPH);
	printCLK(ALT_CLK_L3_MAIN);
	printCLK(ALT_CLK_L3_MP);
	printCLK(ALT_CLK_L3_SP);
	printCLK(ALT_CLK_L4_MAIN);
	printCLK(ALT_CLK_L4_MP);
	printCLK(ALT_CLK_L4_SP);
	printCLK(ALT_CLK_DBG_BASE);
	printCLK(ALT_CLK_DBG_AT);
	printCLK(ALT_CLK_DBG_TRACE);
	printCLK(ALT_CLK_DBG_TIMER);
	printCLK(ALT_CLK_DBG);
	printCLK(ALT_CLK_MAIN_QSPI);
	printCLK(ALT_CLK_MAIN_NAND_SDMMC);
	printCLK(ALT_CLK_CFG);
	printCLK(ALT_CLK_H2F_USER0);
	printCLK(ALT_CLK_PERIPHERAL_PLL_C0);
	printCLK(ALT_CLK_PERIPHERAL_PLL_C1);
	printCLK(ALT_CLK_PERIPHERAL_PLL_C2);
	printCLK(ALT_CLK_PERIPHERAL_PLL_C3);
	printCLK(ALT_CLK_PERIPHERAL_PLL_C4);
	printCLK(ALT_CLK_PERIPHERAL_PLL_C5);
	printCLK(ALT_CLK_USB_MP);
	printCLK(ALT_CLK_SPI_M);
	printCLK(ALT_CLK_QSPI);
	printCLK(ALT_CLK_NAND_X);
	printCLK(ALT_CLK_NAND);
	printCLK(ALT_CLK_SDMMC);
	printCLK(ALT_CLK_EMAC0);
	printCLK(ALT_CLK_EMAC1);
	printCLK(ALT_CLK_CAN0);
	printCLK(ALT_CLK_CAN1);
	printCLK(ALT_CLK_GPIO_DB);
	printCLK(ALT_CLK_H2F_USER1);
	printCLK(ALT_CLK_SDRAM_PLL_C0);
	printCLK(ALT_CLK_SDRAM_PLL_C1);
	printCLK(ALT_CLK_SDRAM_PLL_C2);
	printCLK(ALT_CLK_SDRAM_PLL_C3);
	printCLK(ALT_CLK_SDRAM_PLL_C4);
	printCLK(ALT_CLK_SDRAM_PLL_C5);
	printCLK(ALT_CLK_DDR_DQS);
	printCLK(ALT_CLK_DDR_2X_DQS);
	printCLK(ALT_CLK_DDR_DQ);
	printCLK(ALT_CLK_H2F_USER2);
	printCLK(ALT_CLK_OUT_PIN_EMAC0_TX);
	printCLK(ALT_CLK_OUT_PIN_EMAC1_TX);
	printCLK(ALT_CLK_OUT_PIN_SDMMC);
	printCLK(ALT_CLK_OUT_PIN_I2C0_SCL);
	printCLK(ALT_CLK_OUT_PIN_I2C1_SCL);
	printCLK(ALT_CLK_OUT_PIN_I2C2_SCL);
	printCLK(ALT_CLK_OUT_PIN_I2C3_SCL);
	printCLK(ALT_CLK_OUT_PIN_SPIM0);
	printCLK(ALT_CLK_OUT_PIN_SPIM1);
	printCLK(ALT_CLK_OUT_PIN_QSPI);
}

/* MMU Page table - 16KB aligned at 16KB boundary */
static uint32_t __attribute__ ((aligned (0x4000))) alt_pt_storage[4096];

static void *alt_pt_alloc(const size_t size, void *context)
{
	return context;
}

void mmu_init(void)
{
	uint32_t *ttb1 = NULL;

	/* Populate the page table with sections (1 MiB regions). */
	ALT_MMU_MEM_REGION_t regions[] = 
	{
		///* Memory area: 1 GiB */
		//{
		//	.va = (void *)0x00000000,
		//	.pa = (void *)0x00000000,
		//	.size = 0x40000000,
		//	.access = ALT_MMU_AP_FULL_ACCESS,
		//	.attributes = ALT_MMU_ATTR_WBA,
		//	.shareable = ALT_MMU_TTB_S_NON_SHAREABLE,
		//	.execute = ALT_MMU_TTB_XN_DISABLE,
		//	.security = ALT_MMU_TTB_NS_SECURE
		//},

		///* Device area: Everything else */
		//{
		//	.va = (void *)0x40000000,
		//	.pa = (void *)0x40000000,
		//	.size = 0xc0000000,
		//	.access = ALT_MMU_AP_FULL_ACCESS,
		//	.attributes = ALT_MMU_ATTR_DEVICE_NS,
		//	.shareable = ALT_MMU_TTB_S_NON_SHAREABLE,
		//	.execute = ALT_MMU_TTB_XN_ENABLE,
		//	.security = ALT_MMU_TTB_NS_SECURE
		//}

		/* Memory area: 1 GiB */
		{
			.va = (void *)0x00000000,
			.pa = (void *)0x00000000,
			.size = 0x40000000,
			.access = ALT_MMU_AP_FULL_ACCESS,
			.attributes = ALT_MMU_ATTR_DEVICE_NS, // wba idi
			.shareable = ALT_MMU_TTB_S_NON_SHAREABLE,
			.execute = ALT_MMU_TTB_XN_DISABLE,
			.security = ALT_MMU_TTB_NS_SECURE
		},

		/* Device area: Everything else */
		{
			.va = (void *)0x40000000,
			.pa = (void *)0x40000000,
			.size = 0xc0000000,
			.access = ALT_MMU_AP_FULL_ACCESS,
			.attributes = ALT_MMU_ATTR_DEVICE_NS,
			.shareable = ALT_MMU_TTB_S_NON_SHAREABLE,
			.execute = ALT_MMU_TTB_XN_ENABLE,
			.security = ALT_MMU_TTB_NS_SECURE
		}
	};

	alt_mmu_init();
	alt_mmu_va_space_storage_required(regions, sizeof(regions)/sizeof(regions[0]));
	alt_mmu_va_space_create(&ttb1, regions, sizeof(regions)/sizeof(regions[0]), alt_pt_alloc, alt_pt_storage);
	alt_mmu_va_space_enable(ttb1);
}

void main(void)
{
	FL_FILE* file;

	mmu_init();
	alt_cache_system_enable();

	media_init();
	fl_init();

	// Attach media access functions to library
	if (fl_attach_media(media_read, media_write) != FAT_INIT_OK)
	{
		print_debug("ERROR: Media attach failed\n");
		return; 
	}

	//TestClocks();

	while(true)
	{
		uint64_t start = alt_globaltmr_get64();

		file = fl_fopen("/boun.avi", "r");

		fl_fseek(file, 0, SEEK_END); // seek to end of file
		uint32_t size = fl_ftell(file); // get current file pointer
		fl_fseek(file, 0, SEEK_SET);

		if (file)
		{
			print_debug("Reading...\n");

			if (fl_fread(&data[0], 1, size, file) != size)
			{
				print_debug("ERROR: Read file failed\n");
			}				
		}

		fl_fclose(file);

		uint64_t stop = alt_globaltmr_get64();

		alt_freq_t freq = 0;
		alt_clk_freq_get(ALT_CLK_MPU_PERIPH, &freq);
		
		print_debug("data[0] = %x\n", data[0]);
		print_debug("data[1] = %x\n", data[1]);
		print_debug("data[2] = %x\n", data[2]);
		print_debug("data[3] = %x\n", data[3]);
		print_debug("data[4] = %x\n", data[4]);
		print_debug("data[%d] = %x\n", size-1, data[size-1]);
		print_debug("file size %d has read in %d secs.\n", size, (stop - start) / freq);
	}
	

	
	fl_shutdown();
}
