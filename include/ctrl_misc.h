/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_misc.c
** Description : handling fonctions for miscellaneous controls
**      Author : Alain BOUTE
**     Created : Feb 17 2002
*********************************************************************/


/*********************************************************************
** Function : ctrl_add_statictext
** Description : handles STATICTEXT controls
*********************************************************************/
int ctrl_add_statictext(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_image
** Description : handles IMAGE controls
*********************************************************************/
int ctrl_add_image(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_anchor
** Description : handles ANCHOR controls
*********************************************************************/
int ctrl_add_anchor(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_unknown
** Description : handles unknowned type controls
*********************************************************************/
int ctrl_add_unknown(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);
