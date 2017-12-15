(letrec* ((p
           (lambda (x)
             (+ '1 (q (- x '1)))))
          (q
           (lambda (y)
             (if (= y '0)
                 '0
                 (+ '1 (p (- y '1))))))
          (x (p '5))
          (y x))
  y)
