-- Copyright 2018 Daniel de Kock <daniel@riot.network>
-- Licensed to the public under the Apache License 2.0.

--[[
Map (config, title, description)
    config: configuration name to be mapped, see uci documentation and the files in /etc/config
    title: title shown in the UI
    description: description shown in the UI
]]--
-- working with /etc/config/gpsd
m = Map("gpsd", "GPSd", translate("gpsd is a service daemon that monitors GNSS or AIS receivers attached through serial or USB ports, making all data on the location/course/velocity of the sensors available to be queried on a TCP port of the host system."))

-- TypedSection (type, title, description)
s = m:section(TypedSection, "gpsd", "Configuration options")
s.addremove = false -- don't allow user to add more , or remove the exiting section

s.anonymous = false

enable=s:option(Flag, "enabled", translate("Enabled"))
enable.enabled="1"
enable.disabled="0"
enable.default = "1"
enable.rmempty = false

--  Value (option, title, description)
device=s:option(Value, "device", translate("Receiver"))
device.rmempty = true
device.datatype = "device"

op_port=s:option(Value, "port", translate("Location data port"))
op_port.default = "2947"
op_port.rmempty = false
op_port.datatype = "port"

listen_global=s:option(Flag, "listen_globally", translate("Listen Globally (external interfaces)"))
listen_global.enabled="1"
listen_global.disabled="0"
listen_global.default = "0"
listen_global.rmempty = false


-- create a section in the view for status info which shall be rendered from /usr/lib/lua/luci/view/gpsd_status.htm
m:section(SimpleSection).template = "gpsd_status"



return m


--[[ 
expects the following gpsd config file to exist/ creates it

/etc/config/gpsd
config gpsd 'core'
        option enabled '1'
        option device '/dev/ttyACM0'
        option port '2947'
        option listen_globally '0'

]]--
