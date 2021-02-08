#define CTRL ((volatile unsigned int *) 0XFF704000)
#define PWREN ((volatile unsigned int *) 0XFF704004)
#define CLKDIV ((volatile unsigned int *) 0XFF704008)
#define CLKSRC ((volatile unsigned int *) 0XFF70400C)
#define CLKENA ((volatile unsigned int *) 0XFF704010)
#define TMOUT ((volatile unsigned int *) 0XFF704014)
#define CTYPE ((volatile unsigned int *) 0XFF704018)
#define BLKSIZ ((volatile unsigned int *) 0XFF70401C)
#define BYTCNT ((volatile unsigned int *) 0XFF704020)
#define INTMASK ((volatile unsigned int *) 0XFF704024)
#define CMDARG ((volatile unsigned int *) 0XFF704028)
#define CMD ((volatile unsigned int *) 0XFF70402C)
#define RESP0 ((volatile unsigned int *) 0XFF704030)
#define RESP1 ((volatile unsigned int *) 0XFF704034)
#define RESP2 ((volatile unsigned int *) 0XFF704038)
#define RESP3 ((volatile unsigned int *) 0XFF70403C)
#define MINTSTS ((volatile unsigned int *) 0XFF704040)
#define RINTSTS ((volatile unsigned int *) 0XFF704044)
#define STATUS ((volatile unsigned int *) 0XFF704048)
#define FIFOTH ((volatile unsigned int *) 0XFF70404C)
#define CDETECT ((volatile unsigned int *) 0XFF704050)
#define WRTPRT ((volatile unsigned int *) 0XFF704054)
#define TCBCNT ((volatile unsigned int *) 0XFF70405C)
#define TBBCNT ((volatile unsigned int *) 0XFF704060)
#define DEBNCE ((volatile unsigned int *) 0XFF704064)
#define USRID ((volatile unsigned int *) 0XFF704068)
#define VERID ((volatile unsigned int *) 0XFF70406C)
#define HCON ((volatile unsigned int *) 0XFF704070)
#define UHS_REG ((volatile unsigned int *) 0XFF704074)
#define RST_N ((volatile unsigned int *) 0XFF704078)
#define BMOD ((volatile unsigned int *) 0XFF704080)
#define PLDMND ((volatile unsigned int *) 0XFF704084)
#define DBADDR ((volatile unsigned int *) 0XFF704088)
#define IDSTS ((volatile unsigned int *) 0XFF70408C)
#define IDINTEN ((volatile unsigned int *) 0XFF704090)
#define DSCADDR ((volatile unsigned int *) 0XFF704094)
#define BUFADDR ((volatile unsigned int *) 0XFF704098)
#define CARDTHRCTL ((volatile unsigned int *) 0XFF704100)
#define BACK_END_POWER_R ((volatile unsigned int *) 0XFF704104)
#define DATA ((volatile unsigned int *) 0XFF704200)
//Clock manager peripheral
#define PERIPHERALS_EN ((volatile unsigned int *) 0xFFD040A0)
//sdmmc controller control register
#define SYSTEM_SDMMC_CTRL ((volatile unsigned int *) 0xFFD08108)
// Original CMD Value
#define CMD_O ( volatile unsigned int) 0x20000000
