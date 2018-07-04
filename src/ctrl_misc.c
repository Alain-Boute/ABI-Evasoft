/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_misc.c
** Description : handling fonctions for miscellaneous controls
**      Author : Alain BOUTE
**     Created : Feb 17 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_statictext
** Description : handles STATICTEXT controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_statictext"
#define ERR_CLEANUP	M_FREE(buf)
int ctrl_add_statictext(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *buf = NULL;
	DynTableCell *text = CTRL_ATTR_CELL(TEXT);
	int b_html = CTRL_ATTR_CELL(HTML) != NULL;
	CHECK_HTML_STATUS;
	if(!text) RETURN_OK;

	switch(form->step)
	{
	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		/* Set STATICTEXT default attributes values */
		if(!ctrl->LABELPOS[0]) ctrl->LABELPOS = "_EVA_NONE";

		/* Handle brackets evaluation */
		if(CTRL_ATTR_CELL(EVALSQL))
		{
			if(mailmerge_brackets(cntxt, &buf, text->txt, text->len, i_ctrl, 0)) CLEAR_ERROR;
		}
		else
			DYNBUF_ADD_CELLP(&buf, text, NO_CONV);

		/* Output text */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		if(buf && dynbuf_add(html, buf->data, buf->cnt, b_html ? NULL : plain_to_html, 1)) RETURN_ERR_MEMORY;
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_anchor
** Description : handles ANCHOR controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_anchor"
#define ERR_CLEANUP
int ctrl_add_anchor(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl							/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;

	switch(form->step)
	{
	case CtrlRead:
		if(!*ctrl->LABELPOS) ctrl->LABELPOS = "_EVA_NONE";
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		/* Output text */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		DYNBUF_ADD3_CELLP(form->html, "<a href='", CTRL_ATTR_CELL(URL), HTML_NO_QUOTE, "' target=_blank");
		if(*ctrl->NOTES)
			DYNBUF_ADD3_CELLP(form->html, "title='", CTRL_ATTR_CELL(NOTES), HTML_TOOLTIP, "'");
		DYNBUF_ADD3_CELLP(form->html, ">", CTRL_ATTR_CELL(LABEL), TO_HTML, "</a>");
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_unknown
** Description : handles unknowned type controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_unknown"
#define ERR_CLEANUP
int ctrl_add_unknown(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl							/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;

	switch(form->step)
	{
	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		/* Output text */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		DYNBUF_ADD_STR(form->html, "???");
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
