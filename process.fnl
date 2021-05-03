(local watches (require :watches))
(local inspect (require :inspect))

(fn clock [] (os.time))

(fn start-process [p]
  (let [(pid errno stdout stderr) (upd.pfork)]
    (if (= pid 0)
        (upd.execve (table.unpack p.command))
        (do
          (tset p :stdout (watches.for-fd stdout))
          (tset p :stderr (watches.for-fd stderr))
          (tset p :pid pid)
          (tset p :backoff-until nil)
          (tset p :running? true)))))

(fn new-process [command]
  {
   :command command
   "running?" false
   "exit-status" nil
   :start (fn [p] (start-process p p.command))
   :stop (fn [p] (set p.running? false))
   :join (fn [p] (set p.exit-status 0))
   "backoff-until" nil
   "backoff-interval" 1
   :backoff
   (fn [p]
     (when (= nil p.backoff-until)
       (set p.backoff-until (+ (clock) p.backoff-interval))
       (set p.backoff-interval (* 2 p.backoff-interval))))
   "died?" (fn [p] (and (not p.running?) (= p.backoff-until nil)))
   "backoff-expired?"
   (fn [p]
     (and p.backoff-until (<= p.backoff-until (clock))))
   "aver-healthy" (fn [p] (set p.backoff-interval 1))
   })

{
 :new new-process
 :clock clock
 :run (fn [command] (let [p (new-process command)] (p:join)))
 }
