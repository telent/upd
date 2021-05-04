(local inspect (require :inspect))
(local process (require :process))

(local all-tests
       [
        (lambda starts []
          (let [env (os.getenv "ENV")
	        p (process.new [env
                                ["env" "echo" "cookie"]
                                ["TERM=vt100"
				 (.. "PATH=" (os.getenv "PATH"))]
                                ])]
            (p:start)
            (let [out p.stdout
                  outstring (out:consume)]
              (assert (= outstring "cookie\n")
	       (.. "expected " (inspect "cookie\n")
		   " got " (inspect outstring))))))
        ])

(each [_ value (pairs all-tests)] (value))
