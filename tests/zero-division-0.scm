(map (lambda (args) (guard (e [else 'error]) (apply / args)))
     '((60 3 2)
       (20 5)
       (8 4 0)
       (8 2 4)
       (16 3 0 8)
       (10 0)
       ()
       (0 5)))
