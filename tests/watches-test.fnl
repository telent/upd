(local inspect (require :inspect))
(local watch (. (require :watches) :watch))

(local all-tests
       [
        (lambda test-fileno []
          (assert (= 2 (upd.fileno io.stderr)))
          (assert (= 1 (upd.fileno io.stdout)))
          (assert (= 0 (upd.fileno io.stdin)))
          (assert (< 2 (upd.fileno (io.open "/dev/null")))))
        ])

(each [_ value (pairs all-tests)] (value))
