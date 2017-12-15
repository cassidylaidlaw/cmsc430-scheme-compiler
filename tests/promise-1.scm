(let* ([x '0] [p (delay (begin (set! x (+ x '1)) x))])
  (list x (force p) (force p)))
