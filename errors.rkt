#lang racket

(require "utils.rkt")
(require (only-in "desugar.rkt" letrec-uninitialized-tag))

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

; run before desugar
(define (handle-nonprocedure-application e)
  (match e
    [(or (cons (? (one-of/c 'letrec* 'letrec 'let* 'guard 'raise 'dynamic-wind 'delay 'force 'cond
                            'case 'and 'or 'begin 'let 'if 'when 'unless 'lambda 'quote 'apply 'prim
                            'call/cc 'let/cc 'apply-prim 'set!)) _)
         (? symbol?))
     (map-exps handle-nonprocedure-application e)]
    [`(apply ,(? (not/c prim?) e0) ,e1)
     (define proc-sym (gensym 'proc))
     `(let ([,proc-sym ,e0])
        (if (prim procedure? ,proc-sym)
            ,(map-exps handle-nonprocedure-application `(apply ,proc-sym ,e1))
            (raise '(error nonprocedure-application))))]
    [`(,(? (not/c prim?) e0) ,es ...)
     (define proc-sym (gensym 'proc))
     `(let ([,proc-sym ,e0])
        (if (prim procedure? ,proc-sym)
            ,(map-exps handle-nonprocedure-application `(,proc-sym ,@es))
            (raise '(error nonprocedure-application))))]
    [else
     (map-exps handle-nonprocedure-application e)]))

; run before handle-wrong-arity
(define (handle-zero-division e)
  (match e
    [(or '/ 'quotient)
     '(lambda (numer denom0 . denoms)
        (let ([denom (prim * denom0 (apply-prim * denoms))])
          (if (= '0 denom)
              (raise '(error zero-division))
              (prim / numer denom))))]
    [else
     (map-exps handle-zero-division e)]))

; run before desugar
(define (handle-letrec-uninitialized e)
  (define ((wrap-references letrec-vars) e)
    (match e
      [`(,(? (one-of/c 'letrec 'letrec*) letrec-tag) ([,xs ,es] ...) ,e0)
       (define letrec-vars+ (set-union letrec-vars (list->set xs)))
       (define es+ (map (wrap-references letrec-vars+) es))
       (define e0+ ((wrap-references letrec-vars) e0))
       `(,letrec-tag ,(map list xs es+) ,e0+)]
      [(? symbol? x)
       (if (set-member? letrec-vars x)
           `(if (eq? ,x ',letrec-uninitialized-tag)
                (raise '(error uninitialized-variable))
                ,x)
           x)]
      [else
       (map-exps (wrap-references letrec-vars) e)]))
  ((wrap-references (set)) e))
