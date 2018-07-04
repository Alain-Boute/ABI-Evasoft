/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_contain.h
** Description : handing functions for container controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

/*********************************************************************
** Function : ctrl_add_tab
** Description : handles TAB controls
*********************************************************************/
int ctrl_add_tab(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_tab_header
** Description : handles headr for main TAB controls
*********************************************************************/
int ctrl_add_tab_header(			/* return : 0 on success, other on error */
	EVA_context *cntxt				/* in/out : execution context data */
);

/*********************************************************************
** Function : ctrl_add_group
** Description : handles GROUP controls
*********************************************************************/
int ctrl_add_group(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_menubar
** Description : handles MENUBAR controls
*********************************************************************/
int ctrl_add_menubar(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_pack
** Description : handles GROUP controls
*********************************************************************/
int ctrl_add_pack(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_table
** Description : handles CONTAINER controls of type TABLE
*********************************************************************/
int ctrl_add_table(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
)

;
