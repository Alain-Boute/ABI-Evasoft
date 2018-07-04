/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : obj_utils.c
** Description : misceleanous functions for handling objects
**      Author : Alain BOUTE
**     Created : Jan 26 2003
**
*********************************************************************/

/*********************************************************************
** Function : obj_check_user_access
** Description : get access status of an object for the identified user
*********************************************************************/
int obj_check_user_access(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *data,					/* in : object data */
	unsigned long beg,				/* in : 1st row to use in data */
	int *acc						/* out : access rights */
);

/*********************************************************************
** Function : obj_put_html_btn_label
** Description : output open button(s) & label for an object
*********************************************************************/
int obj_put_html_btn_label(			/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *id_obj				/* in : object Id in (0,0) */
);
