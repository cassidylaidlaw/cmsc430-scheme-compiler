#lang racket

(provide compile)

(require "utils.rkt")
(require "errors.rkt")
(require "top-level.rkt")
(require "desugar.rkt")
(require "cps.rkt")
(require (only-in "closure-convert.rkt" closure-convert))
(require (only-in "llvm.rkt" proc->llvm))

(define passes (list handle-wrong-arity
                     top-level
                     desugar
                     simplify-ir
                     assignment-convert
                     alphatize
                     anf-convert
                     cps-convert
                     closure-convert
                     proc->llvm))

(define (compile e)
  ; Apply each pass to e
  (foldl (lambda (pass e) (pass e)) e passes))
