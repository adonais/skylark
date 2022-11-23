/*******************************************************************************
 * This file is part of Skylark project
 * Copyright ©2022 Hua andy <hua.andy@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * at your option any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef _EU_TARGETVER_H_
#define _EU_TARGETVER_H_

#ifndef IDC_STATIC
#define IDC_STATIC -1
#endif

#define TO_STR(arg)                        TEXT(#arg)
#define MAKE_VERSION_NUM(a, b, c, sep)     TO_STR(a) sep TO_STR(b) sep TO_STR(c)
#define MAKE_VERSION_STR(a, b, c, d, sep)  (TO_STR(a)TEXT(" ")TO_STR(b) sep TO_STR(c) sep TO_STR(d))

// 定义版本信息
#define __EU_INFO_VERSION 4
#define __EU_INFO_VERSION_MINOR 0
#define __EU_INFO_VERSION_PATCHLEVEL 0
#define __ORIGINAL_NAME TEXT("skylark.exe")

#if 1
#define __LIFE_CYCLE TEXT("-DEV")
#else
#define __LIFE_CYCLE
#endif

#define __EU_INFO_RELEASE_VERSION MAKE_VERSION_NUM(__EU_INFO_VERSION,__EU_INFO_VERSION_MINOR,__EU_INFO_VERSION_PATCHLEVEL,".")

#define EU_DESCRIPTION TEXT("Copyright © 2020-2022, Skylark project authors. All rights reserved. \r\n\r\n") \
                       TEXT("Note that the GPL places important restrictions on 'derived works', \r\n") \
                       TEXT("yet it does not provide a detailed definition of that term. ") \
                       TEXT("To avoid misunderstandings, we consider an application to constitute a 'derivative work' for ") \
                       TEXT("the purpose of this license if it does any of the following: \r\n") \
                       TEXT("1. Integrates source code from Skylark project \r\n") \
                       TEXT("2. Integrates/includes/aggregates Skylark into a proprietary executable ")\
                       TEXT("installer, such as those produced by InstallShield \r\n") \
                       TEXT("3. Links to a library or executes a program that does any of the above. \r\n") \
                       TEXT("This program is distributed in the hope that it will be useful, ") \
                       TEXT("but WITHOUT ANY WARRANTY; without even the implied warranty of ") \
                       TEXT("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the ") \
                       TEXT("GNU General Public License for more details. \r\n")
#ifdef _UNICODE
#define VER_CHARSET TEXT("UNICODE")
#else
#define VER_CHARSET TEXT("MBCS")
#endif

#ifdef _WIN64
#define VER_PLATFORM TEXT("64bit")
#else
#define VER_PLATFORM TEXT("32bit")
#endif

#ifdef _DEBUG
#define VER_CONFIG TEXT("DEBUG")
#else
#define VER_CONFIG TEXT("RELEASE")
#endif

#if (__clang__)
#define CLANG_VER_STR (MAKE_VERSION_STR(clang,__clang_major__,__clang_minor__,__clang_patchlevel__,"."))
#define VC_BUILDER CLANG_VER_STR
#elif (__GNUC__)
#define GCC_VER_STR (MAKE_VERSION_STR(gcc,__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__,"."))
#define VC_BUILDER GCC_VER_STR
#elif _MSC_VER >= 1930
#define VC_BUILDER TEXT("VC17")
#elif _MSC_VER >= 1920
#define VC_BUILDER TEXT("VC16")
#elif _MSC_VER >= 1910
#define VC_BUILDER TEXT("VC15")
#elif _MSC_VER >= 1900
#define VC_BUILDER TEXT("VC14")
#elif _MSC_VER >= 1800
#define VC_BUILDER TEXT("VC12")
#elif _MSC_VER == 1700
#define VC_BUILDER TEXT("VC11")
#elif defined(_MSC_VER)
#define VC_BUILDER TEXT("VC")
#endif

#define __EU_INFO_RELEASE TEXT("Skylark Edit v")      \
                          __EU_INFO_RELEASE_VERSION   \
                          TEXT(" ")                   \
                          VER_PLATFORM                \
                          __LIFE_CYCLE                \
                          TEXT(" ")                   \
                          TEXT("(")                   \
                          VER_CHARSET                 \
                          TEXT(")")

// 资源标识符
#define IDR_LBREAK_MENU  10000
#define IDR_CODEING_MENU 10001
#define IDR_TYPES_MENU   10002

#define IDM_UNI_UTF8     10014
#define IDM_UNI_UTF8B    10015
#define IDM_UNI_UTF16LE  10016
#define IDM_UNI_UTF16LEB 10017
#define IDM_UNI_UTF16BE  10018
#define IDM_UNI_UTF16BEB 10019
#define IDM_UNI_UTF32LE  10020
#define IDM_UNI_UTF32BE  10021
#define IDM_UNI_ASCII    10022

#define IDM_ANSI_1       10023
#define IDM_ANSI_2       10024
#define IDM_ANSI_3       10025
#define IDM_ANSI_4       10026
#define IDM_ANSI_5       10027
#define IDM_ANSI_6       10028
#define IDM_ANSI_7       10029
#define IDM_ANSI_8       10030
#define IDM_ANSI_9       10031
#define IDM_ANSI_10      10032
#define IDM_ANSI_11      10033
#define IDM_ANSI_12      10034
#define IDM_ANSI_13      10035
#define IDM_ANSI_14      10036

#define IDM_IBM_1        10037
#define IDM_IBM_2        10038
#define IDM_IBM_3        10039

#define IDM_ISO_1        10040
#define IDM_ISO_2        10041
#define IDM_ISO_3        10042
#define IDM_ISO_4        10043
#define IDM_ISO_5        10044
#define IDM_ISO_6        10045
#define IDM_ISO_7        10046
#define IDM_ISO_8        10047
#define IDM_ISO_9        10048
#define IDM_ISO_10       10049
#define IDM_ISO_11       10050
#define IDM_ISO_13       10051
#define IDM_ISO_15       10052
#define IDM_ISO_16       10053
#define IDM_ISO_KR       10054
#define IDM_ISO_CN       10055
#define IDM_ISO_JP_2     10056
#define IDM_ISO_JP_2004  10057
#define IDM_ISO_JP_MS    10058

#define IDM_EUC_1        10059
#define IDM_EUC_2        10060

#define IDM_OTHER_HZ     10061
#define IDM_OTHER_1      10062
#define IDM_OTHER_2      10063
#define IDM_OTHER_3      10064
#define IDM_OTHER_ANSI   10065
#define IDM_OTHER_BIN    10066
#define IDM_UNKNOWN      10067

#define IDM_LBREAK_1     10080
#define IDM_LBREAK_2     10081
#define IDM_LBREAK_3     10082
#define IDM_LBREAK_4     10083
#define IDM_SCI_VIEWZONE 10084
#define IDM_BTN_RW       10085

#define IDD_DIALOG_CLIP         10090
#define IDC_CLIPBORAD           10091
#define IDC_STATUSBAR           10092
#define IDC_STATIC_MYICON       10093
#define IDC_STATIC_MYICON_DARK  10094
#define IDM_COMBO1              10095
#define IDM_COMBO2              10096
#define IDM_COMBO3              10097

#define IDS_COLUMN_0         10110
#define IDS_COLUMN_1         10111
#define IDS_COLUMN_2         10112
#define IDS_COLUMN_3         10113

#define IDC_BUFFERS_LABEL    10114
#define IDC_LOCK_LABEL       10115
#define IDB_COPY             10116
#define IDB_COPY_DARK        10117
#define IDB_BUTTON_BG_BMP    10118
#define IDB_AC_CLOSE_BMP     10119

#define IDC_BUTTON0   10122
#define IDC_BUTTON1   10123
#define IDC_BUTTON2   10124
#define IDC_BUTTON3   10125
#define IDC_BUTTON4   10126
#define IDC_BUTTON5   10127
#define IDC_BUTTON6   10128
#define IDC_BUTTON7   10129
#define IDC_BUTTON8   10130
#define IDC_BUTTON9   10131
#define IDC_BUTTON10  10132
#define IDC_BUTTON11  10133

#define IDC_EDIT0   10140
#define IDC_EDIT1   10141
#define IDC_EDIT2   10142
#define IDC_EDIT3   10143
#define IDC_EDIT4   10144
#define IDC_EDIT5   10145
#define IDC_EDIT6   10146
#define IDC_EDIT7   10147
#define IDC_EDIT8   10148
#define IDC_EDIT9   10149
#define IDC_EDIT10  10150
#define IDC_EDIT11  10151

#define IDC_CHECK0   10160
#define IDC_CHECK1   10161
#define IDC_CHECK2   10162
#define IDC_CHECK3   10163
#define IDC_CHECK4   10164
#define IDC_CHECK5   10165
#define IDC_CHECK6   10166
#define IDC_CHECK7   10167
#define IDC_CHECK8   10168
#define IDC_CHECK9   10169
#define IDC_CHECK10  10170
#define IDC_CHECK11  10171

#define IDS_BUTTON_CAP 10172
#define IDS_BUTTON_R   10173
#define IDS_BUTTON_W   10174
#define IDS_STATUS_F1  10175
#define IDS_STATUS_XY  10176
#define IDS_STATUS_LC  10177
#define IDS_STATUS_LD  10178
#define IDS_STATUS_HXY 10179
#define IDS_STATUS_HLC 10279

#define IDS_TOOLBAR_0    10180
#define IDS_TOOLBAR_1    10181
#define IDS_TOOLBAR_2    10182
#define IDS_TOOLBAR_3    10183
#define IDS_TOOLBAR_4    10184
#define IDS_TOOLBAR_5    10185
#define IDS_TOOLBAR_6    10186
#define IDS_TOOLBAR_7    10187
#define IDS_TOOLBAR_8    10188
#define IDS_TOOLBAR_9    10189
#define IDS_TOOLBAR_10   10190
#define IDS_TOOLBAR_11   10191
#define IDS_TOOLBAR_12   10192
#define IDS_TOOLBAR_13   10193
#define IDS_TOOLBAR_14   10194
#define IDS_TOOLBAR_15   10195
#define IDS_TOOLBAR_16   10196
#define IDS_TOOLBAR_17   10197
#define IDS_TOOLBAR_18   10198
#define IDS_TOOLBAR_19   10199
#define IDS_TOOLBAR_20   10200
#define IDS_TOOLBAR_21   10201
#define IDS_TOOLBAR_22   10202
#define IDS_TOOLBAR_23   10203
#define IDS_TOOLBAR_24   10204
#define IDS_TOOLBAR_25   10205
#define IDS_TOOLBAR_26   10206
#define IDS_TOOLBAR_27   10207
#define IDS_TOOLBAR_28   10208
// file type
#define IDM_TYPES_0   10300

#define IDD_DIALOG_FC 20000
#define IDC_FC_STC1   20001
#define IDC_FC_BTN1   20002
#define IDC_FC_BTN2   20003
#define IDC_FC_BTN3   20004
#define IDC_FC_BTN4   20005
#define IDC_FC_CHK1   20006

#define IDD_DIALOG_NOEXIST 20010
#define IDC_NOEXIST_STC1   20001
#define IDC_NOEXIST_BTN1   20012
#define IDC_NOEXIST_BTN2   20013
#define IDC_NOEXIST_BTN3   20014
#define IDC_NOEXIST_BTN4   20015

#define IDD_DLG_INPUT  20020
#define IDC_INPUT_STC1 20021
#define IDC_INPUT_EDT1 20022
#define IDC_INPUT_BTN1 20023
#define IDC_INPUT_BTN2 20024

// toolbar newid
#define IDM_FILE_NEW                    30000
#define IDM_FILE_OPEN                   30001
#define IDM_FILE_SAVE                   30002
#define IDM_FILE_SAVEAS                 30003
#define IDM_FILE_CLOSE                  30004
#define IDM_FILE_PRINT                  30005
#define IDM_FILE_REMOTE_FILESERVERS     30006
#define IDM_EDIT_CUT                    30007
#define IDM_EDIT_COPY                   30008
#define IDM_EDIT_PASTE                  30009
#define IDM_EDIT_CLIP                   30010
#define IDM_SEARCH_FIND                 30011
#define IDM_SEARCH_FINDPREV             30012
#define IDM_SEARCH_FINDNEXT             30013
#define IDM_EDIT_UNDO                   30014
#define IDM_EDIT_REDO                   30015
#define IDM_SEARCH_TOGGLE_BOOKMARK      30016
#define IDM_SEARCH_GOTO_PREV_BOOKMARK   30017
#define IDM_SEARCH_GOTO_NEXT_BOOKMARK   30018
#define IDM_VIEW_HEXEDIT_MODE           30019
#define IDM_VIEW_FILETREE               30020
#define IDM_VIEW_SYMTREE                30021
#define IDM_VIEW_MODIFY_STYLETHEME      30022
#define IDM_VIEW_ZOOMOUT                30023
#define IDM_VIEW_ZOOMIN                 30024
#define IDM_SCRIPT_EXEC                 30025
#define IDM_CMD_TAB                     30026
#define IDB_TOOLBAR1                    30030
#define IDB_TOOLBAR16                   30031
#define IDC_TOOLBAR                     30032
#define IDB_DARK1                       30033
#define IDB_DARK16                      30034
#define IDB_TOOLBAR_LARGE1              30035
#define IDB_TOOLBAR_LARGE32             30036
#define IDB_DARK_LARGE1                 30037
#define IDB_DARK_LARGE32                30038
#define IDB_TOOLBAR_MIDDLING1           30039
#define IDB_TOOLBAR_MIDDLING24          30040
#define IDB_DARK_MIDDLING1              30041
#define IDB_DARK_MIDDLING24             30042
#define IDM_VIEW_DOCUMENT_MAP           30043

#define IDM_FILE_CLOSEALL               30100
#define IDM_FILE_CLOSEALL_EXCLUDE       30101
#define IDM_SOURCEE_ENABLE_ACSHOW       30102
#define IDM_SOURCEE_ACSHOW_CHARS        30103
#define IDM_SOURCE_ENABLE_CTSHOW        30104
#define IDM_EDIT_AUTO_CLOSECHAR         30105
#define IDM_EDIT_AUTO_INDENTATION       30106
#define IDM_HISTORY_CLEAN               30107
#define IDM_VIEW_NEWLINE_VISIABLE       30108
#define IDM_FILE_NEWFILE_WINDOWS_EOLS   30109
#define IDM_FILE_NEWFILE_MAC_EOLS       30110
#define IDM_FILE_NEWFILE_UNIX_EOLS      30111
#define IDM_FILE_SESSION                30112
#define IDM_FILE_WRITE_COPY             30113
#define IDM_FILE_RESTORE_RECENT         30114
#define IDM_FILE_EXIT_WHEN_LAST_TAB     30115
#define IDM_FILE_RESTART_ADMIN          30116

#define IDM_EDIT_BASE64_ENCODING        30200
#define IDM_EDIT_BASE64_DECODING        30201
#define IDM_EDIT_MD5                    30202
#define IDM_EDIT_SHA1                   30203
#define IDM_EDIT_SHA256                 30204
#define IDM_EDIT_3DES_CBC_ENCRYPTO      30205
#define IDM_EDIT_3DES_CBC_DECRYPTO      30206
#define IDM_VIEW_BOOKMARK_VISIABLE      30207
#define IDM_SEARCH_REMOVE_ALL_BOOKMARKS 30210

#define IDM_VIEW_INDENTGUIDES_VISIABLE       30300
#define IDM_SEARCH_GOTO_PREV_BOOKMARK_INALL  30301
#define IDM_SEARCH_GOTO_NEXT_BOOKMARK_INALL  30302
#define IDM_SEARCH_NAVIGATE_PREV_THIS        30303
#define IDM_SEARCH_NAVIGATE_PREV_INALL       30304
#define IDM_DATABASE_INSERT_CONFIG           30305
#define IDM_REDIS_INSERT_CONFIG              30306
#define IDM_REDIS_EXECUTE_COMMAND            30307
#define IDM_SEARCH_MULTISELECT_README        30308
#define IDM_SEARCH_COLUMNSELECT_README       30309
#define IDM_RENAME_DIRECTORY                 30310
#define IDM_DELETE_DIRECTORY                 30311
#define IDM_CREATE_SUB_DIRECTORY             30312
#define IDM_CREATE_FILE                      30313
#define IDM_RENAME_FILE                      30314
#define IDM_DELETE_FILE                      30315
#define IDM_COPY_FILE                        30316
#define IDM_DATABASE_SELECT_EXECUTE          30317
#define IDM_REDIS_SELECT_EXECUTE             30318
#define IDM_SEARCH_FILES                     30319

#define IDM_TAB_CONVERT_SPACES               30400
#define IDM_DELETE_SPACE_LINEHEAD            30401
#define IDM_DELETE_SPACE_LINETAIL            30402
#define IDM_DELETE_ALL_SPACE_LINE            30404
#define IDM_EDIT_COPY_FILENAME               30405
#define IDM_EDIT_COPY_PATHNAME               30406
#define IDM_EDIT_COPY_PATHFILENAME           30407
#define IDM_TAB_CLOSE                        30410
#define IDR_FILETREE_POPUPMENU               30411
#define IDM_RELOAD_FILETREE                  30412
#define IDM_EDIT_DELETE                      30413
#define IDM_SEARCH_REPLACE                   30414
#define IDM_SEARCH_SELECTALL                 30415
#define IDM_VIEW_ZOOMRESET                   30416
#define IDM_RELOAD_SYMBOLLIST                30417
#define IDM_REFRESH_FILETREE                 30418
#define IDM_FILE_SAVEALL                     30419
#define IDM_EDIT_CUTLINE                     30420
#define IDM_EDIT_CUTLINE_AND_PASTELINE       30421
#define IDM_EDIT_COPYLINE                    30422
#define IDM_EDIT_COPYLINE_AND_PASTELINE      30423
#define IDM_EDIT_REMOVE_DUP_LINES            30424
#define IDM_EDIT_LINETRANSPOSE               30425
#define IDM_EDIT_DELETELINE                  30426
#define IDM_EDIT_LOWERCASE                   30427
#define IDM_EDIT_UPPERCASE                   30428
#define IDM_EDIT_JOINLINE                    30429
#define IDM_SEARCH_SELECTWORD                30430
#define IDM_SEARCH_SELECTLINE                30431
#define IDM_SEARCH_ADDSELECT_LEFT_WORD       30432
#define IDM_SEARCH_ADDSELECT_RIGHT_WORD      30433
#define IDM_SEARCH_ADDSELECT_LEFT_WORDGROUP  30434
#define IDM_SEARCH_ADDSELECT_RIGHT_WORDGROUP 30435
#define IDM_SEARCH_MOVE_LEFT_WORD            30436
#define IDM_SEARCH_MOVE_RIGHT_WORD           30437
#define IDM_SEARCH_MOVE_LEFT_WORDGROUP       30438
#define IDM_SEARCH_MOVE_RIGHT_WORDGROUP      30439
#define IDM_DATABASE_EXECUTE_SQL             30440
#define IDM_SOURCECODE_GOTODEF               30441
#define IDM_ENV_FILE_POPUPMENU               30442
#define IDM_ENV_DIRECTORY_POPUPMENU          30443
#define IDM_VIEW_WRAPLINE_MODE               30444
#define IDM_VIEW_TAB_WIDTH                   30445
#define IDM_RELOAD_FILESEARCH                30446
#define IDM_EDIT_MOVE_LINEUP                 30447
#define IDM_EDIT_MOVE_LINEDOWN               30448
#define IDM_DELETE_ALL_SPACE_LINEHEAD        30449
#define IDM_DELETE_ALL_SPACE_LINETAIL        30450
#define IDM_VIEW_SWITCH_TAB                  30451
#define IDM_EDIT_WORD_UPPERCASE              30452
#define IDM_EDIT_SENTENCE_UPPERCASE          30453

#define IDM_VIEW_WHITESPACE_VISIABLE      30500
#define IDM_VIEW_LINENUMBER_VISIABLE      30501
#define IDM_SOURCE_BLOCKFOLD_VISIABLE     30502
#define IDM_SOURCE_BLOCKFOLD_TOGGLE       30503

#define IDM_SOURCE_BLOCKFOLD_CONTRACTALL  30506
#define IDM_SOURCE_BLOCKFOLD_EXPANDALL    30507
#define IDM_SEARCH_GOTOLINE               30508
#define IDM_SEARCH_GOTOHOME               30509
#define IDM_SEARCH_GOTOEND                30510
#define IDM_SEARCH_MOVETOP_FIRSTLINE      30511
#define IDM_SEARCH_MOVEBOTTOM_FIRSTLINE   30512
#define IDM_SEARCH_SELECTTOP_FIRSTLINE    30513
#define IDM_SEARCH_SELECTBOTTOM_FIRSTLINE 30514
#define IDM_ENV_SET_ASSOCIATED_WITH       30515
#define IDM_CHANGELOG                     30516
#define IDM_INTRODUTION                   30517

#define IDM_HISTORY_BASE                  30600
#define IDM_STYLETHEME_BASE               30700
#define IDM_VIEW_COPYNEW_STYLETHEME       30800

#define IDD_REGEXT_BOX                   31000
#define IDC_REGEXT_LANG_LIST             (IDD_REGEXT_BOX + 1)
#define IDC_REGEXT_LANGEXT_LIST          (IDD_REGEXT_BOX + 2)
#define IDC_REGEXT_REGISTEREDEXTS_LIST   (IDD_REGEXT_BOX + 3)
#define IDC_ADDFROMLANGEXT_BUTTON        (IDD_REGEXT_BOX + 4)
#define IDI_POUPELLE_ICON                (IDD_REGEXT_BOX + 5)
#define IDC_CUSTOMEXT_EDIT               (IDD_REGEXT_BOX + 6)
#define IDC_REMOVEEXT_BUTTON             (IDD_REGEXT_BOX + 7)
#define IDC_WIN10_STATIC                 (IDD_REGEXT_BOX + 8)
#define IDC_SUPPORTEDEXTS_STATIC         (IDD_REGEXT_BOX + 9)
#define IDC_REGISTEREDEXTS_STATIC        (IDD_REGEXT_BOX + 10)

#define IDS_SNIPPETS_STR                31094
#define IDS_CHECK_VER_ERR               31095
#define IDS_CHECK_VER_DEC               31096
#define IDS_CHECK_VER_NEW               31097
#define IDC_STATIC_VER                  31098
#define IDC_STATIC_URL_HOMEPAGE         31099
#define IDC_STATIC_URL_CAPTION          31100
#define IDC_STATIC_URL_UR               31101
#define IDC_STATIC_EDIT                 31102
#define IDC_BUTTON_COPY                 31103
#define IDC_EDIT_VER                    31104
#define IDC_EDIT_ABOUT                  31105
#define IDS_ABOUT_VERSION               31106
#define IDS_ABOUT_DESCRIPTION           31107
#define IDC_COMPILER                    31108

#define IDM_RESULT_COPY                 31109
#define IDM_RESULT_WRAPLINE             31110
#define IDM_RESULT_SETSEL               31111
#define IDM_RESULT_UNSETSEL             31112
#define IDM_RESULT_CLEARALL             31113
#define IDM_RESULT_CLOSE                31114
#define IDR_RESULT_MENU                 31115

#define IDC_MSG_ERROR          40000
#define IDC_MSG_WARN           40001
#define IDC_MSG_TIPS           40002
#define IDC_MSG_PASS           40003
#define IDC_MSG_ATTACH_FAIL    40004
#define IDC_MSG_ATTACH_FAIL1   40005
#define IDC_MSG_ATTACH_FAIL2   40006
#define IDC_MSG_ATTACH_FAIL3   40007
#define IDC_MSG_ATTACH_SUCCESS 40008
#define IDC_MSG_DIR_HOME       40009
#define IDC_MSG_DIR_ROOT       40010
#define IDC_MSG_DIR_SUB        40011
#define IDC_MSG_DIR_FAIL       40012
#define IDC_MSG_MK_FILE        40013
#define IDC_MSG_EXIST_FILE     40014
#define IDC_MSG_FILE_FAIL      40015
#define IDC_MSG_COPY_FAIL      40016
#define IDC_MSG_SFTP_FAIL      40017
#define IDC_MSG_EXPLORER       40018
#define IDC_MSG_THEME_NAME     40019
#define IDC_MSG_THEME_ERR1     40020
#define IDC_MSG_THEME_ERR2     40021
#define IDC_MSG_THEME_ERR3     40022
#define IDC_MSG_THEME_ERR4     40023
#define IDC_MSG_JSON_ERR1      40024
#define IDC_MSG_JSON_ERR2      40025
#define IDC_MSG_TAB_LEN        40026
#define IDC_MSG_SCINTILLA_ERR1 40027
#define IDC_MSG_OPEN_FAIL      40028
#define IDC_MSG_NEW_FILE       40029
#define IDC_MSG_WRITE_FAIL     40030
#define IDC_MSG_ICONV_FAIL1    40031
#define IDC_MSG_ICONV_FAIL2    40032
#define IDC_MSG_OPENSSL_FAIL   40033
#define IDC_MSG_ATTACH_ERRORS  40034

#define IDM_FILE_NEWFILE_ENCODING_UTF8      40100
#define IDM_FILE_NEWFILE_ENCODING_UTF8B     40101
#define IDM_FILE_NEWFILE_ENCODING_UTF16LE   40102
#define IDM_FILE_NEWFILE_ENCODING_UTF16BE   40103
#define IDM_FILE_NEWFILE_ENCODING_ANSI      40104
#define IDC_MSG_TABCONTROL_FAIL             40105
#define IDC_MSG_CONV_TIPS                   40106
#define IDC_MSG_CONV_FAIL1                  40107
#define IDC_MSG_CONV_FAIL2                  40108
#define IDC_MSG_DIR_WRITE_FAIL              40109
#define IDC_MSG_MEM_NOT_ENOUGH              40110
#define IDC_MSG_MEM_NOT_AVAIL               40111
#define IDC_MSG_SYMLIST_FAIL                40112
#define IDC_MSG_PCRE_FAIL                   40113
#define IDC_MSG_HISTORY_ZERO                40114
#define IDC_MSG_OPEN_FILE_EDIT              40115
#define IDC_MSG_DIRECTORYFILES              40116
#define IDC_MSG_LOC_DIRECTORY               40117
#define IDC_MSG_REGIST_ERR1                 40119
#define IDC_MSG_REGIST_ERR2                 40120
#define IDC_MSG_REGIST_ERR3                 40121
#define IDC_MSG_REGIST_ERR4                 40122
#define IDC_MSG_REGIST_ERR5                 40123
#define IDC_MSG_REGIST_ERR6                 40124
#define IDC_MSG_REGIST_ERR7                 40125
#define IDC_MSG_REGIST_ERR8                 40126
#define IDC_MSG_REGIST_ERR9                 40127
#define IDC_MSG_REGIST_ERR10                40128
#define IDC_MSG_REGIST_ERR11                40129
#define IDC_MSG_REGIST_ERR12                40130
#define IDC_MSG_REGIST_ERR13                40131
#define IDC_MSG_REGIST_ERR14                40132
#define IDC_MSG_REGIST_ERR15                40133
#define IDC_MSG_REGIST_ERR16                40134
#define IDC_MSG_REGIST_ERR17                40135
#define IDC_MSG_REGIST_ERR18                40136
#define IDC_MSG_REGIST_ERR19                40137
#define IDC_MSG_REGIST_ERR20                40138
#define IDC_MSG_REGIST_ERR21                40139
#define IDC_MSG_REGIST_ERR22                40140
#define IDC_MSG_REGIST_ERR23                40141
#define IDC_MSG_REGIST_ERR24                40142
#define IDC_MSG_REGIST_ERR25                40143
#define IDC_MSG_REGIST_ERR26                40144
#define IDC_MSG_REGIST_ERR27                40145
#define IDC_MSG_REGIST_ERR28                40146
#define IDC_MSG_REGIST_ERR29                40147
#define IDC_MSG_REGIST_ERR30                40148
#define IDC_MSG_REGIST_ERR31                40149
#define IDC_MSG_REGIST_ERR32                40150
#define IDC_MSG_REGIST_ERR33                40151
#define IDC_MSG_REGIST_ERR34                40152
#define IDC_MSG_REGIST_ERR35                40153
#define IDC_MSG_REGIST_ERR36                40154
#define IDC_MSG_REGIST_ERR37                40155
#define IDC_MSG_JUST_HELP                   40156
#define IDC_MSG_HELP_INF1                   40157
#define IDC_MSG_HELP_INF2                   40158
#define IDC_MSG_EXEC_ERR1                   40159
#define IDC_MSG_EDIT_ERR1                   40160
#define IDC_MSG_SYMTREE_ERR1                40161
#define IDC_MSG_SYMTREE_ERR2                40162
#define IDC_MSG_SYMTREE_ERR3                40163
#define IDC_MSG_SYMTREE_ERR4                40164
#define IDC_MSG_SYMTREE_ERR5                40165
#define IDC_MSG_SYMTREE_ERR6                40166
#define IDC_MSG_SYMTREE_ERR7                40167
#define IDC_MSG_SYMTREE_ERR8                40168
#define IDC_MSG_AC_STR                      40169
#define IDC_MSG_SCI_ERR1                    40170
#define IDC_MSG_ENC_ERR1                    40171
#define IDC_MSG_ENC_ERR2                    40172
#define IDC_MSG_ENC_STR                     40173
#define IDC_MSG_DEC_STR                     40174
#define IDC_MSG_SAVE_STR1                   40175
#define IDC_MSG_SAVE_STR2                   40176
#define IDC_MSG_OPEN_ERR1                   40177
#define IDC_MSG_SYMTREE_ERR9                40178
#define IDC_MSG_SYMTREE_ERR10               40179
#define IDC_MSG_SYMTREE_ERR11               40180
#define IDC_MSG_SYMTREE_ERR12               40181
#define IDC_MSG_SYMTREE_ERR13               40182
#define IDC_MSG_SYMTREE_ERR14               40183
#define IDC_MSG_SYMTREE_ERR15               40184
#define IDC_MSG_QUERY_ERR1                  40185
#define IDC_MSG_QUERY_ERR2                  40186
#define IDC_MSG_QUERY_ERR3                  40187
#define IDC_MSG_QUERY_ERR4                  40188
#define IDC_MSG_QUERY_ERR5                  40189
#define IDC_MSG_QUERY_ERR6                  40190
#define IDC_MSG_QUERY_ERR7                  40191
#define IDC_MSG_QUERY_ERR8                  40192
#define IDC_MSG_QUERY_ERR9                  40193
#define IDC_MSG_QUERY_ERR10                 40194
#define IDC_MSG_QUERY_ERR11                 40195
#define IDC_MSG_QUERY_ERR12                 40196
#define IDC_MSG_QUERY_ERR13                 40197
#define IDC_MSG_QUERY_ERR14                 40198
#define IDC_MSG_QUERY_ERR15                 40199
#define IDC_MSG_QUERY_ERR16                 40200
#define IDC_MSG_QUERY_ERR17                 40201
#define IDC_MSG_QUERY_ERR18                 40202
#define IDC_MSG_QUERY_ERR19                 40203
#define IDC_MSG_QUERY_ERR20                 40204
#define IDC_MSG_QUERY_ERR21                 40205
#define IDC_MSG_QUERY_ERR22                 40206
#define IDC_MSG_QUERY_ERR23                 40207
#define IDC_MSG_QUERY_ERR24                 40208
#define IDC_MSG_QUERY_ERR25                 40209
#define IDC_MSG_QUERY_ERR26                 40210
#define IDC_MSG_QUERY_STR1                  40211
#define IDC_MSG_QUERY_STR2                  40212
#define IDC_MSG_QUERY_STR3                  40213
#define IDC_MSG_QUERY_STR4                  40214
#define IDC_MSG_QUERY_STR5                  40215
#define IDC_MSG_QUERY_STR6                  40216
#define IDC_MSG_QUERY_STR7                  40217
#define IDC_MSG_QUERY_STR8                  40218
#define IDC_MSG_QUERY_STR9                  40219
#define IDC_MSG_QUERY_STR10                 40220
#define IDC_MSG_QUERY_STR11                 40221
#define IDC_MSG_QUERY_STR12                 40222
#define IDC_MSG_QUERY_STR13                 40223
#define IDC_MSG_QUERY_STR14                 40224
#define IDC_MSG_SEARCH_ERR1                 40225
#define IDC_MSG_SEARCH_ERR2                 40226
#define IDC_MSG_SEARCH_ERR3                 40227
#define IDC_MSG_SEARCH_ERR4                 40228
#define IDC_MSG_SEARCH_ERR5                 40229
#define IDC_MSG_SEARCH_ERR6                 40230
#define IDC_MSG_SEARCH_ERR7                 40231
#define IDC_MSG_SEARCH_ERR8                 40232
#define IDC_MSG_SEARCH_ERR9                 40233
#define IDC_MSG_SEARCH_ERR10                40234
#define IDC_MSG_SEARCH_ERR11                40235
#define IDC_MSG_SEARCH_ERR12                40236
#define IDC_MSG_SEARCH_ERR13                40237
#define IDC_MSG_SEARCH_ERR14                40238
#define IDC_MSG_SEARCH_ERR15                40239
#define IDC_MSG_SEARCH_ERR16                40240
#define IDC_MSG_SEARCH_ERR17                40241
#define IDC_MSG_SEARCH_ERR18                40242
#define IDC_MSG_SEARCH_ERR19                40243
#define IDC_MSG_SEARCH_ERR20                40244
#define IDC_MSG_SEARCH_TIT1                 40245
#define IDC_MSG_SEARCH_TIT2                 40246
#define IDC_MSG_SEARCH_TIT3                 40247
#define IDC_MSG_SEARCH_STR1                 40248
#define IDC_MSG_SEARCH_STR2                 40249
#define IDC_MSG_SEARCH_STR3                 40250
#define IDC_MSG_SEARCH_STR4                 40251
#define IDC_MSG_SEARCH_STR5                 40252
#define IDC_MSG_DO_READONLY                 40253

#define IDC_SEARCH_BTN_ON                   40300
#define IDC_SEARCH_BTN_OFF                  40301

#define IDM_EDIT_GB_BIG5                    41000
#define IDM_EDIT_BIG5_GB                    41001

#define IDD_SEARCH_TAB_DLG                  41100
#define IDD_SEARCH_TAB_1                    42100

#define IDD_SEARCH_DLG                      41101
#define IDC_WHAT_STATIC_FIFOLDER            41102
#define IDC_WHAT_FOLDER_CBO                 41103
#define IDC_MATCH_ALL_FILE                  41104
#define IDC_MATCH_LOOP                      41105
#define IDC_MATCH_WORD                      41106
#define IDC_MATCH_CASE                      41107
#define IDC_MODE_STATIC                     41108
#define IDC_MODE_NORMAL                     41109
#define IDC_MODE_REGEXP                     41110
#define IDC_SEARCH_NEXT_BTN                 41111
#define IDC_SEARCH_PRE_BTN                  41112
#define IDC_SEARCH_COUNT_BTN                41113
#define IDC_SEARCH_CLOSE_BTN                41114
#define IDC_SEARCH_TIPS_STC                 41115
#define IDC_SEARCH_RP_STC                   41116
#define IDC_SEARCH_RP_CBO                   41117
#define IDC_SEARCH_FY_STC                   41118
#define IDC_SEARCH_FY_CBO                   41119
#define IDC_SEARCH_DIR_STC                  41120
#define IDC_SEARCH_DIR_CBO                  41121
#define IDC_FILES_BROWSE_BTN                41122
#define IDC_SEARCH_RE_BTN                   41123
#define IDC_SEARCH_REALL_BTN                41124
#define IDC_SEARCH_CD_CHK                   41125
#define IDC_SEARCH_SUB_CHK                  41126
#define IDC_SEARCH_START_ENGINE             41127
#define IDC_MATCH_WDSTART                   41128
#define IDC_SEARCH_HIDE_CHK                 41129
#define IDC_SEARCH_FOUNDLIST                41130
#define IDC_PGB1                            41131
#define IDC_PGB_STC                         41132
#define IDC_SEARCH_UTF8_CHK                 41133
#define IDC_SEARCH_HEX_STC                  41134
#define IDS_SEARCH_HEX_TIPS                 41135
#define IDC_SEARCH_HEX_STRINGS              41136
#define IDC_SEARCH_ALL_BTN                  41137
#define IDC_SEARCH_SELRE_BTN                41138

// additional controls
#define IDD_PAGESETUP                        42000

#define IDC_PAGESETUP_HEADER_FOOTER_BOX      42006
#define IDC_PAGESETUP_PRINT_COLOR_BOX        42007
#define IDC_PAGESETUP_PRINTER                42008
#define IDC_ZOOM_STATIC                      42009
#define IDC_PREVIEW_STATIC                   42010

#define IDM_FILE_PAGESETUP                   42040
#define IDM_VIEW_HIGHLIGHT_STR               42041
#define IDM_VIEW_HIGHLIGHT_FOLD              42042
#define IDM_EDIT_LINECOMMENT                 42043
#define IDM_EDIT_STREAMCOMMENT               42044
#define IDM_VIEW_HIGHLIGHT_BRACE             42045

// Page Setup
// based on prnsetup.dlg from Windows SDK
#define IDC_PAGESETUP_ZOOMLEVEL_EDIT         0x001e
#define IDC_PAGESETUP_ZOOMLEVEL_CTLS         0x001f
#define IDC_PAGESETUP_HEADER_LIST            0x0020
#define IDC_PAGESETUP_FOOTER_LIST            0x0021
#define IDC_PAGESETUP_COLOR_MODE_LIST        0x0022
#define IDC_PAGESETUP_PAGER_BOX              0x0431
#define IDC_PAGESETUP_SIZE                   0x0441
#define IDC_PAGESETUP_SOURCE_LIST            0x0471
#define IDC_PAGESETUP_SOURCE_LABEL           0x0442
#define IDC_PAGESETUP_ORIENTATION_LIST       0x0472
#define IDC_PAGESETUP_ORIENTATION_BOX        0x0430
#define IDC_PAGESETUP_PORTRAIT               0x0420
#define IDC_PAGESETUP_LANDSCAPE              0x0421
#define IDC_PAGESETUP_MARGIN_BOX             0x0433
#define IDC_PAGESETUP_MARGIN_LEFT_LABEL      0x044e
#define IDC_PAGESETUP_MARGIN_RIGHT_LABEL     0x044f
#define IDC_PAGESETUP_MARGIN_TOP_LABEL       0x0450
#define IDC_PAGESETUP_MARGIN_BOTTOM_LABEL    0x0451
#define IDC_PAGESETUP_MARGIN_LEFT            0x0483
#define IDC_PAGESETUP_MARGIN_RIGHT           0x0484
#define IDC_PAGESETUP_MARGIN_TOP             0x0485
#define IDC_PAGESETUP_MARGIN_BOOTOM          0x0486
#define IDC_PAGESETUP_PREVIEW_WHITE_RECT     0x0438
#define IDC_PAGESETUP_PREVIEW_VERT_RECT      0x0439
#define IDC_PAGESETUP_PREVIEW_HOR_RECT       0x043a

// ids for print
#define IDS_PRINT_HEADER                    42100
#define IDS_PRINT_FOOTER                    42101
#define IDS_PRINT_COLOR                     42102
#define IDS_PRINT_PAGENUM                   42103
#define IDS_PRINT_EMPTY                     42104
#define IDS_PRINT_ERROR                     42105
#define IDS_SELRECT                         42106
#define IDS_PRINT_HEX_WARNS                 42107

#define IDM_THEME_CANCEL                    42200

#define IDM_FORMAT_REFORMAT_JSON            42250
#define IDM_FORMAT_COMPRESS_JSON            42251
#define IDM_FORMAT_WHOLE_FILE               42252
#define IDM_FORMAT_RANGLE_STR               42253
#define IDM_FORMAT_REFORMAT_JS              42254
#define IDM_FORMAT_COMPRESS_JS              42255

#define IDM_LOCALES_BASE                    42300
#define IDR_SYMBOLTREE_REFRESH_POPUPMENU    42400
#define IDR_SYMBOLTREE_TABLE_POPUPMENU      42401
#define IDR_SYMBOLTREE_ROW_POPUPMENU        42402
#define IDM_RELOAD_SYMBOLTREE               42403
#define IDM_SELECT_TABLE_SYMBOLTREE         42404
#define IDM_SELECT_ROW_SYMBOLTREE           42405

#define IDM_FORMAT_RUN_SCRIPT               42406
#define IDM_FORMAT_BYTE_CODE                42407
#define IDS_LUA_CONV_SUCCESS                42408
#define IDS_LUA_CONV_FAIL                   42409
#define IDS_THEME_DESC_DEFAULT              42410
#define IDS_THEME_DESC_BLOCK                42411
#define IDS_THEME_DESC_WHITE                42412

#define IDD_REMOTE_FILESERVERS_DIALOG       42500
#define IDC_REMOTE_FILESERVERS_LISTBOX      42501
#define IDC_ADD_SERVER_BUTTON               42502
#define IDC_REMOVE_SERVER_BUTTON            42503
#define IDC_FILESERVER_NAME_EDIT            42504
#define IDC_COMMPROTOCOL_COMBOBOX           42505
#define IDC_NETWORK_ADDRESS_EDIT            42506
#define IDC_TEST_SERVER_BUTTON              42507
#define IDC_NETWORK_PORT_EDIT               42508
#define IDC_LOGIN_USER_EDIT                 42509
#define IDC_LOGIN_PASS_EDIT                 42510
#define IDC_ACCESS_AREA_COMBOBOX            42511
#define IDM_APPLY_NOW                       42512
#define IDC_LOGIN_PRIVATE_EDIT              42513
#define IDC_PRIVATE_KEY_BUTTON              42514
#define IDC_LOGIN_PASSPHRASE_EDIT           42515
#define IDC_USE_PRIVATE                     42516
#define IDC_PRIVATE_FILE                    42517
#define IDC_PRIVATE_KEY                     42518
#define IDC_REMOTE_PASS                     42519

#define IDM_VIEW_FONTQUALITY_NONE           42550
#define IDM_VIEW_FONTQUALITY_STANDARD       42551
#define IDM_VIEW_FONTQUALITY_CLEARTYPE      42552
#define IDM_SET_RENDER_TECH_GDI             42560
#define IDM_SET_RENDER_TECH_D2D             42561
#define IDM_SET_RENDER_TECH_D2DRETAIN       42562
#define IDM_SETTING_FONTQUALITY             42563
#define IDM_SETTING_RENDER                  42564
#define IDM_SOURCE_SNIPPET_GROUP            42565
#define IDM_EDIT_SELECT_GROUP               42566

#define IDR_TOOLBAR_POPUPMENU               42600
#define IDM_VIEW_MENUBAR                    42601
#define IDM_VIEW_FULLSCREEN                 42602
#define IDM_VIEW_TOOLBAR                    42603
#define IDM_VIEW_STATUSBAR                  42604

#define IDD_QRBOX                           42700
#define IDM_EDIT_QRCODE                     42701
#define IDC_IMG_QR                          42702
#define IDM_UPDATE_SELECTION                42703
#define IDM_OPEN_FILE_PATH                  42704
#define IDM_OPEN_CONTAINING_FOLDER          42705
#define IDM_ONLINE_SEARCH_GOOGLE            42706
#define IDM_ONLINE_SEARCH_BAIDU             42707
#define IDM_ONLINE_SEARCH_BING              42708

#define IDM_EDIT_PLACEHOLDE_JS              42800
#define IDM_EDIT_PLACEHOLDE1                42801
#define IDM_EDIT_PLACEHOLDE2                42802
#define IDM_EDIT_PLACEHOLDE3                42803
#define IDM_EDIT_PLACEHOLDE4                42804
#define IDM_EDIT_PLACEHOLDE5                42805
#define IDM_EDIT_PLACEHOLDE6                42806
#define IDM_EDIT_PLACEHOLDE7                42807
#define IDM_EDIT_PLACEHOLDE8                42808
#define IDM_EDIT_PLACEHOLDE9                42809
#define IDM_EDIT_PLACEHOLDE10               42810
#define IDM_EDIT_PLACEHOLDE11               42811
#define IDM_EDIT_PLACEHOLDE12               42812
#define IDM_EDIT_PLACEHOLDE13               42813
#define IDM_EDIT_PLACEHOLDE14               42814
#define IDM_EDIT_PLACEHOLDE_ZOOM            42815
#define IDM_EDIT_PLACEHOLDE_JSON            42816
#define IDM_EDIT_PLACEHOLDE_CLANG           42817
#define IDM_EDIT_PLACEHOLDE_LUA             42818
#define IDM_EDIT_PLACEHOLDE_ICODE           42819
#define IDM_EDIT_PLACEHOLDE_PROGRAM         42820
#define IDM_EDIT_ASCENDING_SORT             42821
#define IDM_EDIT_DESCENDING_SORT            42822
#define IDM_EDIT_ASCENDING_SORT_IGNORECASE  42823
#define IDM_EDIT_DESCENDING_SORT_IGNORECASE 42824
#define IDM_EDIT_TAB_SPACE                  42825
#define IDM_EDIT_SPACE_TAB                  42826
#define IDM_EDIT_COMMENT_GROUP              42827
#define IDM_EDIT_OTHER_EDITOR               42828
#define IDM_EDIT_OTHER_BCOMPARE             42829

#define IDR_HEXVIEW_MENU                    42830
#define IDM_HEXVIEW_COPY                    42831
#define IDM_HEXVIEW_PASTE                   42832
#define IDM_HEXVIEW_CUT                     42833
#define IDM_HEXVIEW_DEL                     42834
#define IDM_HEXVIEW_INS                     42835
#define IDM_HEXVIEW_COPY_ADDR               42836

#define IDS_THEME_CARET_TIPS                42839
#define IDC_THEME_CARTETLINE_EDT            42840
#define IDC_THEME_INDICATOR_EDT             42841
#define IDC_THEME_CARTETLINE_UDN            42844
#define IDC_THEME_INDICATOR_UDN             42845
#define IDS_THEME_EDIT_TIPS                 42846
#define IDC_THEME_TIPS_STC                  42847
#define IDS_THEME_TIPS                      42848
#define IDS_RE_ERROR                        42849

#define IDM_SELECTION_RECTANGLE             42850
#define IDM_FILE_WORKSPACE                  42851
#define IDM_FILE_MD5_CLIP                   42852
#define IDM_FILE_SHA1_CLIP                  42853
#define IDM_FILE_SHA256_CLIP                42854
#define IDM_TREE_BAR                        42855
#define IDM_TABPAGE_BAR                     42856
#define IDM_TABLE_BAR                       42857
#define IDM_FILE_EXPLORER                   42858

#define IDS_STATUSBAR_TIPS1                 42860
#define IDS_STATUSBAR_TIPS2                 42861
#define IDS_STATUSBAR_TIPS3                 42862
#define IDS_USERTAB_TIPS1                   42863
#define IDS_USER32_ERROR                    42864
#define IDS_THEMEM_WHITE_TO                 42865
#define IDM_PROGRAM_EXECUTE_ACTION          42866
#define IDS_USER32_UNFINISHED               42867
#define IDS_SKYLARK_DONATION                42868
#define IDM_DONATION                        42869

#define IDD_DONATION_BOX                    42880
#define IDC_DONA_STATIC1                    42881
#define IDC_DONA_STATIC2                    42882
#define IDC_DONA_STATIC3                    42883
#define IDC_DONA_STATIC4                    42884
#define IDC_DONA_STATIC5                    42885
#define IDS_EXTRA_PATH                      42886
#define IDC_REGXP_TIPS_STC                  42887
#define IDS_HEXVIEW_BYTES                   42888
#define IDS_RESULT_STRINGS1                 42889
#define IDS_RESULT_STRINGS2                 42890
#define IDS_EDITOR_PATH                     42891
#define IDS_EDITOR_BCOMPARE                 42892

#define IDM_VIEW_LEFT_TAB                   42990
#define IDM_VIEW_RIGHT_TAB                  42991
#define IDM_VIEW_FAR_LEFT_TAB               42992
#define IDM_VIEW_FAR_RIGHT_TAB              42993
#define IDM_VIEW_TIPS_ONTAB                 42994
#define IDM_TAB_CLOSE_LEFT                  42995
#define IDM_TAB_CLOSE_RIGHT                 42996
#define IDC_CURSOR_DRAG                     42997
#define IDS_COMMAND_TITLE                   42998
#define IDS_HELP_COMMAND                    42999
#define IDM_HELP_COMMAND                    43000
#define IDS_LOADLIBRARY_SQL                 43001
#define IDM_TABPAGE_SAVE                    43002
#define IDM_TABPAGE_FULLSCREEN              43003

#define IDM_VIEW_TAB_RIGHT_CLICK            43010
#define IDM_VIEW_TAB_LEFT_DBCLICK           43011

#define IDM_SEARCH_SELECT_HEAD              43020
#define IDM_SEARCH_SELECT_END               43021
#define IDM_SEARCH_MATCHING_BRACE           43022
#define IDM_SEARCH_MATCHING_BRACE_SELECT    43023

#define IDD_VIEWZONE                        43030
#define IDD_VIEWZONE_CLASSIC                43031
#define IDC_VIEWZONE_CANVAS                 43032
#define IDD_DOCUMENTMAP                     43033

#define IDM_SOURCE_SNIPPET_CONFIGURE        44000
#define IDD_SNIPPET_DLG                     44001
#define IDC_SNIPPET_LST                     44002
#define IDC_SNIPPET_EDT1                    44003
#define IDC_SNIPPET_CBO1                    44004
#define IDC_SNIPPET_STC1                    44005
#define IDC_SNIPPET_STC2                    44006
#define IDC_SNIPPET_DELETE                  44007
#define IDC_SNIPPET_NEW                     44008
#define IDC_SNIPPET_BTN_CLOSE               44009
#define IDC_SNIPPET_BTN_APPLY               44010
#define IDS_SNIPPET_EDT_DEFAULT             44011
#define IDS_SNIPPET_EXAMPLE_DEC             44012
#define IDS_SNIPPET_COMBO_DEC               44013
#define IDM_SOURCE_SNIPPET_ENABLE           44014

#define IDS_TABPAGE_CLOSE_NUM               44020
#define IDS_BUTTON_RW_TIPS                  44021
#define IDS_BUTTON_ROUTE_TIPS               44022

#define IDS_PLUGINS_MSG1                    44030

#define IDM_VIEW_TAB_RIGHT_NEW              44040
#define IDM_VIEW_TAB_DBCLICK_NEW            44041

#define MSGBOX_IDICON                       0x0440
#define MSGBOX_IDTEXT                       0xffff

#endif  // _EU_TARGETVER_H_
