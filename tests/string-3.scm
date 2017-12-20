(define (converse s)
  (define (starts? s2) ; local to converse
    (define len2 (string-length s2))  ; local to starts?
    (and (>= (string-length s) len2)
         (equal? s2 (substring s 0 len2))))
  (cond
   [(starts? "hello") "hi!"]
   [(starts? "goodbye") "bye!"]
   [else "huh?"]))
 
(list (converse "hello!") (converse "urp"))
