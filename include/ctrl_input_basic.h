/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_basic.h
** Description : handling functions for basic input controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/


/*********************************************************************
** Function : ctrl_autofocus
** Description : set focus to given control if applicable
*********************************************************************/
int ctrl_autofocus(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *name, size_t sz_name,		/* in : control CGI name */
	int b_empty						/* in : control value is empty if not zero */
);

/*********************************************************************
** Function : put_html_text_input
** Description : output HTML code for text input
*********************************************************************/
int put_html_text_input(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name, size_t sz_name, 		/* in : input name */
	char *val, size_t sz_val,			/* in : value  of the control */
	int b_passwd,						/* in : treat as password */
	int b_onblur,						/* in : use JavaScript OnBlur if 1 */
	int lines, int columns,				/* in : size of the control (in chars) */
	int maxlength		   				/* in : maximum input length (in chars) */
);

/*********************************************************************
** Function : ctrl_add_text_value
** Description : output HTML code for text input with given value
*********************************************************************/
int ctrl_add_text_value(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,	 				/* in : control to process */
	unsigned long i_val,			/* in : value index in ctrl->val */
	char *txt, size_t len,			/* in : text to use for value - if NULL use ctrl->val[i] */
	DynBuffer *name1 				/* in : CGI input name - if NULL build control value name */
);

/*********************************************************************
** Function : ctrl_add_text
** Description : handles INPUT controls of type TEXT
*********************************************************************/
int ctrl_add_text(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_input_image
** Description : handles INPUT controls of type TEXT
*********************************************************************/
int ctrl_add_input_image(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_input_field
** Description : handles INPUT controls of type TEXT
*********************************************************************/
int ctrl_add_input_field(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_date
** Description : handles INPUT controls of type DATE
*********************************************************************/
int ctrl_add_date(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_hour
** Description : handles INPUT controls of type HOUR
*********************************************************************/
int ctrl_add_hour(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_input_email
** Description : handles INPUT controls of type EMAIL
*********************************************************************/
int ctrl_add_input_email(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_input_tel
** Description : handles INPUT controls of type TELEPHONE
*********************************************************************/
int ctrl_add_input_tel(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_number
** Description : handles INPUT controls of type NUMBER
*********************************************************************/
int ctrl_add_number(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : put_html_chkbox
** Description : output HTML code for a chkbox input
*********************************************************************/
int put_html_chkbox(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name, size_t sz_name, 		/* in : input name */
	char *value, size_t sz_value, 		/* in : input value */
	int b_status,				 		/* in : checked if 1 */
	int b_submit						/* in : use OnClick JavaScript if 1 */
);

/*********************************************************************
** Function : ctrl_add_chkbox
** Description : handles INPUT controls of type CHECKBOX
*********************************************************************/
int ctrl_add_chkbox(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);
