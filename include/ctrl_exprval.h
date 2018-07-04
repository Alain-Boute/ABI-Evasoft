/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_exprval.c
** Description : handling functions for expression value controls 
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/


/*********************************************************************
** Function : ctrl_add_exprval
** Description : handles CALC controls
*********************************************************************/
int ctrl_add_exprval(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : put_value_fmt
** Description : output a formated value in text mode
*********************************************************************/
int put_value_fmt(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer **buf,					/* ou : buffer to add text to */
	char *txt, size_t len,				/* in : value to format */
	char *fmt							/* in : format specification */
);

/*********************************************************************
** Function : html_put_value_fmt
** Description : output a formated cell for a table of objects
*********************************************************************/
int html_put_value_fmt(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl,						/* in : output control */
	DynTable *table,					/* in : table values */
	unsigned long row,					/* in : index of value to process in table */
	char *fmt,							/* in : format specification */
	size_t strip,						/* in : max length for output values */
	size_t *tot_char,					/* in/out : output values char counter */
	int *b_stripped						/* out : 1 if value was stripped, 0 else */
);

/*********************************************************************
** Function : ctrl_output_exprval
** Description : compute values of CALC controls
*********************************************************************/
int ctrl_output_exprval(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : control tout output */
	unsigned long options			/* in : output options
										0 : formated values using tables
										1 : formated values in standard text */
);
