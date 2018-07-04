/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : cgi_utils.h
** Description : handling of cgi input format
**      Author : Alain BOUTE
**     Created : Feb 15 2002
*********************************************************************/


/*********************************************************************
** Function : cgi_init_call
** Description : read args & configuration for cgi calls
*********************************************************************/
int cgi_init_call(				/* return : 0 on success, other on error */
	EVA_context *cntxt
);

/*********************************************************************
** Function : output_log_end
** Description : output termination line in cgi log file
*********************************************************************/
void output_log_end(EVA_context *cntxt);

/*********************************************************************
** Fonction : basename
** Description : return the base name of a path
*********************************************************************/
char *basename(char *path, size_t path_len);

/*********************************************************************
** Fonction : cgi_free
** Description : free CGI data
*********************************************************************/
void cgi_free(EVA_context *cntxt);

/*********************************************************************
** Function : cgi_extract
** Description : extract tokens and put theim in a table
*********************************************************************/
int cgi_extract(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	char *val,						/* in : text containing tokens */
	char sep,						/* in : tokens separator */
	DynTable *res, ...				/* out : list of DynTable* pointers to receive values in cell 0,0 
											MUST BE NULL TERMINATED if # of tokens may exceed # of tables */
);

/*********************************************************************
** Function : cgi_build_name
** Description : build a CGI input name
*********************************************************************/
int cgi_build_name(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **name,				/* out : built cgi name */
	char type,						/* in : input type is first name char - see file header */
	unsigned long id_ctrl,			/* in : control id */
	unsigned long id_form,			/* in : form id */
	unsigned long id_obj,			/* in : object id */
	char *field,					/* in : field name */
	size_t field_sz,				/* in : field name string size */
	char *subfield,					/* in : subfield name */
	size_t subfield_sz,				/* in : subfield name string size */
	unsigned long num,				/* in : value index to handle multiple values */
	unsigned long row,				/* in : value index to handle table rows */
	unsigned long lvl				/* in : value index to handle list options level */
);

/*********************************************************************
** Function : cgi_build_basename
** Description : build the CGI input name of a control with value indexes
*********************************************************************/
int cgi_build_basename(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **ret,				/* in : initial data is freed
									  out : built cgi name */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char type						/* in : input type is first name char - see file header */
);

/*********************************************************************
** Function : cgi_build_finalname
** Description : build the complete CGI input name of a control
*********************************************************************/
int cgi_build_finalname(			/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **name,				/* in : CGI base name
									  out : final CGI name with value indexes */
	char type,						/* in : input type is first name char :
									 NULL = ignore (basename type is used)
										F = Form context data
										B = Button click
										D = user input data
										I = Control info button click
										U = unspecified */
	char *subfield,					/* in : subfield name */
	size_t subfield_sz,				/* in : subfield name string size */
	unsigned long num,				/* in : value index to handle multiple values */
	unsigned long row,				/* in : value index to handle table rows */
	unsigned long lvl				/* in : value index to handle list options level */
);

/*********************************************************************
** Macro : CGI_VALUES_DELETE
** Description : clear name for read cgi values
*********************************************************************/
#define CGI_VALUES_DELETE(cgival) { unsigned long _i; for(_i = 0; _i < (cgival)->nbrows; _i++) cgi_value_setkeep(cntxt, cgival, _i, 2); }

/*********************************************************************
** Macro : CGI_VALUES_DONTKEEP
** Description : set b_dontkeep flag for read cgi values
*********************************************************************/
#define CGI_VALUES_DONTKEEP(cgival) { unsigned long _i; for(_i = 0; _i < (cgival)->nbrows; _i++) cgi_value_setkeep(cntxt, cgival, _i, 1); }

/*********************************************************************
** Macro : CGI_VALUES_KEEP
** Description : clear b_dontkeep flag for read cgi values
*********************************************************************/
#define CGI_VALUES_KEEP(cgival) { unsigned long _i; for(_i = 0; _i < (cgival)->nbrows; _i++) cgi_value_setkeep(cntxt, cgival, _i, 0); }

/*********************************************************************
** Function : cgi_get_values
** Description : return	cgi inputs matching the given name
*********************************************************************/
int cgi_get_values(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *ret,					/* out : CGI values table */
	char *name,						/* in : name to search for in cgi data */
	int mode						/* in : search mode :
											0 = exact search
											1 = type+form+obj+field match
											2 = type+obj+field match
											3 = type+field match */
);

/*********************************************************************
** Function : cgi_filter_values
** Description : return	cgi inputs matching the given name
*********************************************************************/
int cgi_filter_values(				/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *ret,					/* out : matching CGI inputs table */
	char type,						/* in : 1st letter of name to search for - do not use if 0 */
	unsigned long IdCtrl,			/* in : control to search for - do not use if ~0L */
	unsigned long IdForm,			/* in : form to search for - do not use if ~0L */
	unsigned long IdObj,			/* in : object to search for - do not use if ~0L */
	char *field,					/* in : field to search for - do not use if NULL */
	char *subfield,					/* in : subfield to search for - do not use if NULL */
	unsigned long Num,				/* in : number index to search for - do not use if 0 */
	unsigned long Line				/* in : line index to search for - do not use if 0 */
);

/*********************************************************************
** Function : cginame_setkeep
** Description : mark lines of cntxt->cgi associated with given name
*********************************************************************/
void cginame_setkeep(
	EVA_context *cntxt,				/* in : execution context data */
	char *name,						/* in : input name to search for in cntxt->cgi */
	int b_dontkeep					/* in : how to mark
										0 = keep value
										1 = dont keep value
										2 = clear name */
);

/*********************************************************************
** Function : cgi_setkeep
** Description : mark lines of cntxt->cgi associated with given value
*********************************************************************/
void cgi_setkeep(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i,				/* in : value index to process in cntxt->cgi */
	int b_dontkeep					/* in : how to mark
										0 = keep value
										1 = dont keep value
										2 = clear name */
);

/*********************************************************************
** Function : cgi_value_setkeep
** Description : mark lines of cntxt->cgi associated with given value
*********************************************************************/
void cgi_value_setkeep(
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *cgival,				/* in : values from cgi_filter_values() */
	unsigned long i,				/* in : value index to process in cgival */
	int b_dontkeep					/* in : set flag */
);

/*********************************************************************
** Function : cgi_check_form_change
** Description : ouput hidden input for CGI values not marked
*********************************************************************/
int cgi_check_form_change(			/* return : 0 = unchanged 
												1 = modified 
												2 = menubar or  home page */		
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long idform,			/* in : form id to search for */
	unsigned long idobj			/* in : object id to search for */
);

/*********************************************************************
** Function : cgi_clear_form_inputs
** Description : mark lines of cntxt->cgi with given form & obj
*********************************************************************/
void cgi_clear_form_inputs(
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idform,			/* in : form id to search for */
	unsigned long idobj,			/* in : object id to search for */
	int b_keepstatus				/* in : keep status info if not 0 */
);

/*********************************************************************
** Function : cgi_put_hiddenold
** Description : ouput hidden old input for a control
*********************************************************************/
int cgi_put_hiddenold(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer *name,				/* in : CGI name for data input */
	char *txt, size_t len			/* in original value */
);

/*********************************************************************
** Function : cgi_put_hidden
** Description : ouput hidden input for CGI values not marked
*********************************************************************/
int cgi_put_hidden(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt				/* in/out : execution context data */
);

/*********************************************************************
** Macro : CGI_CLICK_BTN_SUBFIELD
** Description : get CGI values for form/obj/field given in a control cginame
*********************************************************************/
#define CGI_CLICK_BTN_SUBFIELD ((cntxt->cgi && cntxt->cgi[cntxt->cgibtn].name[0] == 'B' && cntxt->cgi[cntxt->cgibtn].subfield) ? \
								cntxt->cgi[cntxt->cgibtn].subfield : "")

/*********************************************************************
** Function : cgi_add_inputold
** Description : add an input / value pair to cntxt->cgi with corresponding old value
*********************************************************************/
int cgi_add_inputold(				/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long *i_cgi,			/* out : index of added value in cntxt->cgi */
	char *name, size_t name_sz,		/* in : name data */
	char *value, size_t value_sz	/* in : value data */
);

/*********************************************************************
** Function : cgi_parse_name
** Description : parse name of a cgi input in cntxt->cgi
*********************************************************************/
int cgi_parse_name(			/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in : context data */
	CGIData *cgi			/* in/out : CGI input to process */
);

/*********************************************************************
** Function : cgi_add_input
** Description : add an input / value pair to cntxt->cgi
*********************************************************************/
int cgi_add_input(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long *i_cgi,			/* out : index of added value in cntxt->cgi */
	char *name, size_t name_sz,		/* in : name data */
	char *value, size_t value_sz	/* in : value data */
);

/*********************************************************************
** Function : cgi_add_input_with_old
** Description : add an input / value pair to cntxt->cgi with corresponding old value
*********************************************************************/
int cgi_add_input_with_old(			/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long *i_cgi,			/* out : index of added value in cntxt->cgi */
	char *name, size_t name_sz,		/* in : name data */
	char *value, size_t value_sz	/* in : value data */
);

/*********************************************************************
** Function : cgi_free_files
** Description : free allocated memory for uploaded files
*********************************************************************/
void cgi_free_files(EVA_context *cntxt);

/*********************************************************************
** Function : cgi_save_files
** Description : write uploaded files to disk & create objects in DB
*********************************************************************/
int cgi_save_files(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt				/* in/out : execution context data */
);


/*********************************************************************
** Function : cgi_trace_input
** Description : output CGI data in a trace file
*********************************************************************/
void cgi_trace_input(EVA_context *cntxt);

/*********************************************************************
** Function : cgi_read_urlencoded
** Description : read CGI data in www-form-urlencoded format
*********************************************************************/
int cgi_read_urlencoded(	/* return : 0 on success, other on error */
	EVA_context *cntxt		/* in : 
								cntxt->input : CGI input to process
							   out : 
								cntxt->cgi : table of read & parsed CGI inputs */
);

/*********************************************************************
** Function : cgi_read_data
** Description : read CGI data into a DynTable
*********************************************************************/
int cgi_read_data(			/* return : 0 on success, other on error */
	EVA_context *cntxt		/* in : context data
							   out : 
								cntxt->cgi : table of read & parsed CGI inputs
									col 0 : name
									col 1 : value
									col 2 : number
									col 3 : object id
									col 4 : form id
								cntxt->session : session id
								cntxt->id_form : current form id
								cntxt->cgibtn : index of the first clicked button
							*/
);
