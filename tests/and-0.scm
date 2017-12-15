(list (and) (and 1) (let* ([x 1] [y (and #f (set! x (+ x 1)))]) (cons y x))
  (let* ([x 1] [y (and '#t (set! x (+ x 1)))]) (cons (void? y) x)))
