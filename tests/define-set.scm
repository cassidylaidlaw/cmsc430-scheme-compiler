(define (f x)
  (define y (lambda () z))
  (set! x (+ x 1))
  (define z x)
  (y))
(f 5)