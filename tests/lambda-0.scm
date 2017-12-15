(letrec ([f (lambda (a b . r) (if (null? r) '() (cons (+ (* (car r) a) b) (apply f (cons a (cons b (cdr r)))))))])
  (append (f '3 '1 '7 '10 '2) (f '7 '6)))
