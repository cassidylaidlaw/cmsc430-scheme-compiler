(define (f x y)
  (guard (e [else `(error ,x ,y)])
    (quotient (begin (set! x (- x 3)) x) (begin (set! y (- y 5)) y))))

(map f '(1 1 3 3 5 5) '(3 5 3 5 3 5))