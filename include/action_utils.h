/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_utils.h
** Description : utility fonctions declarations for action handlers
**      Author : Alain BOUTE
**     Created : July 1 2003
*********************************************************************/


/*********************************************************************
** Function : form_btn_do_save
** Description : handles form data save before & after button clicks
*********************************************************************/
int form_btn_do_save(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : button index in cntxt->form->ctrl */
	int *b_done						/* out : 0 if data was not saved */
);

/*********************************************************************
** Function : form_get_field_values
** Description : read updated object field (search in CGI & form data)
*********************************************************************/
int form_get_field_values(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : copied values */
	char *field,								/* in : field to search for */
	unsigned long idform,						/* in : source form id */
	unsigned long idobj							/* in : source object id */
);

/*********************************************************************
** Function : form_get_fields_values
** Description : read field values for an object (search in CGI & form data)
*********************************************************************/
int form_get_fields_values(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : copied values */
	DynTable *fields,							/* in : fields to search for */
	unsigned long idform,						/* in : source form id (use current form if 0 and idobj = 0) */
	unsigned long idobj							/* in : source object id (use current obj if 0 and idform = 0) */
);

/*********************************************************************
** Function : form_eval_fieldexpr
** Description : evaluate a field expression in current form context
*********************************************************************/
int form_eval_fieldexpr(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : resulting values */
	unsigned long idform,						/* in : source form id (read from DB if 0) */
	unsigned long idobj,						/* in : source object id */
	char *field, size_t field_sz,				/* in : field expression to evaluate in current form context */
	DynTable *data,								/* in : object data - for optimisation purposes */
	unsigned long beg							/* in : first line to process in data */
);

/*********************************************************************
** Function : ctrl_eval_fieldexpr
** Description : evaluate a field expression in a control
*********************************************************************/
int ctrl_eval_fieldexpr(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	EVA_ctrl *ctrl,								/* in : control to process */
	DynTable *res,								/* out : resulting values - always freed before processing */
	char *expr									/* in : field expression to evaluate in control */
);

/*********************************************************************
** Function : cgi_set_field_values
** Description : define field values in CGI data for given form & obj
*********************************************************************/
int cgi_set_field_values(						/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	unsigned long idform,						/* in : destination form id */
	unsigned long idobj,						/* in : destination object id */
	char *destfield, size_t destfield_sz,		/* in : destination field */
	DynTable *values,							/* in : source values */
	char *setmode,								/* in : set mode (replace, add, ...) */
	int b_old									/* in : also set corresponding old CGI input value if non zero */
);

/*********************************************************************
** Function : form_set_values
** Description : define values in current form
*********************************************************************/
int form_set_values(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	unsigned long idform,						/* in : destination form id */
	unsigned long idobj,						/* in : destination object id */
	char *destfield, size_t destfield_sz,		/* in : destination field */
	char *srctype,								/* in : source type */
	DynTable *srcobj,							/* in : source objects */
	DynTable *srcval,							/* in : source values / field expression */
	char *setmode								/* in : set mode (replace, add, ...) */
);

/*********************************************************************
** Function : qry_parse_valtype
** Description : evaluate a value type / expression pair
*********************************************************************/
int qry_parse_valtype(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in : execution context data */
	char *srctype,								/* in : value type */
	DynTable *listobj,							/* int : list of objects */
	DynTable **id_form,							/* out : selected form */
	DynTable **id_obj							/* out : selected object */
);

/*********************************************************************
** Function : form_eval_valtype
** Description : evaluate a value type / expression pair
*********************************************************************/
int form_eval_valtype(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* out : resulting values */
	char *srctype,								/* in : value type */
	DynTable *id_obj,							/* in : objects id for srctype=LISTOBJ */
	DynTable *srcval							/* in : source values / field expression */
);

/*********************************************************************
** Function : ctrl_eval_valtyp
** Description : evaluate a value type / expression pair
*********************************************************************/
int ctrl_eval_valtyp(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	EVA_ctrl *ctrl,								/* in : control to process */
	DynTable *res,								/* out : resulting values */
	char *srctype,								/* in : value type */
	DynTable *listobj,							/* in : objects id for srctype=LISTOBJ or srctype=FILTER */
	DynTable *srcval							/* in : source values / field expression */
);

/*********************************************************************
** Function : mailmerge_brackets
** Description : evaluate bracketed SQL instructions in a string
*********************************************************************/
int mailmerge_brackets(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **res,				/* out : result string */
	char *str, size_t sz_str,		/* in : string with brackets */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl (for variables) */
	int conv						/* in : conversion mode for result
										0 : NO_CONV
										1 : TO_HTML
										2 : NO_CONV if result starts with '<', TO_HTML else */
);
