/* Copyright (c) 2003, 2016, Oracle and/or its affiliates. 
All rights reserved.*/
 
/* 
   NAME 
     ocixml.h - OCIXMLType functions 

   DESCRIPTION 
     This file contains all OCIXMLType functions defined in ocixml.c 


     ****************************IMPORTANT***********************************
     *** If you change the signatures of any fucntions in this file, make sure
     *** to make same changes to Windows OSD file ociclnt.c. Otherwise, users
     *** of OCI instant client like ODP.NET will have build issues or crashes
     ****************************IMPORTANT***********************************

   PUBLIC FUNCTION(S) 
     OCIXMLTypeNew()
     OCIXMLTypeCreateFromSrc()
     OCIXMLTypeCreateFromSrcWithSchema()
     OCIXMLTypeTransform()
     OCIXMLTypeExtract()
     OCIXMLTypeIsSchemaBased()
     OCIXMLTypeValidate()
     OCIXMLTypeExists()
     OCIXMLTypeGetDOM()
     OCIXMLTypeGetFromDOM()
     OCIDOMFree()
     OCIXMLSEMutexAcq()
     OCIXMLSEMutexRel()
     OCIXMLUpdateNodeValues()

   INTERNAL FUNCTION(S)

   EXAMPLES

   NOTES

   MODIFIED   (MM/DD/YY)
   joalvizo    09/06/16 - add OCIXMLTypeIsBinXml. 
   yinlu       07/26/12 - remove xml0.h, a private header
   spetride    03/02/09 - add isdoc to OCIXMLTypeCreateFromSrc*
   bsthanik    01/17/07 - 5753599: wrappers for service mutex acq/rel
   bkhaladk    05/10/06 - add setpicklepref 
   nitgupta    01/30/06 - add signatures for OCIBinXMl*
   dmukhin     06/16/05 - ANSI prototypes; miscellaneous cleanup 
   dmukhin     06/14/05 - ANSI prototypes; miscellaneous cleanup 
   ataracha    12/04/03 - convert public oci api to ansi
   ataracha    01/21/03 - ataracha_uni_capi_cleanup
   ataracha    01/08/03 - Creation

*/
#ifndef OCI_ORACLE
# include <oci.h>
#endif

#ifndef XML_ORACLE
#include <xml.h>
#endif

#ifndef OCIXML_ORACLE
# define OCIXML_ORACLE

/*---------------------------------------------------------------------------
                     PUBLIC TYPES AND CONSTANTS
  ---------------------------------------------------------------------------*/

/* parameters for OCIXMLUpdateNodeValues */
struct OCIXMLunv
{
  void * xpth_OCIXMLunv;               /* xpath expression */
  void * val_OCIXMLunv;                /* value - string literal or xmltype */
  ub4    xpthL_OCIXMLunv;              /* length of xpath expression string */
  ub4    valL_OCIXMLunv;               /* length of value string */
  ub1    tp_OCIXMLunv;                 /* type of value - xmltype of string */
  
#define OCIXMLUNV_XTP    0x00          /* param is xmltype */
#define OCIXMLUNV_STP    0x01          /* param is string literal */
#define OCIXMLUNV_STM    0x02          /* param is a stream (kghsstream *) */
#define OCIXMLUNV_CLOB   0x03          /* param is a CLOB locator (kolblc *) */
#define OCIXMLUNV_BLOB   0x04          /* param is a BLOB locator (kolblc *) */
};
typedef struct OCIXMLunv OCIXMLunv;

/*---------------------------------------------------------------------------
                     PRIVATE TYPES AND CONSTANTS
  ---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
                           EXPORT FUNCTIONS
  ---------------------------------------------------------------------------*/
sword  OCIXMLTypeNew(OCISvcCtx *svchp, OCIError *errhp, OCIDuration dur,
                     OraText *elname, ub4 elname_Len,
                     OraText *schemaURL, ub4 schemaURL_Len,
                     OCIXMLType **retInstance);

sword  OCIXMLTypeCreateFromSrc(OCISvcCtx *svchp, OCIError *errhp,
                               OCIDuration dur, ub1 src_type, void  *src_ptr,
                               sb4 ind, OCIXMLType **retInstance, ub4 csid);
sword  OCIXMLTypeCreateFromSrcInt(OCISvcCtx *svchp, OCIError *errhp,
                               OCIDuration dur, ub1 src_type, void  *src_ptr,
                               sb4 ind, OCIXMLType **retInstance, ub4 csid,
                               boolean isdoc);

sword  OCIXMLTypeCreateFromSrcWithSchema(OCISvcCtx *svchp, OCIError *errhp,
                     OCIDuration dur, ub1 src_type, void  *src_ptr,
                     sb4 ind, OraText *schemaURL, ub4 schemaURL_Len,
                     boolean wellformed, boolean valid,
                     OCIXMLType **retInstance, ub4 csid);
sword  OCIXMLTypeCreateFromSrcWithSchemaInt(OCISvcCtx *svchp, OCIError *errhp,
                     OCIDuration dur, ub1 src_type, void  *src_ptr,
                     sb4 ind, OraText *schemaURL, ub4 schemaURL_Len,
                     boolean wellformed, boolean valid,
                     OCIXMLType **retInstance, ub4 csid,
                     boolean isdoc);

sword OCIXMLTypeExtract(OCIError *errhp,
              OCIXMLType *doc, OCIDuration dur,
              OraText *xpathexpr, ub4 xpathexpr_Len,
              OraText *nsmap, ub4 nsmap_Len,
              OCIXMLType **retDoc);

sword OCIXMLTypeTransform(OCIError *errhp, OCIDuration dur,
               OCIXMLType *doc, OCIXMLType *xsldoc,
               OCIXMLType **retDoc);

/* Note: xpathexpr is case sensitive */
sword OCIXMLTypeExists(OCIError *errhp, OCIXMLType *doc,
                 OraText *xpathexpr, ub4 xpathexpr_Len,
                 OraText *nsmap, ub4 nsmap_Len,
                 boolean *retval);

sword OCIXMLTypeIsSchemaBased(OCIError *errhp,
                              OCIXMLType *doc, boolean *retval);

sword OCIXMLTypeIsFragment(OCIError *errhp, OCIXMLType *doc, boolean *retval);

sword OCIXMLTypeGetSchema(OCIError *errhp, OCIXMLType *doc,
             OCIXMLType **schemadoc,
             OraText **schemaURL, ub4 *schemaURL_Len,
             OraText **rootelem, ub4 *rootelem_Len);

sword OCIXMLTypeValidate(OCIError *errhp, OCIXMLType *doc,
                   OraText *schemaURL, ub4 schemaURL_Len, boolean *retval);

sword OCIXMLTypeGetDOM(OCIError *errhp, OCIXMLType *doc, OCIDuration dur,
                       OCIDOMDocument **retDom);

sword OCIXMLTypeGetFromDOM(OCIError *errhp, OCIDOMDocument *domdoc,
                           OCIXMLType **retXMLType);

sword OCIXMLTypeGetNS(OCIError *errhp, OCIXMLType *domdoc,
                      OraText **ns, ub4 *ns_len);

sword OCIDOMFree(OCIError *errhp, OCIDOMDocument *domdoc);

sword OCIBinXmlCreateReposCtxFromConn(OCIEnv *env, OCISvcCtx *svcctx,
                                      OCIError *err, OCIBinXmlReposCtx **ctx);
sword OCIBinXmlCreateReposCtxFromCPool(OCIEnv *env, OCICPool *cpool,
                                       OCIError *err, OCIBinXmlReposCtx **ctx);
sword OCIBinXmlSetReposCtxForConn(OCISvcCtx *dataconn,
                                  OCIBinXmlReposCtx *reposctx);

sword OCIXMLTypeIsBinXml(OCIError *errhp,
                         OCIXMLType *doc, boolean *retval);

#define OCIXML_FORMATTYPE_TEXT   0
#define OCIXML_FORMATTYPE_BINXML 1

sword OCIBinXmlSetFormatPref(xmldocnode *doc, ub4 format);

/* OCI Wrapper to acquire mutex associated with service handle and 
 * env handle 
 */
sword OCIXMLSEMutexAcq(OCISvcCtx *svchp, OCIError *errhp);

/* release wrapper corresponding to OCIXMLSEMutexAcq */
sword OCIXMLSEMutexRel(OCISvcCtx *svchp, OCIError *errhp);

/* acquires OCI svc and env mutexes, updates values of nodes pointed to by
 * given XPATH locations, and releases mutexes.
 */
sword OCIXMLUpdateNodeValues(OCISvcCtx *svchp, OCIError *errhp, OCIXMLType 
              **docp, struct OCIXMLunv *values, ub4 numvalues, oratext *nsmap, 
                ub4 nsmapl);
/*---------------------------------------------------------------------------
                          INTERNAL FUNCTIONS
  ---------------------------------------------------------------------------*/


#endif                                              /* OCIXML_ORACLE */
