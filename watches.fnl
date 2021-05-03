(local inspect (require :inspect))

(fn watch-for-fd [fd]
  {"fd" fd
   "eof?" false
   "consume" (fn [this]
               (if (not this.eof?)
                   (let [r (upd.read-fd this.fd)]
                     (if (= r "")
                         (set this.eof? true))
                     r)))
   }
  )

(fn watch [sources timeout-ms]
  (fn []
    (let [sources-hash (collect [_ v (pairs sources)]
                                (values v.fd v))
          fds (icollect [_ v (pairs sources)]
                        (and (not v.eof?) v.fd))
          ready (upd.poll-fds fds timeout-ms)]
      (if ready
          (let [source (. sources-hash ready)]
            (source:consume))))))
{
 :watch watch
 :null #(watch-for-fd  (io.open "/dev/null"))
 :for-fd watch-for-fd
 }
