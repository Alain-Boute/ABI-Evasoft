/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : obj_utils.c
** Description : misceleanous functions for handling objects
**      Author : Alain BOUTE
**     Created : Jan 26 2003
**
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : obj_put_html_btn_label
** Description : output open button(s) & label for an object
*********************************************************************/
#define ERR_FUNCTION "obj_put_html_btn_label"
#define ERR_CLEANUP	DYNTAB_FREE(objdata)
int obj_put_html_btn_label(			/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *id_obj				/* in : object Id in (0,0) */
){
	DynTable objdata = { 0 };
	CHECK_HTML_STATUS;

	DYNBUF_ADD_STR(html,
		"<table cellspacing=0 cellpadding=0 border=0 rules=none><tr>");
	if(qry_obj_data(cntxt, &objdata, id_obj, NULL) ||
		ctrl_add_symbol_btn(cntxt, NULL, NULL, &objdata, 0, "", "SYMBOL+LABEL+OBJNOTES"))
		STACK_ERROR;
	DYNBUF_ADD_STR(html,
		"</tr></table>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

