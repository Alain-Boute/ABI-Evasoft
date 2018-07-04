/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_misc.h
** Description : handling fonctions for misceleanous actions
**      Author : Alain BOUTE
**     Created : March 27 2002
*********************************************************************/



/*********************************************************************
** Function : action_formbuttons
** Description : button click handler for PRINT, SAVE, CLOSE, EDIT, ...
*********************************************************************/
int action_formbuttons(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : office_launchproc
** Description : lauch an external office program
*********************************************************************/
int office_launchproc(			/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	char *proctyp,				/* in : office program to lanch (_EVA_EXCEL/_EVA_WORD/_EVA_ACCESS) */
	char *procname				/* in : office template to transfer to current directory & open */
);

/*********************************************************************
** Function : action_launchproc
** Description : button click handler for OPENFORM
*********************************************************************/
int action_launchproc(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_openform
** Description : button click handler for OPENFORM
*********************************************************************/
int action_openform(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_closeall
** Description : button click handler for CLOSEALL
*********************************************************************/
int action_closeall(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_export
** Description : button click handler for EXPORT
*********************************************************************/
int action_export(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : taskplan_sequence
** Description : handler for scheduled actions
*********************************************************************/
int taskplan_sequence(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *task						/* in : task type (DayTask / HourTask) */
);

/*********************************************************************
** Function : action_sequence
** Description : button click handler for ACTION_SEQ
*********************************************************************/
int action_sequence(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_import
** Description : button click handler for IMPORT
*********************************************************************/
int action_import(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_update
** Description : button click handler for SQL_SCRIPT
*********************************************************************/
int action_update(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_sql_script
** Description : button click handler for UPDATE
*********************************************************************/
int action_sql_script(			/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_email
** Description : button click handler for EMAIL
*********************************************************************/
int action_email(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_end_session
** Description : button click handler for END_SESSION
*********************************************************************/
int action_end_session(			/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_delete
** Description : button click handler for DELETE
*********************************************************************/
int action_delete(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_copyobj
** Description : button click handler for COPYOBJ
*********************************************************************/
int action_copyobj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_default
** Description : default handler for form clicks
*********************************************************************/
int action_default(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_create_objects
** Description : button click handler for CREATE
*********************************************************************/
int action_create_objects(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : action_pay_site
** Description : button click handler for PAY_SITE
*********************************************************************/
int action_pay_site(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);
