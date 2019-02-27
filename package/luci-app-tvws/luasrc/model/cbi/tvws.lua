-- Copyright 2018 Daniel de Kock <daniel@riot.network>
-- Licensed to the public under the Apache License 2.0.

--[[
Map (config, title, description)
    config: configuration name to be mapped, see uci documentation and the files in /etc/config
    title: title shown in the UI
    description: description shown in the UI
]]--
-- working with /etc/config/tvws
m = Map("tvws", "TVWS control", translate("Frequency translation for Wifi to TVWS conversion"))

-- TypedSection (type, title, description)
s = m:section(TypedSection, "tvws", "Configuration options")
s.addremove = false -- don't allow user to add more , or remove the exiting section

s.anonymous = false

enable=s:option(Flag, "enabled", translate("Enabled"))
enable.enabled="1"
enable.disabled="0"
enable.default = "1"
enable.rmempty = false

--  Value (option, title, description)
UHFchan=s:option(Value, "UHFChan", translate("UHF TV channel"))
UHFchan.rmempty = false
UHFchan.default = "21"
UHFchan.datatype = "range(21,48)" -- must be between 21 and 48  : ITU TVWS frequency allocation


-- create a section in the view for status info which shall be rendered from /usr/lib/lua/luci/view/gpsd_status.htm
m:section(SimpleSection).template = "tvws_status"



return m


--[[ 
expects the following gpsd config file to exist/ creates it
/etc/config/tvws

config tvws
	option enabled 1
	option UHFChan 21
	option Wifichan 7
	option chanbw 5
	option txpower 1

]]--
