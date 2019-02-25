#!/bin/sh

# set the Wifi channel and Mixer LO so that we can use 26 TVWS channels while the 2.4GHs filter will only allow 13 to pass
# arguments are : -c TV channel [21 - 26] : corresponsds to ITU band IV channles ( 470- 694 MHz )
#		: -b bandwidth [5,10,20] MHz
#
# effects	: sets chanbw and channel parameters for radio0 ( using UCI )
#		: sets mixer LO by setting config file "freq" to 1918 for lower (12-34) or "freq" to 1814 for upper (35-47) TV channels ( using UCI )
# TODO call form LuCi in the web interface/use a config file and use  procd so we can use it from UCI / LuCI.


LOWEST_UHF=21		# cannot set to channels lower than this
HIGHEST_UHF=48
UHF_OFFSET=20	 	# we start at UHF channel 21

# gain settings for atheros to have a constant-ish output - channel 21 .. 48 // unused for now
GAINARRAY=" 4 4 4 4 5 4 4 4 3 3 3 3 3 7 7 7 7 5 3 3 2 1 1 1 1 2 2 2 " 

# mixer settings for atheros to have a constant channel (7, 2442 MHz) setting - channel 21 .. 48 - Standard Wifi configuration
MIXERARRAY="1968 1960 1952 1944 1936 1928 1920 1912 1904 1896 1888 1880 1872 1864 1856 1848 1840 1832 1824 1816 1808 1800 1792 1784 1776 1768 1760 1752" 
CONSTCHANNEL=7
CONSTCHANNELFREQ=2442
CONSTGAIN=1



calc_power_for_Channel()
{
#	if ! [[ $1 =~ ^-?[0-9]+([.][0-9]+)?$ ]]; then 
#		>&2 echo "$1 is not a number"
#		return $E_BADARGS; 
# 	fi
	local gain
	local gainIndex	
		#calculate the index of the gain we're interetsed in ( lookup starts at pos 1 )
	gainIndex=`expr $1 - $UHF_OFFSET`
	# cut out everything except the position we really want
	echo "$GAINARRAY" | cut -d ' ' -f $gainIndex
}


vary_mixer_for_Channel()
{
	local mixerIndex	
		#calculate the index of the setting we're interetsed in ( lookup starts at pos 1 )
	mixerIndex=`expr $1 - $UHF_OFFSET`
	# cut out everything except the position we really want
	echo "$MIXERARRAY" | cut -d ' ' -f $mixerIndex
}


get_UHF_Freq()
{
	local UHFFreq	
		#calculate the UHF cnahhel frequency based on mixer value and constant channel frequency
	UHFFreq=`expr $CONSTCHANNELFREQ - $1`
	# "return"
	echo $UHFFreq
}



## body /"main" of script

# check that we got an input
if [ $# -lt 1 ]
then
  echo "Usage: $0 [channel no 21..48]"
  exit 1
fi
# check that input was valid
#if [[ ! $1 || $1 = *[^0-9]* ]]; then
#    echo "Error: '$1' is not a number." >&2
#    exit 1
#fi

if [ $1 -lt $LOWEST_UHF ];then 
		>&2 echo "$1 is not a valid channel ($LOWEST_UHF .. $HIGHEST_UHF)"
elif [ $1 -gt $HIGHEST_UHF ];then 
		>&2 echo "$1 is not a valid channel ($LOWEST_UHF .. $HIGHEST_UHF)"
else :

	MIXER_SETTING=$(vary_mixer_for_Channel $1)
	WIFI_SETTING=$CONSTCHANNEL
	WIFI_POWER=$CONSTGAIN
	UHFFREQ=$(get_UHF_Freq $MIXER_SETTING)

	echo  "UHF channel $1: Mixer: $MIXER_SETTING MHz, Wifi channel: $WIFI_SETTING, UHF Freq: $UHFFREQ MHz"

## mixer settings
	uci set mixer.@mixer[0].freq=$MIXER_SETTING
	uci set mixer.@mixer[0].UHFChan=$1
	uci set mixer.@mixer[0].UHFFreq=$UHFFREQ

## Wifi radio settings
	uci set wireless.radio0.chanbw=5
	uci set wireless.radio0.txpower=$WIFI_POWER
	uci set wireless.radio0.channel=$WIFI_SETTING

## commit changes to disk 
	uci commit

## relaod/restart relevant services
	/etc/init.d/mixer reload
	/etc/init.d/network reload
fi
## set mixer LO
#uci set mixer.@mixer[0].freq='1918' # lower 13 channels

## set wifi settings
#uci set wireless.radio0.chanbw='5'
#uci set wireless.radio0.channel='13'

## apply settings
# uci commit
