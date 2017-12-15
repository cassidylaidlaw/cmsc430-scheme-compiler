(let ([x (void)])
  (or
    (guard (e [else 'a])
           (begin
             (set! x (delay (raise '1)))
             '#f))
    (guard (e [else 'b])
           (begin
             (force x)))))