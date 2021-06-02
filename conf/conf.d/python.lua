python = {}

function python.get_keywords()
    local keywords0_set = "False None True and as assert break class continue def del elif else except finally for from global if import in is lambda nonlocal not or pass raise return try while with yield"
    local keywords1_set = "abs delattr hash memoryview set all dict help min setattr any dir hex next slice ascii divmod id object sorted bin enumerate input oct staticmethod bool eval int open str breakpoint exec isinstance ord sum bytearray filter issubclass pow super bytes float iter print tuple callable format len property type chr frozenset list range vars classmethod getattr locals repr zip compile globals map reversed __import__ complex hasattr max round __main__ _dummy_thread _thread abc aifc argparse array ast asynchat asyncio asyncore atexit audioop base64 bdb binascii binhex bisect builtins bz2 calendar cgi cgitb chunk cmath cmd code codecs codeop collections colorsys compileall concurrent configparser contextlib copy copyreg crypt csv ctypes curses datetime dbm decimal difflib dis distutils dummy_threading email ensurepip enum errno faulthandler fcntl filecmp fileinput fnmatch formatter fpectl fractions ftplib functools gc getopt getpass gettext glob grp gzip hashlib heapq hmac html http http imaplib imghdr importlib inspect io ipaddress itertools json keyword linecache locale logging lzma macpath mailbox mailcap marshal math mimetypes mmap modulefinder msilib msvcrt multiprocessing netrc nis nntplib numbers operator os os ossaudiodev parser pathlib pdb pickle pickletools pipes pkgutil platform plistlib poplib posix pprint pty pwd py_compile pyclbr queue quopri random re readline reprlib resource rlcompleter runpy sched select selectors shelve shlex shutil signal site smtpd smtplib sndhdr socket socketserver spwd sqlite3 ssl stat statistics string stringprep struct subprocess sunau symbol symtable sys sysconfig syslog tabnanny tarfile telnetlib tempfile termios textwrap threading time timeit tkinter token tokenize trace traceback tracemalloc tty turtle types unicodedata unittest urllib uu uuid venv warnings wave weakref webbrowser winreg winsound wsgiref xdrlib xml xmlrpc zipfile zipimport zlib"
	return keywords0_set,keywords1_set
end	

function python.get_autocomplete()
    local autocomplete_set = "abs delattr hash memoryview set all dict help min setattr any dir hex next slice ascii divmod id object sorted bin enumerate input oct staticmethod bool eval int open str breakpoint exec isinstance ord sum bytearray filter issubclass pow super bytes float iter print tuple callable format len property type chr frozenset list range vars classmethod getattr locals repr zip compile globals map reversed __import__ complex hasattr max round __main__ _dummy_thread _thread abc aifc argparse array ast asynchat asyncio asyncore atexit audioop base64 bdb binascii binhex bisect builtins bz2 calendar cgi cgitb chunk cmath cmd code codecs codeop collections colorsys compileall concurrent configparser contextlib copy copyreg crypt csv ctypes curses datetime dbm decimal difflib dis distutils dummy_threading email ensurepip enum errno faulthandler fcntl filecmp fileinput fnmatch formatter fpectl fractions ftplib functools gc getopt getpass gettext glob grp gzip hashlib heapq hmac html http http imaplib imghdr importlib inspect io ipaddress itertools json keyword linecache locale logging lzma macpath mailbox mailcap marshal math mimetypes mmap modulefinder msilib msvcrt multiprocessing netrc nis nntplib numbers operator os os ossaudiodev parser pathlib pdb pickle pickletools pipes pkgutil platform plistlib poplib posix pprint pty pwd py_compile pyclbr queue quopri random re readline reprlib resource rlcompleter runpy sched select selectors shelve shlex shutil signal site smtpd smtplib sndhdr socket socketserver spwd sqlite3 ssl stat statistics string stringprep struct subprocess sunau symbol symtable sys sysconfig syslog tabnanny tarfile telnetlib tempfile termios textwrap threading time timeit tkinter token tokenize trace traceback tracemalloc tty turtle types unicodedata unittest urllib uu uuid venv warnings wave weakref webbrowser winreg winsound wsgiref xdrlib xml xmlrpc zipfile zipimport zlib"
	return autocomplete_set
end	

function python.get_calltip()
    local calltip_add = 
    {
        "abs|abs(x)",
        "all|all(iterable)",
        "any|any(iterable)",
        "ascii|ascii(object)",
        "bin|bin(x)",
        "class|class bool([x])\\nbytearray([source[, encoding[, errors]]])\\nbytes([source[, encoding[, errors]]])\\ncomplex([real[, imag]])\\ndict(**kwarg)\\ndict(mapping, **kwarg)\\ndict(iterable, **kwarg)\\nfloat([x])\\nfrozenset([iterable])\\nint([x])\\nint(x, base=10)\\nlist([iterable])\\nmemoryview(obj)\\nobject\\nproperty(fget=None, fset=None, fdel=None, doc=None)\\nrange(stop)\\nrange(start, stop[, step])\\nset([iterable])\\nslice(stop)\\nslice(start, stop[, step])\\nstr(object='')\\nstr(object=b'', encoding='utf-8', errors='strict')\\ntuple([iterable])\\ntype(object)\\ntype(name, bases, dict)",
        "breakpoint|breakpoint(*args, **kws)",
        "callable|callable(object)",
        "chr|chr(i)",
        "compile|compile(source, filename, mode, flags=0, dont_inherit=False, optimize=-1)",
        "delattr|delattr(object, name)",
        "dir|dir([object])",
        "divmod|divmod(a, b)",
        "enumerate|enumerate(iterable, start=0)",
        "eval|eval(expression[, globals[, locals]])",
        "exec|exec(object[, globals[, locals]])",
        "filter|filter(function, iterable)",
        "format|format(value[, format_spec])",
        "getattr|getattr(object, name[, default])",
        "globals|globals()",
        "hasattr|hasattr(object, name)",
        "hash|hash(object)",
        "help|help([object])",
        "hex|hex(x)",
        "id|id(object)",
        "input|input([prompt])",
        "isinstance|isinstance(object, classinfo)",
        "issubclass|issubclass(class, classinfo)",
        "iter|iter(object[, sentinel])",
        "len|len(s)",
        "locals|locals()",
        "map|map(function, iterable, ...)",
        "max|max(arg1, arg2, *args[, key])\\nmax(iterable, *[, key, default])",
        "min| min(arg1, arg2, *args[, key])\\nmin(iterable, *[, key, default])",
        "next|next(iterator[, default])",
        "oct|oct(x)",
        "open|open(file, mode='r', buffering=-1, encoding=None, errors=None, newline=None, closefd=True, opener=None)",
        "ord|ord(c)",
        "pow|pow(base, exp[, mod])",
        "print|print(*objects, sep=' ', end='\\n', file=sys.stdout, flush=False)",
        "repr|repr(object)",
        "reversed|reversed(seq)",
        "round|round(number[, ndigits])",
        "setattr|setattr(object, name, value)",
        "sorted|sorted(iterable, *, key=None, reverse=False)",
        "sum|sum(iterable, /, start=0)",
        "super|super([type[, object-or-type]])",
        "vars|vars([object])",
        "zip|zip(*iterables)",
        "__import__|__import__(name, globals=None, locals=None, fromlist=(), level=0)"
    }
    return calltip_add
end

function python.get_reqular()
    local symbol_reqular_exp = "[ \\t]*def[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9]*)\\("
    return symbol_reqular_exp
end

return python
