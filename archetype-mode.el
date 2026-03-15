;;; archetype-mode.el --- Major mode for the Archetype adventure game language  -*- lexical-binding: t; -*-

;;; Commentary:

;; Major mode for editing Archetype source files.  Archetype is a
;; message-passing, object-oriented language for writing text-based
;; adventure games.
;;
;; File extension: .ach
;;
;; See https://www.derektjones.net/archetype/archetype.html

;;; Code:

;;;; Customization --------------------------------------------------------

(defgroup archetype nil
  "Major mode for the Archetype adventure game language."
  :group 'languages
  :prefix "archetype-")

;;;; Faces ----------------------------------------------------------------

(defface archetype-message-face
  '((default :inherit font-lock-builtin-face))
  "Face for Archetype message literals (\\='single-quoted\\=')."
  :group 'archetype)

(defface archetype-heredoc-face
  '((default :inherit font-lock-string-face :slant italic))
  "Face for Archetype verbatim output lines (>> ...)."
  :group 'archetype)

;;;; Syntax table ---------------------------------------------------------

(defvar archetype-mode-syntax-table
  (let ((st (make-syntax-table)))
    ;; # begins a line comment
    (modify-syntax-entry ?#  "<" st)
    (modify-syntax-entry ?\n ">" st)
    ;; Double-quoted strings
    (modify-syntax-entry ?\" "\"" st)
    ;; Single quotes are punctuation by default.  syntax-propertize
    ;; selectively promotes matched 'message literal' pairs to generic
    ;; string delimiters — but only outside comments and heredoc lines,
    ;; so that apostrophes in "# it's the 'INITIAL' method" or
    ;; ">>I don't think so." stay inert.
    (modify-syntax-entry ?'  "."  st)
    ;; Underscore is a word constituent (identifiers like start_room)
    (modify-syntax-entry ?_  "w"  st)
    ;; Punctuation / operators
    (modify-syntax-entry ?&  "." st)
    (modify-syntax-entry ?@  "." st)
    (modify-syntax-entry ?~  "." st)
    (modify-syntax-entry ?-  "." st)
    (modify-syntax-entry ?+  "." st)
    (modify-syntax-entry ?>  "." st)
    st)
  "Syntax table for `archetype-mode'.")

(defun archetype--syntax-propertize (start end)
  "Apply syntax properties to the region between START and END.
Marks the opening and closing single quotes of message literals as
generic-string delimiters (syntax class \"||\"), but only when:
  - the quotes are paired on the same line, and
  - they are not inside a comment or a >> heredoc line.
All other single quotes (apostrophes, stray ticks) remain punctuation."
  (goto-char start)
  (while (re-search-forward "'[^'\n]*'" end t)
    (let ((ppss (save-excursion (syntax-ppss (match-beginning 0)))))
      ;; nth 4 = inside a comment; nth 3 = inside a string
      (when (and (not (nth 4 ppss))
                 (not (nth 3 ppss))
                 ;; Not on a >> heredoc line
                 (not (save-excursion
                        (goto-char (match-beginning 0))
                        (beginning-of-line)
                        (looking-at "[ \t]*>>"))))
        (put-text-property (match-beginning 0) (1+ (match-beginning 0))
                           'syntax-table '(15 . nil))  ; generic string open
        (put-text-property (1- (match-end 0)) (match-end 0)
                           'syntax-table '(15 . nil))))))  ; generic string close

;;;; Font-lock ------------------------------------------------------------

;; Archetype's declaration keywords — these open a top-level block.
;; Note: "object", "room", "lex", "Verb", "direction" are standard-library
;; type names, but they appear so frequently as declaration heads that they
;; are highlighted the same as the true keywords ("class", "null", etc.).
(defconst archetype--decl-keywords
  '("class" "based" "on" "object" "room" "null"
    "lex" "Verb" "direction" "keyword" "include"))

;; Block-structure keywords
(defconst archetype--block-keywords
  '("methods" "end"))

;; Control-flow and action keywords
(defconst archetype--stmt-keywords
  '("if" "then" "else" "while" "do" "for" "each"
    "case" "of" "default" "write" "writes" "stop"
    "create" "named"))

;; Special value constants
(defconst archetype--constants
  '("TRUE" "FALSE" "UNDEFINED" "ABSENT"))

;; Built-in variables and string/list operators
(defconst archetype--builtins
  '("self" "sender" "message" "main" "system" "player"
    "before" "after" "compass" "everything"
    "head" "tail" "length" "within" "leftfrom" "rightfrom"
    "not" "and" "or" "read" "reads"
    "string" "numeric" "character"))

(defvar archetype-font-lock-keywords
  (let* ((kw   (regexp-opt (append archetype--decl-keywords
                                   archetype--block-keywords)
                           'words))
         (stmt (regexp-opt archetype--stmt-keywords  'words))
         (val  (regexp-opt archetype--constants      'words))
         (blt  (regexp-opt archetype--builtins       'words))
         ;; Operator face: use font-lock-operator-face when available (Emacs 29+),
         ;; fall back to font-lock-builtin-face.
         (op-face (if (facep 'font-lock-operator-face)
                      ''font-lock-operator-face
                    ''font-lock-builtin-face)))
    `(;; Heredoc verbatim lines:  >>text to end of line.
      ;; These are "raw" output statements; color the entire suffix.
      (">>.*$" 0 'archetype-heredoc-face t)
      ;; Message literals in 'single quotes'.
      ;; syntax-propertize marks these as generic strings; we override
      ;; the resulting string face so they look distinct from "double-quoted" strings.
      ("'[^'\n]*'" 0 'archetype-message-face prepend)
      ;; Declaration and block structure keywords
      (,kw   . font-lock-keyword-face)
      ;; Control-flow and action keywords
      (,stmt . font-lock-keyword-face)
      ;; Constants
      (,val  . font-lock-constant-face)
      ;; Built-in identifiers
      (,blt  . font-lock-builtin-face)
      ;; Message-send operators: --> (broadcast to class) and -> (send to object)
      ("-->?" 0 ,op-face)
      ;; Assignment operators: := and the compound forms +:= -:= &:=
      ("[+\\-&]?:=" 0 ,op-face)))
  "Font-lock specification for `archetype-mode'.")

;;;; Indentation ----------------------------------------------------------

;; No automatic indentation is provided.  Archetype's grammar — with its
;; interleaved attribute/method sections, one-liner declarations, and
;; braceless control flow — is difficult to indent reliably without a full
;; parser.  Instead we use `indent-relative', which lets TAB align the
;; cursor with the previous line's whitespace-delimited columns.  This
;; gives the programmer full manual control while still being convenient
;; for lining up attribute blocks.

;;;; Mode definition ------------------------------------------------------

;;;###autoload
(define-derived-mode archetype-mode prog-mode "Archetype"
  "Major mode for editing Archetype adventure game source files.

Archetype is a message-passing, object-oriented language for writing
text-based adventure games.  Source files use the .ach extension.

Syntax overview:
  # comment to end of line
  class NAME based on PARENT  ... methods  'MSG' : stmt  end
  object NAME  attr : value  methods  'MSG' : stmt  end
  'MESSAGE' -> object          # send a message, get return value
  'MESSAGE' --> class          # send to nearest ancestor class
  expr := value                # assignment
  >>verbatim text here         # print exactly as written

\\{archetype-mode-map}"
  :syntax-table archetype-mode-syntax-table
  (setq-local comment-start "# ")
  (setq-local comment-end "")
  (setq-local comment-start-skip "#+ *")
  (setq-local font-lock-defaults
              '(archetype-font-lock-keywords
                nil   ; do perform syntactic fontification (strings, comments)
                nil   ; case-sensitive
                nil)) ; no special syntax-alist
  (setq-local syntax-propertize-function #'archetype--syntax-propertize)
  (setq-local indent-line-function #'indent-relative)
  (setq-local indent-tabs-mode nil))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.ach\\'" . archetype-mode))

(provide 'archetype-mode)

;;; archetype-mode.el ends here
