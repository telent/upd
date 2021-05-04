(local inspect (require :inspect))
(local watches (require :watches))
(local watch watches.watch)

(local all-tests
       [
        (fn watch-fds []
          (var captured {})
          (var order 0)
	  (var bytes 0)
          (let [sources (icollect [_ v (ipairs [40 41 42 43])]
                                 (watches.for-fd v))]
            (each [m (watch sources 1000)]
              (when (not (= m ""))
		(set bytes (+ bytes (length m)))
                (tset captured m order)
                (set order (+ order 1))))
            (assert (= bytes 77)
	            (.. "incorrect length " bytes " received"))
            (assert (. captured "charlie\n") "missing data read")
            (assert (. captured "charlie says\n") "missing data read")
            (assert (. captured "dibbler\n") "missing data read")
            (assert (< (. captured "able\n")
                       (. captured "able bodied\n"))
                    "reads received out of order")
            (assert (not (. captured "tardy\n")) "failed to timeout")
            ))

        ])

(each [_ value (pairs all-tests)] (value))
