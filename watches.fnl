(local inspect (require :inspect))


(fn watch [sources]
  (fn []
    (let [s (. sources 1)]
      (s:consume))))
{
 :watch watch
 }
