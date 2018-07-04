/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : err_utils.c
** Description : error handling functions
**      Author : Alain BOUTE
**     Created : Feb 15 2002
*********************************************************************/

/*********************************************************************
** Function : err_add_context
** Description : report error context
*********************************************************************/
void err_add_context(
	EVA_context *cntxt,		/* in/out : execution context data */
	char *function, 
	char *file, 
	int line
);

/*********************************************************************
** Function : err_clear
** Description : clear error status
*********************************************************************/
void err_clear(
	EVA_context *cntxt,		/* in/out : execution context data */
	char *function, 
	char *file, 
	int line
);


/*********************************************************************
** Function : err_save_context
** Description : save error status in cntxt->debug_html
*********************************************************************/
void err_save_context(EVA_context *cntxt);

/*********************************************************************
** Function : put_html_error
** Description : output HTML error text to cntxt->html
*********************************************************************/
void put_html_error(EVA_context *cntxt);

/*********************************************************************
** Function : put_debug_msg
** Description : output debug message
*********************************************************************/
void put_debug_msg(EVA_context *cntxt, FILE *f);

/*********************************************************************
** Function : put_debug_sqltable
** Description : output debug message
*********************************************************************/
void put_debug_sqltable(EVA_context *cntxt, char *table);

/*********************************************************************
** Function : err_print_filter
** Description : output a filter to a buffer for debug purposes
*********************************************************************/
void err_print_filter(
	DynBuffer **buf,		/* in/out : destination buffer */
	QryBuild *flt			/* in : filter to output */ 
);

/*********************************************************************
** Function : err_print_dyntab
** Description : output a DynTable to a buffer for debug purposes
*********************************************************************/
void err_print_dyntab(
	DynBuffer **buf,		/* in/out : destination buffer */
	DynTable *tab,			/* in : table to output */ 
	unsigned long rows,		/* in : max # of rows to print */
	ReplaceTable *sr,		/* in : search & replace table */
	int sz_sr				/* in : search & replace direction (1=direct, 0=none, -1=reverse) */
);

/*********************************************************************
** Function : err_print_data
** Description : output DynTable data for col 0 to a buffer for debug purposes
*********************************************************************/
void err_print_data(
	DynBuffer **buf,		/* in/out : destination buffer */
	DynTable *tab,			/* in : table to output */ 
	unsigned long beg,		/* in : 1st row to print */
	unsigned long end,		/* in : last row to print */
	ReplaceTable *sr,		/* in : search & replace table */
	int sz_sr				/* in : search & replace direction (1=direct, 0=none, -1=reverse) */
);

/*********************************************************************
** Function : debug_put_cgi
** Description : output CGI inputs
*********************************************************************/
void debug_put_cgi(EVA_context *cntxt);

/*********************************************************************
** Function : debug_put_info
** Description : output basic debug info
*********************************************************************/
void debug_put_info(EVA_context *cntxt);

/*********************************************************************
** Macro : RETURN_ERR_MEMORY
** Description : handles out of memory error
*********************************************************************/
#define RETURN_ERR_MEMORY RETURN_ERROR("Mémoire serveur saturée", NULL)

/*********************************************************************
** Macro : RETURN_ERR_MEMORY
** Description : handles directory access error
*********************************************************************/
#define RETURN_ERR_DIRECTORY(code)	RETURN_ERROR("Droits d'accès incorrects sur un dossier", code)

/*********************************************************************
** Macro : RETURN_ERR_PARAM
** Description : handles programmer's errors
*********************************************************************/
#define RETURN_ERR_PARAM(code)	RETURN_ERROR("Il y a un bug - mauvais paramètres", code)

/*********************************************************************
** Macro : RETURN_ERR_PROG
** Description : handles programmer's errors
*********************************************************************/
#define RETURN_ERR_PROG(code)	RETURN_ERROR("Il y a un bug - incohérence du programme", code)

/*********************************************************************
** Macro : RETURN_ERROR
** Description : handles error returns in functions
*********************************************************************/
#define RETURN_ERROR(txt, code) { code; cntxt->err.text = txt; cntxt->err.errline = __LINE__; goto eva_err; }

/*********************************************************************
** Macro : STACK_ERROR
** Description : handles error returns in functions at calling levels
*********************************************************************/
#define STACK_ERROR { cntxt->err.errline = __LINE__; goto eva_err; }
#define STACK_ERROR_INFO(code) { code; cntxt->err.errline = __LINE__; goto eva_err; }

/*********************************************************************
** Macro : RETURN_OK
** Description : handle normal return before end of functions body
*********************************************************************/
#define RETURN_OK goto eva_noerr

/*********************************************************************
** Macro : CLEAR_ERROR
** Description : clear error status
*********************************************************************/
#define CLEAR_ERROR err_clear(cntxt, ERR_FUNCTION, __FILE__, __LINE__)
#define CLEAR_ERROR_CONT { CLEAR_ERROR; continue; }
#define CLEAR_ERROR_NOWARN { cntxt->err.text = NULL; CLEAR_ERROR; }
#define CLEAR_ERROR_RETURN { CLEAR_ERROR; RETURN_OK; }

/*********************************************************************
** Macro : RETURN_OK_CLEANUP
** Description : handle normal return at end of functions body
*********************************************************************/
#define RETURN_OK_CLEANUP goto eva_noerr;\
eva_noerr: err_clear(cntxt, ERR_FUNCTION, __FILE__, 0); goto eva_err;\
eva_err: \
	if(cntxt->err.text) err_add_context(cntxt, ERR_FUNCTION, __FILE__, cntxt->err.errline); \
	ERR_CLEANUP; return cntxt->err.text != NULL;

/*********************************************************************
** Macro : RETURN_WARNING
** Description : handle normal return before end of functions body
*********************************************************************/
#define RETURN_WARNING(label) { dynbuf_add(&cntxt->debug_msg, add_sz_str(label), NO_CONV); goto eva_noerr; }

/*********************************************************************
** Macro : ERR_PUT_TXT
** Description : add text to detailed error message
*********************************************************************/
#define ERR_PUT_TXT(label, txt, len) {\
	dynbuf_add(&cntxt->err.detail, add_sz_str(label), NO_CONV); \
	dynbuf_add(&cntxt->err.detail, (txt) ? (txt) : "(null)", (txt) ? len : 0, NO_CONV); }

/*********************************************************************
** Macro : ERR_PUT_FILE
** Description : add file content to detailed error message
*********************************************************************/
#define ERR_PUT_FILE(label, file) { FILE *f = fopen(file, "r"); char buf[1024]; size_t sz = f ? fread(buf, 1, sizeof(buf), f) : 0;\
	dynbuf_add(&cntxt->err.detail, add_sz_str(label), NO_CONV); \
	while(sz) { dynbuf_add(&cntxt->err.detail, buf, sz, NO_CONV); sz = fread(buf, 1, sizeof(buf), f); } }

/*********************************************************************
** Macro : ERR_PUT_BUF
** Description : add text to detailed error message
*********************************************************************/
#define ERR_PUT_BUF(label, buf) ERR_PUT_TXT(label, buf ? buf->data : "", buf ? buf->cnt : 0)

/*********************************************************************
** Macro : ERR_PUT_CELL
** Description : add text to detailed error message
*********************************************************************/
#define ERR_PUT_CELL(label, tab, r, c) ERR_PUT_TXT(label, dyntab_val(tab, r, c), dyntab_sz(tab, r, c))

/*********************************************************************
** Macro : ERR_PUT_INT
** Description : add integer to detailed error message
*********************************************************************/
#define ERR_PUT_INT(label, i) { \
	char txt[32]; itoa(i, txt, 10);\
	dynbuf_add(&cntxt->err.detail, add_sz_str(label), NO_CONV); \
	dynbuf_add(&cntxt->err.detail, txt, 0, NO_CONV); \
}

/*********************************************************************
** Macro : ERR_PUT_TABLE
** Description : add table contents to detailed error message
*********************************************************************/
#define ERR_PUT_TABLE(label, tab) {\
	dynbuf_add(&cntxt->err.detail, add_sz_str(label), NO_CONV); \
	err_print_dyntab(&cntxt->err.detail, tab, NO_CONV); \
}
