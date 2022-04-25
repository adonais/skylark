matlab = {}

require("eu_sci")
require("eu_core")

function matlab.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  eu_core.euapi.on_doc_enable_foldline(pnode)                         -- enable fold line
  return eu_core.euapi.on_doc_init_after_scilexer(pnode, "matlab")
end

function matlab.get_comments()
  -- 行注释与块注释, 注释头与注释尾用&&分开
  -- matlab的块注释需要以换行符开始
  local line_t = "% "
  local block_t = "\n%{&&\n%}"
  return line_t, block_t
end

function matlab.get_styles()
  local style_t = {
    [SCE_MATLAB_COMMENT] = 0xC0C0C0,
    -- 给关键字加上粗体
    [SCE_MATLAB_KEYWORD] = 0x00B050 + SCE_BOLD_FONT,
    [SCE_MATLAB_COMMAND] = 0x00B050,
    [SCE_MATLAB_OPERATOR] = 0,
    [SCE_MATLAB_IDENTIFIER] = 0,
    [SCE_MATLAB_STRING] = 0xC080FF,
    [SCE_MATLAB_DOUBLEQUOTESTRING] = 0xC080FF,
  }
  return style_t
end

function matlab.get_keywords()
  local keywords0_set = "break case catch classdef continue else elseif end for function global if otherwise parfor persistent return spmd switch try while double single char logical int8 uint8 int16 uin16 int32 uint32 int64 uint64 cell struct function_handle methods properties events enumeration public protected private mutable immutable internal import all and any false not or true xor bitand bitor bitxor sym syms complex real vpa scalar class typecast cast handle throw rethrow typeinfo shared inline do until endfor endfunction endif endswitch end_try_catch end_unwind_protect endwhile unwind_protect unwind_protect_cleanup select then off on eps pi ans Inf inf NaN nan null nargin nargout varargin varargout gca gcf gco gcbf gcbo intmax intmin realmax realmin AbortSet Abstract Access AllowedSubclasses Constant ConstructOnLoad Dependent GetAccess GetObservable HandleCompatible Hidden InferiorClasses ListenAccess NonCopyable NotifyAccess Sealed SetAccess SetObservable Static Transient DiscreteState Logical Nontunable PositiveInteger clc clear delete demo exit load open pause quit save test testif tic toc which who whos axis box cla clf close grid hold pan shg zoom axes figure uicontrol uimenu"
  local keywords1_set = "assert deal disp display error eval evalc evalin fail feval find get input length warning message set size find full sparse isa iscell iscellstr ischar iscolumn iscom isdir isempty isequal isfield isfloat isfinite isglobal ishandle ishold isindex isinf isinterface isinteger isjava islogical ismatrix ismethod ismember isnan isnumeric isobject isprop isreal isrow isscalar issparse issquare isstr isstruct isvalid isvector"
  return keywords0_set,keywords1_set
end

function matlab.get_autocomplete()
  local autocomplete_set = "break case catch classdef continue else elseif end for function global if otherwise parfor persistent return spmd switch try while double single char logical int8 uint8 int16 uin16 int32 uint32 int64 uint64 cell struct function_handle methods properties events enumeration public protected private mutable immutable internal import all and any false not or true xor bitand bitor bitxor sym syms complex real vpa scalar class typecast cast handle throw rethrow typeinfo shared inline do until endfor endfunction endif endswitch end_try_catch end_unwind_protect endwhile unwind_protect unwind_protect_cleanup select then off on eps pi ans Inf inf NaN nan null nargin nargout varargin varargout gca gcf gco gcbf gcbo intmax intmin realmax realmin AbortSet Abstract Access AllowedSubclasses Constant ConstructOnLoad Dependent GetAccess GetObservable HandleCompatible Hidden InferiorClasses ListenAccess NonCopyable NotifyAccess Sealed SetAccess SetObservable Static Transient DiscreteState Logical Nontunable PositiveInteger clc clear delete demo exit load open pause quit save test testif tic toc which who whos axis box cla clf close grid hold pan shg zoom axes figure uicontrol uimenu assert deal disp display error eval evalc evalin fail feval find get input length warning message set size find full sparse isa iscell iscellstr ischar iscolumn iscom isdir isempty isequal isfield isfloat isfinite isglobal ishandle ishold isindex isinf isinterface isinteger isjava islogical ismatrix ismethod ismember isnan isnumeric isobject isprop isreal isrow isscalar issparse issquare isstr isstruct isvalid isvector"
  return autocomplete_set
end

function matlab.get_calltip()
  local calltip_add =
  {
    "assert|assert(cond,errID,msg,A1,...,An);",
    "airy|airy(k,Z,scale);",
    "besselh|besselh(nu,K,Z,scale);",
    "beta|beta(Z,W);",
    "erf|erf(x);",
    "gamma|gamma(X);"
  }
  return calltip_add
end

function matlab.create_bakup(path)
  local matlab_code = {
    "user_matlab = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_matlab.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  eu_core.euapi.on_doc_enable_foldline(pnode)                         -- enable fold line\n",
    "  return eu_core.euapi.on_doc_init_after_scilexer(pnode, \"matlab\")\n",
    "end\n",
    "\n",
    "function user_matlab.get_comments()\n",
    "  -- 行注释与块注释, 注释头与注释尾用&&分开\n",
    "  -- matlab的块注释需要以换行符开始\n",
    "  local line_t = \"% \"\n",
    "  local block_t = \"\\n%{&&\\n%}\"\n",
    "  return line_t, block_t\n",
    "end\n",
    "\n",
    "function user_matlab.get_styles()\n",
    "  local style_t = {\n",
    "    [SCE_MATLAB_COMMENT] = 0xC0C0C0,\n",
    "    -- 给关键字加上粗体\n",
    "    [SCE_MATLAB_KEYWORD] = 0x00B050 + SCE_BOLD_FONT,\n",
    "    [SCE_MATLAB_COMMAND] = 0x00B050,\n",
    "    [SCE_MATLAB_OPERATOR] = 0,\n",
    "    [SCE_MATLAB_IDENTIFIER] = 0,\n",
    "    [SCE_MATLAB_STRING] = 0xC080FF,\n",
    "    [SCE_MATLAB_DOUBLEQUOTESTRING] = 0xC080FF,\n",
    "  }\n",
    "  return style_t\n",
    "end\n",
    "\n",
    "function user_matlab.get_keywords()\n",
    "  local keywords0_set = \"break case catch classdef continue else elseif end for function global if otherwise parfor persistent return spmd switch try while double single char logical int8 uint8 int16 uin16 int32 uint32 int64 uint64 cell struct function_handle methods properties events enumeration public protected private mutable immutable internal import all and any false not or true xor bitand bitor bitxor sym syms complex real vpa scalar class typecast cast handle throw rethrow typeinfo shared inline do until endfor endfunction endif endswitch end_try_catch end_unwind_protect endwhile unwind_protect unwind_protect_cleanup select then off on eps pi ans Inf inf NaN nan null nargin nargout varargin varargout gca gcf gco gcbf gcbo intmax intmin realmax realmin AbortSet Abstract Access AllowedSubclasses Constant ConstructOnLoad Dependent GetAccess GetObservable HandleCompatible Hidden InferiorClasses ListenAccess NonCopyable NotifyAccess Sealed SetAccess SetObservable Static Transient DiscreteState Logical Nontunable PositiveInteger clc clear delete demo exit load open pause quit save test testif tic toc which who whos axis box cla clf close grid hold pan shg zoom axes figure uicontrol uimenu\"\n",
    "  local keywords1_set = \"assert deal disp display error eval evalc evalin fail feval find get input length warning message set size find full sparse isa iscell iscellstr ischar iscolumn iscom isdir isempty isequal isfield isfloat isfinite isglobal ishandle ishold isindex isinf isinterface isinteger isjava islogical ismatrix ismethod ismember isnan isnumeric isobject isprop isreal isrow isscalar issparse issquare isstr isstruct isvalid isvector\"\n",
    "  return keywords0_set,keywords1_set\n",
    "end\n",
    "\n",
    "function user_matlab.get_autocomplete()\n",
    "  local autocomplete_set = \"break case catch classdef continue else elseif end for function global if otherwise parfor persistent return spmd switch try while double single char logical int8 uint8 int16 uin16 int32 uint32 int64 uint64 cell struct function_handle methods properties events enumeration public protected private mutable immutable internal import all and any false not or true xor bitand bitor bitxor sym syms complex real vpa scalar class typecast cast handle throw rethrow typeinfo shared inline do until endfor endfunction endif endswitch end_try_catch end_unwind_protect endwhile unwind_protect unwind_protect_cleanup select then off on eps pi ans Inf inf NaN nan null nargin nargout varargin varargout gca gcf gco gcbf gcbo intmax intmin realmax realmin AbortSet Abstract Access AllowedSubclasses Constant ConstructOnLoad Dependent GetAccess GetObservable HandleCompatible Hidden InferiorClasses ListenAccess NonCopyable NotifyAccess Sealed SetAccess SetObservable Static Transient DiscreteState Logical Nontunable PositiveInteger clc clear delete demo exit load open pause quit save test testif tic toc which who whos axis box cla clf close grid hold pan shg zoom axes figure uicontrol uimenu assert deal disp display error eval evalc evalin fail feval find get input length warning message set size find full sparse isa iscell iscellstr ischar iscolumn iscom isdir isempty isequal isfield isfloat isfinite isglobal ishandle ishold isindex isinf isinterface isinteger isjava islogical ismatrix ismethod ismember isnan isnumeric isobject isprop isreal isrow isscalar issparse issquare isstr isstruct isvalid isvector\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_matlab.get_calltip()\n",
    "  local calltip_add =\n",
    "  {\n",
    "    \"assert|assert(cond,errID,msg,A1,...,An);\",\n",
    "    \"airy|airy(k,Z,scale);\",\n",
    "    \"besselh|besselh(nu,K,Z,scale);\",\n",
    "    \"beta|beta(Z,W);\",\n",
    "    \"erf|erf(x);\",\n",
    "    \"gamma|gamma(X);\"\n",
    "  }\n",
    "  return calltip_add\n",
    "end\n",
    "return user_matlab",
  }
  local shell_code = table.concat(matlab_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  matlab_code = nil
end

return matlab