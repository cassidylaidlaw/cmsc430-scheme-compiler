(guard (condition
         (else
          'exception))
  (+ '1 (raise 'an-error)))
