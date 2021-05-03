(local inspect (require :inspect))
(local watches (require :watches))
(local watch watches.watch)

(local all-tests
       [
        (fn watch-fds []
          (let [sources (icollect [_ v (ipairs [40 41 42 43])]
                                 (watches.for-fd v))]
            (each [m (watch sources 10000)]
              (print m))
            (assert false)))
        ])

(each [_ value (pairs all-tests)] (value))
