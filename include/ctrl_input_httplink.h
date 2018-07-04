/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_httplink.h
** Description : HTML handling functions for INPUT controls of type HTTPLINK
**      Author : Alain BOUTE
**     Created : Feb 7 2003
*********************************************************************/


/*********************************************************************
** Function : ctrl_add_httplink
** Description : handles INPUT controls of type HTTPLINK
*********************************************************************/
int ctrl_add_httplink(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);
