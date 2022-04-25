fortran = {}

require("eu_sci")
require("eu_core")

function fortran.init_after_callback(p)
  local pnode = eu_core.ffi.cast("void *", p)
  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, "fortran")
  if (res ~= 1) then
    eu_core.euapi.on_doc_enable_foldline(pnode)                                              -- enable fold line
    eu_core.euapi.on_doc_keyword_light(pnode, SCE_F_WORD, 0, 0)                              -- 8, SCE_F_WORD, keywords0
    eu_core.euapi.on_doc_keyword_light(pnode, SCE_F_WORD2, 1, 0)                             -- 9, SCE_F_WORD2, keywords1
    eu_core.euapi.on_doc_keyword_light(pnode, SCE_F_WORD3, 2, 0xB000B0)                      -- 10, SCE_F_WORD3, keywords2
    eu_core.euapi.on_doc_commentblock_light(pnode, SCE_F_COMMENT, 0)
    eu_core.euapi.on_doc_number_light(pnode, SCE_F_NUMBER, 0)
    eu_core.euapi.on_doc_preprocessor_light(pnode, SCE_F_PREPROCESSOR, -1, 0xB000B0)
  end
  return res;
end

function fortran.get_keywords()
  local keywords0_set = "access action advance allocatable allocate apostrophe assign assignment associate asynchronous backspace bind blank blockdata call case character class close common complex contains continue critical cycle data deallocate decimal delim default dimension direct do dowhile double doubleprecision else elseif elsewhere encoding end endassociate endblockdata endcritical enddo endenum endfile endforall endfunction endif endinterface endmodule endprocedure endprogram endselect endsubmodule endsubroutine endtype endwhere entry enum eor equivalence err errmsg exist exit external file flush fmt forall form format formatted function go goto id if implicit in include inout integer inquire intent interface intrinsic iomsg iolength iostat kind len logical module name named namelist nextrec nml none nullify number only open opened operator optional out pad parameter pass pause pending pointer pos position precision print private procedure program protected public quote read readwrite real rec recl recursive result return rewind save select selectcase selecttype sequential sign size stat status stop stream submodule subroutine target then to type unformatted unit use value volatile wait where while write"
  local keywords1_set = "abs achar acos acosd adjustl adjustr aimag aimax0 aimin0 aint ajmax0 ajmin0 akmax0 akmin0 all allocated alog alog10 amax0 amax1 amin0 amin1 amod anint any asin asind associated atan atan2 atan2d atand bitest bitl bitlr bitrl bjtest bit_size bktest break btest cabs ccos cdabs cdcos cdexp cdlog cdsin cdsqrt ceiling cexp char clog cmplx conjg cos cosd cosh count cpu_time cshift csin csqrt dabs dacos dacosd dasin dasind datan datan2 datan2d datand date date_and_time dble dcmplx dconjg dcos dcosd dcosh dcotan ddim dexp dfloat dflotk dfloti dflotj digits dim dimag dint dlog dlog10 dmax1 dmin1 dmod dnint dot_product dprod dreal dsign dsin dsind dsinh dsqrt dtan dtand dtanh eoshift epsilon errsns exp exponent float floati floatj floatk floor fraction free huge iabs iachar iand ibclr ibits ibset ichar idate idim idint idnint ieor ifix iiabs iiand iibclr iibits iibset iidim iidint iidnnt iieor iifix iint iior iiqint iiqnnt iishft iishftc iisign ilen imax0 imax1 imin0 imin1 imod index inint inot int int1 int2 int4 int8 iqint iqnint ior ishft ishftc isign isnan izext jiand jibclr jibits jibset jidim jidint jidnnt jieor jifix jint jior jiqint jiqnnt jishft jishftc jisign jmax0 jmax1 jmin0 jmin1 jmod jnint jnot jzext kiabs kiand kibclr kibits kibset kidim kidint kidnnt kieor kifix kind kint kior kishft kishftc kisign kmax0 kmax1 kmin0 kmin1 kmod knint knot kzext lbound leadz len len_trim lenlge lge lgt lle llt log log10 logical lshift malloc matmul max max0 max1 maxexponent maxloc maxval merge min min0 min1 minexponent minloc minval mod modulo mvbits nearest nint not nworkers number_of_processors pack popcnt poppar precision present product radix random random_number random_seed range real repeat reshape rrspacing rshift scale scan secnds selected_int_kind selected_real_kind set_exponent shape sign sin sind sinh size sizeof sngl snglq spacing spread sqrt sum system_clock tan tand tanh tiny transfer transpose trim ubound unpack verify"
  local keywords2_set = "cotan cotand dcmplx dconjg dcotan dcotand decode dimag dll_export dll_import doublecomplex dreal dvchk encode find flen flush getarg getcharqq getcl getdat getenv gettim hfix ibchng identifier imag int1 int2 int4 intc intrup invalop iostat_msg isha ishc ishl jfix lacfar locking locnear map nargs nbreak ndperr ndpexc offset ovefl peekcharqq precfill prompt qabs qacos qacosd qasin qasind qatan qatand qatan2 qcmplx qconjg qcos qcosd qcosh qdim qexp qext qextd qfloat qimag qlog qlog10 qmax1 qmin1 qmod qreal qsign qsin qsind qsinh qsqrt qtan qtand qtanh ran rand randu rewrite segment setdat settim system timer undfl unlock union val virtual volatile zabs zcos zexp zlog zsin zsqrt"
  return keywords0_set,keywords1_set,keywords2_set
end

function fortran.get_autocomplete()
  local autocomplete_set = "access action advance allocatable allocate apostrophe assign assignment associate asynchronous backspace bind blank blockdata call case character class close common complex contains continue critical cycle data deallocate decimal delim default dimension direct do dowhile double doubleprecision else elseif elsewhere encoding end endassociate endblockdata endcritical enddo endenum endfile endforall endfunction endif endinterface endmodule endprocedure endprogram endselect endsubmodule endsubroutine endtype endwhere entry enum eor equivalence err errmsg exist exit external file flush fmt forall form format formatted function go goto id if implicit in include inout integer inquire intent interface intrinsic iomsg iolength iostat kind len logical module name named namelist nextrec nml none nullify number only open opened operator optional out pad parameter pass pause pending pointer pos position precision print private procedure program protected public quote read readwrite real rec recl recursive result return rewind save select selectcase selecttype sequential sign size stat status stop stream submodule subroutine target then to type unformatted unit use value volatile wait where while write abs achar acos acosd adjustl adjustr aimag aimax0 aimin0 aint ajmax0 ajmin0 akmax0 akmin0 all allocated alog alog10 amax0 amax1 amin0 amin1 amod anint any asin asind associated atan atan2 atan2d atand bitest bitl bitlr bitrl bjtest bit_size bktest break btest cabs ccos cdabs cdcos cdexp cdlog cdsin cdsqrt ceiling cexp char clog cmplx conjg cos cosd cosh count cpu_time cshift csin csqrt dabs dacos dacosd dasin dasind datan datan2 datan2d datand date date_and_time dble dcmplx dconjg dcos dcosd dcosh dcotan ddim dexp dfloat dflotk dfloti dflotj digits dim dimag dint dlog dlog10 dmax1 dmin1 dmod dnint dot_product dprod dreal dsign dsin dsind dsinh dsqrt dtan dtand dtanh eoshift epsilon errsns exp exponent float floati floatj floatk floor fraction free huge iabs iachar iand ibclr ibits ibset ichar idate idim idint idnint ieor ifix iiabs iiand iibclr iibits iibset iidim iidint iidnnt iieor iifix iint iior iiqint iiqnnt iishft iishftc iisign ilen imax0 imax1 imin0 imin1 imod index inint inot int int1 int2 int4 int8 iqint iqnint ior ishft ishftc isign isnan izext jiand jibclr jibits jibset jidim jidint jidnnt jieor jifix jint jior jiqint jiqnnt jishft jishftc jisign jmax0 jmax1 jmin0 jmin1 jmod jnint jnot jzext kiabs kiand kibclr kibits kibset kidim kidint kidnnt kieor kifix kind kint kior kishft kishftc kisign kmax0 kmax1 kmin0 kmin1 kmod knint knot kzext lbound leadz len len_trim lenlge lge lgt lle llt log log10 logical lshift malloc matmul max max0 max1 maxexponent maxloc maxval merge min min0 min1 minexponent minloc minval mod modulo mvbits nearest nint not nworkers number_of_processors pack popcnt poppar precision present product radix random random_number random_seed range real repeat reshape rrspacing rshift scale scan secnds selected_int_kind selected_real_kind set_exponent shape sign sin sind sinh size sizeof sngl snglq spacing spread sqrt sum system_clock tan tand tanh tiny transfer transpose trim ubound unpack verify cotan cotand dcmplx dconjg dcotan dcotand decode dimag dll_export dll_import doublecomplex dreal dvchk encode find flen flush getarg getcharqq getcl getdat getenv gettim hfix ibchng identifier imag int1 int2 int4 intc intrup invalop iostat_msg isha ishc ishl jfix lacfar locking locnear map nargs nbreak ndperr ndpexc offset ovefl peekcharqq precfill prompt qabs qacos qacosd qasin qasind qatan qatand qatan2 qcmplx qconjg qcos qcosd qcosh qdim qexp qext qextd qfloat qimag qlog qlog10 qmax1 qmin1 qmod qreal qsign qsin qsind qsinh qsqrt qtan qtand qtanh ran rand randu rewrite segment setdat settim system timer undfl unlock union val virtual volatile zabs zcos zexp zlog zsin zsqrt"
  return autocomplete_set
end

function fortran.get_reqular()
  local symbol_reqular_exp = "(?:^program|^module|^function|^subroutine)[ \\t]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\s*(?:|\\(.*\\)).*$"
  return symbol_reqular_exp
end

function fortran.get_calltip()
  local calltip_add =
  {
    "associated|ASSOCIATED(p[,t]);",
    "eoshift|EOSHIFT(ARRAY, SHIFT [, BOUNDARY, DIM]);",
    "erfc_scaled|ERFC_SCALED(X);",
    "extends_type_of|EXTENDS_TYPE_OF(A, MOLD);",
    "fnum|FNUM(UNIT);",
    "ierrno|IERRNO();",
    "ishftc|ISHFTC(I, SHIFT [, SIZE]);",
    "is_contiguous|IS_CONTIGUOUS(ARRAY);",
    "mvbits|CALL MVBITS(FROM, FROMPOS, LEN, TO, TOPOS);",
    "move_alloc|CALL MOVE_ALLOC(FROM, TO);",
    "selected_char_kind|SELECTED_CHAR_KIND(NAME);",
    "size|SIZE(ARRAY[, DIM [, KIND]]);",
    "rand|RAND(I);",
    "random_init|CALL RANDOM_INIT(REPEATABLE, IMAGE_DISTINCT);",
    "random_number|RANDOM_NUMBER(HARVEST);",
    "random_seed|CALL RANDOM_SEED([SIZE, PUT, GET]);",
    "selected_int_kind|SELECTED_INT_KIND(R);",
    "selected_real_kind|SELECTED_REAL_KIND([P, R, RADIX]);",
    "acos|ACOS(x);",
    "acosd|ACOSD(x);",
    "asin|ASIN(x);",
    "atan|ATAN(x);",
    "atand|ATAND(x);",
    "atan2|ATAN2(y,x);",
    "atan2d|ATAN2D(y,x);",
    "cos|COS(x);",
    "cosd|COSD(x);",
    "acosh|COSH(x);",
    "cotan|COTAN(x)",
    "sin|SIN(x);"
  }
  return calltip_add
end

function fortran.create_bakup(path)
  local fortran_code = {
    "user_fortran = {}\n",
    "\n",
    "require(\"eu_sci\")\n",
    "require(\"eu_core\")\n",
    "\n",
    "function user_fortran.init_after_callback(p)\n",
    "  local pnode = eu_core.ffi.cast(\"void *\", p)\n",
    "  local res = eu_core.euapi.on_doc_init_after_scilexer(pnode, \"fortran\")\n",
    "  if (res ~= 1) then\n",
    "    eu_core.euapi.on_doc_enable_foldline(pnode)                                              -- enable fold line\n",
    "    eu_core.euapi.on_doc_keyword_light(pnode, SCE_F_WORD, 0, 0)                              -- 8, SCE_F_WORD, keywords0\n",
    "    eu_core.euapi.on_doc_keyword_light(pnode, SCE_F_WORD2, 1, 0)                             -- 9, SCE_F_WORD2, keywords1\n",
    "    eu_core.euapi.on_doc_keyword_light(pnode, SCE_F_WORD3, 2, 0xB000B0)                      -- 10, SCE_F_WORD3, keywords2\n",
    "    eu_core.euapi.on_doc_commentblock_light(pnode, SCE_F_COMMENT, 0)\n",
    "    eu_core.euapi.on_doc_number_light(pnode, SCE_F_NUMBER, 0)\n",
    "    eu_core.euapi.on_doc_preprocessor_light(pnode, SCE_F_PREPROCESSOR, -1, 0xB000B0)\n",
    "  end\n",
    "  return res;\n",
    "end\n",
    "\n",
    "function user_fortran.get_keywords()\n",
    "  local keywords0_set = \"access action advance allocatable allocate apostrophe assign assignment associate asynchronous backspace bind blank blockdata call case character class close common complex contains continue critical cycle data deallocate decimal delim default dimension direct do dowhile double doubleprecision else elseif elsewhere encoding end endassociate endblockdata endcritical enddo endenum endfile endforall endfunction endif endinterface endmodule endprocedure endprogram endselect endsubmodule endsubroutine endtype endwhere entry enum eor equivalence err errmsg exist exit external file flush fmt forall form format formatted function go goto id if implicit in include inout integer inquire intent interface intrinsic iomsg iolength iostat kind len logical module name named namelist nextrec nml none nullify number only open opened operator optional out pad parameter pass pause pending pointer pos position precision print private procedure program protected public quote read readwrite real rec recl recursive result return rewind save select selectcase selecttype sequential sign size stat status stop stream submodule subroutine target then to type unformatted unit use value volatile wait where while write\"\n",
    "  local keywords1_set = \"abs achar acos acosd adjustl adjustr aimag aimax0 aimin0 aint ajmax0 ajmin0 akmax0 akmin0 all allocated alog alog10 amax0 amax1 amin0 amin1 amod anint any asin asind associated atan atan2 atan2d atand bitest bitl bitlr bitrl bjtest bit_size bktest break btest cabs ccos cdabs cdcos cdexp cdlog cdsin cdsqrt ceiling cexp char clog cmplx conjg cos cosd cosh count cpu_time cshift csin csqrt dabs dacos dacosd dasin dasind datan datan2 datan2d datand date date_and_time dble dcmplx dconjg dcos dcosd dcosh dcotan ddim dexp dfloat dflotk dfloti dflotj digits dim dimag dint dlog dlog10 dmax1 dmin1 dmod dnint dot_product dprod dreal dsign dsin dsind dsinh dsqrt dtan dtand dtanh eoshift epsilon errsns exp exponent float floati floatj floatk floor fraction free huge iabs iachar iand ibclr ibits ibset ichar idate idim idint idnint ieor ifix iiabs iiand iibclr iibits iibset iidim iidint iidnnt iieor iifix iint iior iiqint iiqnnt iishft iishftc iisign ilen imax0 imax1 imin0 imin1 imod index inint inot int int1 int2 int4 int8 iqint iqnint ior ishft ishftc isign isnan izext jiand jibclr jibits jibset jidim jidint jidnnt jieor jifix jint jior jiqint jiqnnt jishft jishftc jisign jmax0 jmax1 jmin0 jmin1 jmod jnint jnot jzext kiabs kiand kibclr kibits kibset kidim kidint kidnnt kieor kifix kind kint kior kishft kishftc kisign kmax0 kmax1 kmin0 kmin1 kmod knint knot kzext lbound leadz len len_trim lenlge lge lgt lle llt log log10 logical lshift malloc matmul max max0 max1 maxexponent maxloc maxval merge min min0 min1 minexponent minloc minval mod modulo mvbits nearest nint not nworkers number_of_processors pack popcnt poppar precision present product radix random random_number random_seed range real repeat reshape rrspacing rshift scale scan secnds selected_int_kind selected_real_kind set_exponent shape sign sin sind sinh size sizeof sngl snglq spacing spread sqrt sum system_clock tan tand tanh tiny transfer transpose trim ubound unpack verify\"\n",
    "  local keywords2_set = \"cotan cotand dcmplx dconjg dcotan dcotand decode dimag dll_export dll_import doublecomplex dreal dvchk encode find flen flush getarg getcharqq getcl getdat getenv gettim hfix ibchng identifier imag int1 int2 int4 intc intrup invalop iostat_msg isha ishc ishl jfix lacfar locking locnear map nargs nbreak ndperr ndpexc offset ovefl peekcharqq precfill prompt qabs qacos qacosd qasin qasind qatan qatand qatan2 qcmplx qconjg qcos qcosd qcosh qdim qexp qext qextd qfloat qimag qlog qlog10 qmax1 qmin1 qmod qreal qsign qsin qsind qsinh qsqrt qtan qtand qtanh ran rand randu rewrite segment setdat settim system timer undfl unlock union val virtual volatile zabs zcos zexp zlog zsin zsqrt\"\n",
    "  return keywords0_set,keywords1_set,keywords2_set\n",
    "end\n",
    "\n",
    "function user_fortran.get_autocomplete()\n",
    "  local autocomplete_set = \"access action advance allocatable allocate apostrophe assign assignment associate asynchronous backspace bind blank blockdata call case character class close common complex contains continue critical cycle data deallocate decimal delim default dimension direct do dowhile double doubleprecision else elseif elsewhere encoding end endassociate endblockdata endcritical enddo endenum endfile endforall endfunction endif endinterface endmodule endprocedure endprogram endselect endsubmodule endsubroutine endtype endwhere entry enum eor equivalence err errmsg exist exit external file flush fmt forall form format formatted function go goto id if implicit in include inout integer inquire intent interface intrinsic iomsg iolength iostat kind len logical module name named namelist nextrec nml none nullify number only open opened operator optional out pad parameter pass pause pending pointer pos position precision print private procedure program protected public quote read readwrite real rec recl recursive result return rewind save select selectcase selecttype sequential sign size stat status stop stream submodule subroutine target then to type unformatted unit use value volatile wait where while write abs achar acos acosd adjustl adjustr aimag aimax0 aimin0 aint ajmax0 ajmin0 akmax0 akmin0 all allocated alog alog10 amax0 amax1 amin0 amin1 amod anint any asin asind associated atan atan2 atan2d atand bitest bitl bitlr bitrl bjtest bit_size bktest break btest cabs ccos cdabs cdcos cdexp cdlog cdsin cdsqrt ceiling cexp char clog cmplx conjg cos cosd cosh count cpu_time cshift csin csqrt dabs dacos dacosd dasin dasind datan datan2 datan2d datand date date_and_time dble dcmplx dconjg dcos dcosd dcosh dcotan ddim dexp dfloat dflotk dfloti dflotj digits dim dimag dint dlog dlog10 dmax1 dmin1 dmod dnint dot_product dprod dreal dsign dsin dsind dsinh dsqrt dtan dtand dtanh eoshift epsilon errsns exp exponent float floati floatj floatk floor fraction free huge iabs iachar iand ibclr ibits ibset ichar idate idim idint idnint ieor ifix iiabs iiand iibclr iibits iibset iidim iidint iidnnt iieor iifix iint iior iiqint iiqnnt iishft iishftc iisign ilen imax0 imax1 imin0 imin1 imod index inint inot int int1 int2 int4 int8 iqint iqnint ior ishft ishftc isign isnan izext jiand jibclr jibits jibset jidim jidint jidnnt jieor jifix jint jior jiqint jiqnnt jishft jishftc jisign jmax0 jmax1 jmin0 jmin1 jmod jnint jnot jzext kiabs kiand kibclr kibits kibset kidim kidint kidnnt kieor kifix kind kint kior kishft kishftc kisign kmax0 kmax1 kmin0 kmin1 kmod knint knot kzext lbound leadz len len_trim lenlge lge lgt lle llt log log10 logical lshift malloc matmul max max0 max1 maxexponent maxloc maxval merge min min0 min1 minexponent minloc minval mod modulo mvbits nearest nint not nworkers number_of_processors pack popcnt poppar precision present product radix random random_number random_seed range real repeat reshape rrspacing rshift scale scan secnds selected_int_kind selected_real_kind set_exponent shape sign sin sind sinh size sizeof sngl snglq spacing spread sqrt sum system_clock tan tand tanh tiny transfer transpose trim ubound unpack verify cotan cotand dcmplx dconjg dcotan dcotand decode dimag dll_export dll_import doublecomplex dreal dvchk encode find flen flush getarg getcharqq getcl getdat getenv gettim hfix ibchng identifier imag int1 int2 int4 intc intrup invalop iostat_msg isha ishc ishl jfix lacfar locking locnear map nargs nbreak ndperr ndpexc offset ovefl peekcharqq precfill prompt qabs qacos qacosd qasin qasind qatan qatand qatan2 qcmplx qconjg qcos qcosd qcosh qdim qexp qext qextd qfloat qimag qlog qlog10 qmax1 qmin1 qmod qreal qsign qsin qsind qsinh qsqrt qtan qtand qtanh ran rand randu rewrite segment setdat settim system timer undfl unlock union val virtual volatile zabs zcos zexp zlog zsin zsqrt\"\n",
    "  return autocomplete_set\n",
    "end\n",
    "\n",
    "function user_fortran.get_reqular()\n",
    "  local symbol_reqular_exp = \"(?:^program|^module|^function|^subroutine)[ \\\\t]+([_a-zA-Z]+[_a-zA-Z0-9:]*)\\\\s*(?:|\\\\(.*\\\\)).*$\"\n",
    "  return symbol_reqular_exp\n",
    "end\n",
    "\n",
    "function user_fortran.get_calltip()\n",
    "  local calltip_add =\n",
    "  {\n",
    "    \"associated|ASSOCIATED(p[,t]);\",\n",
    "    \"eoshift|EOSHIFT(ARRAY, SHIFT [, BOUNDARY, DIM]);\",\n",
    "    \"erfc_scaled|ERFC_SCALED(X);\",\n",
    "    \"extends_type_of|EXTENDS_TYPE_OF(A, MOLD);\",\n",
    "    \"fnum|FNUM(UNIT);\",\n",
    "    \"ierrno|IERRNO();\",\n",
    "    \"ishftc|ISHFTC(I, SHIFT [, SIZE]);\",\n",
    "    \"is_contiguous|IS_CONTIGUOUS(ARRAY);\",\n",
    "    \"mvbits|CALL MVBITS(FROM, FROMPOS, LEN, TO, TOPOS);\",\n",
    "    \"move_alloc|CALL MOVE_ALLOC(FROM, TO);\",\n",
    "    \"selected_char_kind|SELECTED_CHAR_KIND(NAME);\",\n",
    "    \"size|SIZE(ARRAY[, DIM [, KIND]]);\",\n",
    "    \"rand|RAND(I);\",\n",
    "    \"random_init|CALL RANDOM_INIT(REPEATABLE, IMAGE_DISTINCT);\",\n",
    "    \"random_number|RANDOM_NUMBER(HARVEST);\",\n",
    "    \"random_seed|CALL RANDOM_SEED([SIZE, PUT, GET]);\",\n",
    "    \"selected_int_kind|SELECTED_INT_KIND(R);\",\n",
    "    \"selected_real_kind|SELECTED_REAL_KIND([P, R, RADIX]);\",\n",
    "    \"acos|ACOS(x);\",\n",
    "    \"acosd|ACOSD(x);\",\n",
    "    \"asin|ASIN(x);\",\n",
    "    \"atan|ATAN(x);\",\n",
    "    \"atand|ATAND(x);\",\n",
    "    \"atan2|ATAN2(y,x);\",\n",
    "    \"atan2d|ATAN2D(y,x);\",\n",
    "    \"cos|COS(x);\",\n",
    "    \"cosd|COSD(x);\",\n",
    "    \"acosh|COSH(x);\",\n",
    "    \"cotan|COTAN(x);\",\n",
    "    \"sin|SIN(x);\"\n",
    "  }\n",
    "  return calltip_add\n",
    "end\n",
    "return user_fortran",
  }
  local shell_code = table.concat(fortran_code)
  eu_core.save_file(path, shell_code)
  shell_code = nil
  fortran_code = nil
end

return fortran