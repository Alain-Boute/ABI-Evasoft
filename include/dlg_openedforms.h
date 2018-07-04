/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dlg_openedforms.c
** Description : handling fonctions for opened forms list dialog
**      Author : Alain BOUTE
**     Created : March 4 2003
*********************************************************************/


/*********************************************************************
** Function : dlg_openedforms
** Description : button click handler for OPENEDFORMS_DLG
*********************************************************************/
int dlg_openedforms(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);
