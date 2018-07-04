/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input.c
** Description : handling functions for input controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

/*********************************************************************
** Function : ctrl_read_values
** Description : read input control values in ctrl->val
*********************************************************************/
int ctrl_read_values(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_input
** Description : handles INPUT controls
*********************************************************************/
int ctrl_add_input(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);
