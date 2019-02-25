/* BIT BANGING VERSION 
 * RF2071 Mixer configuration utility
 * based on SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 * Copyright (c) 2018  Daniel de kock <daniel@riot.network>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <math.h>
#include <sys/mman.h> /* mmap() is defined in this header */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*GPIO bit banging */

#define PAGE_SIZE   4096        
#define BLOCK_SIZE PAGE_SIZE

#define AR71XX_APB_BASE         0x18000000
#define AR71XX_GPIO_BASE        (AR71XX_APB_BASE + 0x00040000)

#define AR71XX_GPIO_REG_OE		0
#define AR71XX_GPIO_REG_IN 		1  //for get value, ????? out status?
#define AR71XX_GPIO_REG_OUT		2
#define AR71XX_GPIO_REG_SET		3
#define AR71XX_GPIO_REG_CLEAR		4

#define AR71XX_GPIO_REG_INT_MODE	0x14
#define AR71XX_GPIO_REG_INT_TYPE	0x18
#define AR71XX_GPIO_REG_INT_POLARITY	0x1c
#define AR71XX_GPIO_REG_INT_PENDING	0x20
#define AR71XX_GPIO_REG_INT_ENABLE	0x24
#define AR71XX_GPIO_REG_FUNC		0x28

int  mem_fd     = 0;
char *gpio_mmap = NULL;
char *gpio_ram  = NULL;
volatile unsigned long *gpio = NULL;

/**************MIXER register map********************/
#define	LF  0
#define	XO  1
#define	CAL_TIME  2
#define	VCO_CTRL  3
#define	CT_CAL1 4
#define	CT_CAL2  5
#define	PLL_CAL1 6
#define	PLL_CAL2 7
#define	VCO_AUTO  8
#define	PLL_CTRL 9
#define	PLL_BIAS  10
#define	MIX_CONT  11
#define	P1_FREQ1 12
#define	P1_FREQ2 13
#define	P1_FREQ3 14
#define	P2_FREQ1  15
#define	P2_FREQ2 16
#define	P2_FREQ3  17
#define	FN_CTRL  18
#define	EXT_MOD  19
#define	FMOD  20
#define	SDI_CTRL  21
#define	GPO  22
#define	T_VCO  23
#define	IQMOD1  24
#define	IQMOD2  25
#define	IQMOD3  26
#define	IQMOD4  27
#define	T_CTRL  28
#define	DEV_CTRL  29
#define	TEST  30
#define	READBACK  31
#define	RWBIT  24

/**************MIXER constants and defaults ********************/
#define REF_FREQ_MHZ 26 // main xtal is 26 MHz
#define F_VCO_MAX_MHZ 5400 // 5.4 GHz max VCO

#define SDI_CTRL_reset 0x0002
#define SDI_CTRL_set 0x8000
#define VCO_AUTO_DEFAULT 0xFF00
#define TEST_DEFAULT 0x0005
#define MIX_CONT_DEFAULT 0xDA00 // full duplex, linearity 5, 
#define GPO_DEFAULT 0x0001 // sdo is used as a lock detect

#define CT_CALx_DEFAULT 0xACBF
//LO set to 1918 MHz
#define LO_DEFAULT_MHZ 1918
#define LINEARITY_DEFAULT 5
#define Px_FREQ1_DEFAULT 0x1218
#define Px_FREQ2_DEFAULT 0xE276
#define Px_FREQ3_DEFAULT 0x2700
#define PLL_CALx_DEFAULT 0x0028

#define SDI_CTRL_ENABLE_DONE 0xE000 //Enable device : set SDI_CTRL.sipin = 1, SDI_CTRL.enable = 1,SDI_CTRL.mode = 1

/****************** GPIO ************************/
#define RIOT_GPIO_MIXER_SPI_SS		26
#define RIOT_GPIO_MIXER_SPI_SCK		11 
#define RIOT_GPIO_MIXER_SPI_MOSI	27
#define RIOT_GPIO_MIXER_SPI_MISO	8

#define RIOT_GPIO_RF_ENABLE		7
#define RIOT_GPIO_MIXER_RESET		18
#define RIOT_GPIO_MIXER_ENABLE		19
#define RIOT_GPIO_MIXER_MODE		20
#define RIOT_GPIO_MIXER_LDO		21

#define bit(b) (1UL << (b))

/**************MIXER variables********************/
// from the RFFC2071 mixer programming guide
static double fvco,ndiv;
static uint16_t nlo,fn,nummsb,numlsb,lodiv,fbkdiv;

static uint16_t px_freq1 = Px_FREQ1_DEFAULT;
static uint16_t px_freq2 = Px_FREQ2_DEFAULT;
static uint16_t px_freq3 = Px_FREQ3_DEFAULT;
static uint16_t ct_calx  = CT_CALx_DEFAULT;
static uint16_t mix_cont  = MIX_CONT_DEFAULT;

static uint32_t speed = 50000;
static uint16_t delay = 1000;
static int verbose;
static uint8_t linearity = 0; /*mixer Linearity 1-7*/
static uint16_t lo_freq_MHz = 0; /*mixer LO frequency in MHz*/
static uint8_t defaultConfig = 0;

char *input_tx;

//  Mixer setup blurt :  LO is at 1918 MHz

static uint16_t mixerDefaults[][2] = {
	// integrated mixer programming guide
	// step 0 : Reset device
	{ SDI_CTRL, 0x0002 },
	{ SDI_CTRL, 0x8000 },
	// step 1 : Set device operation
	{ P2_FREQ1, 0x1298 },
	{ VCO_AUTO, 0xFF00 },
	{ CT_CAL1, 0xACBF },
	{ CT_CAL2, 0xACBF },
	{ TEST, 0x0005 },
	// 1 : set 3 wire control : SDI_CTRL.sipin = 1
	// 1 : set Full duplex mode : MIX_CONT.fulld = 1
	// 1 : set Linearity mode : MIX_CONT.p1mixidd = 5 , MIX_CONT.p2mixidd = 5
	{ MIX_CONT, 0xDA00 },
	// step 2 : Set additional features
	{ GPO, 0x0001 },
	//{DEV_CTRL,0x0027}, 
	// none used
	// step 3 : Set operating frequencies 
	{ P1_FREQ1, 0x1218 },
	{ P2_FREQ1, 0x1218 },
	{ CT_CAL1, 0xACBF },
	{ CT_CAL2, 0xACBF },
	{ P1_FREQ2, 0xE276 },
	{ P2_FREQ2, 0xE276 },
	{ P1_FREQ3, 0x2700 },
	{ P2_FREQ3, 0x2700 },
	// step 4 : Set Calibration mode
	{ PLL_CAL1, 0x0028 },
	{ PLL_CAL2, 0x0028 },
	// step 5 : Enable device : set SDI_CTRL.sipin = 1, SDI_CTRL.enable = 1,SDI_CTRL.mode = 1
	{ SDI_CTRL, 0xE000 }
};



// log2 of an integer from : https://stackoverflow.com/questions/758001/log2-not-found-in-my-math-h
// If you're looking for an integral result, you can just determine the highest bit set in the value and return its position.

unsigned int log2_int( unsigned int x )
{
  unsigned int ans = 0 ;
  while( x>>=1 ) ans++;
  return ans ;
}

// Read GPIO state
int gpioRead(int pin)
{
    unsigned long value = *(gpio + AR71XX_GPIO_REG_IN);
    return (value & (1 << pin));
}

// set GPIO state
void gpioSet(unsigned int pin, unsigned int state)
{
    if (state == 0)
    {
        *(gpio + AR71XX_GPIO_REG_CLEAR) = (1 << pin);
    }
    else
    {
        *(gpio + AR71XX_GPIO_REG_SET) = (1 << pin);
    }
}

//set GPIO direction : /* Call gpioDirection(27, 1) to set GPIO27 as output. */
void gpioDirection(unsigned int pin, unsigned int state)
{
   // unsigned long value = *(gpio + AR71XX_GPIO_REG_OE); // obtain current settings

    if (state)
    {
        *(gpio+AR71XX_GPIO_REG_OE) |= (1 << pin); // set bit to 1 ( set as output )
    }
    else
    {
        *(gpio+AR71XX_GPIO_REG_OE) &= ~(1 << pin); // clear bit ( set at input )
    }

   // *(gpio + AR71XX_GPIO_REG_OE) = value;
}





/*Dend data to device*/
static void transfer( uint32_t data )
{

/* start transmission */

/*
	gpioSet(RIOT_GPIO_MIXER_SPI_SS, 1);	// Mixer SS / ENx
	gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 0);	// Mixer SCK
	gpioSet(RIOT_GPIO_MIXER_SPI_MOSI, 0);	// Mixer MOSI
*/

// send data , stable on rising edge, start at bit 25
	gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 1);	// Mixer SCK HIGH
	usleep(1);
	// CS low, Clock LOW
	gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 0);	// Mixer SCK LOW
	gpioSet(RIOT_GPIO_MIXER_SPI_SS, 0);	// Mixer SS LOW
	usleep(1); 
	for (int16_t i = 24; i >= 0; i--) { // only send lower 25 bits )
		gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 0);	// Mixer SCK LOW
		gpioSet(RIOT_GPIO_MIXER_SPI_MOSI, ((data & bit(i)) >= 1)); // set bit
		usleep(1); 
		gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 1);	// Mixer SCK HIGH
		usleep(1);
	}
	gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 0);	// Mixer SCK LOW
	usleep(1);
	gpioSet(RIOT_GPIO_MIXER_SPI_SS, 1);	// Mixer SS LOW
	gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 1);	// Mixer SCK HIGH
	usleep(1);
	gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 0);	// Mixer SCK LOW

/* end transmission */
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-sbdo3vpLl]\n", prog);
	puts("  -s --speed    max speed (Hz)\n"
	     "  -d --delay    delay (usec)\n"
	     "  -v --verbose  Verbose (show tx buffer)\n"
	     "  -f --freq     set LO frequency in MHz (e.g. -f 1918) \n"
	     "  -l --linear   set mixer linearity (1-7) \n"
);
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = { /* char *name ,int has_arg ,int *flag , int val */
			
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "verbose", 0, 0, 'v' },

			{ "freq"   , 1, 0, 'f' },
			{ "linear" , 1, 0, 'l' },
			{ NULL, 0, 0, 0 },
		};
/*
 Names for the values of the `has_arg' field of `struct option'. 
#define	no_argument		0
#define required_argument	1
#define optional_argument	2

*/
		int c;

		c = getopt_long(argc, argv, "s:d:vf:l:", // v doen't require any arguments , thus no : after it
				lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case 'f':
			lo_freq_MHz = atoi(optarg);
			break;
		case 'l':
			linearity = atoi(optarg); 
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}



//take addr and setting and create a 32bit value from them
uint32_t assembleData(uint16_t addr, uint16_t val) {
	uint32_t data;
	//uint32_t dataMask = 0x1FFFFFF;
		data = 0;
		// set Address and RW bit ( RW bit is MSB of addr)
		data |= ((uint32_t)addr & 0x7F) << 16;
		// set Data
		data |= ((uint32_t)val & 0xFFFF);
		//apply 25 bit mask( make sure upper bits are cleared)
		data &= 0x1FFFFFF;
	return data;
}

//take addr and setting and transfer them
static void setRegister(uint16_t addr, uint16_t val) {
	uint32_t data;
	data = assembleData(addr,val);
	transfer(data);
	usleep(delay); // wait delay between transfers
}


static void setMixerDefaults(){

	uint32_t register_tx;
// 'total' will be 70 = 10 * 7
	uint16_t total = sizeof(mixerDefaults);
	// 'column' will be 7 = size of first row
	uint16_t column = sizeof(mixerDefaults[0]);
	// 'row' will be 10 = 70 / 7
	uint16_t row = total / column;
	for (int i = 0; i<row; i++) {  // cycle thru all the rows
		// build transfer packet from the configuration array 
		register_tx = assembleData(mixerDefaults[i][0], mixerDefaults[i][1]);
		// send to the device ( tell it to interpret the 32bit values as 8bit arrays ) 
		// works because machine endianness matches the MSB first of the data interpretation
		transfer( register_tx );
 		usleep(delay); // wait delay between transfers
	}
}

static void setMixerCustom(){ 

// do this a few times.. once for every register we want to set.. ugh..
/*
static uint16_t px_freq1 = Px_FREQ1_DEFAULT;
static uint16_t px_freq2 = Px_FREQ2_DEFAULT;
static uint16_t px_freq3 = Px_FREQ3_DEFAULT;
static uint16_t ct_calx  = CT_CALx_DEFAULT;
static uint16_t mix_cont  = MIX_CONT_DEFAULT;
*/

	// build transfer packet from calculated data

	// integrated mixer programming guide
	// step 0 : Reset device
	setRegister(SDI_CTRL, 0x0002);
	setRegister(SDI_CTRL, 0x8000);
	// step 1 : Set device operation
	setRegister( P2_FREQ1, 0x1298);
	setRegister(VCO_AUTO, 0xFF00 );
	setRegister(CT_CAL1, ct_calx );
	setRegister(CT_CAL2, ct_calx );
	setRegister( TEST, 0x0005 );
	// 1 : set 3 wire control : SDI_CTRL.sipin = 1
	// 1 : set Full duplex mode : MIX_CONT.fulld = 1
	// 1 : set Linearity mode : MIX_CONT.p1mixidd = 5 , MIX_CONT.p2mixidd = 5
	setRegister(MIX_CONT, mix_cont );
	// step 2 : Set additional features
	setRegister(GPO, 0x0001 );
	//{DEV_CTRL,0x0027}, 
	// none used
	// step 3 : Set operating frequencies 
	setRegister(P1_FREQ1, px_freq1 );
	setRegister(P2_FREQ1, px_freq1 );
	setRegister(CT_CAL1, ct_calx );
	setRegister(CT_CAL2, ct_calx );
	setRegister(P1_FREQ2, px_freq2 );
	setRegister(P2_FREQ2, px_freq2 );

	setRegister(P1_FREQ3, px_freq3 );
	setRegister( P2_FREQ3, px_freq3 );
	// step 4 : Set Calibration mode
	setRegister(PLL_CAL1, 0x0028 );
	setRegister(PLL_CAL2, 0x0028 );
	// step 5 : Enable device : set SDI_CTRL.sipin = 1, SDI_CTRL.enable = 1,SDI_CTRL.mode = 1
	setRegister(SDI_CTRL, 0xE000 );
}

static void calcMixerPrescalers(uint16_t lo_freq_MHz_local){

		nlo = (uint8_t)log2_int(F_VCO_MAX_MHZ/lo_freq_MHz_local);
		lodiv = pow(2,nlo);

		fvco = lodiv*lo_freq_MHz;

		uint8_t fbkdiv_flag =0;// because what is set in the Regsiters differ from the  actual value of the fb Div
		uint8_t iodiv_flag =0; // because what is set in the Regsiters differ from the  actual value of the Lo div..
		double ndiv_frac = 0;
		double nummsb_d,numlsb_d;

		if(fvco <= 3200){
			fbkdiv = 2; // fbkdiv = 2 ( divide by 2 )
		 	fbkdiv_flag = 0x01; // register setting for Div2
		}else{
			fbkdiv = 4; // fbkdiv = 4 (divide by 4)
		 	fbkdiv_flag = 0x02; // register setting for Div4
		}

		switch(lodiv){
		case 2 :
			iodiv_flag = 1;
		break;
		case 4 :
			iodiv_flag = 2;
		break;
		case 8 :
			iodiv_flag = 3;
		break;
		case 16 :
			iodiv_flag = 4;
		break;
		case 32 :
			iodiv_flag = 5;
		break;
		default:
			iodiv_flag = 1;
		break;
		}
		ndiv = fvco/fbkdiv/REF_FREQ_MHZ;
		fn = (int)ndiv;
		ndiv_frac = ndiv-(double)fn;

		nummsb_d =   65536.0 * ndiv_frac ;
		nummsb = (uint16_t)nummsb_d;

		numlsb_d =  256.0 * ( (65536.0*ndiv_frac) - nummsb);
		numlsb = (uint16_t)numlsb_d;

		// set the values

		px_freq1 = 0;
		px_freq2 = 0;
		px_freq3 = 0;
	
		px_freq1 = (uint16_t)(fn << 7);
		px_freq1 |= (uint16_t)((iodiv_flag) << 4); 
		px_freq1 |= (uint16_t)((fbkdiv_flag) << 2);

		px_freq2 = nummsb; 	// N-divider numerator value, most significant 16 bits
		px_freq3 = numlsb<<8; 	// N divider numerator value, least significant 8 bits
	
		printf("LO set to: %d MHz\n", lo_freq_MHz_local);

}

static void setLinearity(uint16_t linearity_local){
	mix_cont = 0;
	mix_cont = (1<<15)|(linearity_local<<12)|(linearity_local<<9); // (full duplex)(mixer A linearity)(mixer b linearity)

		printf("Linearity set to: %d\n", linearity_local);
		printf("mix_cont: 0x%x\n", mix_cont);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	parse_opts(argc, argv);

//  if the LO or linearity was specified, calculate new values and set them, otherwise defauls will be sent

	if ((lo_freq_MHz >30) && (linearity >0 )){ // LO & Linearity set
		if(lo_freq_MHz < 30 || lo_freq_MHz > 3000){
			printf("LO frequency out of bounds (30-3000): %d\nSetting to 1918 MHz\n", lo_freq_MHz);
			lo_freq_MHz = 1918;
		}
		if(linearity <1 || linearity >7){
			printf("Linearity out of bounds (1-7) : %d\nSetting to 5\n", linearity);
				linearity = 5;
		}

		calcMixerPrescalers(lo_freq_MHz);
		setLinearity(linearity);
		defaultConfig = 0;

	}
	else if (lo_freq_MHz > 30){ // LO only set
		if(lo_freq_MHz < 30 || lo_freq_MHz > 3000){
			printf("LO frequency out of bounds (30-3000): %d\nSetting to 1918 MHz\n", lo_freq_MHz);
			lo_freq_MHz = 1918;
		}

		calcMixerPrescalers(lo_freq_MHz);
		setLinearity(LINEARITY_DEFAULT);
		defaultConfig = 0;
	}
	else if (linearity > 0 ){ // Linearity only set
		if(linearity <1 || linearity >7){
			printf("Linearity out of bounds (1-7) : %d\nSetting to 5\n", linearity);
			linearity = 5;
		}
		calcMixerPrescalers((uint16_t)LO_DEFAULT_MHZ);
		setLinearity(linearity);
		defaultConfig = 0;
	}
	else{
	// no parameters for transfer given, use defaults :
		
		defaultConfig = 1;
	}


	
	// set GPIO ( mmap )
	getpagesize();
	/* open /dev/mem to get acess to physical ram */
	if ((mem_fd = open("/dev/mem", O_RDWR) ) < 0) {
		printf("can't open /dev/mem. Did you run the program with administrator rights?\n");
		exit (-1);
	}
	/* Allocate a block of virtual RAM in our application's address space */
	if ((gpio_ram = malloc(BLOCK_SIZE + (PAGE_SIZE-1))) == NULL) {
		printf("allocation error \n");
		exit (-1);
	}

	if ((unsigned long)gpio_ram % PAGE_SIZE)
		gpio_ram += PAGE_SIZE - ((unsigned long)gpio_ram % PAGE_SIZE);


	gpio_mmap = (unsigned long *)mmap(
		(void *)gpio_ram,
		BLOCK_SIZE,
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		mem_fd,
		AR71XX_GPIO_BASE
	);

	//memread_memory(gpio_mmap, 16, 4);
	if ((long)gpio_mmap < 0) {
		printf("unable to map the memory. Did you run the program with administrator rights?\n");
		exit (-1);
	}
	/* Always use a volatile pointer to hardware registers */
	gpio = (volatile unsigned long*)gpio_mmap;	




/*set GPIO drive directions*/
	//gpioDirection(RIOT_GPIO_RF_ENABLE, 1) ;	//OUTPUT	
	gpioDirection(RIOT_GPIO_MIXER_RESET, 1) ;	//OUTPUT
	//gpioDirection(RIOT_GPIO_MIXER_ENABLE, 1) ;	//OUTPUT
	//gpioDirection(RIOT_GPIO_MIXER_MODE, 1) ;	//OUTPUT
	//gpioDirection(RIOT_GPIO_MIXER_LDO, 1) ;	//OUTPUT
	printf("GPIO Direction setup \n");
	gpioDirection(RIOT_GPIO_MIXER_SPI_SS, 1) ;	//OUTPUT
	gpioDirection(RIOT_GPIO_MIXER_SPI_SCK, 1) ;	//OUTPUT
	gpioDirection(RIOT_GPIO_MIXER_SPI_MOSI, 1) ;	//OUTPUT
	//gpioDirection(RIOT_GPIO_MIXER_SPI_MISO, 0) ;	//INPUT

/*set GPIO states*/
	//gpioSet(RIOT_GPIO_MIXER_LDO, 0);	// Mixer power supplieds active ( L ) - active LOW
	//gpioSet(RIOT_GPIO_MIXER_ENABLE, 1);	// Mixer enabled ( H ) // we use SPI control so this GPIO insnt needed..
	//gpioSet(RIOT_GPIO_MIXER_MODE, 1);	// Mixer synth 1 used ( H ) // we use SPI control so this GPIO insnt needed..
	//gpioSet(RIOT_GPIO_RF_ENABLE, 1);	// RF switch enabled ( H )
	
	printf("GPIO State setup \n");
	gpioSet(RIOT_GPIO_MIXER_RESET, 0);	// Reset mixer
	usleep(1);
	gpioSet(RIOT_GPIO_MIXER_SPI_SS, 1);	// Mixer SS / ENx
	gpioSet(RIOT_GPIO_MIXER_SPI_SCK, 0);	// Mixer SCK
	gpioSet(RIOT_GPIO_MIXER_SPI_MOSI, 0);	// Mixer MOSI

/*enable the mixer*/
	gpioSet(RIOT_GPIO_MIXER_RESET, 1);	// Mixer Active ( H )

	// set SPI regsiters
	printf("Sending config: ");
	if(defaultConfig){
	printf("Default\n");
		setMixerDefaults( );
	}else{
	printf("Custom\n");
		setMixerCustom( );
	}

	close (mem_fd);
	free(gpio_ram);

	return ret;
}
