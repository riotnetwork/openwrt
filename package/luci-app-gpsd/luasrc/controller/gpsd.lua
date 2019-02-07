
-- Copyright 2018 Daniel de Kock <daniel@riot.network>
-- Licensed to the public under the Apache License 2.0.


module("luci.controller.gpsd", package.seeall)

function index()
        if not nixio.fs.access("/etc/config/gpsd") then
                return
        end

        local page =  entry({"admin", "services", "gpsd"}, cbi("gpsd"), _("GPSd"))        -- create "Configuration" tab
        page.dependent = true

        entry({"admin", "services", "gpsd_status"}, call("gpsd_status"))        -- create "Status" section, but its a callback to the htm template in the cbi
end



function gpsd_status()
        local sys  = require "luci.sys"
        local uci  = require "luci.model.uci".cursor()
        local port = uci:get("gpsd", "core", "port")
        local socket = require "nixio"

        local status = {
                running = (sys.call("pidof gpsd >/dev/null") == 0),
                lock   = 0,
                lon   = 0,
                lat = 0
        }

        if status.running then
                local fd = socket.connect("127.0.0.1",(port or 2947),"any","stream")
                if fd then -- connected to the socket
                        --local html = fd:read("*a") -- read data form socket
                        --if html then
                        --      status.lock = (tonumber(html:match("Audio files</td><td>(%d+)")) or 0)
                        --      status.lon = (tonumber(html:match("Video files</td><td>(%d+)")) or 0)
                        --      status.lat = (tonumber(html:match("Image files</td><td>(%d+)")) or 0)
                        --end
                        fd:close()
                end
        end


	luci.http.prepare_content("application/json")
	luci.http.write_json(status)
end


--[[



module("luci.controller.gpsd", package.seeall)

function index()
        if not nixio.fs.access("/etc/config/gpsd") then
                return
        end

        local page = entry({"admin", "services", "gpsd"}, alias("admin", "services", "gpsd", "config"), _("GPSd")) -- create menu entry and a "landing page"
        entry({"admin", "services", "gpsd", "config"}, cbi("gpsd"), _("Configuration"),0)	-- create "configuration" tab
        entry({"admin", "services", "gpsd", "status"}, template("gpsd_status"), _("Status"))	-- create "status" tab
        page.dependent = true

end


function gpsd_Status()
	local status = ut.ubus("gpsd", "status", {})

	luci.http.prepare_content("application/json")
	if status ~= nil then
		luci.http.write_json(status)
	else
		luci.http.write_json({})
	end
end
]]--
