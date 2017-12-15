(map (lambda (p)
       (guard (e [else (cons 'z p)])
              (let ([l (void)] [d (call/cc (lambda (u) u))])
                (begin
                  (when (void? d) (raise d))
                  (list (guard (x [(= x '0) 'a] [(= x '1) 'b] [else (cons 'c x)])
                               (begin
                                 (set! p (- p '10))
                                 (let ([q p])
                                   (dynamic-wind
                                    (lambda () (set! p (+ p '10)))
                                    (lambda () (begin
                                                 (set! l (let/cc cc cc))
                                                 (cond
                                                   [(cons? l) (raise (car l))]
                                                   [(vector? l) ((vector-ref l '0) q)])
                                                 p))
                                    (lambda () (void))))))
                               
                        (guard (e [(= e '10) 'd])
                               (if (= p '7)
                                   (l p)
                                   (guard (e [(= e '9) 'f])
                                          (cond
                                            [(<= p '3) (l (cons p '()))]
                                            [(= p '4) (raise (let/cc g (l (vector g))))]
                                            [(= p '5) (let/cc g (l (vector g)))]
                                            [(<= p '10) (raise p)]
                                            [else p])))))))))
     (list '0 '1 '2 '3 '4 '5 '6 '7 '8 '9 '10))
