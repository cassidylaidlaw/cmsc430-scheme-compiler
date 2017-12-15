#lang racket

(require "utils.rkt")

(provide handle-wrong-arity
         handle-nonprocedure-application
         handle-zero-division
         handle-letrec-uninitialized)

; run before top-level
(define (handle-wrong-arity e)
  (define (wrap-lambda e [min-args 0] [max-args 2147483647])
    #;(match-define `(lambda ,xs ,es ... ,e0) e)
    (define arglist-sym (gensym 'argv))
    (define arglen-sym (gensym 'argc))
    (define inner-lambda-wrapped
      `(apply ,(map-exps handle-wrong-arity e) ,arglist-sym))
    (define min-wrapped
      (if (> min-args 0)
          `(if (< ,arglen-sym ,min-args)
               (raise '(error too-few-arguments))
               ,inner-lambda-wrapped)
          inner-lambda-wrapped))
    (define max-wrapped
      (if (< max-args 2147483647)
          `(if (> ,arglen-sym ,max-args)
               (raise '(error too-many-arguments))
               ,min-wrapped)
          min-wrapped))
    `(lambda ,arglist-sym
       (let ([,arglen-sym (length ,arglist-sym)])
         ,max-wrapped)))
  (match e
    [`(define (,x0 . ,xs) ,es ...)
     (handle-wrong-arity `(define ,x0 (lambda ,xs ,@es)))]
    [`(lambda (,(? symbol? xs) ... [,xds ,eds] ...) ,es ...)
     (wrap-lambda e (length xs) (+ (length xs) (length xds)))]
    [`(lambda (,(? symbol? xs) ... . ,(? symbol? xr)) ,es ...)
     (wrap-lambda e (length xs))]
    [`(lambda ,(? symbol? xv) ,es ...)
     (wrap-lambda e)]
    [else
     (map-exps handle-wrong-arity e)]))

(define (handle-nonprocedure-application e) e)

(define (handle-zero-division e) e)

(define (handle-letrec-uninitialized e)
  e)