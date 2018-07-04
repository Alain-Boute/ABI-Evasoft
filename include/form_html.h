/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : form_html.c
** Description : generic forms functions (display, save, print, ...)
**      Author : Alain BOUTE
**     Created : Sept 23 2001
*********************************************************************/


/*********************************************************************
** Function : form_put_html
** Description : outputs HTML code for the given form
*********************************************************************/
int form_put_html(
	EVA_context *cntxt		/* in/out : execution context data */
);

/*********************************************************************
** Function : form_status_data
** Description : output encoded form status data
*********************************************************************/
int form_status_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **html				/* in/out : output buffer */
);

/*********************************************************************
** Function : form_tab_selection
** Description : handle CGI status for form main tab selection
*********************************************************************/
int form_tab_selection(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	int mode						/* in : read if set, write else */
);

/*********************************************************************
** Function : process_form
** Description : handle form interaction
*********************************************************************/
int process_form(					/* return : 0 on success, other on error */
	EVA_context *cntxt				/* in/out : execution context data */
);
