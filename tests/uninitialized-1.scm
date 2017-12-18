(define (f g n) (if (= n 0) g (f (g) (- n 1))))

(map (lambda (n)
       (guard (e [else 'error])
              (define (a) b)
              (define (b) c)
              (define (c) d)
              (define (d) e)
              (define e (f a n))
              (procedure? e)))
     '(0 1 2 3 4 5))
       