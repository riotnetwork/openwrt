
#ifndef IFCONFIG_H_INCLUDED
#define IFCONFIG_H_INCLUDED


const char *ifconfig1 = " \
ens33     Link encap:Ethernet  HWaddr 00:0c:29:77:84:23  \
          inet addr:192.168.164.207  Bcast:192.168.164.255  Mask:255.255.255.0 \
          inet6 addr: fe80::87d0:a367:45b2:fb9d/64 Scope:Link \
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1 \
          RX packets:124476 errors:174 dropped:0 overruns:0 frame:0 \
          TX packets:61605 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000  \
          RX bytes:148456790 (148.4 MB)  TX bytes:5552475 (5.5 MB) \
          Interrupt:19 Base address:0x2000 \
\
lo        Link encap:Local Loopback  \
          inet addr:127.0.0.1  Mask:255.0.0.0 \
          inet6 addr: ::1/128 Scope:Host \
          UP LOOPBACK RUNNING  MTU:65536  Metric:1 \
          RX packets:7346 errors:0 dropped:0 overruns:0 frame:0 \
          TX packets:7346 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000 \
          RX bytes:637941 (637.9 KB)  TX bytes:637941 (637.9 KB) \
\
wlxc025e927f8d2 Link encap:Ethernet  HWaddr c0:25:e9:27:f8:d2   \
          UP BROADCAST MULTICAST  MTU:1500  Metric:1 \
          RX packets:0 errors:0 dropped:0 overruns:0 frame:0 \
          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000  \
          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B) \
          ";
const char *ifconfig2 = " \
ens33     Link encap:Ethernet  HWaddr 00:0c:29:77:84:23  \
          inet addr:192.168.164.207  Bcast:192.168.164.255  Mask:255.255.255.0 \
          inet6 addr: fe80::87d0:a367:45b2:fb9d/64 Scope:Link \
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1 \
          RX packets:124477 errors:174 dropped:0 overruns:0 frame:0 \
          TX packets:61605 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000 \
          RX bytes:148457036 (148.4 MB)  TX bytes:5552475 (5.5 MB) \
          Interrupt:19 Base address:0x2000 \
\
lo        Link encap:Local Loopback  \
          inet addr:127.0.0.1  Mask:255.0.0.0 \
          inet6 addr: ::1/128 Scope:Host \
          UP LOOPBACK RUNNING  MTU:65536  Metric:1 \
          RX packets:7346 errors:0 dropped:0 overruns:0 frame:0 \
          TX packets:7346 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000  \
          RX bytes:637941 (637.9 KB)  TX bytes:637941 (637.9 KB) \
\
wlxc025e927f8d2 Link encap:Ethernet  HWaddr c0:25:e9:27:f8:d2   \
          UP BROADCAST MULTICAST  MTU:1500  Metric:1 \
          RX packets:0 errors:0 dropped:0 overruns:0 frame:0 \
          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000 \
          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B) \
";
const char *ifconfig3 = " \
ens33     Link encap:Ethernet  HWaddr 00:0c:29:77:84:23   \
          inet6 addr: fe80::87d0:a367:45b2:fb9d/64 Scope:Link \
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1 \
          RX packets:124485 errors:174 dropped:0 overruns:0 frame:0 \
          TX packets:61616 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000 \
          RX bytes:148457775 (148.4 MB)  TX bytes:5553293 (5.5 MB) \
          Interrupt:19 Base address:0x2000 \
\
lo        Link encap:Local Loopback  \
          inet addr:127.0.0.1  Mask:255.0.0.0 \
          inet6 addr: ::1/128 Scope:Host \
          UP LOOPBACK RUNNING  MTU:65536  Metric:1 \
          RX packets:7362 errors:0 dropped:0 overruns:0 frame:0 \
          TX packets:7362 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000 \
          RX bytes:639093 (639.0 KB)  TX bytes:639093 (639.0 KB) \
\
wlxc025e927f8d2 Link encap:Ethernet  HWaddr c0:25:e9:27:f8:d2  \
          inet6 addr: fe80::c3a0:c197:3d9a:66d7/64 Scope:Link \
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1 \
          RX packets:18 errors:0 dropped:45 overruns:0 frame:0 \
          TX packets:23 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000 \
          RX bytes:5630 (5.6 KB)  TX bytes:4836 (4.8 KB) \
";
const char *ifconfig4 = " \
ens33     Link encap:Ethernet  HWaddr 00:0c:29:77:84:23  \
          inet addr:192.168.164.207  Bcast:192.168.164.255  Mask:255.255.255.0 \
          inet6 addr: fe80::87d0:a367:45b2:fb9d/64 Scope:Link \
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1 \
          RX packets:124508 errors:174 dropped:0 overruns:0 frame:0 \
          TX packets:61634 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000 \
          RX bytes:148461063 (148.4 MB)  TX bytes:5555117 (5.5 MB) \
          Interrupt:19 Base address:0x2000 \
\
lo        Link encap:Local Loopback  \
          inet addr:127.0.0.1  Mask:255.0.0.0 \
          inet6 addr: ::1/128 Scope:Host \
          UP LOOPBACK RUNNING  MTU:65536  Metric:1 \
          RX packets:7391 errors:0 dropped:0 overruns:0 frame:0 \
          TX packets:7391 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000  \
          RX bytes:641372 (641.3 KB)  TX bytes:641372 (641.3 KB) \
\
wlxc025e927f8d2 Link encap:Ethernet  HWaddr c0:25:e9:27:f8:d2  \
          inet addr:192.168.0.67  Bcast:192.168.0.255  Mask:255.255.255.0 \
          inet6 addr: fe80::c3a0:c197:3d9a:66d7/64 Scope:Link \
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1 \
          RX packets:120 errors:0 dropped:78 overruns:0 frame:0 \
          TX packets:64 errors:0 dropped:0 overruns:0 carrier:0 \
          collisions:0 txqueuelen:1000 \
          RX bytes:37819 (37.8 KB)  TX bytes:18410 (18.4 KB) \
          ";




#endif // IFCONFIG_H_INCLUDED
