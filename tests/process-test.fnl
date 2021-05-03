(local inspect (require :inspect))
(local process (require :process))

(local all-tests
       [
        (lambda starts []
          (let [p (process.new ["/usr/bin/env"
                                ["env" "echo" "cookie"]
                                ["TERM=vt100"]]
                                )]
            (p:start)
            (let [out p.stdout
                  outstring (out:consume)]
              (assert (= outstring "cookie\n")))))
        ])

(each [_ value (pairs all-tests)] (value))
