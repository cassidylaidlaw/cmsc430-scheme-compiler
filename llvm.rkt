#lang racket

(require racket/format)
(require "utils.rkt")

(provide closure-convert
         proc->llvm)



; Pass that removes lambdas and datums as atomic and forces them to be let-bound
;   ...also performs a few small optimizations
(define (simplify-ae e)
  (define (wrap-aes aes wrap)
    (match-define (cons xs wrap+)
                  (foldr (lambda (ae xs+wrap)
                           (define gx (gensym 'arg))
                           (if (symbol? ae)
                               (cons (cons ae (car xs+wrap))
                                     (cdr xs+wrap))
                               (cons (cons gx (car xs+wrap))
                                     (lambda (e)
                                       (match ae
                                              [`(lambda ,xs ,body) 
                                               `(let ([,gx (lambda ,xs ,(simplify-ae body))])
                                                  ,((cdr xs+wrap) e))]
                                              [`',dat
                                               `(let ([,gx ',dat])
                                                  ,((cdr xs+wrap) e))])))))
                         (cons '() wrap)
                         aes))
    (wrap+ xs))
  (match e
         [`(let ([,x (lambda ,xs ,elam)]) ,e0)
          `(let ([,x (lambda ,xs ,(simplify-ae elam))]) ,(simplify-ae e0))]

         [`(let ([,x ',dat]) ,e0)
          `(let ([,x ',dat]) ,(simplify-ae e0))]

         [`(let ([,x (prim ,op ,aes ...)]) ,e0)
          (wrap-aes aes (lambda (xs) `(let ([,x (prim ,op ,@xs)]) ,(simplify-ae e0))))]
         [`(let ([,x (apply-prim ,op ,aes ...)]) ,e0)
          (wrap-aes aes (lambda (xs) `(let ([,x (apply-prim ,op ,@xs)]) ,(simplify-ae e0))))]

         [`(if (lambda . ,_) ,et ,ef)
          (simplify-ae et)]
         [`(if '#f ,et ,ef)
          (simplify-ae ef)]
         [`(if ',dat ,et ,ef)
          (simplify-ae et)]
         [`(if ,(? symbol? x) ,et ,ef)
          `(if ,x ,(simplify-ae et) ,(simplify-ae ef))]

         [`(apply ,ae0 ,ae1)
          (wrap-aes `(,ae0 ,ae1) (lambda (xs)
                                   (match-define `(,x0 ,x1) xs)
                                   `(apply ,x0 ,x1)))]
         
         [`(,aes ...)
          (wrap-aes aes (lambda (xs) xs))]))


; Perhaps write a helper to remove vararg lambdas/callsites
(define (remove-varargs scps)
  (define (unpack-args arglist args e)
    (if (null? args)
        e
        (let ([arglist- (gensym 'arglist)])
          `(let ([,(car args) (prim car ,arglist)])
             (let ([,arglist- (prim cdr ,arglist)])
               ,(unpack-args arglist- (cdr args) e))))))
  (define (pack-args args f)
    (define arglist-inner (gensym 'arglist))
    (match-define (cons arglist-outer e)
      (foldl (lambda (arg arglist+e)
               (match-define (cons arglist e) arglist+e)
               (define arglist- (gensym 'arglist))
               (cons arglist- `(let ([,arglist (prim cons ,arg ,arglist-)]) ,e)))
             (cons arglist-inner `(,f ,arglist-inner))
             args))
    `(let ([,arglist-outer '()]) ,e))
  
  (match scps
         [`(let ([,x (lambda (,xs ...) ,elam)]) ,e0)
          (define arglist (gensym 'arglist))
          `(let ([,x (lambda (,arglist)
                             ,(unpack-args arglist xs (remove-varargs elam)))])
             ,(remove-varargs e0))]
         [`(let ([,x (lambda ,x0 ,elam)]) ,e0)
          `(let ([,x (lambda (,x0) ,(remove-varargs elam))]) ,(remove-varargs e0))]
         [`(let ([,x ',dat]) ,e0)
          `(let ([,x ',dat]) ,(remove-varargs e0))]
         [`(let ([,x (prim ,op ,xs ...)]) ,e0)
          `(let ([,x (prim ,op ,@xs)]) ,(remove-varargs e0))]
         [`(let ([,x (apply-prim ,op ,x0)]) ,e0)
          `(let ([,x (apply-prim ,op ,x0)]) ,(remove-varargs e0))]
         [`(if ,(? symbol? x) ,et ,ef)
          `(if ,x ,(remove-varargs et) ,(remove-varargs ef))]
         [`(apply ,x0 ,x1)
          `(,x0 ,x1)]
         [`(,f ,xs ...)
          (pack-args xs f)]))


; call simplify-ae on input to closure convert, then cfa-0, then walk ast
(define (closure-convert cps)
  (define scps (simplify-ae cps))
  (define no-varargs-cps (remove-varargs scps))
  
  (define (bottom-up e procs)
    (match e
      [`(let ([,x ',dat]) ,e0)
       (match-define `(,e0+ ,free+ ,procs+)
                     (bottom-up e0 procs))
       `((let ([,x ',dat]) ,e0+)
         ,(set-remove free+ x)
         ,procs+)]
      [`(let ([,x (,(? (one-of/c 'prim 'apply-prim) prim) ,op ,xs ...)]) ,e0)
       (match-define `(,e0+ ,free+ ,procs+)
                     (bottom-up e0 procs))
       `((let ([,x (,prim ,op ,@xs)]) ,e0+)
         ,(set-remove (set-union free+ (list->set xs)) x)
         ,procs+)]
      [`(let ([,x (lambda (,xs ...) ,body)]) ,e0)
       (match-define `(,e0+ ,free0+ ,procs0+)
                     (bottom-up e0 procs))
       (match-define `(,body+ ,freelam+ ,procs1+)
                     (bottom-up body procs0+))
       (define env-vars (foldl (lambda (x fr) (set-remove fr x))
                               freelam+
                               xs))
       (define ordered-env-vars (set->list env-vars))
       (define lamx (gensym 'lam))
       (define envx (gensym 'env))
       (define body++ (cdr (foldl (lambda (x count+body)
                                    (match-define (cons cnt bdy) count+body)
                                     (cons (+ 1 cnt)
                                           `(let ([,x (env-ref ,envx ,cnt)])
                                              ,bdy)))
                                  (cons 1 body+)
                                  ordered-env-vars)))
       `((let ([,x (make-closure ,lamx ,@ordered-env-vars)]) ,e0+)
         ,(set-remove (set-union free0+ env-vars) x)
         ((proc (,lamx ,envx ,@xs) ,body++) . ,procs1+))]
      [`(if ,(? symbol? x) ,e0 ,e1)
       (match-define `(,e0+ ,free0+ ,procs0+)
                     (bottom-up e0 procs))
       (match-define `(,e1+ ,free1+ ,procs1+)
                     (bottom-up e1 procs0+))
       `((if ,x ,e0+ ,e1+)
         ,(set-union free1+ free0+ (set x))
         ,procs1+)]
      [`(,(? symbol? xs) ...)
       `((clo-app ,@xs)
         ,(list->set xs)
         ,procs)]))
  (match-define `(,main-body ,free ,procs) (bottom-up no-varargs-cps '()))
  `((proc (main) ,main-body) . ,procs))

; Extra header for LLVM files that contains closure-processing functions
(define llvm-header "define i64 @make_clo(i64*, i64) #0 {\n\
  %3 = alloca i64*, align 8\n\
  %4 = alloca i64, align 8\n\
  %5 = alloca i64*, align 8\n\
  store i64* %0, i64** %3, align 8\n\
  store i64 %1, i64* %4, align 8\n\
  %6 = call i8* @_Znam(i64 16) #3\n\
  %7 = bitcast i8* %6 to i64*\n\
  store i64* %7, i64** %5, align 8\n\
  %8 = load i64*, i64** %3, align 8\n\
  %9 = ptrtoint i64* %8 to i64\n\
  %10 = load i64*, i64** %5, align 8\n\
  %11 = getelementptr inbounds i64, i64* %10, i64 0\n\
  store i64 %9, i64* %11, align 8\n\
  %12 = load i64, i64* %4, align 8\n\
  %13 = load i64*, i64** %5, align 8\n\
  %14 = getelementptr inbounds i64, i64* %13, i64 1\n\
  store i64 %12, i64* %14, align 8\n\
  %15 = load i64*, i64** %5, align 8\n\
  %16 = ptrtoint i64* %15 to i64\n\
  ret i64 %16\n\
}\n\
\n\
define i64* @get_clo_func(i64) #2 {\n\
  %2 = alloca i64, align 8\n\
  %3 = alloca i64*, align 8\n\
  store i64 %0, i64* %2, align 8\n\
  %4 = load i64, i64* %2, align 8\n\
  %5 = and i64 %4, -8\n\
  %6 = inttoptr i64 %5 to i64*\n\
  store i64* %6, i64** %3, align 8\n\
  %7 = load i64*, i64** %3, align 8\n\
  %8 = getelementptr inbounds i64, i64* %7, i64 0\n\
  %9 = load i64, i64* %8, align 8\n\
  %10 = inttoptr i64 %9 to i64*\n\
  ret i64* %10\n\
}\n\
\n\
define i64 @get_clo_env(i64) #2 {\n\
  %2 = alloca i64, align 8\n\
  %3 = alloca i64*, align 8\n\
  store i64 %0, i64* %2, align 8\n\
  %4 = load i64, i64* %2, align 8\n\
  %5 = and i64 %4, -8\n\
  %6 = inttoptr i64 %5 to i64*\n\
  store i64* %6, i64** %3, align 8\n\
  %7 = load i64*, i64** %3, align 8\n\
  %8 = getelementptr inbounds i64, i64* %7, i64 1\n\
  %9 = load i64, i64* %8, align 8\n\
  ret i64 %9\n\
}\n")

(define (proc->llvm procs)
  (define (symbol->llvm-val sym)
    (c-name sym))
  (define (make-arglist args)
    (string-join (map (lambda (arg) (string-append "i64 %" (symbol->llvm-val arg))) args) ", "))
  (define (add-global decl)
    (set! globals (cons decl globals)))
  (define (add-global-string s)
    (define strlen (string-length s))
    (define sname (gensym 'str))
    (define escaped-str (string-join (map (lambda (c)
                                            (string-append "\\"
                                                           (~r (char->integer c)
                                                               #:base '(up 16)
                                                               #:pad-string "0"
                                                               #:min-width 2)))
                                          (string->list s))
                                     ""))
    (begin
      (add-global (format "@~a = constant [~a x i8] c\"~a\\00\", align 8\n"
                          (symbol->llvm-val sname)
                          (+ strlen 1)
                          escaped-str))
      sname))
  (define (encode-int i)
    (modulo (bitwise-ior (arithmetic-shift i 32) 2) (expt 2 64)))
  ; make a vector with xs as the values and put it in x
  (define (T-vector v xs)
    (define len (length xs))
    (string-append (format "%~a = call i64 @~a(i64 ~a, i64 ~a)\n"
                           (symbol->llvm-val v)
                           (prim-name 'make-vector)
                           (encode-int len)
                           (encode-int 0))
                   (string-join (map (lambda (x i)
                                       (format "call i64 @~a(i64 %~a, i64 ~a, i64 %~a)\n"
                                               (prim-name 'vector-set!)
                                               (symbol->llvm-val v)
                                               (encode-int i)
                                               (symbol->llvm-val x)))
                                     xs
                                     (range len))
                                "")))

  (define (T-p proc)
    (match-define `(proc (,name ,args ...) ,body) proc)
     (format "define void @~a(~a) {\n~aret void\n}\n"
             (symbol->llvm-val name)
             (make-arglist args)
             (T-e body)))
  (define (T-e e)
    (match e
      [`(let ([,x ,ae]) ,e0)
       (string-append (T-ae x ae) (T-e e0))]
      [`(clo-app ,c ,xs ...)
       (define fptrsym (gensym 'fptr))
       (define psym (gensym 'proc))
       (define envsym (gensym 'env))
       (define proc-type (format "void (~a)*"
                                 (string-join (map (lambda (arg) "i64") (cons envsym xs)) ", ")))
       (string-append (format "; ~s\n" e)
                      (format "%~a = call i64* @get_clo_func(i64 %~a)\n"
                              (symbol->llvm-val fptrsym)
                              (symbol->llvm-val c))
                      (format "%~a = bitcast i64* %~a to ~a\n"
                              (symbol->llvm-val psym)
                              (symbol->llvm-val fptrsym)
                              proc-type)
                      (format "%~a = call i64 @get_clo_env(i64 %~a)\n"
                              (symbol->llvm-val envsym)
                              (symbol->llvm-val c))
                      (format "tail call fastcc void %~a(~a)\n"
                              (symbol->llvm-val psym)
                              (string-join (map (lambda (x) (string-append "i64 %"
                                                                           (symbol->llvm-val x)))
                                                (cons envsym xs))
                                           ", ")))]
      [`(if ,x ,e1 ,e2)
       (define cmpsym (gensym 'cmp))
       (define thensym (gensym 'then))
       (define elsesym (gensym 'else))
       (string-append (format "%~a = icmp ne i64 %~a, 15\n" ; 15 is encoded #f
                              (symbol->llvm-val cmpsym)
                              (symbol->llvm-val x))
                      (format "br i1 %~a, label %~a, label %~a\n\n"
                              (symbol->llvm-val cmpsym)
                              (symbol->llvm-val thensym)
                              (symbol->llvm-val elsesym))
                      (format "~a:\n~aret void\n\n"
                              (symbol->llvm-val thensym)
                              (T-e e1))
                      (format "~a:\n~a\n"
                              (symbol->llvm-val elsesym)
                              (T-e e2)))]))
  (define (T-ae x ae)
    (match ae
      [`(prim ,op ,xs ...)
       (format "%~a = call i64 @~a(~a)\n"
               (symbol->llvm-val x)
               (prim-name op)
               (make-arglist xs))]
      [`(apply-prim ,op ,x1)
       (format "%~a = call i64 @~a(~a)\n"
               (symbol->llvm-val x)
               (prim-applyname op)
               (make-arglist (list x1)))]
      [`(make-closure ,p ,xs ...)
       (define envsym (gensym 'env))
       (define fptrsym (gensym 'fptr))
       (string-append (format "; %~a = ~s\n" x ae)
                      (format "%~a = bitcast ~a @~a to i64*\n"
                              (symbol->llvm-val fptrsym)
                              (hash-ref proc-types p)
                              (symbol->llvm-val p))
                      (T-vector envsym xs)
                      (format "%~a = call i64 @make_clo(i64* %~a, i64 %~a)\n"
                              (symbol->llvm-val x)
                              (symbol->llvm-val fptrsym)
                              (symbol->llvm-val envsym)))]
      [`(env-ref ,x1 ,nat)
       (format "%~a = call i64 @~a(i64 %~a, i64 ~a)\n"
               (symbol->llvm-val x)
               (prim-name 'vector-ref)
               (symbol->llvm-val x1)
               (encode-int (- nat 1)))]
      [`(quote ,dat)
       (T-dat x dat)]))
  (define (T-dat x dat)
    (match dat
      [`#(,(? datum?) ...) "; vector constants not implemented\n"]
      [(cons d1 d2)
       (define x1 (gensym 'car))
       (define x2 (gensym 'cdr))
       (string-append (T-dat x1 d1)
                      (T-dat x2 d2)
                      (format "%~a = call i64 @prim_cons(~a)\n"
                              (symbol->llvm-val x)
                              (make-arglist (list x1 x2))))]
      [(? string? s)
       (T-str x s 'string)]
      [(? integer? i)
       (format "%~a = call i64 @const_init_int(i64 ~a)\n" (symbol->llvm-val x) i)]
      [(? symbol? s)
       (T-str x (symbol->string s) 'symbol)]
      ['()
       (format "%~a = call i64 @const_init_null()\n" (symbol->llvm-val x))]
      [#t
       (format "%~a = call i64 @const_init_true()\n" (symbol->llvm-val x))]
      [#f
       (format "%~a = call i64 @const_init_false()\n" (symbol->llvm-val x))]))
  (define (T-str x s str-type)
    (define sname 
      (if (and (eq? str-type 'symbol) (hash-has-key? interned-symbols s))
          (hash-ref interned-symbols s)
          (add-global-string s)))
    (define strlen+1 (+ (string-length s) 1))
    (begin
      (set! interned-symbols (hash-set interned-symbols s sname))
      (format "%~a = call i64 @const_init_~a(\
i8* getelementptr inbounds ([~a x i8], [~a x i8]* @~a, i32 0, i32 0))\n"
              (symbol->llvm-val x)
              (symbol->string str-type)
              strlen+1
              strlen+1
              (symbol->llvm-val sname))))

  (define globals '())
  (define interned-symbols (hash))
  (define proc-types (foldl (lambda (proc proc-types)
                              (match-define `(proc (,name ,args ...) ,body) proc)
                              (hash-set proc-types
                                        name
                                        (format "void (~a)*"
                                                (string-join (map (lambda (arg) "i64") args) ", "))))
                            (hash)
                            procs))
  (define procs-llvm (map T-p procs))
  (define globals-str (string-join globals ""))
  (string-join (cons llvm-header (cons globals-str procs-llvm)) "\n"))

(define (prim op . args) (apply op args))
(define apply-prim apply)
(define halt (void))
(let/ec ec (print (let/cc cc (ec (set! halt cc)))))
(require "cps.rkt")
(require "desugar.rkt")