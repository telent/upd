{
 :listen (fn [name]
           (let [fd (upd.netlink-listener name)]
             {"fd"
              fd
              "consume"
              (fn [this]
               (upd.netlink-read-message this.fd))
              }))
 }
