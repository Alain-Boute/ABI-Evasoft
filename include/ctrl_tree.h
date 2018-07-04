/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_tree.h
** Description : handling functions for tree controls
**      Author : Alain BOUTE
**     Created : Apr 01 2002
*********************************************************************/


/*********************************************************************
** Function : ctrl_add_reltree
** Description : handle controls OUTPUT/RELTREE
*********************************************************************/
int ctrl_add_reltree(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);
