(begin
  (define v (make-vector 10 3))
  (let f ([x 0])
    (when (< x 5)
      (begin
        (vector-set! v x (- 10 x))
        (f (+ 1 x)))))
  (list v (vector-ref v 3) (vector-ref v 9) (vector-length v) (vector v '() 'el)))
