(list (or) (or 1) (let* ([x 1] [y (or #f (set! x (+ x 1)))]) (cons (void? y) x))
  (let* ([x 1] [y (or #t (set! x (+ x 1)))]) (cons y x)))
