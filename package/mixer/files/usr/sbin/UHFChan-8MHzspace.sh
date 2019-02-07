#!/bin/sh

# set the Wifi channel and Mixer LO so that we can use 26 TVWS channels while the 2.4GHs filter will only allow 13 to pass
# arguments are : -c TV channel [21 - 26] : corresponsds to ITU band IV channles ( 470- 694 MHz )
#		: -b bandwidth [5,10,20] MHz
#
# effects	: sets chanbw and channel parameters for radio0 ( using UCI )
#		: sets mixer LO by setting config file "freq" to 1918 for lower (12-34) or "freq" to 1814 for upper (35-47) TV channels ( using UCI )
# TODO call form LuCi in the web interface.


MIXER_BAND_1=1918	# lower band LO freq (MHz)( channel 21-33  | fc 474 - 570 MHz )
MIXER_BAND_2=1814 	# upper band LO freq (MHz)( channel 34-46  | fc 578 - 674 MHz )
BAND1_HIGHEST_UHF=33 	# lower band highest UHF channel
LOWEST_UHF=21		# cannot set to channels lower than this
HIGHEST_UHF=46
HIGHEST_WIFI=13 	# Highest Wifi channel number
UHF_OFFSET=20	 	# we start at UHF channel 21

# gain settings for atheros to have a constant-ish output - channel 21 .. 46 
GAINARRAY="4 4 4 4 5 4 4 4 3 3 3 3 3 7 7 7 7 5 3 3 2 1 1 1 1 2" 

# mixer settings for atheros to have a constant channel (7) setting - channel 21 .. 46 - Standard Wifi configuration
MIXERARRAY="1968 1960 1952 1944 1936 1928 1920 1912 1904 1896 1888 1880 1872 1864 1856 1848 1840 1832 1824 1816 1808 1800 1792 1784 1776 1768 1760 1752" 
CONSTCHANNEL=7
CONSTGAIN=1



# given a ITU TV channel, calculate the Mixer offset
calc_mixer_for_Channel()
{
 local mixerVal	
#	if ! [[ $1 =~ ^-?[0-9]+([.][0-9]+)?$ ]]; then 
#		>&2 echo "$1 is not a number"
#		return $E_BADARGS; 
#	fi
   
	 # decide which mixer band we should be in
	if [ $1 -le $BAND1_HIGHEST_UHF ];then 
			mixerVal=$MIXER_BAND_1
			#echo "$1 is in band 1, mixer set to $mixerVal MHz"
	else :
	
			mixerVal=$MIXER_BAND_2
			#echo "$1 is in band 2, mixer set to $mixerVal MHz"
	fi
	echo $mixerVal
}

# given a ITU TV channel, calculate the Wifi channel

calc_Wifi_for_Channel()
{
 local wifiChan	
#	if ! [[ $1 =~ ^-?[0-9]+([.][0-9]+)?$ ]]; then 
#		>&2 echo "$1 is not a number"
#		return $E_BADARGS; 
# 	fi
   
	 # decide which mixer band we should be in and tehn calculate the wifi channel
	if [ $1 -le $BAND1_HIGHEST_UHF ];then 
			wifiChan=`expr $1 - $UHF_OFFSET`
			#echo "$1 is in band 1, Wifi channel will be set to $wifiChan"
	else :
			wifiChan=`expr $1 - $BAND1_HIGHEST_UHF` 
			#echo "$1 is in band 2, Wifi channel will be set to $wifiChan"
	fi
	echo $wifiChan
}

calc_power_for_Channel()
{
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


## body /"main" of script

#some utility functions

if [ $# -lt 1 ]
then
  echo "Usage: $0 [channel no 21..46] [-f : fixed wifi and vary mixer]"
  exit 1
fi

if [ -z "$2" ];then
	echo "No second argument supplied, using variable wifi settings"
	MIXER_SETTING=$(vary_mixer_for_Channel $1)
	WIFI_SETTING=$CONSTCHANNEL
	WIFI_POWER=$CONSTGAIN
	echo  "UHF channel $1: Mixer: $MIXER_SETTING, Wifi channel: $WIFI_SETTING, Wifi power: $WIFI_POWER"

	uci set mixer.@mixer[0].freq=$MIXER_SETTING
	uci set wireless.radio0.chanbw=5
	uci set wireless.radio0.txpower=$WIFI_POWER
	uci set wireless.radio0.channel=$WIFI_SETTING
	uci commit
	/etc/init.d/mixer reload
	/etc/init.d/network reload


else :
	echo "second argument supplied, using variable wifi settings"

	if [ $1 -lt $LOWEST_UHF ];then 
		>&2 echo "$1 is not a valid channel ($LOWEST_UHF .. $HIGHEST_UHF)"
	elif [ $1 -gt $HIGHEST_UHF ];then 
		>&2 echo "$1 is not a valid channel ($LOWEST_UHF .. $HIGHEST_UHF)"
	else :

		MIXER_SETTING=$(calc_mixer_for_Channel $1)
		WIFI_SETTING=$(calc_Wifi_for_Channel $1)
		WIFI_POWER=$(calc_power_for_Channel $1)
		echo  "UHF channel $1: Mixer: $MIXER_SETTING, Wifi channel: $WIFI_SETTING, Wifi power: $WIFI_POWER"

		uci set mixer.@mixer[0].freq=$MIXER_SETTING
		uci set wireless.radio0.chanbw=5
		uci set wireless.radio0.txpower=$WIFI_POWER
		uci set wireless.radio0.channel=$WIFI_SETTING
		uci commit
		/etc/init.d/mixer reload
		/etc/init.d/network reload
	fi

fi

## set mixer LO
#uci set mixer.@mixer[0].freq='1918' # lower 13 channels
#uci set mixer.@mixer[0].freq='1814' # upper 13 channels

## set wifi settings
#uci set wireless.radio0.chanbw='5'
#uci set wireless.radio0.channel='13'

## apply settings
# uci commit
