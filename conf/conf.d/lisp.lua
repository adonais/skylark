lisp = {}

function lisp.get_keywords()
  local keywords0_set = "not defun + - * / = < > <= >= princ  eval apply funcall quote identity function complement backquote lambda set setq setf  defun defmacro gensym make symbol intern symbol name symbol value symbol plist get  getf putprop remprop hash make array aref car cdr caar cadr cdar cddr caaar caadr cadar  caddr cdaar cdadr cddar cdddr caaaar caaadr caadar caaddr cadaar cadadr caddar cadddr  cdaaar cdaadr cdadar cdaddr cddaar cddadr cdddar cddddr cons list append reverse last nth  nthcdr member assoc subst sublis nsubst  nsublis remove length list length  mapc mapcar mapl maplist mapcan mapcon rplaca rplacd nconc delete atom symbolp numberp  boundp null listp consp minusp zerop plusp evenp oddp eq eql equal cond case and or let l if prog  prog1 prog2 progn go return do dolist dotimes catch throw error cerror break  continue errset baktrace evalhook truncate float rem min max abs sin cos tan expt exp sqrt  random logand logior logxor lognot bignums logeqv lognand lognor  logorc2 logtest logbitp logcount integer length nil"
  return keywords0_set
end

function lisp.get_reqular()
  local symbol_reqular_exp = "[ \\t]*\\([ \\t]*defun[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9\\-]*)[ \\t]*\\("
  return symbol_reqular_exp
end

function lisp.create_bakup(path)
  local lisp_code = {
    "user_lisp = {}\n",
    "function user_lisp.get_keywords()\n",
    "  local keywords0_set = \"not defun + - * / = < > <= >= princ  eval apply funcall quote identity function complement backquote lambda set setq setf  defun defmacro gensym make symbol intern symbol name symbol value symbol plist get  getf putprop remprop hash make array aref car cdr caar cadr cdar cddr caaar caadr cadar  caddr cdaar cdadr cddar cdddr caaaar caaadr caadar caaddr cadaar cadadr caddar cadddr  cdaaar cdaadr cdadar cdaddr cddaar cddadr cdddar cddddr cons list append reverse last nth  nthcdr member assoc subst sublis nsubst  nsublis remove length list length  mapc mapcar mapl maplist mapcan mapcon rplaca rplacd nconc delete atom symbolp numberp  boundp null listp consp minusp zerop plusp evenp oddp eq eql equal cond case and or let l if prog  prog1 prog2 progn go return do dolist dotimes catch throw error cerror break  continue errset baktrace evalhook truncate float rem min max abs sin cos tan expt exp sqrt  random logand logior logxor lognot bignums logeqv lognand lognor  logorc2 logtest logbitp logcount integer length nil\"\n",
    "  return keywords0_set\n",
    "end\n",
    "\n",
    "function user_lisp.get_reqular()\n",
    "  local symbol_reqular_exp = \"[ \\\\t]*\\\\([ \\\\t]*defun[ \\\\t]+([_a-zA-Z]+[_a-zA-Z0-9\\\\-]*)[ \\\\t]*\\\\(\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "return user_lisp",
  }
  local shell_code = table.concat(lisp_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  lisp_code = nil
end

return lisp