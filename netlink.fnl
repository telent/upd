{
 :listen (fn [name]
           (let [fd (upd.netlink-listener name)]
             {"fd"
              fd
              "state"
              4
              "consume"
              (fn [this]
                (or (upd.netlink-read-message this.fd)
                    ))
              }))
 }
