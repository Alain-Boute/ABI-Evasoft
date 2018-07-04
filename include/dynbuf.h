/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dynbuf.h
** Description : dynamic buffer structure definition & macros
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include <stdio.h>
#include <string.h>
#include "mem_utils.h"
#include "mem_macros.h"
#include "dynbuf_sr.h"

/*********************************************************************
** Typedef : DynBuffer
** Description : dynamic buffer allocation structure
*********************************************************************/
typedef struct _DynBuffer
{
	size_t sz;					/* data size */
	size_t cnt;					/* data used */
	char data[2];				/* buffer data (more is alloc-ed) */
}
	DynBuffer;

/*********************************************************************
** Function : dynbuf_init
** Description : return an initialized DynBuffer structure
*********************************************************************/
DynBuffer *dynbuf_init(		/* return : pointer on success, NULL on error */
	size_t sz				/* in : initial size of the buffer data */
);

/*********************************************************************
** Function : dynbuf_add
** Description : add text to a DynBuffer structure
**				 handles initialization & resize
*********************************************************************/
int dynbuf_add(				/* return : 0 if Ok, other on error */
	DynBuffer **buf,		/* in : original DynBuffer pointer or NULL
							   out : possibly realloc-ed DynBuffer pointer */
	char *txt,				/* in : text to add to buffer */
	size_t sz,				/* in : # of chars to add from txt (0 = use strlen) */
	ReplaceTable *sr,		/* in : search & replace table */
	int srmode				/* in : # of elements in sr */
);

/*********************************************************************
** Function : dynbuf_add_int
** Description : add integer to a DynBuffer structure
*********************************************************************/
int dynbuf_add_int(			/* return : 0 if Ok, other on error */
	DynBuffer **buf,		/* in : original DynBuffer pointer or NULL
							   out : possibly realloc-ed DynBuffer pointer */
	unsigned long nb		/* in : number to add */
);

/*********************************************************************
** Function : dynbuf_add3
** Description : add delimited text to a DynBuffer structure
*********************************************************************/
int dynbuf_add3(			/* return : 0 if Ok, other on error */
	DynBuffer **buf,		/* in : original DynBuffer pointer or NULL
							   out : possibly realloc-ed DynBuffer pointer */
	char *txt0,				/* in : delimiter before */
	size_t sz0,				/* in : # of chars to add from txt0 (0 = use strlen) */
	char *txt,				/* in : text to add to buffer */
	size_t sz,				/* in : # of chars to add from txt (0 = use strlen) */
	ReplaceTable *sr,		/* in : search & replace table for txt */
	int srmode,				/* in : # of elements in sr */
	char *txt1,				/* in : delimiter afer */
	size_t sz1				/* in : # of chars to add from txt1 (0 = use strlen) */
);

/*********************************************************************
** Function : dynbuf_add3_int
** Description : add delimited text to a DynBuffer structure
*********************************************************************/
int dynbuf_add3_int(		/* return : 0 if Ok, other on error */
	DynBuffer **buf,		/* in : original DynBuffer pointer or NULL
							   out : possibly realloc-ed DynBuffer pointer */
	char *txt0,				/* in : delimiter before */
	size_t sz0,				/* in : # of chars to add from txt0 (0 = use strlen) */
	unsigned long nb,		/* in : number to add */
	char *txt1,				/* in : delimiter afer */
	size_t sz1				/* in : # of chars to add from txt1 (0 = use strlen) */
);

/*********************************************************************
** Macro : add_sz_str
** Description : stands for string const and string const length
*********************************************************************/
#define add_sz_str(_obj) (_obj), sizeof(_obj)-1

/*********************************************************************
** Macro : nbelem
** Description : return # of elements in a fixed array
*********************************************************************/
#define nbelem(_obj) (sizeof(_obj) / sizeof(_obj[0]))

/*********************************************************************
** Macro : DYNBUF_ADD
** Description : add a string in a DynBuffer with error handling
*********************************************************************/
#define DYNBUF_ADD(buf, txt, len, conv) { if(dynbuf_add(buf, txt, len, conv)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD_STR
** Description : add a static string to a buffer
*********************************************************************/
#define DYNBUF_ADD_STR(buf, txt) { if(dynbuf_add(buf, add_sz_str(txt), NO_CONV)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD_BUF
** Description : add a DynBuffer to a buffer
*********************************************************************/
#define DYNBUF_ADD_BUF(buf, buf1, conv) { if(buf1 && dynbuf_add(buf, (buf1)->data, (buf1)->cnt, conv)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD_INT
** Description : add an integer value to a buffer
*********************************************************************/
#define DYNBUF_ADD_INT(buf, nb) { if(dynbuf_add_int(buf, nb)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD3_INT
** Description : add an integer surrounded by 2 static strings
*********************************************************************/
#define DYNBUF_ADD3_INT(buf, txt1, nb, txt2)  {  if(dynbuf_add3_int(buf, add_sz_str(txt1), nb, add_sz_str(txt2))) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_PRINTF
** Description : add formatted value to a buffer
*********************************************************************/
#define DYNBUF_PRINTF(dynbuf, maxlen, fmt, value, conv) { char buf[maxlen+1] = {0}; \
    if(dynbuf_add(dynbuf, buf, snprintf(buf, maxlen, fmt, value), conv)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : dynbuf_printf
** Description : add formatted value to a buffer using local buf buffer
*********************************************************************/
#define _dynbuf_print(_code) { printbuf[sizeof(printbuf)-1] = 0; dynbuf_add(dynbuf, printbuf, _code, NO_CONV); }
#define dynbuf_print(dynbuf, fmt, v1) dynbuf_add(dynbuf, printbuf, snprintf(printbuf, sizeof(printbuf)-1, fmt, v1), NO_CONV)
#define dynbuf_print2(dynbuf, fmt, v1, v2) dynbuf_add(dynbuf, printbuf, snprintf(printbuf, sizeof(printbuf)-1, fmt, v1, v2), NO_CONV)
#define dynbuf_print3(dynbuf, fmt, v1, v2, v3) dynbuf_add(dynbuf, printbuf, snprintf(printbuf, sizeof(printbuf)-1, fmt, v1, v2, v3), NO_CONV)
#define dynbuf_print4(dynbuf, fmt, v1, v2, v3, v4) dynbuf_add(dynbuf, printbuf, snprintf(printbuf, sizeof(printbuf)-1, fmt, v1, v2, v3, v4), NO_CONV)
#define dynbuf_print5(dynbuf, fmt, v1, v2, v3, v4, v5) dynbuf_add(dynbuf, printbuf, snprintf(printbuf, sizeof(printbuf)-1, fmt, v1, v2, v3, v4, v5), NO_CONV)
#define dynbuf_print6(dynbuf, fmt, v1, v2, v3, v4, v5, v6) dynbuf_add(dynbuf, printbuf, snprintf(printbuf, sizeof(printbuf)-1, fmt, v1, v2, v3, v4, v5, v6), NO_CONV)

/*********************************************************************
** Macro : DYNBUF_ADD_CELL
** Description : add a DynTable cell to a buffer
*********************************************************************/
#define DYNBUF_ADD_CELL(buf, tab, row, col, conv) { if(dynbuf_add(buf, DYNTAB_VAL_SZ(tab, row, col), conv)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD_CELLP
** Description : add a DynTable cell to a buffer
*********************************************************************/
#define DYNBUF_ADD_CELLP(buf, cell, conv) { DynTableCell *_c = cell; if(_c && dynbuf_add(buf, _c->txt, _c->len, conv)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD3
** Description : add a string surrounded by 2 static strings
*********************************************************************/
#define DYNBUF_ADD3(buf, txt1, val, sz, conv, txt2) {\
	if(dynbuf_add3(buf, add_sz_str(txt1), val, sz, conv, add_sz_str(txt2))) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD3_BUF
** Description : add a DynBuffer surrounded by 2 static strings
*********************************************************************/
#define DYNBUF_ADD3_BUF(buf, txt1, buf1, conv, txt2) { \
if(dynbuf_add3(buf, add_sz_str(txt1), (buf1) ? (buf1)->data : NULL, (buf1) ? (buf1)->cnt : 0, conv, add_sz_str(txt2))) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD3_CELL
** Description : add a DynTable cell surrounded by 2 static strings
*********************************************************************/
#define DYNBUF_ADD3_CELL(buf, txt1, tab, l, c, conv, txt2) {\
	if(dynbuf_add3(buf, add_sz_str(txt1), DYNTAB_VAL_SZ(tab, l, c), conv, add_sz_str(txt2))) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD3_CELLP
** Description : add a DynTable cell surrounded by 2 static strings
*********************************************************************/
#define DYNBUF_ADD3_CELLP(buf, txt1, cell, conv, txt2) { DynTableCell *_c = cell; \
	if(dynbuf_add3(buf, add_sz_str(txt1), _c ? _c->txt : NULL, _c ? _c->len : 0, conv, add_sz_str(txt2))) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_ADD3_CGI
** Description : add a CGI item (name or value) surrounded by 2 static strings
*********************************************************************/
#define DYNBUF_ADD3_CGI(buf, txt1, ival, item, conv, txt2) {\
	if(dynbuf_add3(buf, add_sz_str(txt1), cntxt->cgi[ival].item, cntxt->cgi[ival].item##_sz, conv, add_sz_str(txt2))) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : DYNBUF_VAL_SZ
** Description : add an integer surrounded by 2 static strings
*********************************************************************/
#define DYNBUF_VAL_SZ(buf) (buf) ? (buf)->data : NULL, (buf) ? (buf)->cnt : 0

/*********************************************************************
** Macro : STRCMPNUL
** Description : wrapper for strcmp that supports null strings
*********************************************************************/
#define STRCMPNUL(a,b) strcmp((a) ? (a) : "",(b) ? (b) : "")
#define STRICMPNUL(a,b) stricmp((a) ? (a) : "",(b) ? (b) : "")

/*********************************************************************
** Function : dynbuf_strip
** Description : handle stripping of a DynBuffer structure
*********************************************************************/
int dynbuf_strip(			/* return : 0 if not stripped, 1 else */
	DynBuffer **_buf,		/* in/out : buffer to strip */
	size_t _cnt0,			/* in : initial size of the buffer data */
	size_t _maxsize,		/* in : stripped size of the buffer data */
	size_t *_counter		/* in/out : output counter */
);

/*********************************************************************
** Macro : DYNBUF_HANDLE_STRIP
** Description : handle stripped output to buffer
*********************************************************************/
#define DYNBUF_HANDLE_STRIP(_buf, _maxsize, _counter, _bstripped, _outputcode) if(_buf) { \
	size_t _cnt0 = *_buf ? (*_buf)->cnt : 0; \
	_outputcode; \
	_bstripped = dynbuf_strip(_buf, _cnt0, _maxsize, &_counter); }
