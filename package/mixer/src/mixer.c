/*
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
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <math.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/*GPIO bit banging */
#define GPIO_ADDR 0x18040000 // base address
#define GPIO_BLOCK 48 // memory block size

volatile unsigned long *gpioAddress;

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

/**************MIXER variables********************/
// from the RFFC2071 mixer programming guide
static double fvco,ndiv;
static uint16_t nlo,fn,nummsb,numlsb,lodiv,fbkdiv;

static uint16_t px_freq1 = Px_FREQ1_DEFAULT;
static uint16_t px_freq2 = Px_FREQ2_DEFAULT;
static uint16_t px_freq3 = Px_FREQ3_DEFAULT;
static uint16_t ct_calx  = CT_CALx_DEFAULT;
static uint16_t mix_cont  = MIX_CONT_DEFAULT;

//  Mixer setup blurt :  LO is at 1918 MHz

static uint16_t mixerDefaults[][2] = {
		// integrated mixer programming guide
		// step 0 : Reset device
		{ SDI_CTRL, 0x0002 },
		{ SDI_CTRL, 0x8000 }, // enable sipin : SPI control
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
		{ CT_CAL1, 0xACBF },
		{ CT_CAL2, 0xACBF },

		{ P1_FREQ1, 0x1218 },
		{ P1_FREQ2, 0xE276 },
		{ P1_FREQ3, 0x2700 },

		{ P2_FREQ1, 0x1218 },
		{ P2_FREQ2, 0xE276 },
		{ P2_FREQ3, 0x2700 },

		// step 4 : Set Calibration mode
		{ PLL_CAL1, 0x0028 },
		{ PLL_CAL2, 0x0028 },
		// step 5 : Enable device : set SDI_CTRL.sipin = 1, SDI_CTRL.enable = 1,SDI_CTRL.mode = 1
		{ SDI_CTRL, 0xE000 } // Software control
		//{ SDI_CTRL, 0x0000 } // hardware pin control
};




// log2 of an integer from : https://stackoverflow.com/questions/758001/log2-not-found-in-my-math-h
// If you're looking for an integral result, you can just determine the highest bit set in the value and return its position.

unsigned int log2_int( unsigned int x )
{
  unsigned int ans = 0 ;
  while( x>>=1 ) ans++;
  return ans ;
}

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev1.0";
static uint32_t mode;
static uint8_t bits = 25;
static char *input_file;
static char *output_file;
static uint32_t speed = 50000;
static uint16_t delay;
static int verbose;
static int transfer_size;
static int iterations;
static int interval = 5; /* interval in seconds for showing transfer rate */
static uint8_t linearity = 0; /*mixer Linearity 1-7*/
static uint16_t lo_freq_MHz = 0; /*mixer LO frequency in MHz*/
static uint8_t defaultConfig = 0;

char *input_tx;

static void hex_dump(const void *src, size_t length, size_t line_size,
		     char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

/*
 *  Unescape - process hexadecimal escape character
 *      converts shell input "\x23" -> 0x23
 */
static int unescape(char *_dst, char *_src, size_t len)
{
	int ret = 0;
	int match;
	char *src = _src;
	char *dst = _dst;
	unsigned int ch;

	while (*src) {
		if (*src == '\\' && *(src+1) == 'x') {
			match = sscanf(src + 2, "%2x", &ch);
			if (!match)
				pabort("malformed input string");

			src += 4;
			*dst++ = (unsigned char)ch;
		} else {
			*dst++ = *src++;
		}
		ret++;
	}
	return ret;
}

static void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;
	int out_fd;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
			tr.rx_buf = 0;
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
			tr.tx_buf = 0;
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	if (verbose)
		hex_dump(tx, len, 32, "TX");

	if (output_file) {
		out_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (out_fd < 0)
			pabort("could not open output file");

		ret = write(out_fd, rx, len);
		if (ret != len)
			pabort("not all bytes written to output file");

		close(out_fd);
	}

	if (verbose)
		hex_dump(rx, len, 32, "RX");
}

static void print_usage(const char *prog)
{
	printf("Usage: %s [-Dsbdo3vpLl]\n", prog);
	puts("  -D --device   device to use (default /dev/spidev1.0)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -d --delay    delay (usec)\n"
	     "  -b --bpw      bits per word\n"
	     "  -i --input    input data from a file (e.g. \"test.bin\")\n"
	     "  -o --output   output data to a file (e.g. \"results.bin\")\n"
	     "  -3 --3wire    SI/SO signals shared\n"
	     "  -v --verbose  Verbose (show tx buffer)\n"
	     "  -p            Send data (e.g. \"1234\\xde\\xad\")\n"
	     "  -f --freq     set LO frequency in MHz (e.g. -L 1918) \n"
	     "  -l --linear   set mixer linearity (1-7) \n"
);
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = { /* char *name ,int has_arg ,int *flag , int val */
			{ "device",  1, 0, 'D' },
			{ "speed",   1, 0, 's' },
			{ "delay",   1, 0, 'd' },
			{ "bpw",     1, 0, 'b' },
			{ "input",   1, 0, 'i' },
			{ "output",  1, 0, 'o' },
			{ "3wire",   0, 0, '3' },
			{ "verbose", 0, 0, 'v' },

			{ "LO"	   , 1, 0, 'f' },
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

		c = getopt_long(argc, argv, "D:s:d:b:i:o:3vp:f:l:", // 3 and v don't require any arguments , thus no : after them
				lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'D':
			device = optarg;
			break;
		case 's':
			speed = atoi(optarg);
			break;
		case 'd':
			delay = atoi(optarg);
			break;
		case 'b':
			bits = atoi(optarg);
			break;
		case 'i':
			input_file = optarg;
			break;
		case 'o':
			output_file = optarg;
			break;
		case '3':
			mode |= SPI_3WIRE;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'p':
			input_tx = optarg;
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

static void transfer_escaped_string(int fd, char *str)
{
	size_t size = strlen(str);
	uint8_t *tx;
	uint8_t *rx;

	tx = malloc(size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = malloc(size);
	if (!rx)
		pabort("can't allocate rx buffer");

	size = unescape((char *)tx, str, size);
	transfer(fd, tx, rx, size);
	free(rx);
	free(tx);
}

static void transfer_file(int fd, char *filename)
{
	ssize_t bytes;
	struct stat sb;
	int tx_fd;
	uint8_t *tx;
	uint8_t *rx;

	if (stat(filename, &sb) == -1)
		pabort("can't stat input file");

	tx_fd = open(filename, O_RDONLY);
	if (tx_fd < 0)
		pabort("can't open input file");

	tx = malloc(sb.st_size);
	if (!tx)
		pabort("can't allocate tx buffer");

	rx = malloc(sb.st_size);
	if (!rx)
		pabort("can't allocate rx buffer");

	bytes = read(tx_fd, tx, sb.st_size);
	if (bytes != sb.st_size)
		pabort("failed to read input file");

	transfer(fd, tx, rx, sb.st_size);
	free(rx);
	free(tx);
	close(tx_fd);
}

static uint64_t _read_count;
static uint64_t _write_count;

static void show_transfer_rate(void)
{
	static uint64_t prev_read_count, prev_write_count;
	double rx_rate, tx_rate;

	rx_rate = ((_read_count - prev_read_count) * 8) / (interval*1000.0);
	tx_rate = ((_write_count - prev_write_count) * 8) / (interval*1000.0);

	printf("rate: tx %.1fkbps, rx %.1fkbps\n", rx_rate, tx_rate);

	prev_read_count = _read_count;
	prev_write_count = _write_count;
}

static void transfer_buf(int fd, int len)
{
	uint8_t *tx;
	uint8_t *rx;
	int i;

	tx = malloc(len);
	if (!tx)
		pabort("can't allocate tx buffer");
	for (i = 0; i < len; i++)
		tx[i] = random();

	rx = malloc(len);
	if (!rx)
		pabort("can't allocate rx buffer");

	transfer(fd, tx, rx, len);

	_write_count += len;
	_read_count += len;

	if (mode & SPI_LOOP) {
		if (memcmp(tx, rx, len)) {
			fprintf(stderr, "transfer error !\n");
			hex_dump(tx, len, 32, "TX");
			hex_dump(rx, len, 32, "RX");
			exit(1);
		}
	}

	free(rx);
	free(tx);
}


//take addr and setting and create a 32bit value from them
uint32_t setRegister(uint16_t addr, uint16_t val) {
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


static void setMixerDefaults(int fd){

	uint32_t register_tx, dummy_rx ;


// 'total' will be 70 = 10 * 7
	uint16_t total = sizeof(mixerDefaults);
	// 'column' will be 7 = size of first row
	uint16_t column = sizeof(mixerDefaults[0]);
	// 'row' will be 10 = 70 / 7
	uint16_t row = total / column;
	for (int i = 0; i<row; i++) {  // cycle thru all the rows
		// build transfer packet from the configuration array 
		register_tx = setRegister(mixerDefaults[i][0], mixerDefaults[i][1]);
		// send to the device ( tell it to interpret the 32bit values as 8bit arrays ) 
		// works because machine endianness matches the MSB first of the data interpretation
		transfer(fd,  (char*)&register_tx, (char*)&dummy_rx, sizeof(register_tx));
 		usleep(1000); // wait 2 ms
	}
}

static void setMixerCustom(int fd){ 

	uint32_t register_tx, dummy_rx ;
// do this a few times.. once for every register we want to set.. ugh..

	// build transfer packet from calculated data
	// register_tx = setRegister(addr, value);

	// send to the device ( tell it to interpret the 32bit values as 8bit arrays ) 
	// works because machine endianness matches the MSB first of the data interpretation
	// transfer(fd,  (char*)&register_tx, (char*)&dummy_rx, sizeof(register_tx));
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

/*		
		printf("nlo: %d\n", nlo);
		printf("lodiv: %d\n", lodiv);
		printf("iodiv_flag: 0x%x\n", iodiv_flag);
		printf("fvco: %f\n", fvco);
		printf("ndiv: %f\n", ndiv);
		printf("fbdiv: %d\n", fbkdiv);
		printf("fbdiv_flag: 0x%x\n", fbkdiv_flag);
		printf("n: 0x%x\n", fn);
		printf("nummsb: %d\n", nummsb);
		printf("numlsb: %d\n", numlsb);

		printf("n shift: 0x%x\n", (uint16_t)(fn << 7));
		printf("lodiv flag shift: 0x%x\n", (uint16_t)((iodiv_flag) << 4));
		printf("fbdiv flag  shift: 0x%x\n", (uint16_t)((fbkdiv_flag ) << 2));
		
		printf("LO set to: %d\n", lo_freq_MHz);
		printf("px_freq1: 0x%x\n", px_freq1); // 1218
		printf("px_freq2: 0x%x\n", px_freq2);
		printf("px_freq3: 0x%x\n", px_freq3);
*/		
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
	int fd;

	parse_opts(argc, argv);

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");


	printf("spi device : %s\n", device);
	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	if (input_tx && input_file)
		pabort("only one of -p and --input may be selected");

	if (input_tx)
		transfer_escaped_string(fd, input_tx);
	else if (input_file)
		transfer_file(fd, input_file);

//  if the LO or linearity was specified, calculate new values and set them, otherwise defauls will be sent

	else if ((lo_freq_MHz >30) && (linearity >0 )){ // LO & Linearity set
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
		printf("Sending default config\n");
		defaultConfig = 1;
	}
	
	// set GPIO

	// set SPI regsiters
	if(defaultConfig){
		setMixerDefaults(fd);
	}else{
		setMixerCustom(fd);
	}

	close(fd);

	return ret;
}
