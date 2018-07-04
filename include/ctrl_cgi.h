/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_cgi.h
** Description : CGI handling utilities for controls
**      Author : Alain BOUTE
**     Created : Dec 28 2002
*********************************************************************/


/*********************************************************************
** Macro : CTRL_CGINAMEVAL
** Description : build the complete CGI input name of a control	value
*********************************************************************/
#define	CTRL_CGINAMEVAL(name, i) { \
	if(ctrl_cgi_name(cntxt, ctrl, dyntab_cell(&ctrl->val, i, 0), 1, name, 0, NULL, 0)) STACK_ERROR; \
	ctrl->b_selected = 1; }

/*********************************************************************
** Macro : CTRL_CGINAMEBTN
** Description : build the complete CGI input name of a control	button
*********************************************************************/
#define	CTRL_CGINAMEBTN(name, val, btn) { if(ctrl_cgi_name(cntxt, ctrl, val, 1, name, 'B', btn)) STACK_ERROR; }
#define	CTRL_CGINAMEBTN1(name, val, btn, sz_btn) { if(ctrl_cgi_name(cntxt, ctrl, val, 1, name, 'B', btn, sz_btn)) STACK_ERROR; }

/*********************************************************************
** Macro : CTRL_CGINAMESUBFIELD
** Description : build the complete CGI input name of a control	subfield
*********************************************************************/
#define	CTRL_CGINAMESUBFIELD(name, val, field) { if(ctrl_cgi_name(cntxt, ctrl, val, 1, name, 'D', add_sz_str(field))) STACK_ERROR; }
#define	CTRL_CGINAMESUBFIELD1(name, val, txt, len) { if(ctrl_cgi_name(cntxt, ctrl, val, 1, name, 'D', txt, len)) STACK_ERROR; }

/*********************************************************************
** Function : ctrl_cgi_name
** Description : build final CGI name for a control value
*********************************************************************/
int ctrl_cgi_name(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : control to make name for */
	DynTableCell *val,				/* in : value to make name for */
	int b_renumber,					/* in : build value number if 1 and val is NULL */
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
	size_t subfield_sz				/* in : subfield name string size */
);

/*********************************************************************
** Macro : CTRL_GET_CGI_SUBFIELD
** Description : get a control option from CGI data
*********************************************************************/
#define CTRL_GET_CGI_SUBFIELD(tag) { if(cgi_get_subfield(cntxt, &cgival, ctrl, tag, 0, 0, 0)) STACK_ERROR; }
#define CTRL_GET_CGI_SUBFIELD1(tag) { if(cgi_get_subfield(cntxt, &cgival, ctrl, tag, 1, 0, 0)) STACK_ERROR; }

/*********************************************************************
** Macro : CTRL_GET_CGI_SUBFIELD_NL
** Description : get a control option for current Num & Line from CGI data
*********************************************************************/
#define CTRL_GET_CGI_SUBFIELD_NL(tag) { if(cgi_get_subfield(cntxt, &cgival, ctrl, tag, 0, cntxt->form->Num, cntxt->form->Line)) STACK_ERROR; }
#define CTRL_GET_CGI_SUBFIELD1_NL(tag) { if(cgi_get_subfield(cntxt, &cgival, ctrl, tag, 1, cntxt->form->Num, cntxt->form->Line)) STACK_ERROR; }

/*********************************************************************
** Function : ctrl_put_hidden
** Description : output a control value as CGI hidden input
*********************************************************************/
int ctrl_put_hidden(				/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : related control */
	unsigned long i					/* in : value index in ctrl->val */
);

/*********************************************************************
** Function : cgi_get_subfield
** Description : return	cgi inputs matching the given name
*********************************************************************/
int cgi_get_subfield(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *cgival,				/* out : CGI values table */
	EVA_ctrl *ctrl,					/* in : related control */
	char *subfield,					/* in : subfield name */
	int b_keep,						/* in : mark for no reoutput if 0 */
	unsigned long num,				/* in : value index */
	unsigned long line				/* in : value index */
);

/*********************************************************************
** Macro : CTRL_PUT_CGI_SUBFIELD_VAL
** Description : output hidden CGI input for control options
*********************************************************************/
#define CTRL_PUT_CGI_SUBFIELD_VAL(tag, val, val_sz) { if(cgi_put_subfield(cntxt, ctrl, tag, val, val_sz)) STACK_ERROR; }

/*********************************************************************
** Macro : CTRL_PUT_CGI_SUBFIELD
** Description : output hidden CGI input for control options
*********************************************************************/
#define CTRL_PUT_CGI_SUBFIELD(tag, val) CTRL_PUT_CGI_SUBFIELD_VAL(tag, dyntab_val(val, 0, 0), dyntab_sz(val, 0, 0))

/*********************************************************************
** Macro : CTRL_PUT_CGI_SUBFIELD_INT
** Description : output hidden CGI input for control options - int version
*********************************************************************/
#define CTRL_PUT_CGI_SUBFIELD_INT(tag, val) \
{ \
	char txt[32];	\
	sprintf(txt, "%lu", val); \
	if(cgi_put_subfield(cntxt, ctrl, tag, txt, 0)) STACK_ERROR; \
}

/*********************************************************************
** Function : cgi_put_subfield
** Description : output hidden CGI input for control options
*********************************************************************/
int cgi_put_subfield(				/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : related control */
	char *subfield,					/* in : subfield name */
	char *val, size_t val_sz		/* in : value to use */
);
