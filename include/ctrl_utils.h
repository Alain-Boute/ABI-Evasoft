/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_utils.h
** Description : HTML controls handling utilities
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/


/*********************************************************************
** Macro : ATTR_VAL
** Description : return the 1st value of an attribute in a control
*********************************************************************/
#define	ATTR_VAL(ctrl, txt) ((ctrl) ? DYNTAB_FIELD_VAL(&((ctrl)->attr), txt) : "")

/*********************************************************************
** Macro : ATTR_CELL
** Description : return the 1st value of an attribute in a control
*********************************************************************/
#define	ATTR_CELL(ctrl, txt) ((ctrl) ? DYNTAB_FIELD_CELL(&((ctrl)->attr), txt) : NULL)

/*********************************************************************
** Macro : CTRL_ATTR_VAL
** Description : return the 1st value of an attribute in a control
*********************************************************************/
#define	CTRL_ATTR_VAL(txt) ATTR_VAL(ctrl, txt)

/*********************************************************************
** Macro : CTRL_ATTR_CELL
** Description : return the 1st value of an attribute in a control
*********************************************************************/
#define	CTRL_ATTR_CELL(txt) ATTR_CELL(ctrl, txt)

/*********************************************************************
** Macro : ATTR_OPTIONAL
** Description : read an optionnal attribute in a control
*********************************************************************/
#define	ATTR_OPTIONAL(res, field, ctrl) DYNTAB_FIELD(&(res), &((ctrl)->attr), field)

/*********************************************************************
** Macro : CTRL_ATTR
** Description : read an optionnal attribute in a control
*********************************************************************/
#define	CTRL_ATTR(res, txt) ATTR_OPTIONAL(res, txt, ctrl)

/*********************************************************************
** Macro : CTRL_ATTR_TAB
** Description : read a control attribute with lines reordering
*********************************************************************/
#define	CTRL_ATTR_TAB(res, txt) { ATTR_OPTIONAL(res, txt, ctrl); \
										if(dyntab_order_lines(&res)) RETURN_ERR_MEMORY; }

/*********************************************************************
** Macro : CTRL_ATTR_GET
** Description : read an optionnal attribute in a control
*********************************************************************/
#define	CTRL_ATTR_GET(attrname) ctrl->attrname = ATTR_VAL(ctrl, attrname)

/*********************************************************************
** Macro : CTRL_ATTR_GETFORM
** Description : read an optionnal attribute in a control - get form default if not found
*********************************************************************/
#define	CTRL_ATTR_GETFORM(attrname) \
{ \
	CTRL_ATTR_GET(attrname); \
	if(!ctrl->attrname[0] && cntxt->form->ctrl->attrname) ctrl->attrname = cntxt->form->ctrl->attrname; \
	if(!ctrl->attrname[0] && ctrl != &cntxt->srvfmt && cntxt->srvfmt.attrname) ctrl->attrname = cntxt->srvfmt.attrname; \
}

/*********************************************************************
** Macro : CTRL_OPTIONAL_NULLIF
** Description : read a mandatory attribute in a control
*********************************************************************/
#define	CTRL_OPTIONAL_NULLIF(res, txt, val) \
{ \
	CTRL_ATTR(res, txt) \
	if(res.nbrows < 2 && !dyntab_txt_cmp(add_sz_str(val), &res, 0, 0)) dyntab_free(&res); \
}

/*********************************************************************
** Macro : CTRL_ATTR_NULLIF_GETFORM
** Description : read an attribute & set to nul if given value - get form default if not found
*********************************************************************/
#define	CTRL_ATTR_NULLIF_GETFORM(attrname, ignore) \
{ \
	ctrl->attrname = ctrl_get_attr_val(ctrl, add_sz_str("_EVA_"#attrname), add_sz_str(ignore)); \
	if(!ctrl->attrname[0]) ctrl->attrname = cntxt->form->ctrl->attrname; \
}

/*********************************************************************
** Macro : CTRL_MANDATORY
** Description : read a mandatory attribute in a control
*********************************************************************/
#define	CTRL_MANDATORY(res, txt, container) \
{ \
	CTRL_ATTR(res, txt) \
	if(!res.nbrows) RETURN_ERROR("Contrôle" #container " incorrect : pas de valeur pour " #txt, 1, NULL); \
}

/*********************************************************************
** Macro : CTRL_SETATTRDEF
** Description : set a default value for an attribute in a control
*********************************************************************/
#define	CTRL_SETATTRDEF(val, txt) { \
	if(!dyntab_field_cell(&ctrl->attr, "_EVA_"#txt, 0, 0))  \
		CTRL_SETATTR(val, sizeof(val)-1, txt); }

/*********************************************************************
** Macro : CTRL_SETATTR_TXT
** Description : set a default value for an attribute in a control
*********************************************************************/
#define	CTRL_SETATTR_TXT(val, txt) CTRL_SETATTR(val, sizeof(val)-1, txt)

/*********************************************************************
** Macro : CTRL_SETATTR_CELLP
** Description : set a default value for an attribute in a control
*********************************************************************/
#define	CTRL_SETATTR_CELLP(cell, tag) { DynTableCell *c = cell; \
				CTRL_SETATTR(c ? c->txt : "", c ? c->len : 0, tag); }

/*********************************************************************
** Macro : CTRL_SETATTR_CELL
** Description : set a default value for an attribute in a control
*********************************************************************/
#define	CTRL_SETATTR_CELL(tab, row, col, tag) CTRL_SETATTR_CELLP(dyntab_cell(tab, row, col), tag)

/*********************************************************************
** Macro : CTRL_DEFVAL_CELL
** Description : set a default value for an attribute in a control
*********************************************************************/
#define	CTRL_DEFVAL_CELL(tab, row, col, tag) { \
	if(!dyntab_field_cell(&ctrl->attr, "_EVA_"#tag, 0, 0))  \
		CTRL_SETATTR(dyntab_val(tab, row, col), dyntab_sz(tab, row, col), txt); }

/*********************************************************************
** Macro : CTRL_ATTR_DEFVAL
** Description : get an attribute value in a control with default value
*********************************************************************/
#define	CTRL_ATTR_DEFVAL(res, val, txt) { \
	CTRL_ATTR(res, txt); \
	if(!res.nbrows) DYNTAB_SET(&res, 0, 0, val); }

/*********************************************************************
** Macro : CTRL_SETATTR
** Description : set value for an attribute in a control
*********************************************************************/
#define	CTRL_SETATTR(val, sz_val, txt) \
	if(ctrl_set_attr_txt(cntxt, ctrl, "_EVA_"#txt, val, sz_val)) STACK_ERROR; 

/*********************************************************************
** Macro : CTRL_SETATTR_TAB
** Description : set values for an attribute in a control
*********************************************************************/
#define	CTRL_SETATTR_TAB(data, txt) \
	if(ctrl_set_attr(cntxt, ctrl, "_EVA_"#txt, data)) STACK_ERROR; 


/*********************************************************************
** Function : ctrl_from_cginame
** Description : search the control matching the given CGI name
*********************************************************************/
int ctrl_from_cginame(				/* return : control index in cntxt->form->ctrl or 0 if no match */
	EVA_context *cntxt,				/* in : execution context data */
	char *cginame, size_t sz		/* in : searched CGI name */
);

/*********************************************************************
** Function : ctrl_read_baseobj
** Description : complete derived control with base objects data
*********************************************************************/
int ctrl_read_baseobj(				/* return : control index in cntxt->form->ctrl or 0 if no match */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *ctrldata				/* in/out : control data to complete */
);

/*********************************************************************
** Function : ctrl_from_id
** Description : search the control in a control children matching the given id
*********************************************************************/
unsigned long ctrl_from_id(					/* return : control index in cntxt->form->ctrl or -1 if no match */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,						/* in : parent control index in cntxt->form->ctrl */
	char *id, size_t sz_id			/* in : searched control id */
);
/*********************************************************************
** Function : ctrl_add_value
** Description : add a value to the given control (handling CGI input)
*********************************************************************/
int ctrl_add_value(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *txt, size_t len,			/* in : value to add */
	unsigned long num,				/* in : number index for value */
	unsigned long line,				/* in : line index for value */
	char b_modified					/* in : modification status */
);

/*********************************************************************
** Function : ctrl_del_value
** Description : delete a value in the given control (handling CGI input)
*********************************************************************/
void ctrl_del_value(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	unsigned long i_val				/* in : value index in cntxt->form->ctrl->val */
);

/*********************************************************************
** Function : ctrl_set_format_attr
** Description : read control format attributes in ctrl->attr or form / default form
*********************************************************************/
int ctrl_set_format_attr(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	EVA_ctrl *ctrl					/* in : control to process */
);

/*********************************************************************
** Function : ctrl_set_common_attr
** Description : read control attributes in ctrl->attr or form / default form
*********************************************************************/
int ctrl_set_common_attr(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	EVA_ctrl *ctrl					/* in : control to process */
);

/*********************************************************************
** Function : ctrl_add_new
** Description : add a control to form list
*********************************************************************/
int ctrl_add_new(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idctrl,			/* in : IdObj of control to add */
	unsigned long *i_ctrl			/* out : index of new control in cntxt->form->ctrl */
);

/*********************************************************************
** Function : ctrl_add_child
** Description : add child controls except otpions
*********************************************************************/
#define CTRL_ADD_CHILD(i, children) {\
		if(ctrl_add_child(cntxt, i, children)) STACK_ERROR; \
		ctrl = form->ctrl + i_ctrl; }

/*********************************************************************
** Function : ctrl_add_child
** Description : add child controls except otpions
*********************************************************************/
int ctrl_add_child(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	DynTable *childlist				/* in : list of children controls ids */
);

/*********************************************************************
** Function : ctrl_set_attr
** Description : set the values of a control attribute
*********************************************************************/
int ctrl_set_attr(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : control to read */
	char *attr,						/* in : control field to search for */
	DynTable *data					/* in : values to store */
);

/*********************************************************************
** Function : ctrl_set_attr_txt
** Description : set text value of a control attribute
*********************************************************************/
int ctrl_set_attr_txt(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : control to read */
	char *attr,						/* in : control field to search for */
	char *txt, size_t len			/* in : text to store */
);

/*********************************************************************
** Function : ctrl_check_error_status
** Description : check for warnings or errors on a control
*********************************************************************/
int ctrl_check_error_status(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl					/* in : control to process */
);

/*********************************************************************
** Function : ctrl_add_opt_btn
** Description : output HTML code for the 'input options' button of a control
*********************************************************************/
int ctrl_add_opt_btn(
	EVA_context *cntxt,			/* in/out : execution context data */
	EVA_ctrl *ctrl	 			/* in : input control to process */
);

/*********************************************************************
** Function : ctrl_put_label
** Description : output HTML code for the label of a control
*********************************************************************/
int ctrl_put_label(
	EVA_context *cntxt,			/* in/out : execution context data */
	EVA_ctrl *ctrl,	 			/* in : input control to process */
	char *position				/* in : cell position */
);

/*********************************************************************
** Function : ctrl_format_pos
** Description : output control position & format with label if needed
*********************************************************************/
int ctrl_format_pos(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,	 				/* in : control to process */
	int b_head		 				/* in : output header if 1, footer else */
);

/*********************************************************************
** Function : ctrl_put_table_header
** Description : handles GROUP controls
*********************************************************************/
int ctrl_put_table_header(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl						/* in : control to process */
);

/*********************************************************************
** Function : ctrl_put_table_footer
** Description : output HTML footer for a new table
*********************************************************************/
int ctrl_put_table_footer(
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl						/* in : control to process */
);

/*********************************************************************
** Function : ctrl_put_hidden_old
** Description : add hidden input with initial control value to detect changes
*********************************************************************/
int ctrl_put_hidden_old(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	EVA_ctrl *ctrl,					/* in : control to process */
	unsigned long i,				/* in : value index to proces in ctrl->val */
	DynBuffer *name,				/* in : CGI name of input holding ctrl->val[i] */
	char *txt, size_t len			/* in original value (use ctrl->val[i] if NULL) */
);

/*********************************************************************
** Function : ctrl_primary_handler
** Description : call primary handler for a control	with access check
*********************************************************************/
int ctrl_primary_handler(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : put_showhelp
** Description : output html code for javascript call to ShowHelp
*********************************************************************/
int put_showhelp(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **html				/* in/out : HTML output buffer */
);

/*********************************************************************
** Function : ctrl_parse_access
** Description : parse access check keywords
*********************************************************************/
void ctrl_parse_access(
	int *access,					/* in/out : access bits are or-ed to the given value */
	char *exec,						/* in : control execution mode */
	char *objcreat,					/* in : create object access */
	char *objdel					/* in : delete object access */
);

/*********************************************************************
** Function : ctrl_check_condlist
** Description : evaluate a condition table line
*********************************************************************/
int ctrl_check_condlist(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	int *res,						/* out : 0 if contion false, non zero else */
	DynTable *cond,					/* in : table of conditions to process (IdObj) */
	unsigned long line				/* in : table line to process (AND combination on columns of line)
											if ~0UL : process all lines (AND combination on column 0) */
);

/*********************************************************************
** Function : ctrl_check_condseq
** Description : evaluate a condition sequence - return matched line
*********************************************************************/
int ctrl_check_condseq(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *cond,					/* in : table of conditions to process (IdObj) */
	unsigned long *match			/* out : index of first true condition - cond->nbrows if no match */
);

/*********************************************************************
** Function : ctrl_check_access
** Description : get access status of a control for the identified user
*********************************************************************/
int ctrl_check_access(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl					/* in : control to check for access status
									  out : access status is set in ctrl->access */
);

/*********************************************************************
** Function : build_open_btn_name
** Description : build CGI name for an open button
*********************************************************************/
size_t build_open_btn_name(				/* return : length of opname */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *opname,						/* out : button CGI name (64 chars min) */
	unsigned long id_form,				/* in : form id to open */
	unsigned long id_obj,				/* in : object id to open */
	int loc,							/* in : opened window location */
	int mode							/* in : opened window display mode */
);

/*********************************************************************
** Function : html_put_open_btn
** Description : output an open button
*********************************************************************/
int html_put_open_btn(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *opname,						/* out : button CGI name */
	DynBuffer *label,					/* in : form label */
	DynBuffer *objname,					/* in : object label */
	DynBuffer *notes,					/* in : object notes */
	DynBuffer *img,						/* in : symbol for normal image */
	DynBuffer *imgsel,					/* in : symbol for selected image */
	unsigned long id_form,				/* in : form id to open */
	unsigned long id_obj,				/* in : object id to open */
	int loc,							/* in : opened window location */
	int b_disabled,						/* in : button is disabled if not 0 */
	int mode							/* in : open mode */
);

/*********************************************************************
** Function : ctrl_add_symbol_btn
** Description : output an open button for an object
*********************************************************************/
int ctrl_add_symbol_btn(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl,						/* in : related control */
	DynTableCell *val,					/* in : value associated with button */
	DynTable *data,						/* in : object data to be opened by the button */
	unsigned long beg,					/* in : first line to process in data */
	char *bg_color,						/* in : output cell separators with given color if not NULL */
	char *dispmode						/* in : display mode */
);

/*********************************************************************
** Function : ctrl_filter_idobj
** Description : create a heap table IdList with ids of objects matching a filter
*********************************************************************/
int ctrl_filter_idobj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : list of objects Ids matching control filter
											if NULL, a temporary table IdList is created to hold the results */
	EVA_ctrl *ctrl					/* in : control containing filter information */
);

