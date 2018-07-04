/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : form_utils.c
** Description : utility forms functions
**      Author : Alain BOUTE
**     Created : Sept 23 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : form_free_ctrl
** Description : free cntxt->form->ctrl structure
*********************************************************************/
void form_free_ctrl(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long beg				/* control index to start with */
){
	unsigned long i;
	if(!cntxt->form) return;
	for(i = beg; i <= cntxt->form->nb_ctrl; i++)
	{
		if(cntxt->form->ctrl[i].objtbl)
		{
			table_free(cntxt->form->ctrl[i].objtbl);
			M_FREE(cntxt->form->ctrl[i].objtbl);
		}
		M_FREE(cntxt->form->ctrl[i].cginame);
		M_FREE(cntxt->form->ctrl[i].errmsg);
		DYNTAB_FREE(cntxt->form->ctrl[i].id);
		DYNTAB_FREE(cntxt->form->ctrl[i].attr);
		DYNTAB_FREE(cntxt->form->ctrl[i].val);
		DYNTAB_FREE(cntxt->form->ctrl[i].cgival);
		DYNTAB_FREE(cntxt->form->ctrl[i].dbval);
		DYNTAB_FREE(cntxt->form->ctrl[i].allval);
		DYNTAB_FREE(cntxt->form->ctrl[i].alldbval);
		if(cntxt->form->ctrl[i].objtbl)
		{
			table_free(cntxt->form->ctrl[i].objtbl);
			M_FREE(cntxt->form->ctrl[i].objtbl);
		}
	}
	if(!beg) M_FREE(cntxt->form->ctrl);
}

/*********************************************************************
** Function : form_free_html
** Description : reinitialize current form's HTML buffers
*********************************************************************/
void form_free_html(
	EVA_context *cntxt				/* in : execution context data */
){
	if(!cntxt->form) return;
	M_FREE(cntxt->form->html_title); 
	M_FREE(cntxt->form->html_top); 
	M_FREE(cntxt->form->html_tab);	
	M_FREE(cntxt->form->html_tabs); 
	M_FREE(cntxt->form->html_menu); 
	cntxt->form->html = NULL; 
}

/*********************************************************************
** Function : form_free
** Description : free cntxt->form structure
*********************************************************************/
void form_free(
	EVA_context *cntxt				/* in/out : execution context data */
){
	if(!cntxt->form || cntxt->form->b_dontfree) return;
	form_free_ctrl(cntxt, 0);
	DYNTAB_FREE(cntxt->form->id_form);
	DYNTAB_FREE(cntxt->form->id_obj);
	M_FREE(cntxt->form->tabs);
	DYNTAB_FREE(cntxt->form->objdata);
	DYNTAB_FREE(cntxt->form->call_data);
	M_FREE(cntxt->form->title);
	form_free_html(cntxt);
	M_FREE(cntxt->form); 
}

/*********************************************************************
** Function : form_set_homepage
** Description : set current form to homepage
*********************************************************************/
#define ERR_FUNCTION "form_set_homepage"
#define ERR_CLEANUP
int form_set_homepage(				/* return : 0 on success, other on error */
	EVA_context *cntxt
){
	DYNTAB_FREE(cntxt->id_obj);
	DYNTAB_FREE(cntxt->id_form);
	DYNTAB_FREE(cntxt->alt_obj);
	DYNTAB_FREE(cntxt->alt_form);
	if(dyntab_sz(&cntxt->homepage, 0, 0))
		DYNTAB_SET_CELL(&cntxt->id_form, 0, 0, &cntxt->homepage, 0, 0);
	if(cntxt->cgi) cntxt->cgi[cntxt->cgibtn].name[0] = 0;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
