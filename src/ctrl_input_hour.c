/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_hour.c
** Description : handling functions for HOUR input controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_hour
** Description : handle controls of type HOUR
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_hour"
#define ERR_CLEANUP	M_FREE(name)
int ctrl_add_hour(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *name = NULL;
	unsigned long i, j;
	int b_multiple = ctrl->MULTIPLE[0] != 0 && strcmp("No", ctrl->MULTIPLE);

	switch(form->step)
	{
	case CtrlRead:
		break;

	case InputCheck:
		/* Check each value */
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			/* Get hour values */
			DynTableCell *val = dyntab_cell(&ctrl->val, i, 0);
			int h = 0, m = 0, s = 0;
			if(!val || !val->txt || !val->len) continue;
			sscanf(val->txt, "%2d%2d%2d", &h, &m, &s);

			/* Check hour is valid */
			for(j = 0; j < val->len && h >= 0; j++) if(val->txt[j] < '0' || val->txt[j] > '9') h = -1;
			if((val->len != 4 && val->len != 6) || h < 0 || h > 23 || m < 0 || m > 59 || s < 0 || s > 59)
			{
				ctrl->error = 2;
				if(ctrl->errmsg) DYNBUF_ADD_STR(&ctrl->errmsg, "\n");
				DYNBUF_ADD3_CELL(&ctrl->errmsg, "L'heure [", &ctrl->val, i, 0, NO_CONV, "] n'est pas correcte\nUtilisez le format 0000 à 2359");
			}
		}
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		if(!form->html) break;
		/* Add HTML code for control */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle empty value in view mode */
		if(form->step != HtmlEdit && !ctrl->val.nbrows && ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);

		/* Add HTML code for each value */
		j = ctrl->val.nbrows + ((form->step == HtmlEdit && (b_multiple || !ctrl->val.nbrows)) ? 1 : 0);
		for(i = 0; i < j; i++)
		{
			/* Separate multiple values with a line break */
			DynTableCell *val = dyntab_cell(&ctrl->val, i, 0);
			if(i) DYNBUF_ADD_STR(form->html, "<br>");

			/* Handle edit mode */
			if(form->step == HtmlEdit)
			{
				CTRL_CGINAMEVAL(&name, i);
				DYNBUF_ADD3_BUF(form->html, "<input type=text size=6 name='", name, NO_CONV, "'");
				DYNBUF_ADD3_CELLP(form->html, " value='", val, HTML_NO_QUOTE, "'>");
				if(ctrl_autofocus(cntxt, DYNBUF_VAL_SZ(name), !val || !val->txt || !val->len)) STACK_ERROR;
			}
			else
				if(put_value_fmt(cntxt, form->html, val->txt, val->len, "_EVA_HOUR")) STACK_ERROR;
		}
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
