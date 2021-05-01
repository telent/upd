(local inspect (require :inspect))
(local netlink (require :netlink))
(local watch (. (require :watches) :watch))

(local all-tests
       [
        (lambda get-link-status-events []
          (let [nl (netlink.listen "tests/netlink-messages.raw")]
            (var ups 0)
            (var downs 0)
            (var others 0)
            (each [event (watch [nl])]
              (match event
                {:link-status "up"} (set ups (+ ups 1))
                {:link-status "down"} (set downs (+ downs 1))
                _ (set others (+ others 1))))
            (assert (= ups 4))
            (assert (= downs 2))))
        ])

(each [_ value (pairs all-tests)] (value))
