/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : form_mode.h
** Description : handling function for form modes
**      Author : Alain BOUTE
**     Created : Jan 21 2003
*********************************************************************/


/*********************************************************************
** Function : form_mode_values
** Description : outputs HTML code for the given form
*********************************************************************/
int form_mode_values(
	EVA_context *cntxt		/* in/out : execution context data */
);

/*********************************************************************
** Function : form_mode_search
** Description : outputs HTML code for the given form
*********************************************************************/
int form_mode_search(
	EVA_context *cntxt		/* in/out : execution context data */
);

/*********************************************************************
** Function : form_mode_list
** Description : outputs HTML code for the given form
*********************************************************************/
int form_mode_list(
	EVA_context *cntxt		/* in/out : execution context data */
);
