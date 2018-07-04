/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_basic.c
** Description : handling functions for basic controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_option
** Description : handles OPTION controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_option"
#define ERR_CLEANUP DYNTAB_FREE(ctrltree)
int ctrl_add_option(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable ctrltree = { 0 };

	/* Do not process if not selected */
	if(!ctrl->b_selected) RETURN_OK;

	/* Read controls under option */
	CTRL_ATTR(ctrltree, CTRLTREE);

	switch(form->step)
	{
	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
	default:
		/* Add lower level controls */
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
