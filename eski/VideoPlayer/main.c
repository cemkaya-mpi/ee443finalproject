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
#include "alt_interrupt.h"
#include "socal/hps.h"
#include "fs/fat_filelib.h"

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
	/*
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
	*/
	// writing total printed frame number into the character buffer
	uint32_t d5 = (val / 100000) % 10;
	d5 = d5 + '0';
	uint32_t d4 = (val / 10000) % 10;
	d4 = d4 + '0';
	uint32_t d3 = (val / 1000) % 10;
	d3 = d3 + '0';
	uint32_t d2 = (val / 100) % 10;
	d2 = d2 + '0';
	uint32_t d1 = (val / 10) % 10;
	d1 = d1 + '0';
	uint32_t d0 = (val) % 10;
	d0 = d0 + '0';
	
	int offset,x =5 ,y =55;
	
	char *text_ptr[6]; //notice that the data size is not dynamic, it is constant. It can be adjusted as in example code below.
	text_ptr[0]= d0;
	text_ptr[1]= d1;
	text_ptr[2]= d2;
	text_ptr[3]= d3;
	text_ptr[4]= d4;
	text_ptr[5]= d5;
	
	int counter=6;
	/* Display a null-terminated text string at coordinates x, y. Assume that the text fits on one line */
	offset = (y <<7) + x;
	while ( counter>=0 ){
		*(unsigned char *)(0xC9000000 + offset) = text_ptr[counter]; // write to the character buffer
		counter--;
		offset++;
	}
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

static void TimerISR(uint32_t icciar, void * context)
{
	// Clear int source don't care about the return value
	alt_globaltmr_int_clear_pending();
	alt_globaltmr_comp_set64(alt_globaltmr_get64() + 230000*4);


}

void timer_init(void)
{
	alt_globaltmr_init();
	alt_int_global_init();
	alt_int_cpu_init();
	alt_int_dist_trigger_set(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL, ALT_INT_TRIGGER_AUTODETECT);
	alt_int_dist_enable(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL);
	alt_int_cpu_enable();
	alt_int_global_enable();

	alt_globaltmr_start();
	alt_int_isr_register(ALT_INT_INTERRUPT_PPI_TIMER_GLOBAL, TimerISR, NULL);
	alt_globaltmr_comp_set64(alt_globaltmr_get64() + 230000*4);
	alt_globaltmr_comp_mode_start();

	
}

typedef struct 
{
	int16_t		Audio[1920];
	uint16_t	Video[320*240];
} TFrame;

TFrame g_Frame[4096];

#define AUDIO_CONTROL		0
#define AUDIO_FIFO_STATUS	1
#define AUDIO_LDATA			2
#define AUDIO_RDATA			3



volatile uint32_t* pAUDIO = (volatile uint32_t*)0xFF203040;
volatile uint32_t* pLED = (volatile uint32_t*)0xFF200000;
volatile uint32_t* pBUTTON = (volatile uint32_t*)0xFF200050;
volatile uint16_t* pVGA = (volatile uint16_t*)0xC8000000;
volatile uint16_t* pVGABACK =(volatile uint16_t*)0xC8040000;
volatile uint32_t* frontBuffer =(volatile uint32_t*)0xFF203020;
volatile uint32_t* backBuffer = (volatile uint32_t*)0xFF203024;





typedef enum 
{
	ST_STOP,
	ST_BEFORE_PLAYING,
	ST_PLAYING,
	ST_AFTER_PLAYING
} TState;

TState g_State = ST_STOP;
uint32_t g_AUDIO_INDEX = 0;
uint32_t g_FRAME_INDEX = 0;

bool IsKeyPressed(void)
{
	bool result = false;

	if(*pBUTTON == 1)
	{
		result = true;
	}

	return(result);
}

void TurnOnLed(void)
{
	*pLED = 1;
}

void TurnOffLed(void)
{
	*pLED = 0;
}
void main(void)
{
	FL_FILE* file;

	mmu_init();
	alt_cache_system_enable();
	
	timer_init();

	media_init();
	fl_init();

	// Attach media access functions to library
	if (fl_attach_media(media_read, media_write) != FAT_INIT_OK)
	{
		print_debug("ERROR: Media attach failed\n");
		return; 
	}

	//TestClocks();

	file = fl_fopen("/test_raw.avi", "rb");

	fl_fseek(file, 0, SEEK_END); // seek to end of file
	uint32_t size = fl_ftell(file); // get current file pointer
	fl_fseek(file, 0, SEEK_SET);

	fl_fread(&g_Frame[0], 1, size, file);
	
	g_State = ST_BEFORE_PLAYING;

	//setting up double buffer vga
	uint8_t inActiveBuffer =1;
	*backBuffer =0xC8040000;
	while (true)
	{
		switch(g_State)
		{
			case ST_STOP:
			{
				if(IsKeyPressed())
				{
					g_State = ST_BEFORE_PLAYING;
				}

				break;
			}

			case ST_BEFORE_PLAYING:
			{
				TurnOnLed();

				g_AUDIO_INDEX = 0;
				g_FRAME_INDEX = 0;

				// flush FIFO content, just to be sure
				pAUDIO[AUDIO_CONTROL] = 0x0C;
				pAUDIO[AUDIO_CONTROL] = 0x00;

				g_State = ST_PLAYING;

				break;
			}

			case ST_PLAYING:
			{				
				ShowVal(g_FRAME_INDEX);

				while((g_AUDIO_INDEX < 1920) && (pAUDIO[AUDIO_FIFO_STATUS] & 0x00FF0000) > 0x00600000)
				{
					if(g_AUDIO_INDEX == 0)
					{
					//double buffer vga
					if(inActiveBuffer==0){
						for (uint32 y = 0; y < 240; y++)			
						{	
							for (uint32 x = 0; x < 320; x++)
							{
								pVGA[y*512 + x] = g_Frame[g_FRAME_INDEX].Video[y*320 + x];
							}
						}
					}else{
						for (uint32 y = 0; y < 240; y++)			
						{	
							for (uint32 x = 0; x < 320; x++)
							{
								pVGABACK[y*512 + x] = g_Frame[g_FRAME_INDEX].Video[y*320 + x];
							}
						}
					
					}
					
					inActiveBuffer= !(inActiveBuffer) & 0x1; //one bit toogle;
					*frontBuffer=1;
					/* single buffer vga
						for (uint32 y = 0; y < 240; y++)			
						for (uint32 x = 0; x < 320; x++)
						{
							pVGA[y*512 + x] = g_Frame[g_FRAME_INDEX].Video[y*320 + x];
						}
						
						*/
					}

					pAUDIO[AUDIO_LDATA] = (int32_t)g_Frame[g_FRAME_INDEX].Audio[g_AUDIO_INDEX]*256;
					pAUDIO[AUDIO_RDATA] = (int32_t)g_Frame[g_FRAME_INDEX].Audio[g_AUDIO_INDEX]*256;

					if(++g_AUDIO_INDEX >= 1920)
					{
						g_AUDIO_INDEX = 0;

						if(++g_FRAME_INDEX >= 3290)
						{
							g_FRAME_INDEX = 0;
							if(inActiveBuffer==0)    //setting double buffer to initial condition in which the buffer controller points pVGA address
							{					//for the usage of double buffer.
							*frontBuffer=1;
							inActiveBuffer=1;
							
							}
						}
					}
				}

				if((g_FRAME_INDEX == 3290) && (pAUDIO[AUDIO_FIFO_STATUS] & 0x00FF0000) >= 0x00800000)
				{
					g_State = ST_AFTER_PLAYING;
				}

				break;
			}

			case ST_AFTER_PLAYING:
			{
				TurnOffLed();
				g_State = ST_STOP;

				break;
			}

			default:
				break;
		}
	}

	fl_fclose(file);
	
	fl_shutdown();
}
