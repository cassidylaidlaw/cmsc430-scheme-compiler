(let fac ([n '10])
    (define n-1 (- n 1))
    (if (= n '0)
        '1
        (* n (fac n-1))))
