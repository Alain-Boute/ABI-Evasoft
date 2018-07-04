/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : form_mode_list.c
** Description : handling function for for values mode
**      Author : Alain BOUTE
**     Created : Jan 31 2003
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : form_mode_list
** Description : outputs HTML code for the given form
*********************************************************************/
#define ERR_FUNCTION "form_mode_list"
#define ERR_CLEANUP
int form_mode_list(
	EVA_context *cntxt		/* in : execution context data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl;
	DynBuffer *cginame = ctrl->cginame;

	/* Fake relation FORMSEL for form control */
	form_free_ctrl(cntxt, 1);
	ctrl->cginame = NULL;
	ctrl->FIELD = "_EVA_FORMSEL";
	CTRL_SETATTR_TXT("_EVA_SEARCHONLOAD", TABLESEARCH);
	CTRL_SETATTR_TXT("_EVA_FILTER_FORM", FILTER_TYPE);
	CTRL_SETATTR_CELL(&form->id_form, 0, 0, FILTER_FORM);
	CTRL_SETATTR_TXT("_EVA_TOP", ALTFORM);
	dyntab_free(&form->id_obj);
	if(cgi_build_basename(cntxt, &ctrl->cginame, 0, 'D')) STACK_ERROR;
	if(ctrl_read_values(cntxt, 0)) STACK_ERROR;

	/* Handle table buttons clics */
	if(table_read_controls(cntxt, 0, NULL) ||
		table_process_controls(cntxt, 0)) STACK_ERROR;

	/* Output relation table */
	if(cntxt->form == form)
	{
		form->step = HtmlEdit;
		ctrl->b_modified = 1;
		if(ctrl_format_pos(cntxt, ctrl, 1) ||
			table_put_html_obj_list(cntxt, 0) ||
			ctrl_format_pos(cntxt, ctrl, 0))
			STACK_ERROR;
		M_FREE(ctrl->cginame);
		ctrl->cginame = cginame;
		ctrl->FIELD = "";
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

