/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : buf_utils.c
** Description : dynamic buffers & tables handling functions
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include <stdlib.h>
#include "dynbuf.h"

/*********************************************************************
** Function : dynbuf_init
** Description : return an initialized DynBuffer structure
*********************************************************************/
DynBuffer *dynbuf_init(		/* return : pointer on success, NULL on error */
	size_t sz				/* in : initial size of the buffer data */
){
	DynBuffer *buf = mem_alloc(sizeof(DynBuffer) + sz);
	if(!buf) return NULL;
	memset(buf, 0, sizeof(DynBuffer) + sz);
	buf->sz = sz;
	buf->data[0] = 0;
	buf->cnt = 0;
	return buf;
}

/*********************************************************************
** Function : dynbuf_resize
** Description : resize a DynBuffer structure
*********************************************************************/
int dynbuf_resize(			/* return : 0 on success, other on error */
	DynBuffer **_buf,		/* in : original DynBuffer pointer (freed by dynbuf_resize)
							   out : new DynBuffer pointer (alloc-ed by dynbuf_resize) */
	size_t sz				/* in : desired size of the buffer data */
){
	DynBuffer *buf;
	size_t newsz, oldsz;

	/* Check params */
	if(!_buf || !*_buf) return 1;
	buf = *_buf;
	oldsz = buf ? buf->sz + sizeof(DynBuffer) + 1 : 0;
	newsz = (buf->sz + sz +sizeof(DynBuffer)) * 3 / 2;
	buf = mem_realloc(buf, oldsz, newsz - oldsz);
	if(!buf) return 1;
	*_buf = buf;
	buf->sz = newsz - sizeof(DynBuffer) - 1;
	return 0;
}

/*********************************************************************
** Function : dynbuf_strip
** Description : handle stripping of a DynBuffer structure
*********************************************************************/
int dynbuf_strip(			/* return : 0 if not stripped, 1 else */
	DynBuffer **buf,		/* in/out : buffer to strip */
	size_t cnt0,			/* in : initial size of the buffer data */
	size_t maxsize,			/* in : stripped size of the buffer data */
	size_t *counter			/* in/out : output counter */
){
	int bstripped = 0;
	if(!buf || !*buf) return 0;
	if(maxsize && *buf && (*buf)->cnt - cnt0 > maxsize - *counter) 
	{ 
		(*buf)->cnt = cnt0 + (*counter ? 0 : maxsize);
		dynbuf_add(buf, add_sz_str("..."), NO_CONV); 
		bstripped = 1;
	} 
	*counter += (*buf)->cnt - cnt0;
	return bstripped;
}

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
	int sz_sr				/* in : search & replace direction (1=direct, 0=none, -1=reverse) */
){
	/* Check params */
	if(!buf || !txt) return 0;
	if(!sz || sz == ~0UL) sz = strlen(txt);

	/* Initialize if structure not alloc-ed - return on error */
	if(!*buf) *buf = dynbuf_init(max(sz+2, 256));
	if(!*buf) return 1;

/* This macro allows to permute search & replace members of the ReplaceTable struct */
#define SR_LOOP(search, replace) { \
		size_t i, k; \
		int j; \
		/* Search, replace & add loop */ \
		for(i = 0, k = (*buf)->cnt; i < sz; i++, k++) \
		{ \
			for(j = 0; sr[j].search; j++) \
				if(!memcmp(txt + i, sr[j].search, sr[j].sz_##search)) break; \
			if(sr[j].search) \
			{ \
				if(k + sr[j].sz_##replace >= (*buf)->sz) if(dynbuf_resize(buf, sz)) return 1; \
				memcpy((*buf)->data + k, sr[j].replace, sr[j].sz_##replace); \
				i += sr[j].sz_##search - 1; \
				k += sr[j].sz_##replace - 1; \
			} \
			else \
			{ \
				if(k + 1 >= (*buf)->sz) if(dynbuf_resize(buf, sz)) return 1; \
				(*buf)->data[k] = txt[i]; \
			} \
		} \
		(*buf)->cnt = k; \
	} 

	/* If table search & replace given */
	if(sr && sz_sr > 0)
		SR_LOOP(search, replace)
	else if(sr && sz_sr < 0)
		SR_LOOP(replace, search)
	else
	{
		/* Resize buffer if needed - return on error */
		if(sz + 1 >= (*buf)->sz - (*buf)->cnt) if(dynbuf_resize(buf, sz)) return 1;

		/* Add text to buffer */
		memcpy((*buf)->data + (*buf)->cnt, txt, sz);
		(*buf)->cnt += sz;
	}
#undef SR_LOOP

	(*buf)->data[(*buf)->cnt] = 0;
	return 0;
}

/*********************************************************************
** Function : dynbuf_add_int
** Description : add integer to a DynBuffer structure
*********************************************************************/
int dynbuf_add_int(			/* return : 0 if Ok, other on error */
	DynBuffer **buf,		/* in : original DynBuffer pointer or NULL
							   out : possibly realloc-ed DynBuffer pointer */
	unsigned long nb		/* in : number to add */
){
	char tmp[32];
	return dynbuf_add(buf, tmp, sprintf(tmp, "%ld", nb), NULL, 0);
}

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
	int sz_sr,				/* in : search & replace direction (1=direct, 0=none, -1=reverse) */
	char *txt1,				/* in : delimiter afer */
	size_t sz1				/* in : # of chars to add from txt1 (0 = use strlen) */
){
	return
		dynbuf_add(buf, txt0, sz0, NULL, 0)	||
		dynbuf_add(buf, txt, sz, sr, sz_sr) ||
		dynbuf_add(buf, txt1, sz1, NULL, 0);
}

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
){
	return
		dynbuf_add(buf, txt0, sz0, NO_CONV)	||
		dynbuf_add_int(buf, nb) ||
		dynbuf_add(buf, txt1, sz1, NO_CONV);
}

/*********************************************************************
** Function : dynbuf_concat_ws
** Description : concat strings in a buffer
*********************************************************************/
char *dynbuf_concat_ws(				/* return : buffer contents or NULL if empty */
	DynBuffer **buf,	 			/* in/out : buffer to concat */
	char *str1,		 				/* in : string to concat */
	char *sep,		 				/* in : separator */
	char *str2		 				/* in : string to concat */
){
	if(buf && *buf) (*buf)->cnt = 0;
	dynbuf_add(buf, str1, 0, NO_CONV);
	if(str2 && *str2 && str1 && *str1) dynbuf_add(buf, sep, 0, NO_CONV);
	dynbuf_add(buf, str2, 0, NO_CONV);
	return (buf && *buf && (*buf)->cnt) ? (*buf)->data : NULL;
}
