/*
 *  RIOT albatross support
 *
 *  Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2015 Hauke Mehrtens <hauke@hauke-m.de>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"
#include <asm/mach-ath79/ar71xx_regs.h>
#include <asm/mach-ath79/ath79.h>
#include <linux/gpio.h>
#include "common.h"

// RF mixer board control
#define RIOT_GPIO_RF_ENABLE		7
#define RIOT_GPIO_MIXER_RESET		18
#define RIOT_GPIO_MIXER_ENABLE		19
#define RIOT_GPIO_MIXER_MODE		20
#define RIOT_GPIO_MIXER_LDO		21

// GPS control
#define RIOT_GPIO_GPS_RESET		6

// Onboard LEDs
#define RIOT_GPIO_LED_WLAN		0
#define RIOT_GPIO_LED_USB		1
#define RIOT_GPIO_LED_ETH		17

// GPIO serial busses ( iniited later, but defined here for clarity )

// Mixer SPI control ( maually bit banged via dev/mem, mixer does not like standard SPI )
#define RIOT_GPIO_MIXER_SPI_SS		26
#define RIOT_GPIO_MIXER_SPI_SCK		11
#define RIOT_GPIO_MIXER_SPI_MOSI		27
#define RIOT_GPIO_MIXER_SPI_MISO		8

// Aux LED controller GPIO / BB_IIC / BB_SPI control
#define RIOT_GPIO_LEDBOARD_1_SCK		22
#define RIOT_GPIO_LEDBOARD_2_SDA		23
#define RIOT_GPIO_LEDBOARD_3_GPIO		24

#define RIOT_KEYS_POLL_INTERVAL		20	/* msecs */
#define RIOT_KEYS_DEBOUNCE_INTERVAL	(3 * RIOT_KEYS_POLL_INTERVAL)

#define RIOT_MAC0_OFFSET			0x0000
#define RIOT_MAC1_OFFSET			0x0006
#define RIOT_CALDATA_OFFSET		0x1000
#define RIOT_WMAC_MAC_OFFSET		0x1002


static struct gpio_led alba_leds_gpio[] __initdata = {
	{
		.name		= "alba:red:system",
		.gpio		= RIOT_GPIO_LED_USB,
		.active_low	= 0,
	},
	{
		.name		= "alba:red:wlan",
		.gpio		= RIOT_GPIO_LED_WLAN,
		.active_low	= 0,
	},
	{
		.name		= "alba:red:eth",
		.gpio		= RIOT_GPIO_LED_ETH,
		.active_low	= 1, 
	},
};



//static struct gpio_keys_button alba_gpio_keys[] __initdata = {
//	{
//		.desc		= "configuration button",
//		.type		= EV_KEY,
//		.code		= KEY_WPS_BUTTON,
//		.debounce_interval = RIOT_KEYS_DEBOUNCE_INTERVAL,
//		.gpio		= RIOT_GPIO_CONF_BTN,
//		.active_low	= 1,
//	},
//};

static void __init alba_common_setup(void)
{
	static u8 mac[6];

	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);
	ath79_register_m25p80(NULL);

	if (ar93xx_wmac_read_mac_address(mac)) {
		ath79_register_wmac(NULL, NULL);
	} else {
		ath79_register_wmac(art + RIOT_CALDATA_OFFSET,
				    art + RIOT_WMAC_MAC_OFFSET);
		memcpy(mac, art + RIOT_WMAC_MAC_OFFSET, sizeof(mac));
	}

	//mac[3] |= 0x08;
	mac[4] -= 1;
	ath79_init_mac(ath79_eth0_data.mac_addr, mac, 0);

	
	//ath79_init_mac(ath79_eth1_data.mac_addr, mac, 0);
	 ath79_register_mdio(0, 0x0);

	/* WAN port */
	ath79_register_eth(0); // this connects directly to "phy4" - no internal switch used
/* LAN ports */ // first come first enumerated , linux calls this "ETH0" even tho ist SoC eth1
	// ath79_register_eth(1); // connects to the internal switch
}

static void __init alba_setup(void)
{
	u32 t;

	alba_common_setup();

	ath79_register_leds_gpio(-1, ARRAY_SIZE(alba_leds_gpio),
				 alba_leds_gpio);
	//ath79_register_gpio_keys_polled(-1, RIOT_KEYS_POLL_INTERVAL,

	//				ARRAY_SIZE(alba_gpio_keys),
	//				alba_gpio_keys);
	ath79_register_usb();

	/* use the swtich_led directly form sysfs */
	ath79_gpio_function_disable(AR933X_GPIO_FUNC_ETH_SWITCH_LED0_EN |
								AR933X_GPIO_FUNC_ETH_SWITCH_LED1_EN |
								AR933X_GPIO_FUNC_ETH_SWITCH_LED2_EN |
								AR933X_GPIO_FUNC_ETH_SWITCH_LED3_EN |
								AR933X_GPIO_FUNC_ETH_SWITCH_LED4_EN);

	//Disable the Function for some pins to have GPIO functionality active
	// GPIO6-7-8 and GPIO11
	ath79_gpio_function_setup(AR933X_GPIO_FUNC_JTAG_DISABLE | AR933X_GPIO_FUNC_I2S_MCK_EN, 0);

	ath79_gpio_function2_setup(AR933X_GPIO_FUNC2_JUMPSTART_DISABLE, 0);

	
	// enable the use of GPIO 26 and 27
	t = ath79_reset_rr(AR933X_RESET_REG_BOOTSTRAP);
	t |= AR933X_BOOTSTRAP_MDIO_GPIO_EN;
	ath79_reset_wr(AR933X_RESET_REG_BOOTSTRAP, t);

	printk("Setting Albatross GPIO\n");
	// Put the GPS reset to high 
	if (gpio_request_one(RIOT_GPIO_GPS_RESET,
	    GPIOF_OUT_INIT_HIGH | GPIOF_EXPORT_DIR_FIXED, "GPS-Reset") != 0)
		printk("Error setting GPIO GPS\n");

	printk("GPIO GPS set (H)\n");

	//  RF mixer amplifier chain
	if (gpio_request_one(RIOT_GPIO_RF_ENABLE,
	    GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED, "RF-Enable") != 0)
		printk("Error setting RF ENABLE\n");

	printk("RF ENABLE set (L)\n"); // Active low, so setting L will enable RF switch 

	//  RF mixer Reset control
	if (gpio_request_one(RIOT_GPIO_MIXER_RESET,
	    GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED, "Mixer-Reset") != 0)
		printk("Error setting GPIO Mixer Reset\n");

	printk("GPIO Mixer Reset (L)\n");

	//  RF mixer Enable control
	if (gpio_request_one(RIOT_GPIO_MIXER_ENABLE,
	    GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED, "Mixer-Enable") != 0)
		printk("Error setting GPIO Mixer Enable\n");

	printk("GPIO Mixer Enable (L)\n");

	// RF mixer Mode control
	if (gpio_request_one(RIOT_GPIO_MIXER_MODE,
	    GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED, "Mixer-Mode") != 0)
		printk("Error setting GPIO Mixer mode\n");

	printk("GPIO Mixer Mode (L)\n");

	// eRF mixer Mode control
	if (gpio_request_one(RIOT_GPIO_MIXER_LDO,
	    GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED, "Mixer-LDO") != 0)
		printk("Error setting GPIO Mixer LDO\n");

	printk("GPIO Mixer LDO (L)\n");
}

MIPS_MACHINE(ATH79_MACH_ALBATROSS, "albatross", "RIOT Albatross", alba_setup);
