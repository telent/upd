(local netdev (require :netdev))
(local ppp (require :ppp))
(local process (require :process))
(local inspect (require :inspect))
(local watch (. (require :watches) :watch))

(fn nil? [x] (= x nil))

(fn pppoe-daemon [transport-device ppp-device]
  (let [ppp-device-name (ppp.device-name ppp-device)
        ipstate-script (ppp.ipstate-script ppp-device)
        pppd (process.new-process
              (.. "pppd " (netdev.device-name transport-device)
                  " --ip-up-script " ipstate-script
                  " --ip-down-script " ipstate-script))]
    (while (watch transport-device pppd)
      (when (pppd:died?) (pppd:backoff))
      (when (and (netdev.link-up? transport-device)
                 (not pppd.running?)
                 (pppd:backoff-expired?))
        (process.run
         (.. "ifconfig "
             (netdev.device-name transport-device)
             " up"))
        (pppd:start))
      (when (and pppd.running?
                 (not (netdev.link-up? transport-device)))
        (pppd:stop))
      (when  (ppp.up? ppp-device)
        (pppd:aver-healthy))
      )))

(lambda main [eth-device-name ppp-device-name]
  (let [pppdev (ppp.find-device ppp-device-name)]
    (pppoe-daemon (netdev.find-device eth-device-name) pppdev)))
