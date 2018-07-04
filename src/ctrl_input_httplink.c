/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_fileupload.c
** Description : handling functions for INPUT controls of type HTTPLINK
**      Author : Alain BOUTE
**     Created : Feb 7 2003
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_httplink
** Description : handles INPUT controls of type HTTPLINK
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_httplink"
#define ERR_CLEANUP
int ctrl_add_httplink(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i;

	switch(form->step)
	{
	case HtmlView:
		/* Output HTTP link */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle empty value in view mode */
		if(!ctrl->val.nbrows && ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);

		/* Add HTML code for each value */
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);

			/* Separate multiple values with a line break */
			if(i) DYNBUF_ADD_STR(form->html, "<br>");

			DYNBUF_ADD_STR(form->html, "<a href='");
			if(!strstr(c->txt, "://") && isalpha(c->txt[0])) DYNBUF_ADD_STR(form->html, "http://");
			DYNBUF_ADD3_CELL(form->html, "", &ctrl->val, i, 0, HTML_NO_QUOTE, "' target=W");
			DYNBUF_ADD_INT(form->html, c->Pkey);
			DYNBUF_ADD3_CELLP(form->html, ">", c, TO_HTML, "</a>");
		}
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	default:
		/* Handle as text control */
		if(ctrl_add_text(cntxt, i_ctrl)) STACK_ERROR;
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

