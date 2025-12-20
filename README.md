# TaHomaCtl

**TaHomaCtl** is a lightweight command-line interface (CLI) designed to control Somfy TaHoma home automation gateways (Switch, Connexoon, etc.) strictly locally.

It is ideal for integration into shell scripts, crontabs, or home automation backends.

## 🚀 Key Features

* **Device Control**: Open/close shutters, toggle lights, or adjust thermostats.
* **State Monitoring**: Retrieve real-time status and sensor data from your equipment.
* **Scenario Execution**: Trigger your pre-configured Somfy scenarios instantly.
* **Low Footprint**: Optimized code, perfect switched for resource limites compulters like single-board Raspberry Pi, Orange Pi, BananaPI, etc.
* **Dev-Friendly**: Output is designed to be easily parsed (JSON or raw text).

## ⚠️ Limitations

**TaHomaCtl** is interacting directly with your TaHoma, will not try to interpret results, will not try to secure dangerous actions : it's only an interface to use the Overkiz's public local interface, no more, no less.
In other words, it has no knowlegde about the devices you're steering.

> [!WARNING]
> The TaHoma is very slow to respond to some requests at first :
> - mDNS discovery (see discovery section)
> - 1st request handling : if you've got a timeout for the 1st request, retry. As per my test, it takes up to 30 seconds to responds. As soon as the 1st request succeeded, further request will succeed smoothly.

## 🛠 Installation

### Prerequisites
* A C compiler (GCC/Clang).

3rd party libraries (on Debian derivate, you need the development version `-dev`)
* `libcurl`: For API communication.
* `libjson-c`: For parsing JSON responses.

You need the TaHoma's developer mode activated : follow [Overkiz's intruction](https://github.com/Somfy-Developer/Somfy-TaHoma-Developer-Mode).

### Compilation

```
git clone [https://github.com/destroyedlolo/TaHomaCtl.git](https://github.com/destroyedlolo/TaHomaCtl.git)
cd TaHomaCtl
make
```

## 📖 Usages

### Shell parameters

Use online help for uptodate list of supported arguments.

```
$ ./TaHomaCtl -h
TaHomaCrl v0.3
	Control your TaHoma box from a command line.
(c) L.Faillie (destroyedlolo) 2025

Scripting :
	-f : source provided script
	-N : don't execute ~/.tahomactl at startup

TaHoma's :
	-H : set TaHoma's hostname
	-p : set TaHoma's port
	-k : set bearer token
	-U : don't verify SSL chaine (unsafe mode)

Limiting scanning :
	-4 : resolve Avahi advertisement in IPv4 only
	-6 : resolve Avahi advertisement in IPv6 only

Misc :
	-v : add verbosity
	-t : add tracing
	-d : add some debugging messages
	-h ; display this help
```

* **-U** : don't try to enforce security SSL chaine. Usefull if you haven't imported Overkiz's root CA.
* **-N** : by default, TaHomaCtl will try to source ~/.tahomactl at startup, which aims to contain TaHoma's connectivity information. This argument prevents it.

### In the application

Inside the application, you will benefit of GNU's readline features :
- history
- completion (hit TAB key)

An online help is also available :

```
TaHomaCtl > ?
List of known commands
======================

TaHoma's Configuration
----------------------
'TaHoma_host' : [name] set or display TaHoma's host
'TaHoma_address' : [ip] set or display TaHoma's ip address
'TaHoma_port' : [num] set or display TaHoma's port number
'TaHoma_token' : [value] indicate application token
'timeout' : [value] specify API call timeout (seconds)
'scan' : Look for Tahoma's ZeroConf advertising
'status' : Display current connection informations

Scripting
---------
'save_config' : <file> save current configuration to the given file
'script' : <file> execute the file

Verbosity
---------
'verbose' : [on|off|] Be verbose
'trace' : [on|off|] Trace every commands

Interacting
-----------
'Gateway' : Query your gateway own configuration
'Devices' : Query and store attached devices
'States' : <device name> query the states of a device

Miscs
-----
'#' : Comment, ignored line
'?' : List available commands
'history' : List command line history
'Quit' : See you
```

#### Discoverying your TaHoma

* **scan** will try to find out your TaHoma.
* Upon success, **status** will show you stored information
* you **have to** provide application token (from Somfy's TaHoma application) using **TaHoma_token** command
* finally, **save_config** to store all those valuable information. 

```
./TaHomaCtl -Uv
*W* SSL chaine not enforced (unsafe mode)
TaHomaCtl > scan 
*I* Service 'gateway-xxxx-xxxx-xxxx' of type '_kizboxdev._tcp' in domain 'local':
	gateway-xxxx-xxxx-xxxx.local:8443 (192.168.0.36)
	TXT="fw_version=2025.5.5-9" "gateway_pin=xxxx-xxxx-xxxx" "api_version=1"
	cookie is 0
	is_local: 0
	our_own: 0
	wide_area: 0
	multicast: 1
	cached: 1
TaHomaCtl > status
*I* Connection :
	Tahoma's host : gateway-xxxx-xxxx-xxxx.local
	Tahoma's IP : 192.168.0.36
	Tahoma's port : 8443
	Token : set
	SSL chaine : not checked (unsafe)
TaHomaCtl > save_config /tmp/tahoma
TaHomaCtl > 
```

> [!TIP]
> As said previously, the TaHoma doesn't react in a timely way to mDNS request and doesn't advertise often.<br>
> The safer way seems to run Avahi's explorator and launch scan when the TaHoma is seen.

like [Majordome](https://github.com/destroyedlolo/Majordome).
