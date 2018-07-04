/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_cgi.c
** Description : CGI handling utilities for controls
**      Author : Alain BOUTE
**     Created : Dec 28 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_cgi_name
** Description : build CGI name for a control value
*********************************************************************/
#define ERR_FUNCTION "ctrl_cgi_name"
#define ERR_CLEANUP 
int ctrl_cgi_name(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : control to make name for */
	DynTableCell *val,				/* in : value to make name for */
	int b_renumber,					/* in : build value number if 1 and val is NULL */
	DynBuffer **name,				/* in : CGI base name
									  out : final CGI name with value indexes */
	char type,						/* in : input type is first name char :
									 NULL = ignore (basename type is used)
										F = Form context data
										B = Button click
										D = user input data
										I = Control info button click
										U = unspecified */
	char *subfield,					/* in : subfield name */
	size_t subfield_sz				/* in : subfield name string size */
){
	EVA_form *form = cntxt->form;
	unsigned long i, num = 0;

	/* Reset given buffer if not NULL & add control CGI base name */
	if(*name) (*name)->cnt = 0;
	DYNBUF_ADD_BUF(name, ctrl->cginame, NO_CONV);

	/* Handle value number if applicable */
	if(form->Num && !val)
		num = form->Num;
	else if(b_renumber && !(val && val->Num))
	{
		num = 1;
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			DynTableCell *val = dyntab_cell(&ctrl->val, i, 0);
			if(val->Num >= num) num = val->Num + 1;
		}
	}

	/* Build final name */
	if(cgi_build_finalname(cntxt, name, type, subfield, subfield_sz, 
		(val && val->Num) ? val->Num : num, 
		(val && val->Line) ? val->Line : form->Line, 
		val ? val->Lang : 0)) 
			STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_put_hidden
** Description : output a control value as CGI hidden input
*********************************************************************/
#define ERR_FUNCTION "ctrl_put_hidden"
#define ERR_CLEANUP	M_FREE(name)
int ctrl_put_hidden(				/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : related control */
	unsigned long i					/* in : value index in ctrl->val */
)
{
	DynBuffer *name = NULL;
	CHECK_HTML_STATUS;

	CTRL_CGINAMEVAL(&name, i);
	DYNBUF_ADD3_BUF(html, "<input type=hidden name='", name, NO_CONV, "'");
	DYNBUF_ADD3_CELL(html, " value='", &ctrl->val, i, 0, HTML_NO_QUOTE, "'>");
	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(cntxt->form->html, "\n");
	if(cntxt->form->b_newobj && ctrl_put_hidden_old(cntxt, ctrl, i, name, NULL, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
