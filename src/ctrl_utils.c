/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_utils.c
** Description : controls handling utilities
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_new
** Description : add a control to form list
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_new"
#define ERR_CLEANUP	
int ctrl_add_new(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idctrl,			/* in : IdObj of control to add */
	unsigned long *_i_ctrl			/* out : index of new control in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	unsigned long i_ctrl = form->nb_ctrl;
	EVA_ctrl *ctrl;

	/* Realloc controls table */
	M_REALLOC(EVA_ctrl, form->ctrl, form->nb_ctrl);
	if(_i_ctrl) *_i_ctrl = i_ctrl;
	ctrl = form->ctrl + i_ctrl;

	/* Read form control in ctrl->attr & store ctrl->id */
	if(qry_cache_idobj(&ctrl->attr, idctrl)) STACK_ERROR;
	DYNTAB_ADD_INT(&ctrl->id, 0, 0, idctrl);

	/* Handle derived control */
	if(ctrl_read_baseobj(cntxt, &ctrl->attr)) STACK_ERROR;

	/* Read control common attributes */
	if(ctrl_set_common_attr(cntxt, ctrl)) STACK_ERROR;

	/* Look for control handlers */
	while(	ctrl_handlers[ctrl->i_handler].control && 
			(	strcmp(ctrl_handlers[ctrl->i_handler].control, ctrl->CONTROL) ||
				ctrl_handlers[ctrl->i_handler].type &&
				strcmp(ctrl_handlers[ctrl->i_handler].type, ctrl->TYPE)))
		ctrl->i_handler++;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_from_cginame
** Description : search the control matching the given CGI name
*********************************************************************/
int ctrl_from_cginame(				/* return : control index in cntxt->form->ctrl or 0 if no match */
	EVA_context *cntxt,				/* in : execution context data */
	char *cginame, size_t sz		/* in : searched CGI name */
){
	EVA_form *form = cntxt->form;
	unsigned long j;

	/* Search loop on form->ctrl */
	for(j = 0; j < form->nb_ctrl; j++)
	{
		DynBuffer *name = form->ctrl[j].cginame;
		if(name && name->cnt <= sz && !strncmp(cginame+1, name->data + 1, name->cnt - 1)) 
			return j;
	}

	return 0;
}

/*********************************************************************
** Function : ctrl_from_id
** Description : search the control in a control children matching the given id
*********************************************************************/
unsigned long ctrl_from_id(			/* return : control index in cntxt->form->ctrl or -1 if no match */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : parent control index in cntxt->form->ctrl */
	char *id, size_t sz_id			/* in : searched control id */
){
	EVA_form *form = cntxt->form;
	unsigned long j;

	for(j = form->ctrl[i_ctrl].i_child; j; j = form->ctrl[j].i_brother)
		if(!dyntab_txt_cmp(id, sz_id, &form->ctrl[j].id, 0, 0))
			return j;

	return 0;
}

/*********************************************************************
** Function : ctrl_add_value
** Description : add a value to the given control (handling CGI input)
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_value"
#define ERR_CLEANUP M_FREE(name)
int ctrl_add_value(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *txt, size_t len,			/* in : value to add */
	unsigned long num,				/* in : number index for value */
	unsigned long line,				/* in : line index for value */
	char b_modified					/* in : modification status */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTableCell *c;
	unsigned long row = ctrl->val.nbrows;
	DynBuffer *name = NULL;

	DYNTAB_ADD(&ctrl->val, row, 0, txt, len, NO_CONV);
	c = dyntab_cell(&ctrl->val, row, 0);
	c->Line = line;
	c->Num = num;
	c->b_modified = b_modified;
	ctrl->b_modified |= b_modified;
	DYNBUF_ADD_BUF(&name, ctrl->cginame, NO_CONV);
	if(cgi_build_finalname(cntxt, &name, 'D', NULL, 0, num, line, 0) ||
		cgi_add_input(cntxt, &c->i_cgi, DYNBUF_VAL_SZ(name), txt, len)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_del_value
** Description : delete a value in the given control (handling CGI input)
*********************************************************************/
void ctrl_del_value(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	unsigned long i_val				/* in : value index in cntxt->form->ctrl->val */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	cgi_value_setkeep(cntxt, &ctrl->val, i_val, 2);
	dyntab_del_rows(&ctrl->val, i_val, 1);
	ctrl->b_modified = 1;
}

/*********************************************************************
** Function : ctrl_add_child
** Description : add child controls except otpions
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_child"
#define ERR_CLEANUP
int ctrl_add_child(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : parent control index in cntxt->form->ctrl */
	DynTable *childlist				/* in : list of children controls ids */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	EVA_ctrl *newctrl;
	unsigned long i_newctrl, i_prevctrl;
	unsigned long i, j;

	if(form->b_noctrltree) RETURN_OK;

	switch(form->step)
	{
	case CtrlRead:
		if(!childlist || !childlist->nbrows) RETURN_OK;

		/* Initialize i_prevctrl with last child */
		for(i_prevctrl = ctrl->i_child; form->ctrl[i_prevctrl].i_brother; i_prevctrl = form->ctrl[i_prevctrl].i_brother); 

		/* Remove controls in childlist already in parents - prevent infinite loops on CTRLTREE */
		for(i = i_ctrl; i != form->ctrl[i].i_parent; i = form->ctrl[i].i_parent)
			for(j = 1; childlist->nbrows && j <= childlist->nbrows; j++)
				if(!dyntab_cmp(&form->ctrl[i].id, 0, 0, childlist, j - 1, 0))
					dyntab_del_rows(childlist, --j, 1);

		/* Read children objects data */
		for(i = 0; i < childlist->nbrows && !cntxt->b_terminate; i++)
		{
			/* Add a new control in form list */
			CGIData *clickcgibtn;
			if(ctrl_add_new(cntxt, DYNTAB_TOULRC(childlist, i, 0), &i_newctrl)) STACK_ERROR;

			/* Store controls tree chaining */
			ctrl = form->ctrl + i_ctrl;
			newctrl = form->ctrl + i_newctrl;
			newctrl->i_parent = i_ctrl;
			if(!ctrl->i_child) ctrl->i_child = i_newctrl;
			if(i_prevctrl) form->ctrl[i_prevctrl].i_brother = i_newctrl;
			i_prevctrl = i_newctrl;

			/* Set control clicked condition */
			clickcgibtn = cntxt->cgi ? cntxt->cgi + cntxt->cgibtn : NULL;
			if(cgi_build_basename(cntxt, &newctrl->cginame, i_newctrl, 'C')) STACK_ERROR;
			if(clickcgibtn && newctrl->cginame->cnt <= clickcgibtn->name_sz && 
				!strncmp(clickcgibtn->name + 1, newctrl->cginame->data + 1, newctrl->cginame->cnt - 1)) 
				cntxt->form->i_ctrl_clic = i_newctrl;
		
			/* Call control handler */
			CTRL_PRI_HDLR(i_newctrl);
			newctrl = form->ctrl + i_newctrl;

			/* Set CGI name if not set by handler */
			if(!newctrl->cginame && cgi_build_basename(cntxt, &newctrl->cginame, i_newctrl, 'C'))
				STACK_ERROR;
		}
		break;

	default:
		/* Call control handler for each child */
		if(childlist)
		{
			for(i = 0; i < childlist->nbrows && !cntxt->b_terminate; i++)
			{
				/* Search control & call control handler if found */
				j = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(childlist, i, 0));
				if(j) CTRL_PRI_HDLR(j);
			}
		}
		else
		{
			for(i = ctrl->i_child; i && !cntxt->b_terminate; i = form->ctrl[i].i_brother)
				CTRL_PRI_HDLR(i);
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_set_attr
** Description : set the values of a control attribute
*********************************************************************/
#define ERR_FUNCTION "ctrl_set_attr"
#define ERR_CLEANUP
int ctrl_set_attr(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : control to read */
	char *attr,						/* in : control field to search for */
	DynTable *data					/* in : values to store */
){
	unsigned long i;

	if(!ctrl || !attr || !*attr) RETURN_OK;

	/* Remove attribute's current values (set field name to null string) */
	for(i = 0; i < ctrl->attr.nbrows; i++)
	{
		DynTableCell *cell = dyntab_cell(&ctrl->attr, i, 0);
		if(cell->field && !strcmp(attr, cell->field))
			cell->field[0] = 0;
	}

	/* Add attribute's new values */
	if(data)
		for(i = 0; i < data->nbrows; i++)
			if(dyntab_copy(&ctrl->attr, ctrl->attr.nbrows, 0, dyntab_cell(data, i, 0), 1))
				RETURN_ERR_MEMORY;
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_set_attr_txt
** Description : set text value of a control attribute
*********************************************************************/
#define ERR_FUNCTION "ctrl_set_attr_txt"
#define ERR_CLEANUP	DYNTAB_FREE(tval)
int ctrl_set_attr_txt(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : control to read */
	char *attr,						/* in : control field to search for */
	char *txt, size_t len			/* in : text to store */
){
	DynTable tval = { 0 }; 
	DynTableCell *cell;

	if(dyntab_add(&tval, 0, 0, txt, len, NO_CONV)) RETURN_ERR_MEMORY;	
	cell = dyntab_cell(&tval, 0, 0);
	cell->field = attr;
	cell->b_dontfreefield = 1;
	cell->IdObj = DYNTAB_TOUL(&ctrl->id);
	if(ctrl_set_attr(cntxt, ctrl, attr, &tval)) STACK_ERROR; 

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_check_error_status
** Description : check for warnings or errors on a control
*********************************************************************/
#define ERR_FUNCTION "ctrl_check_error_status"
#define ERR_CLEANUP DYNTAB_FREE(res); \
					DYNTAB_FREE(chkexpr); \
					DYNTAB_FREE(status); \
					DYNTAB_FREE(message); \
					DYNTAB_FREE(debug)
int ctrl_check_error_status(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl					/* in : control to process */
){
	EVA_form *form = cntxt->form;
	DynTable res = {0};
	DynTable chkexpr = {0};
	DynTable status = {0};
	DynTable message = {0};
	DynTable debug = {0};
	unsigned long i, j, k, cnt;
	char *multiple = CTRL_ATTR_VAL(MULTIPLE);
	int b_editable;

	/* Return if no check needed */
	if(!ctrl->storage) RETURN_OK;

	/* Check if editable in parents - return if not */
	b_editable = ctrl->access & AccessEdit && form->ctrl->access & (form->b_newobj ? AccessCreate : AccessEdit);
	for(i = ctrl->i_parent; i && b_editable; i = form->ctrl[i].i_parent)
		b_editable = (form->ctrl[i].access & AccessEdit) != 0;
	if(!b_editable) RETURN_OK;

	/* Check for multiple values */
	if((!*multiple || !strcmp("No", multiple)) && ctrl->val.nbrows > 1)
	{
		ctrl->error = 2;
		if(ctrl->errmsg) DYNBUF_ADD_STR(&ctrl->errmsg, "\n");
		DYNBUF_ADD_STR(&ctrl->errmsg, "Vous devez sélectionner une seule valeur");
		RETURN_OK;
	}

	if(!*ctrl->ALLOWEMPTY || !strcmp("Yes", ctrl->ALLOWEMPTY)) RETURN_OK;

	/* Count # of non empty values */
	for(i = 0, cnt = 0; i < ctrl->val.nbrows; i++)
		if(dyntab_sz(&ctrl->val, i, 0)) cnt++;

	/* Handle condition */
	if(!strcmp("_EVA_CONDITION", ctrl->ALLOWEMPTY))
	{
		/* Read conditions */
		CTRL_ATTR_TAB(chkexpr, CHECK_EXPR);
		k = chkexpr.nbrows;
		CTRL_ATTR_TAB(status, ERR_STATUS);
		if(k < status.nbrows) k = status.nbrows;
		CTRL_ATTR_TAB(message, ERRMSG);
		if(k < message.nbrows) k = message.nbrows;
		CTRL_ATTR_TAB(debug, DEBUG_SQL_EXPR);

		/* Process each line */
		for(i = 0; i < k; i++)
		{
			/* Evaluate condition if applicable */
			int sql_trace = cntxt->sql_trace;
			char *val, *stat = dyntab_val(&status, i, 0), *msg = dyntab_val(&message, i, 0);
			if(dyntab_sz(&debug, i, 0)) cntxt->sql_trace = DEBUG_SQL_RES;
			if(!dyntab_sz(&chkexpr, i, 0))
			{
				if(!*msg && !cnt) { ctrl->ALLOWEMPTY = stat; break; }
				DYNTAB_ADD_INT(&res, 0, 0, cnt ? 0 : 1);
			}
			else if(ctrl_eval_fieldexpr(cntxt, ctrl, &res, dyntab_val(&chkexpr, i, 0))) CLEAR_ERROR;
			cntxt->sql_trace = sql_trace;

			/* Handle result - OR multiple values */
			val = dyntab_val(&res, 0, 0);
			for(j = 1; j < res.nbrows; j++)
			{
				DynTableCell *c = dyntab_cell(&res, j, 0);
				if((!cntxt->form->Line || !c->Line || c->Line == cntxt->form->Line) &&
					(!cntxt->form->Num || !c->Num || c->Num == cntxt->form->Num) &&
					c->txt && c->txt[0] && strcmp(c->txt, "0"))
				{
					val = c->txt;
					break;
				}
			}

			/* Handle result - continue if condition not matched */
			if(!*val || !strcmp(val, "0")) continue;

			/* Condition matched : handle status & message */
			if(!*stat && !strcmp(val, "1")) RETURN_OK;
			if(!*msg && strcmp(val, "1")) msg =  val; 
			ctrl->error = !strcmp(stat, "Yes") ? 0 : strcmp(stat, "No") ? 1 : 2;
			if(!*msg && ctrl->error) msg = ctrl->error & 2 ? "Saisie obligatoire" : "Saisie recommandée";
			if(ctrl->errmsg) DYNBUF_ADD_STR(&ctrl->errmsg, "\n");
			DYNBUF_ADD(&ctrl->errmsg, msg, 0, NO_CONV);
			RETURN_OK;
		}
		if(i == k) RETURN_OK;
	}

	/* Set error if applicable */
	ctrl->error = (cnt || !*ctrl->ALLOWEMPTY || !strcmp("Yes", ctrl->ALLOWEMPTY)) ? 0 : strcmp("No", ctrl->ALLOWEMPTY) ? 1 : 2;
	if(ctrl->error)
	{
		if(ctrl->errmsg) DYNBUF_ADD_STR(&ctrl->errmsg, "\n");
		DYNBUF_ADD(&ctrl->errmsg, ctrl->error & 2 ? "Saisie obligatoire" : "Saisie recommandée", 0, NO_CONV); 
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_opt_btn
** Description : output HTML code for the 'input options' button of a control
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_opt_btn"
#define ERR_CLEANUP M_FREE(tooltip); \
					M_FREE(cginame); \
					M_FREE(image); \
					M_FREE(imgsel)
int ctrl_add_opt_btn(
	EVA_context *cntxt,			/* in/out : execution context data */
	EVA_ctrl *ctrl	 			/* in : input control to process */
){
	EVA_form *form = cntxt->form;
	DynBuffer *tooltip = NULL;
	DynBuffer *cginame = NULL;
	DynBuffer *image = NULL;
	DynBuffer *imgsel = NULL;
	int b_button = cntxt->opt_btn_mode == OptBtn_OpenSame || cntxt->opt_btn_mode ==  OptBtn_OpenBottom;

	if(form->step == HtmlPrint) RETURN_OK;
	if(STRCMPNUL(ctrl->OPTIONBUTTON, "_EVA_ALWAYS") && !b_button && !ctrl->error && (
		!STRCMPNUL(ctrl->OPTIONBUTTON, "_EVA_NONE") ||
		!(ctrl->NOTES && *ctrl->NOTES) ||
		!STRCMPNUL(ctrl->OPTIONBUTTON, "_EVA_EDIT") && form && form->step != HtmlEdit))
		RETURN_OK;

	DYNBUF_ADD3(&tooltip, " [", ctrl->LABEL, 0, NO_CONV, "]");


	/* Button images depend error status & control type */
	if(!STRCMPNUL(ctrl->CONTROL, "_EVA_OPTION"))
	{
		DYNBUF_ADD_STR(&image, "_eva_opt_help_small.gif");
		DYNBUF_ADD_STR(&imgsel, "_eva_opt_help_small_s.gif");
	}
	else switch(ctrl->error)
	{
	case 1:
		DYNBUF_ADD_STR(&image, "_eva_blink_orange.gif");
		DYNBUF_ADD_STR(&imgsel, "_eva_btn_helpbook_s.gif");
		break;
	case 2:
		DYNBUF_ADD_STR(&image, "_eva_blink_red.gif");
		DYNBUF_ADD_STR(&imgsel, "_eva_btn_helpbook_s.gif");
		break;
	default:
		DYNBUF_ADD_STR(&image, "_eva_btn_helpbook.gif");
		DYNBUF_ADD_STR(&imgsel, "_eva_btn_helpbook_s.gif");
	}

	/* Check allowed user forms if any */
	if(b_button && cntxt->viewforms.nbrows)
	{
		unsigned long i;
		for(i = 0, b_button =0; !b_button && i < cntxt->viewforms.nbrows; i++)
			b_button = !dyntab_cmp(&form->id_form, 0, 0, &cntxt->viewforms, i, 0);
	}

	/* Format button tooltip & CGI name */
	if(ctrl->errmsg) DYNBUF_ADD3_BUF(&tooltip, " - ", ctrl->errmsg, NO_CONV, "");
	if(ctrl->NOTES && *ctrl->NOTES) DYNBUF_ADD3(&tooltip, "\n\n", ctrl->NOTES, 0, NO_CONV, "");
	if(b_button)
	{
		DYNBUF_ADD3(&tooltip, "\n\nCliquez pour plus d'informations sur [", ctrl->LABEL, 0, NO_CONV, "]");
		if(STRCMPNUL(ctrl->TYPE, "_EVA_TITLE"))
			DYNBUF_ADD_BUF(&cginame, ctrl->cginame, NO_CONV)
		else
			DYNBUF_ADD_BUF(&cginame, form->ctrl->cginame, NO_CONV);
		*cginame->data = 'I';

		if(put_html_button(cntxt, cginame->data, "Infos", 
						image->data, imgsel->data, tooltip->data, 0, 4))
			STACK_ERROR;
	}
	else
	{
		if(put_html_image(cntxt, NULL, image->data, tooltip->data,
							NULL, NULL, 0))
			STACK_ERROR;
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_put_label
** Description : output HTML code for the label of a control
*********************************************************************/
#define ERR_FUNCTION "ctrl_put_label"
#define ERR_CLEANUP 
int ctrl_put_label(
	EVA_context *cntxt,			/* in/out : execution context data */
	EVA_ctrl *ctrl,	 			/* in : input control to process */
	char *position				/* in : cell position */
){
	int b_opt = STRCMPNUL(ctrl->OPTIONBUTTON, "_EVA_NONE") != 0 && cntxt->opt_btn_mode != OptBtn_None;
	int b_nobr = *CTRL_ATTR_VAL(LABEL_NOBR) == '1';
	char *br_style = CTRL_ATTR_VAL(BORDER_STYLE);
	
	/* Label color depends on error status if no option button */
	char *labelcolor = (b_opt || ctrl->error < 1) ? ctrl->LABELFONTCOLOR :
						ctrl->error < 2 ? "DD8800" : "DD0000";

	/* Handle unspecified row/col span */
	if(!ctrl->LABELROWSPAN) ctrl->LABELROWSPAN = 1;
	if(!ctrl->LABELCOLSPAN) ctrl->LABELCOLSPAN = 1;

	/* Output cell header for label */
	if(put_html_format_pos(cntxt, position,
		ctrl->LABELALIGN, ctrl->LABELVALIGN, ctrl->LABELWIDTH, ctrl->LABELHEIGHT,
		ctrl->LABELBGCOLOR, ctrl->LABELBACKGROUND, ctrl->LABELCOLSPAN, ctrl->LABELROWSPAN,
		ctrl->LABELFONTFACE, ctrl->LABELFONTSIZE, labelcolor, ctrl->LABEL_STYLE,
		ctrl->LABELBOLD[0] == '1', ctrl->LABELITALIC[0] == '1', ctrl->LABELUNDERLINE[0] == '1', 0,
		1)) STACK_ERROR;

	/* Output <div> for shrink button */
	if(!strncmp(br_style, add_sz_str("_EVA_SHRINKBTN")))
		DYNBUF_ADD_STR(cntxt->form->html, "<div style='float:left'>");

	/* Output label */
	if(b_nobr) DYNBUF_ADD_STR(cntxt->form->html, "<nobr>");
	if(!strcmp(ctrl->CONTROL, "_EVA_BUTTON"))
	{
		/* Output label as button for buttons */
		int b_link = !strcmp(CTRL_ATTR_VAL(ACTION), "_EVA_HTTPLINK");
		char *link = CTRL_ATTR_VAL(URL);
		if(put_html_button(cntxt, b_link ? link : ctrl->cginame->data, ctrl->LABEL, NULL, NULL, ctrl->NOTES, 0, b_link ? 96 : 32)) STACK_ERROR;
	}
	else
		DYNBUF_ADD(cntxt->form->html, ctrl->LABEL, 0, TO_HTML);

	/* Output option button if applicable */
	if(b_opt && ctrl_add_opt_btn(cntxt, ctrl)) STACK_ERROR;
	if(b_nobr) DYNBUF_ADD_STR(cntxt->form->html, "</nobr>");

	/* Output shrink button if applicable */
	if(!strncmp(br_style, add_sz_str("_EVA_SHRINKBTN")))
	{
		char *ud = br_style[14] ? "down" : "up";
		char printbuf[1024];

		DYNBUF_ADD3_CELL(cntxt->form->html,
			"</div><div style='text-align:right;width:100%'><img onClick=ToggleMenu(this,",
			&ctrl->id, 0, 0, NO_CONV, ") title='Cliquez pour réduire' onMouseOver=SwapImg(this)");
		dynbuf_print3(cntxt->form->html,
			" src=/img/shrink_%s.gif img=/img/shrink_%s.gif img1=/img/shrink_%s_s.gif></div>", ud, ud, ud);
	}

	/* Output cell footer for label */
	if(put_html_format_pos(cntxt, position,
		ctrl->LABELALIGN, ctrl->LABELVALIGN, ctrl->LABELWIDTH, ctrl->LABELHEIGHT,
		ctrl->LABELBGCOLOR, ctrl->LABELBACKGROUND, ctrl->LABELCOLSPAN, ctrl->LABELROWSPAN,
		ctrl->LABELFONTFACE, ctrl->LABELFONTSIZE, labelcolor, ctrl->LABEL_STYLE,
		ctrl->LABELBOLD[0] == '1', ctrl->LABELITALIC[0] == '1', ctrl->LABELUNDERLINE[0] == '1', 0,
		0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_format_cell
** Description : output control position & format
*********************************************************************/
#define ERR_FUNCTION "ctrl_format_cell"
#define ERR_CLEANUP 
int ctrl_format_cell(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,	 				/* in : control to process */
	int b_head		 				/* in : output header if 1, footer else */
){
	if(b_head)
	{
		/* Output cell header for control */
		if(put_html_format_pos(cntxt, ctrl->POSITION,
							ctrl->ALIGN, ctrl->VALIGN, ctrl->WIDTH, ctrl->HEIGHT,
							ctrl->BGCOLOR, ctrl->BACKGROUND, ctrl->COLSPAN, ctrl->ROWSPAN,
							ctrl->FONTFACE, ctrl->FONTSIZE, ctrl->FONTCOLOR, ctrl->CELL_STYLE,
							ctrl->BOLD[0] == '1', ctrl->ITALIC[0] == '1', ctrl->UNDERLINE[0] == '1', 0, b_head)) 
			STACK_ERROR;
	}
	else
	{
		/* Handle control notes */
		char *mode = CTRL_ATTR_VAL(HELPTEXT);
		if(ctrl->NOTES && *ctrl->NOTES && (!strcmp(mode, "_EVA_ALWAYS") || !strcmp(mode, "_EVA_EDIT") && cntxt->form->step == HtmlEdit))
		{
			DYNBUF_ADD3(cntxt->form->html, "<div class=EVANotes>", ctrl->NOTES, 0, TO_HTML, "</div>");
		}

		/* Output cell footer for control */
		if(put_html_format_pos(cntxt, ctrl->POSITION,
							ctrl->ALIGN, ctrl->VALIGN, ctrl->WIDTH, ctrl->HEIGHT,
							ctrl->BGCOLOR, ctrl->BACKGROUND, ctrl->COLSPAN, ctrl->ROWSPAN,
							ctrl->FONTFACE, ctrl->FONTSIZE, ctrl->FONTCOLOR, ctrl->CELL_STYLE,
							ctrl->BOLD[0] == '1', ctrl->ITALIC[0] == '1', ctrl->UNDERLINE[0] == '1', 0, b_head)) 
			STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_format_pos
** Description : output control position & format with label & notes if needed
*********************************************************************/
#define ERR_FUNCTION "ctrl_format_pos"
#define ERR_CLEANUP 
int ctrl_format_pos(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,	 				/* in : control to process */
	int b_head		 				/* in : output header if 1, footer else */
){
	if(!cntxt->form->html) RETURN_OK;
	if(cntxt->debug & DEBUG_HTML && b_head)
	{
		DYNBUF_ADD3(cntxt->form->html, "\n<!--- Start ", ctrl->CONTROL, 0, NO_CONV, " / ");
		DYNBUF_ADD(cntxt->form->html, ctrl->TYPE, 0, NO_CONV);
		DYNBUF_ADD3(cntxt->form->html, " / ", ctrl->LABEL, 0, NO_CONV, " -->\n");
	}

	/* Handle NOBR */
	if(!b_head && ctrl->NOBR && *ctrl->NOBR == '1') DYNBUF_ADD_STR(cntxt->form->html, "</nobr>");

	/* Handle label on right side of control */
	if(!strcmp("_EVA_RIGHT", ctrl->LABELPOS))
	{
		ctrl->LABELROWSPAN = ctrl->ROWSPAN;
		if(b_head)
		{
			/* Output cell header for control */
			if(!ctrl->ALIGN[0]) ctrl->ALIGN = "right";
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;
		}
		else
		{
			/* Output cell footer for control */
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;

			/* Output label */
			if(!ctrl->LABELALIGN[0]) ctrl->LABELALIGN = "left";
			if(!ctrl->LABELVALIGN[0]) ctrl->LABELVALIGN = ctrl->VALIGN;
			if(ctrl_put_label(cntxt, ctrl, "_EVA_NewColumn")) STACK_ERROR;
		}
	}
	/* Handle label on top of control */
	else if(!strcmp("_EVA_TOP", ctrl->LABELPOS))
	{
		if(b_head)
		{
			if(!ctrl->LABELALIGN[0]) ctrl->LABELALIGN = ctrl->ALIGN;
			if(!ctrl->COLSPAN) ctrl->COLSPAN = 2;

			/* Output cell header for control & label */
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;

			/* Output label in table */
			if(!ctrl->LABELVALIGN[0]) ctrl->LABELVALIGN = "bottom";
			if(put_html_table_header(cntxt, NULL, "100%", NULL,
				ctrl->LABELBGCOLOR, ctrl->LABELBACKGROUND,0, 0, 0, NULL, NULL, NULL)) STACK_ERROR;
			if(ctrl_put_label(cntxt, ctrl, "_EVA_NewColumn")) STACK_ERROR;
			DYNBUF_ADD_STR(cntxt->form->html, "</tr></table>");

			/* Enclose control in DIV if applicable */
			if(*CTRL_ATTR_VAL(USE_DIV) == '1')
			{
				DYNBUF_ADD_STR(cntxt->form->html, "<table");
				if(*ctrl->TABLE_STYLE) DYNBUF_ADD3(cntxt->form->html, " class=", ctrl->TABLE_STYLE, 0, NO_CONV, "");
				DYNBUF_ADD_STR(cntxt->form->html, "><tr><td>");
			}
		}
		else
		{
			/* Output cell footer for control */
			if(*CTRL_ATTR_VAL(USE_DIV) == '1') DYNBUF_ADD_STR(cntxt->form->html, "</td></tr></table>");
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;
		}
	}
	/* Handle label on bottom of control */
	else if(!strcmp("_EVA_BOTTOM", ctrl->LABELPOS))
	{
		if(b_head)
		{
			if(!ctrl->LABELALIGN[0]) ctrl->LABELALIGN = ctrl->ALIGN;
			if(!ctrl->COLSPAN) ctrl->COLSPAN = 2;

			/* Output cell header for control & label */
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;

			/* Enclose control in DIV if applicable */
			if(*CTRL_ATTR_VAL(USE_DIV) == '1')
			{
				DYNBUF_ADD_STR(cntxt->form->html, "<table");
				if(*ctrl->TABLE_STYLE) DYNBUF_ADD3(cntxt->form->html, " class=", ctrl->TABLE_STYLE, 0, NO_CONV, "");
				DYNBUF_ADD_STR(cntxt->form->html, "><tr><td>");
			}
		}
		else
		{
			if(*CTRL_ATTR_VAL(USE_DIV) == '1') DYNBUF_ADD_STR(cntxt->form->html, "</td></tr></table>");

			/* Output label in table */
			if(!ctrl->LABELVALIGN[0]) ctrl->LABELVALIGN = "top";
			if(put_html_table_header(cntxt, NULL, "100%", NULL, ctrl->LABELBGCOLOR, ctrl->LABELBACKGROUND,0, 0, 0, NULL, NULL, NULL)) STACK_ERROR;
			if(ctrl_put_label(cntxt, ctrl, "_EVA_NewColumn")) STACK_ERROR;
			DYNBUF_ADD_STR(cntxt->form->html, "</tr></table>");

			/* Output cell footer for control & label */
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;
		}
	}
	/* Handle no label */
	else if(!strcmp("_EVA_NONE", ctrl->LABELPOS))
	{
		if(b_head)
		{
			/* Output cell header for control */
			if(!ctrl->COLSPAN) ctrl->COLSPAN = 2;
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;
		}
		else
		{
			/* Output cell footer for control */
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;
		}
	}
	/* Handle label on left side of control */
	else
	{
		ctrl->LABELROWSPAN = ctrl->ROWSPAN;
		if(b_head)
		{
			char *pos = ctrl->POSITION;

			/* Output label */
			if(!ctrl->LABELALIGN[0]) ctrl->LABELALIGN = "right";
			if(!ctrl->LABELVALIGN[0]) ctrl->LABELVALIGN = ctrl->VALIGN;
			if(ctrl_put_label(cntxt, ctrl, pos)) STACK_ERROR;

			/* Output cell header for control */
			ctrl->POSITION = "_EVA_NewColumn";
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;
			ctrl->POSITION = pos;
		}
		else
		{
			/* Output cell footer for control */
			if(ctrl_format_cell(cntxt, ctrl, b_head)) STACK_ERROR;
		}
	}

	/* Handle NOBR */
	if(b_head && ctrl->NOBR && *ctrl->NOBR == '1') DYNBUF_ADD_STR(cntxt->form->html, "<nobr>");

	if(cntxt->debug & DEBUG_HTML && !b_head)
	{
		DYNBUF_ADD3(cntxt->form->html, "\n<!--- EndOf ", ctrl->CONTROL, 0, NO_CONV, " / ");
		DYNBUF_ADD(cntxt->form->html, ctrl->TYPE, 0, NO_CONV);
		DYNBUF_ADD3(cntxt->form->html, " / ", ctrl->LABEL, 0, NO_CONV, " -->\n");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_put_table_header
** Description : handles GROUP controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_put_table_header"
#define ERR_CLEANUP
int ctrl_put_table_header(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl						/* in : control to process */
){
	char *br_style = CTRL_ATTR_VAL(BORDER_STYLE);
	char attr[64] = {0};

	/* Handle shrink button */
	if(!strncmp(br_style, add_sz_str("_EVA_SHRINKBTN")))
		snprintf(add_sz_str(attr), "id=Mnu%s%s", dyntab_val(&ctrl->id, 0, 0), br_style[14] ? " style='display:none'" : "");

	/* Output HTML debug info if applicable */
	if(cntxt->debug & DEBUG_HTML)
	{
		DYNBUF_ADD3(cntxt->form->html, "\n<!--- Start Table ", ctrl->CONTROL, 0, NO_CONV, " / ");
		DYNBUF_ADD(cntxt->form->html, ctrl->TYPE, 0, NO_CONV);
		DYNBUF_ADD3(cntxt->form->html, " / ", ctrl->LABEL, 0, NO_CONV, " -->\n");
	}

	/* Output HTML container table header */
	if(put_html_table_header(cntxt, 
			CTRL_ATTR_VAL(TABLEALIGN),
			ctrl->TABLEWIDTH,
			ctrl->TABLEHEIGHT,
			ctrl->TABLEBGCOLOR,
			ctrl->TABLEBACKGROUND,
			atoi(CTRL_ATTR_VAL(CELLSPACING)),
			atoi(CTRL_ATTR_VAL(CELLPADDING)),
			ctrl->BORDER,
			ctrl->TABLERULES,
			ctrl->TABLE_STYLE, attr))
		STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_put_table_footer
** Description : output HTML footer for a new table
*********************************************************************/
#define ERR_FUNCTION "ctrl_put_table_footer"
#define ERR_CLEANUP 
int ctrl_put_table_footer(
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl						/* in : control to process */
){
	CHECK_HTML_STATUS;
	DYNBUF_ADD_STR(html, "</tr></table>");

	if(cntxt->debug & DEBUG_HTML)
	{
		DYNBUF_ADD3(html, "\n<!--- Endof Table ", ctrl->CONTROL, 0, NO_CONV, " / ");
		DYNBUF_ADD(html, ctrl->TYPE, 0, NO_CONV);
		DYNBUF_ADD3(html, " / ", ctrl->LABEL, 0, NO_CONV, " -->\n");
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_put_hidden_old
** Description : add hidden input with initial control value to detect changes
*********************************************************************/
#define ERR_FUNCTION "ctrl_put_hidden_old"
#define ERR_CLEANUP
int ctrl_put_hidden_old(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	EVA_ctrl *ctrl,					/* in : control to process */
	unsigned long i,				/* in : value index to proces in ctrl->val */
	DynBuffer *name,				/* in : CGI name of input holding ctrl->val[i] */
	char *txt, size_t len			/* in : original value (use ctrl->val[i] if NULL) */
){
	EVA_form *form = cntxt->form;
	DynTableCell *val = ctrl ? dyntab_cell(&ctrl->val, i, 0) : NULL;
	if(form->b_newobj && (!ctrl || (val ? val->b_modified : ctrl->b_modified))) RETURN_OK;
	if(cgi_put_hiddenold(cntxt, name ? name : ctrl ? ctrl->cginame : NULL,
								txt ? txt : val ? val->txt : NULL,
								txt ? len : val ? val->len : 0)) STACK_ERROR;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_primary_handler
** Description : call primary handler for a control	with access check
*********************************************************************/
#define ERR_FUNCTION "ctrl_primary_handler"
#define ERR_CLEANUP	form->i_ctrl = 0
int ctrl_primary_handler(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	int step = form->step;
	form->i_ctrl = i_ctrl;
	
	/* Return if no primary handler for the control */
	if(!ctrl_handlers[ctrl->i_handler].h0) RETURN_OK;

	/* Check control access - return if none */
	if(ctrl_check_access(cntxt, ctrl)) STACK_ERROR; 
	if(!(ctrl->access & (AccessView | AccessEdit))) RETURN_OK;

	/* Limit access if applicable */
	if(!CTRL_ATTR_CELL(NO_ACCESS_INHERIT) && !(ctrl->access & AccessEdit) && step == HtmlEdit) form->step = HtmlView;

	/* Call control primary handler & restore step */
	if(ctrl_handlers[ctrl->i_handler].h0(cntxt, i_ctrl)) STACK_ERROR;
	form->step = step;
	
	RETURN_OK_CLEANUP;
 }
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_showhelp
** Description : output html code for javascript call to ShowHelp
*********************************************************************/
#define ERR_FUNCTION "put_showhelp"
#define ERR_CLEANUP	
int put_showhelp(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **html				/* in/out : HTML output buffer */
){
	EVA_form *form = cntxt->form;

	/* Return if javascript is not enabled */
	if(!cntxt->jsenabled) RETURN_OK;

	/* Output ShowHelp call */
	DYNBUF_ADD_STR(html, " onMouseOver=ShowHelp(this)");

	/* Return if no current control */
	if(!form || !form->nb_ctrl || cntxt->opt_btn_mode <= OptBtn_Help) RETURN_OK;

	/* Output clic info for ShowHelp call */
	DYNBUF_ADD3_CELL(html, " ctrlclic='I", &(form->ctrl[form->i_ctrl].id), 0, 0, NO_CONV, "/");
	DYNBUF_ADD_CELL(html, &form->id_form, 0, 0, NO_CONV);
	DYNBUF_ADD3_CELL(html, "$", &form->id_obj, 0, 0, NO_CONV, "#'");
	
	RETURN_OK_CLEANUP;
 }
#undef ERR_FUNCTION
#undef ERR_CLEANUP

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
){
	EVA_form *form = cntxt->form;
	char *srcform = (form && form->id_form.cell && form->id_form.cell->txt) ? form->id_form.cell->txt : "";
	char *srcobj = (form && form->id_obj.cell && form->id_obj.cell->txt) ? form->id_obj.cell->txt : "";

	if(!opname) return 0;
	*opname = 0;
	if(!id_form && !id_obj)
		return sprintf(opname, "I%s$%s", srcform, srcobj);
	else 
		return sprintf(opname, "B%s$%s#.OPENOBJ=%lu,%lu,%d,%d", srcform, srcobj, id_obj, id_form, loc, mode);
}

/*********************************************************************
** Function : html_put_open_btn
** Description : output an open button
*********************************************************************/
#define ERR_FUNCTION "html_put_open_btn"
#define ERR_CLEANUP M_FREE(tooltip)
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
){
 	DynBuffer *tooltip = NULL;
	char _opname[64];
	char *action = b_disabled ? "Vous n'avez pas accès à cette fiche" :
		(!id_form && !id_obj) ? "Cliquez pour plus d'informations sur la fiche" :
								"Cliquez pour ouvrir la fiche";
	/* Build button name if applicable */
	if(!opname) opname = _opname;
	build_open_btn_name(cntxt, opname, id_form, id_obj, loc, mode);

	/* Build button tooltip */
	DYNBUF_ADD_BUF(&tooltip, label, NO_CONV);
	if(id_form && !id_obj)
		DYNBUF_ADD_STR(&tooltip, " (Nouvelle fiche)")
	else if(cntxt->b_admin)
		DYNBUF_ADD3_INT(&tooltip, " n° ", (!id_form && !id_obj) ? DYNTAB_TOUL(&cntxt->form->id_obj) : id_obj, "");
	if(objname) DYNBUF_ADD3_BUF(&tooltip, " : ", objname, NO_CONV, "");
	if(notes && !b_disabled) DYNBUF_ADD3_BUF(&tooltip, "\n\n", notes, NO_CONV, "\n");
	if(action && !b_disabled) DYNBUF_ADD3(&tooltip, "\n", action, 0, NO_CONV, "");
	if(b_disabled) DYNBUF_ADD_STR(&tooltip, "\n")
		
	/* Output button */
	if(put_html_button(cntxt, opname, NULL,
				img ? img->data : "_eva_open_small.gif", 
				(img && imgsel) ? imgsel->data : "_eva_open_small_s.gif",
				tooltip ? tooltip->data : NULL, 0, b_disabled | 4))
			STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_symbol_btn
** Description : output an open button for an object
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_symbol_btn"
#define ERR_CLEANUP DYNTAB_FREE(formstamp); \
					DYNTAB_FREE(formdata); \
					M_FREE(formname); \
					M_FREE(objname); \
					M_FREE(objtitle); \
					M_FREE(fnotes); \
					M_FREE(notes); \
					M_FREE(img); \
					M_FREE(imgsel); \
					M_FREE(txtlabel); \
					M_FREE(htmlabel); \
					M_FREE(txtnotes)
int ctrl_add_symbol_btn(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl,						/* in : related control */
	DynTableCell *val,					/* in : value associated with button */
	DynTable *data,						/* in : object data to be opened by the button */
	unsigned long beg,					/* in : first line to process in data */
	char *bg_color,						/* in : output cell separators with given color if not NULL */
	char *dispmode						/* in : display mode */
){
	EVA_form *form = cntxt->form;
	DynTable formstamp = {0};
	DynTable formdata = {0};
	DynBuffer *formname = NULL;
	DynBuffer *objname = NULL;
	DynBuffer *objtitle = NULL;
	DynBuffer *notes = NULL;
	DynBuffer *fnotes = NULL;
	DynBuffer *img = NULL;
	DynBuffer *imgsel = NULL;
	DynBuffer *htmlabel = NULL;
	DynBuffer *txtlabel = NULL;
	DynBuffer *txtnotes = NULL;
	char opname[64] = {0};
	unsigned long i, use_form = 0;
	DynTableCell *objdata = dyntab_cell(data, beg, 0);
	char *alt = ctrl ? CTRL_ATTR_VAL(ALTFORM) : "";
	int loc = !strcmp(alt, "_EVA_TOP") ? 1 : !strcmp(alt, "_EVA_BOTTOM") ? 2 : !strcmp(alt, "_EVA_REUSE") ? 3 : 0;
	int cntform = 0;
	int b_formname = dispmode && strstr(dispmode, "NAME") != NULL;
	int b_objname =  dispmode && (strstr(dispmode, "LABEL") != NULL || !strcmp("_EVA_NONE", dispmode));
	int b_button = (!dispmode || !*dispmode || (strcmp("_EVA_NONE", dispmode) && strstr(dispmode, "SYMBOL") != NULL));
	int b_label = b_formname || b_objname;
	int b_tooltip = !dispmode || !*dispmode || strstr(dispmode, "OBJNOTES") || !strncmp(dispmode, add_sz_str("_EVA_"));
	int formaccess = AccessNone;
	char bgcolor[256];
	ObjTableFormat *tbl = ctrl ? ctrl->objtbl : NULL;
	DynTable *tblattr = tbl ? tbl->attr : &ctrl->attr;
	CHECK_HTML_STATUS;

	/* Build formstamp list */
	if(ctrl && DYNTAB_FIELD_CELL(tblattr, LIST_OPENFIXED) && (!val || !val->IdValObj || val->IdValObj == val->IdObj))
	{
		/* Fixed form : read given form - return if none */
		DynTableCell *c = DYNTAB_FIELD_CELL(tblattr, LIST_OPENFORM);
		if(!c || !c->txt || !atoi(c->txt)) RETURN_OK;
		DYNTAB_ADD_CELLP(&formstamp, 0, 0, c);
	}
	else
	{
		i = beg;
		DYNTAB_FIELDBEG(&formstamp, data, FORMSTAMP, &i);
	}
	dyntab_sort(&formstamp, qsort_col0idesc);

	/* Prepare & output cell separator if applicable */
	if(bg_color && *bg_color != '*')
	{
		if(*bg_color) snprintf(add_sz_str(bgcolor), " bgcolor=#%s", bg_color);
		else *bgcolor = 0;
		if(b_button) DYNBUF_ADD3(html, "<td align=right valign=top", bgcolor, 0, NO_CONV, ">");
	}

	/* For each object formstamp */
	for(i = 0; i < formstamp.nbrows; i++)
	{
		/* Read object & form info */
		unsigned long id_form = DYNTAB_TOULRC(&formstamp, i, 0), i_fc;
		int access = AccessNone;
		if(qry_obj_label(cntxt, &formname, &fnotes,
				b_label ? &objname : NULL, NULL, b_tooltip ? &notes : NULL,
				&img, &imgsel, &i_fc, id_form, data, beg)) CLEAR_ERROR;
		if(i_fc >= cntxt->nbfc) continue;

		/* Check user access to object & form */
		if(objdata && objdata->IdObj)
		{
			EVA_ctrl newctrl = {0};
			EVA_form newform = {0};
			newform.ctrl = &newctrl;
			memcpy(&newctrl.attr, &cntxt->fc[i_fc].data, sizeof(newctrl.attr));
			memcpy(&newform.objdata, data, sizeof(newform.objdata));
			newform.objdata.cell = data->cell + data->szcols * beg;
			newform.objdata.nbrows -= beg;
			cntxt->form = &newform;
			DYNTAB_ADD_INT(&newform.id_obj, 0, 0, newform.objdata.cell->IdObj);
			DYNTAB_ADD_INT(&newform.id_form, 0, 0, id_form);
			if(ctrl_check_access(cntxt, &newctrl)) STACK_ERROR;
			newform.step = HtmlView;
			if(ctrl_check_access(cntxt, &newctrl)) STACK_ERROR;
			access = newctrl.access;
			DYNTAB_FREE(newform.id_obj);
			DYNTAB_FREE(newform.id_form);
			cntxt->form = form;
		}
		else
			access = AccessView;

		/* Continue if user has no access to form/object & previous form allowed access */
		if(access == AccessNone && (cntform || i + 1 < formstamp.nbrows)) continue;
		cntform++;

		/* Build tooltip */
		if(txtnotes) DYNBUF_ADD_STR(&txtnotes, "\n\n");
		DYNBUF_ADD3_BUF(&txtnotes, "[", formname, NO_CONV, "] : ");
		DYNBUF_ADD_BUF(&txtnotes, objname, NO_CONV);
		if(access != AccessNone && notes) DYNBUF_ADD3_BUF(&txtnotes, "\n", notes, NO_CONV, "");

		/* Build label */
		if(b_label && !txtlabel)
		{
			use_form = id_form;
			formaccess = access;
			if(b_formname) DYNBUF_ADD_BUF(&txtlabel, formname, NO_CONV);
			if(b_formname && formname && b_objname && objname) DYNBUF_ADD_STR(&txtlabel, " - ");
			if(b_objname) DYNBUF_ADD_BUF(&txtlabel, objname, NO_CONV);
			if(!(objdata && objdata->IdObj) && cntxt->fc[i_fc].fields.nbrows) DYNBUF_ADD_STR(&txtlabel, " (Nouvelle fiche)");
		}

		/* Output button */
		if(b_button)
		{
			if(html_put_open_btn(cntxt, opname, formname, 
							b_label ? NULL : objtitle, !b_tooltip ? NULL : b_label ? fnotes : notes,
							img, imgsel, 
							id_form, objdata ? objdata->IdObj : 0, loc,
							access == AccessNone, atoi(CTRL_ATTR_VAL(OPEN_MODE))))
				STACK_ERROR;
		}
	}
	if(bg_color && b_button) DYNBUF_ADD_STR(html, "</td>\n");

	if(b_label && val && !val->b_relation && val->txt && strtoul(val->txt, NULL, 10) != (objdata ? objdata->IdObj : 0))
	{
		M_FREE(txtlabel);
		DYNBUF_ADD_CELLP(&txtlabel, val, NO_CONV);
	}

	if(txtlabel)
	{
		if(bg_color) DYNBUF_ADD3(html, "<td valign=top ", *bg_color == '*' ? bg_color + 1 : bgcolor, 0, NO_CONV, ">");
		if(!*opname) build_open_btn_name(cntxt, opname, use_form, objdata ? objdata->IdObj : 0, loc, 0);
		if(cntxt->jsenabled || formaccess == AccessNone)
		{
			if(put_html_bold_substring(cntxt, &htmlabel, DYNBUF_VAL_SZ(txtlabel), tbl ? &tbl->srchwords : NULL))
				STACK_ERROR;
		}
		else
			DYNBUF_ADD_BUF(&htmlabel, txtlabel, NO_CONV);
		if(formaccess == AccessNone)
			DYNBUF_ADD_BUF(html, htmlabel, NO_CONV)
		else 
			if(put_html_button(cntxt, opname, htmlabel->data, NULL, NULL, txtnotes ? txtnotes->data : NULL, 0, 32))
			STACK_ERROR;
		if(bg_color && *bg_color != '*') DYNBUF_ADD_STR(html, "</td>");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_filter_idobj
** Description : return the list of objects ids matching the control filter
*********************************************************************/
#define ERR_FUNCTION "ctrl_filter_idobj"
#define ERR_CLEANUP qry_build_free(&flt); \
					M_FREE(sql); \
					cntxt->sql_trace = sql_trace
int ctrl_filter_idobj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : list of objects Ids matching control filter
											if NULL, a temporary table IdList is created to hold the results */
	EVA_ctrl *ctrl					/* in : control containing filter information */
){
	QryBuild flt = {0};
	DynBuffer *sql = NULL;
	int sql_trace = cntxt->sql_trace;
	if(*CTRL_ATTR_VAL(DEBUG_SQL_FILTER) == '1') cntxt->sql_trace = DEBUG_SQL_RES;

	/* Create temporary table IdList for results if no output table */
	if(!res && sql_exec_query(cntxt, "CREATE TEMPORARY TABLE IdList (IdObj INT NOT NULL) TYPE=HEAP \n"))
		STACK_ERROR;

	/* Build IdObj query for filter */
	if(qry_parse_filter(cntxt, &flt, &ctrl->attr) ||
		qry_build_flt_select(cntxt, &sql, res, &flt,
								ctrl->objtbl ? ctrl->objtbl->line : 0,
								ctrl->objtbl ? ctrl->objtbl->lines : 0))
		STACK_ERROR;

	/* Exec Query & read result */
	if(qry_exec_filter(cntxt, &flt, sql->data)) STACK_ERROR;	
	if(ctrl->objtbl) ctrl->objtbl->totlines = cntxt->sql_nbrows;
	if(cntxt->debug & DEBUG_SQL_SLOW && cntxt->sql_restime > DEBUG_SQL_SLOW_TH) err_print_filter(&cntxt->debug_msg, &flt);
	if(res ?
		sql_get_table(cntxt, res, 0) : 
		sql_exec_query(cntxt, "ALTER TABLE IdList ADD INDEX (IdObj)"))
			STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
