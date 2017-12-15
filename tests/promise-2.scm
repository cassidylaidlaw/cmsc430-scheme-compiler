(list
 (force (delay (+ '1 '2)))
 (let ((p (delay (+ '1 '2))))
   (list (force p) (force p)))

 (letrec*
   ([a-stream (letrec ((next
                        (lambda (n)
                          (cons n (delay (next (+ n '1)))))))
                (next '0))]
    [head car]
    [tail (lambda (stream) (force (cdr stream)))])

   (head (tail (tail a-stream)))))