(map (lambda (f) (guard (e [else 'nonprocedure]) (apply f '(5 14))))
     (list (lambda x x)
           5
           (void)
           #t
           +
           (lambda (y [z 2]) (cons y z))))
           
