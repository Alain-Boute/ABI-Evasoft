/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_color.c
** Description : handling functions for INPUT controls of type COLOR
**      Author : Alain BOUTE
**     Created : Feb 9 2003
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_input_color
** Description : handles INPUT controls of type COLOR
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_input_color"	
#define ERR_CLEANUP	M_FREE(name)
int ctrl_add_input_color(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *name = NULL;
	unsigned long i;

	switch(form->step)
	{
	case HtmlPrint:
	case HtmlView:
	case HtmlEdit:
		/* Output control header */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle empty value in view mode */
		if(form->step != HtmlEdit && !ctrl->val.nbrows && *ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);

		/* Output each value */
		for(i = 0; !i || i < ctrl->val.nbrows; i++)
		{
			/* Separate multiple values with a line break */
			if(i) DYNBUF_ADD_STR(form->html, "<br>");

			/* Output table for color display */
			DYNBUF_ADD_STR(form->html, "<table border=1 cellpadding=0 cellspacing=0><tr>");

			/* Output color cell */
			DYNBUF_ADD_STR(form->html, "<td width=50 height=20");
			if(dyntab_sz(&ctrl->val, i, 0))
				DYNBUF_ADD3_CELL(form->html, " bgcolor=#", &ctrl->val, i, 0, NO_CONV, "")
			else
				DYNBUF_ADD_STR(form->html, " background=/img/bg_rayures_grises.gif");

			/* Handle edit mode */
			if(form->step == HtmlEdit)
			{
				CTRL_CGINAMEVAL(&name, i);
				cntxt->jsColorInput = 1;
				if(cntxt->jsenabled)
					DYNBUF_ADD3_BUF(form->html, " onClick=\"showColor(this,'", name, NO_CONV, "')\"");
			}
			DYNBUF_ADD_STR(form->html, "></td><td>");

			/* Output value or text input if edit mode */
			if(form->step == HtmlEdit)
			{
				DYNBUF_ADD3_CELL(form->html, "<input type=text size=6 value='", &ctrl->val, i, 0, HTML_NO_QUOTE, "' name='");
				DYNBUF_ADD_BUF(form->html, name, NO_CONV);
				DYNBUF_ADD_STR(form->html, "'>");
			}
			else
				DYNBUF_ADD_CELL(form->html, &ctrl->val, i, 0, TO_HTML);
			DYNBUF_ADD_STR(form->html, "</td></tr></table>");
		}

		/* Output control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

