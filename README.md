# TaHomaCtl

**TaHomaCtl** is a lightweight command-line interface (CLI) designed to control Somfy TaHoma home automation gateways (Switch, Connexoon, etc.) strictly locally.

It is ideal for integration into shell scripts, crontabs, or home automation backends.

## 🚀 Key Features

* **State Monitoring**: Retrieve real-time status and sensor data from your equipment.
* **Low Footprint**: Optimized code, perfect switched for resource limites compulters like single-board Raspberry Pi, Orange Pi, BananaPI, etc.
* **Dev-Friendly**: Output is designed to be easily parsed (JSON or raw text).

### Planned for next versions

**TaHomaCtl** is in its early stage, providing mainly devices' querying. Following features are planned to be implemented soon :

* **Device Control**: Open/close shutters, toggle lights, or adjust thermostats
* **Scenario Execution**: Trigger your pre-configured Somfy scenarios instantly

## ⚠️ Limitations

**TaHomaCtl** is interacting directly with your TaHoma, will not try to interpret results, will not try to secure dangerous actions : it's only an interface to use the Overkiz's public local interface, no more, no less.  
In other words, it has no knowlegde about the devices you're steering.

As dealing directly with your gateway, you can't interact or control stuffs managed at Somfy's cloud side, like Somfy Protect or Cloud2Cloud processes.  
The solution may be to use the Overkiz's "*end user cloud public API*", but my smart homing aims to be as local as possible. Consequently, it's out of the scope of TaHomaCtl.

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
* `readline` : GNU's readline for command line improvement.

You need the TaHoma's developer mode activated : follow [Overkiz's intruction](https://github.com/Somfy-Developer/Somfy-TaHoma-Developer-Mode).

### Compilation

```
git clone https://github.com/destroyedlolo/TaHomaCtl.git
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
'States' : <device name> [State's name] query the states of a device

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

#### Discovering your devices

**Devices** will query your box for attached (and internal as well) devices. They will be displayed if the *verbose* mode is activated and stored in **TaHomaCtl** for further use.

```
TaHomaCtl > Devices 
*I* HTTP return code : 200
*I* 5 devices
*I* INTERNAL (wifi/0) [internal:WifiComponent]
	URL : internal://xxxx-xxxx-xxxx/wifi/0
	Type : 1, subsystemId : 0
	synced, enabled, available
		Type: ACTUATOR
*I* Boiboite [internal:PodV3Component]
	URL : internal://xxxx-xxxx-xxxx/pod/0
	Type : 1, subsystemId : 0
	synced, enabled, available
		Type: ACTUATOR
*I* ZIGBEE (65535) [zigbee:TransceiverV3_0Component]
	URL : zigbee://xxxx-xxxx-xxxx/65535
	Type : 5, subsystemId : 0
	synced, enabled, available
		Type: PROTOCOL_GATEWAY
*I* Deco [io:OnOffIOComponent]
	URL : io://xxxx-xxxx-xxxx/5335270
	Type : 1, subsystemId : 0
	synced, enabled, available
		Type: ACTUATOR
*I* IO (10069463) [io:StackComponent]
	URL : io://xxxx-xxxx-xxxx/10069463
	Type : 5, subsystemId : 0
	synced, enabled, available
		Type: PROTOCOL_GATEWAY
```
> [!CAUTION]
> This request is very resource-intensive for the TaHoma, especially if you have many connected devices.
> It is therefore advisable to use it as infrequently as possible, generally only once at startup.

#### Querying a device

```
TaHomaCtl > States Deco 
	core:StatusState : "available"
	core:CommandLockLevelsState : [Array]
	core:DiscreteRSSILevelState : "normal"
	core:RSSILevelState : "54"
	core:OnOffState : "off"
	core:PriorityLockTimerState : "0"
	io:PriorityLockOriginatorState : "unknown"
	core:NameState : "Deco"
```

It's also possible to query a single state. In such case, only the value is returned.

```
$ ./TaHomaCtl -U
TaHomaCtl > Devices 
TaHomaCtl > States Deco core:OnOffState
"off"
TaHomaCtl > 
```

> [!NOTE]
> **Devices** command here is still important to refresh attached devices internal information.  
> It would be easy by the way to add a command where the device URI is provided instead of name to ride out it.

Intesting in scripts :

``` bash
$ ./TaHomaCtl -Uf - << eoc
Devices
States Deco core:OnOffState
eoc
"off"
```

> [!IMPORTANT]
> For the moment, I made tests only with the device I'm having : an **IO OnOff switch**.<br>
> Consequently, some figures are not handled as not provided by my device (like Arrays or sub Objects).

## Why TaHomaCtl ?

### Integration in my own automation solution

My own smart home solution is quite efficient and complete ([Marcel](https://github.com/destroyedlolo/Marcel), [Majordome](https://github.com/destroyedlolo/Majordome), ...), no need to replace anything. But I was wondering how I can integrate this nice box to my own ecosystem, as example to steer **IO-homecontrol** devices.

TaHomaCtl was initially made as a Proof of Concept (PoC) before integrating something in Marcel (or as an autonomous daemon).

> [!NOTE]
> Yes, I'm proudly part of Somfy/Overkiz, but this code doesn't contain any internals' and will not to avoid any interest conflict : it was built only from publicly available information and my own testing.<br>
> No, don't ask me about anything not made public, I'll not reply.

### Vibe coding testing

**TaHomaCtl** uses some technologies I hadn't coded before, like mDNS advertising. This small project was a good candidate to test AI companions (ChatGPT and Gemini) and vibe coding. AI generated code, sometime corrected, can be found in TestCodes. The result is quite mixed :

- mDNS : the outcome is poor, sometime bad ! ChatGPT stays on its own mistakes (non-existent functions, stupid assumptions, wrong strategies, ...). Then I ask Gemini to correct and got some improvements. But, all in all, the result is heavy, not flexible and doesn't suite my quality standard, by far.

- GNU readline : generated code was better ... but still not functional until manual corrections.

Well, the AI saves me some time to find technical information, replacing google researches. But it is far far away to build from scratch something.
