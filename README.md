# upd

What's upd?  Upd is yet another process/service monitor (like systemd,
sysvinit, procd etc) but the difference between this one and all the
rest is that it's substantially written in
[Fennel](https://fennel-lang.org/) (a Lua-based Clojure-ish Lisp
dialect)

The name is

 - an acronym for "micro process daemon", or
 - a shortened form of "up daemon", because it brings things up

I prefer the second explanation. Technically it's not a daemon, but
also technically systemd is not a daemon (does it fork into the
background and then exit? Nope) either and nobody complains about that
do they?

## Motivation

* We want to start and monitor services (daemons etc) in NixWRT and
  respond more quickly to changes in their state than by polling them
  every thirty seconds, as currently happens

* services often have state machines more complicated than "on" or "off",
  which informs how the service monitor should treat them.  For example,
  we don't want to start a service when the one we only just started
  is still initialising

  * for example we would like to poll the xl2tpd control socket to get
    l2tp tunnel health, then we could wait until it's set up before
    trying to start a session over it.

* A specific use case: we get data from our ISP via DHCP6 and Router
  Advertisements using odhcp6c. odhcp6c runs a script whenever the
  upstream sends new information, which is responsible for
  reconfiguring all relevant LAN interfaces, possibly the DHCP and
  local DNS services, etc. Why should odhcp6c have to know what its
  downstreams are and how to reconfigure them? We want pubsub!

* we'd like to know when processes die without relying on pids (racey)

* perhaps in future we could do secrets updates or other remote
  reconfig through this mechanism as well: e.g. push a new root ssh
  key onto the device and have ssh restart, or poll an HTTP server
  for an external list of suspect IP addresses that interested services
  could subscribe to, to deny service

* I've added Lua to the NixWRT image anyway because writing shell
  scripts in Busybox `sh` is not scalable beyond ~ 10 lines, so I
  may as well get some use for it

## Design

There is a Fennel script for each service - where a service might be a
process that must be kept running, or a network interface, or
... something else. Each script runs as its own Unix process.

A script represents its service state to other services in a directory
/run/upd/servicename, which contains a bunch of (usually quite small)
files with names, using subdirectories to represent structured data

```
/run/upd/odhcp6c-aaisp/ra_prefixes/1
/run/upd/odhcp6c-aaisp/ra_prefixes/1/prefix
2001:8b0:de3a:40dc::
/run/upd/odhcp6c-aaisp/ra_prefixes/1/length
64
/run/upd/odhcp6c-aaisp/ra_prefixes/1/valid
7200
/run/upd/odhcp6c-aaisp/ra_prefixes/1/preferred
7200
/run/upd/odhcp6c-aaisp/rdnss/1
2001:8b0::2020
/run/upd/odhcp6c-aaisp/rdnss/2
2001:8b0::2021
/run/upd/eth0.1/carrier
no
/run/upd/eth0.1/promiscuous
no
/run/upd/eth0.1/packets/rx
17489375
/run/upd/eth0.1/packets/tx
8201817
```

----
To invoke the pppoe service, do

    src/upscript fennel.lua  --add-fennel-path ./?.fnl pppoe.fnl -e '(main arg)'   eth0 ppp0

To run its tests, do

    src/upscript fennel.lua  --add-fennel-path ./?.fnl tests/pppoe-test.fnl
