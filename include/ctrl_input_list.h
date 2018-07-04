/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_list.c
** Description : HTML handling functions for input controls of type LIST
**      Author : Alain BOUTE
**     Created : Dec 28 2002
*********************************************************************/


/*********************************************************************
** Function : put_html_list
** Description : output HTML code for a list input
*********************************************************************/
int put_html_list(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer *name,			 		/* in : input name */
	DynTable *options, 					/* in : options list (actual & displayed values) */
	DynTable *values, 					/* in : selected values */
	int lines, int columns,			 	/* in : # of lines & columns of input */
	int b_multiple,					 	/* in : multiple selection allowed if 1 */
	int b_submitonchange,			 	/* in : use JavaScript OnChange handler if 1 */
	int b_ignoreunlisted,			 	/* in : do not list values not in options if 1 */
	char *emptyval					 	/* in : add this option at 1st position with empty value */
);

/*********************************************************************
** Function : ctrl_put_html_input_list
** Description : output HTML input list for a control
*********************************************************************/
int ctrl_put_html_input_list(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	DynTable *optlist,				/* in : options list (actual & displayed values) */
	int b_submit					/* in : use OnChange Javascript handler if not null */
);

/*********************************************************************
** Function : ctrl_add_input_list
** Description : handles INPUT controls of type LIST
*********************************************************************/
int ctrl_add_input_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);
