/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Mozilla's universal charset detector C/C++ Wrapping API
 *      Writer(s) :
 *          Detect class by John Gardiner Myers <jgmyers@proofpoint.com>
 *          C wrapping API by JoungKyun.Kim <http://oops.org>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef CHARDET_H
#define CHARDET_H

#define LIBCHARDET_MAJOR_VER 1
#define LIBCHARDET_MINOR_VER 0
#define LIBCHARDET_PATCH_VER 6

#define LIBCHARDET_VERSION "1.0.6"
#define LIBCHARDET_UVERSION "001000006"
#define LIBCHARDET_LVERSION 0x01000006

#if defined(CHARDET_SHARED) && (defined(_WIN32) || defined(__CYGWIN__))
#ifdef BUILDING_CHARDET
#define CHARDET_API __declspec(dllexport)
#else
#define CHARDET_API __declspec(dllimport)
#endif
#else
#define CHARDET_API
#endif

#include <stdio.h>
#include <string.h>

#define CHARDET_OUT_OF_MEMORY -128
#define CHARDET_MEM_ALLOCATED_FAIL -127

#define CHARDET_SUCCESS     0
#define CHARDET_NO_RESULT   1
#define CHARDET_NULL_OBJECT 2

// whether to support detect_r and detect_handledata_r API
#define CHARDET_BINARY_SAFE 1

// whether to support bom member of DetectObj structure
#define CHARDET_BOM_CHECK 1

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct Detect_t Detect;

	typedef struct DetectObject {
		char * encoding;
		float confidence;
		short bom;
	} DetectObj;

	CHARDET_API char * detect_version (void);
	CHARDET_API char * detect_uversion (void);

	CHARDET_API DetectObj * detect_obj_init (void);
	CHARDET_API void detect_obj_free (DetectObj **);

	CHARDET_API Detect * detect_init (void);
	CHARDET_API void detect_reset (Detect **);
	CHARDET_API void detect_dataend (Detect **);
	CHARDET_API short detect_handledata (Detect **, const char *, DetectObj **);
	CHARDET_API short detect_handledata_r (Detect **, const char *, size_t, DetectObj **);
	CHARDET_API void detect_destroy (Detect **);
	CHARDET_API short detect (const char *, DetectObj **);
	CHARDET_API short detect_r (const char *, size_t, DetectObj **);
#ifdef __cplusplus
};
#endif

#endif // close define CHARDET_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
