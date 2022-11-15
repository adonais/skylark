/******************************************************************************
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

#ifndef _EU_RESOURCE_H_
#define _EU_RESOURCE_H_

#ifndef IDC_STATIC
#define IDC_STATIC -1
#endif

#define IDI_SKYLARK                     2
#define IDI_SMALL                       3
#define IDI_DEFAULT                     4
#define IDC_CURSOR_WE                   8
#define IDC_CURSOR_NS                   9
#define IDD_SKYLARK_DIALOG              102
#define IDS_APP_TITLE                   103
#define IDD_ABOUTBOX                    103
#define IDM_ABOUT                       104
#define IDD_SEARCH_REPLACE              104
#define IDD_SEARCH_REPLACE_DIALOG       104
#define IDM_EXIT                        105

#define IDC_SKYLARK                     109
#define IDB_NEWFILE_C                   110
#define IDB_NEWFILE_D                   111
#define IDB_OPENFILE_C                  112
#define IDB_OPENFILE_D                  113
#define IDB_SAVEFILE_C                  114
#define IDB_SAVEFILE_D                  115
#define IDB_SAVEFILEAS_C                116
#define IDB_SAVEFILEAS_D                117
#define IDB_CUTTEXT_C                   118
#define IDB_CUTTEXT_D                   119
#define IDB_COPYTEXT_C                  120
#define IDB_COPYTEXT_D                  121
#define IDB_PASTETEXT_C                 122
#define IDB_PASTETEXT_D                 123
#define IDR_MAINFRAME                   128
#define IDB_CLSDFOLD                    130
#define IDB_DOC                         131
#define IDB_DRIVE                       132
#define IDB_EXE                         133
#define IDB_OPENFOLD                    134
#define IDB_TXT                         135
#define IDR_SYMBOLLIST_POPUPMENU        136
#define IDR_FILETREE_FILE_POPUPMENU     137

#define IDR_TABPAGE_POPUPMENU           140
#define IDR_EDITOR_POPUPMENU            141
#define IDD_STYLETHEME_DIALOG           142
#define IDR_FILETREE_DIR_POPUPMENU      143
#define IDD_SEARCH_FOUND_DIALOG         144
#define IDB_BITMAP1                     146
#define IDB_FIND_C                      147
#define IDB_FIND_D                      148
#define IDB_FINDPREV_C                  149
#define IDB_FINDPREV_D                  150
#define IDB_FINDNEXT_C                  151
#define IDB_FINDNEXT_D                  152
#define IDB_STYLETHEME_C                153
#define IDB_STYLETHEME_D                154
#define IDB_ZOOMOUT_C                   155
#define IDB_ZOOMOUT_D                   156
#define IDB_ZOOMIN_C                    157
#define IDB_ZOOMIN_D                    158
#define IDB_ABOUT_C                     159
#define IDB_ABOUT_D                     160
#define IDB_SAVEALLFILES_C              161
#define IDB_SAVEALLFILES_D              162
#define IDB_CLOSEFILE_C                 163
#define IDB_CLOSEFILE_D                 164
#define IDB_REMOTEFILESERVERS_C         165
#define IDB_REMOTEFILESERVERS_D         166
#define IDB_UNDO_C                      167
#define IDB_UNDO_D                      168
#define IDB_REDO_C                      169
#define IDB_REDO_D                      170
#define IDB_FOUNDLIST_C                 171
#define IDB_FOUNDLIST_D                 172
#define IDB_REPLACE_C                   173
#define IDB_REPLACE_D                   174
#define IDB_TOGGLE_BOOKMARK_C           175
#define IDB_TOGGLE_BOOKMARK_D           176
#define IDB_GOTO_PREV_BOOKMARK_C        177
#define IDB_GOTO_PREV_BOOKMARK_D        178
#define IDB_GOTO_NEXT_BOOKMARK_C        179
#define IDB_GOTO_NEXT_BOOKMARK_D        180
#define IDB_NAVIGATEBACK_C              181
#define IDB_NAVIGATEBACK_D              182
#define IDB_FILETREE_C                  183
#define IDB_FILETREE_D                  184
#define IDB_HEXEDITMODE_C               185
#define IDB_HEXEDITMODE_D               186

#define IDC_SEARCH_TEXT                 1000
#define IDC_SEARCH_TEXT2                1001
#define IDC_TEXTTYPE_GENERAL            1002
#define IDC_IDC_TEXTTYPE_REGEXP         1003
#define IDC_TEXTTYPE_REGEXP             1003
#define IDC_OPTIONS_WHOLEWORD           1004
#define IDC_TEXTTYPE_POSIXREGEXP        1005
#define IDC_AREATYPE_THISFILE           1007
#define IDC_AREATYPE_SELECTSECTION      1008
#define IDC_SEARCH_FROMTEXT             1009
#define IDC_AREATYPE_ALLOPENEDFILES     1010
#define IDC_AREATYPE_ALLFILES           1011
#define IDC_SEARCHLOCATE_LOOP           1011
#define IDC_TEXTTYPE_POSIX              1012
#define IDC_AREATYPE_ALLOPENEDFILES3    1012
#define IDC_AREATYPE_CURRENTDIRECTORY   1012
#define IDC_OPTIONS_MATCHCASE           1013
#define IDC_OPTIONS_WORDSTART           1014
#define IDC_SEARCH_REPLACETEXT          1015
#define IDC_SEARCH_TOTEXT               1016

#define IDC_SETFONT_KEYWORD_BUTTON      1032
#define IDC_SETFONT_KEYWORDS_BUTTON     1033
#define IDC_KEYWORDS_STATIC             1034
#define IDC_KEYWORDS2_STATIC            1035
#define IDC_STRING_STATIC               1036
#define IDC_CHARACTER_STATIC            1037
#define IDC_NUMBER_STATIC               1038
#define IDC_PREPROCESSOR_STATIC         1039
#define IDC_COMMENT_STATIC              1040
#define IDC_SETTEXTCOLOR_KEYWORD_BTN    1041
#define IDC_SETTEXTCOLOR_KEYWORDS_BTN   1042
#define IDC_SETFONT_KEYWORDS2_BTN       1044
#define IDC_SETTEXTCOLOR_KEYWORDS2_BTN  1045
#define IDC_SETFONT_STRING_BTN          1047
#define IDC_SETTEXTCOLOR_STRING_BTN     1048
#define IDC_SETFONT_CHARACTER_BTN       1049
#define IDC_SETTEXTCOLOR_CHARACTER_BTN  1050
#define IDC_COMMENTLINE_STATIC          1051
#define IDC_COMMENTDOC_STATIC           1052
#define IDC_SETFONT_NUMBER_BTN          1053
#define IDC_SETTEXTCOLOR_NUMBER_BTN     1054
#define IDC_SETFONT_PREPRO_BTN          1055
#define IDC_SETTEXTCOLOR_PREPRO_BTN     1056
#define IDC_KEYWORDS_STATIC2            1057
#define IDC_TEXT_STATIC                 1058
#define IDC_SETFONT_TEXT_BTN            1059
#define IDC_SETFONT_COMMENT_BTN         1060
#define IDC_SETTEXTCOLOR_COMMENT_BTN    1061
#define IDC_SETFONT_COMMENTLINE_BTN     1062
#define IDC_SETTEXTCOLOR_COMMENTL_BTN   1063
#define IDC_SETTEXTCOLOR_TEXT_BTN       1064
#define IDC_SETDEFAULT_BUTTON           1065
#define IDC_SETFONT_COMMENTDOC_BTN      1066
#define IDC_SETTEXTCOLOR_COMMENTDOC_BTN 1067
#define IDC_OPERATOR_STATIC             1068
#define IDC_SETFONT_OPERATOR_BTN        1069
#define IDC_SETTEXTCOLOR_OPERATOR_BTN   1070
#define IDC_CARETLINE_STATIC            1071
#define IDC_SETBGCOLOR_TEXT_BTN         1072
#define IDC_SETTEXTCOLOR_CARETLINE_BTN  1073
#define IDC_SETBGCOLOR_CARETLINE_BTN    1073
#define IDC_INDICATOR_STATIC            1074
#define IDC_SETBGCOLOR_INDICATOR_BTN    1075
#define IDC_LINENUMBER_STATIC           1076
#define IDC_SETTEXTCOLOR_LINENUMBER_BTN 1077
#define IDC_SETBGCOLOR_LINENUMBER_BTN   1078
#define IDC_FOLDMARGIN_STATIC           1079
#define IDC_SETBGCOLOR_FOLDMARGIN_BTN   1080

#define IDC_UNKNOWTAGS_STATIC           1094
#define IDC_ATTRIBUTES_STATIC           1095
#define IDC_ATTRIBUTE_STATIC            1095
#define IDC_UNKNOWATTRIBUTES_STATIC     1096
#define IDC_ENTITIES_STATIC             1097
#define IDC_PHPSTART_STATIC             1098
#define IDC_PHPSECTION_STATIC           1098
#define IDC_TAGENDS_STATIC              1099
#define IDC_PHPEND_STATIC               1100
#define IDC_ASPEND_STATIC               1101
#define IDC_ASPSTART_STATIC             1102
#define IDC_ASPSECTION_STATIC           1102
#define IDC_SETFONT_UNKNOWTAGS_BTN      1103
#define IDC_SETTEXTCOLOR_UNKNOWTAGS_BTN 1104
#define IDC_SETFONT_ATTRIBUTES_BTN      1105
#define IDC_SETTEXTCOLOR_ATTRIBUTES_BTN 1106
#define IDC_SETFONT_UNATTRIBUTES_BTN    1107
#define IDC_SETTEXTCOLOR_UNATTRS_BTN    1108
#define IDC_SETFONT_ENTITIES_BTN        1109
#define IDC_SETTEXTCOLOR_ENTITIES_BTN   1110
#define IDC_SETFONT_TAGENDS_BTN         1111
#define IDC_SETTEXTCOLOR_TAGENDS_BTN    1112
#define IDC_SETFONT_PHPSTART_BTN        1113
#define IDC_SETFONT_PHPSECTION_BTN      1113
#define IDC_SETTEXTCOLOR_PHPSECTION_BTN 1114
#define IDC_SETFONT_PHPEND_BUTTON       1115
#define IDC_SETTEXTCOLOR_PHPEND_BTN     1116
#define IDC_SETFONT_ASPSECTION_BTN      1117
#define IDC_SETTEXTCOLOR_ASPSECTION_BTN 1118
#define IDC_TAGS_STATIC                 1121
#define IDC_SETFONT_TAGS_BTN            1122
#define IDC_SETTEXTCOLOR_TAGS_BTN       1123
#define IDC_CDATA_STATIC                1124
#define IDC_SETFONT_CDATA_BTN           1125
#define IDC_SETTEXTCOLOR_CDATA_BTN      1126

#define IDC_THEME_LANGUAGE_STATIC       1130
#define IDC_THEME_MARKUP_STATIC         1131
#define IDC_THEME_EDIT_STATIC           1132
#define IDC_THEME_MARGIN_STATIC         1133
#define IDC_THEME_TAB_STATIC            1134
#define IDC_SETBGCOLOR_TAB_BTN          1135

#define IDT_BIRD                        1140

#endif  // _EU_RESOURCE_H_
