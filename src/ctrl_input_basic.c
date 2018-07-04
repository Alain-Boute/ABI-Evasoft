/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_basic.c
** Description : handling functions for basic input controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_autofocus
** Description : set focus to given control if applicable
*********************************************************************/
#define ERR_FUNCTION "ctrl_autofocus"
#define ERR_CLEANUP
int ctrl_autofocus(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *name, size_t sz_name,		/* in : control CGI name */
	int b_empty						/* in : control value is empty if not zero */
){
	DynBuffer **focus;
	CGIData *cgi = cntxt->cgi ? cntxt->cgi + cntxt->cgibtn : NULL, fcgi = {0};

	/* Parse given input name (not very efficient - would be better to pass ctrl as param) */
	fcgi.name = name;
	fcgi.name_sz = sz_name;
	if(cgi && cgi_parse_name(cntxt, &fcgi)) STACK_ERROR;

	/* Return if autofocus disabled for control */
	if(fcgi.IdCtrl)
	{
		EVA_ctrl *ctrl = cntxt->form ? (cntxt->form->ctrl + cntxt->form->i_ctrl) : NULL;
		if(fcgi.IdCtrl == DYNTAB_TOUL(&ctrl->id) && *CTRL_ATTR_VAL(NO_AUTOFOCUS)) RETURN_OK;
	}

	/* Handle setfocus */
	focus = (cgi && cgi->IdForm == fcgi.IdForm && cgi->IdObj == fcgi.IdObj && !STRCMPNUL(cgi->field, fcgi.field)) ?
				&cntxt->focus1 : b_empty ? &cntxt->focus2 : &cntxt->focus3;
	if(!*focus) DYNBUF_ADD(focus, name, sz_name, NO_CONV);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_html_text_input
** Description : output HTML code for text input
*********************************************************************/
#define ERR_FUNCTION "put_html_text_input"
#define ERR_CLEANUP 
int put_html_text_input(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name, size_t sz_name, 		/* in : input name */
	char *val, size_t sz_val,			/* in : value  of the control */
	int b_passwd,						/* in : treat as password */
	int mode,							/* in : bitmask
											bit 0-1 : use JavaScript HTML editor (1=simple - 2=advanced)
											bit 2 : use calendar date input */
	int lines, int columns,				/* in : size of the control (in chars) */
	int maxlength		   				/* in : maximum input length (in chars) */
){
	CHECK_HTML_STATUS;
	
	/* Output HTML code for input */
	if(lines <= 1 || b_passwd)
	{
		if(b_passwd) DYNBUF_ADD_STR(html, "<input type=password")
		else DYNBUF_ADD_STR(html, "<input type=text");
		DYNBUF_ADD3(html, " name='", name, sz_name, NO_CONV, "'");
		if(mode & 4 && cntxt->jsenabled)
		{
			cntxt->jsCalendarInput = 1;
			DYNBUF_ADD_STR(html, " onClick=scwShow(this,this)");
		}
		if(val && *val) DYNBUF_ADD3(html, " value='", val, sz_val, HTML_NO_QUOTE, "'");
		if(columns > 0)DYNBUF_ADD3_INT(html, " size=", columns, "");
		if(maxlength > 0) DYNBUF_ADD3_INT(html, " maxlength=", maxlength, "");
		DYNBUF_ADD_STR(html, ">");
	}
	else
	{
		DYNBUF_ADD3(html, "<textarea name='", name, sz_name, NO_CONV, "'");
		if(mode & 3 && cntxt->jsenabled)
		{
			cntxt->jsHTMLEditor = mode & 3;
			DYNBUF_ADD_STR(html, " mce_editable=true");
		}
		if(lines > 0) DYNBUF_ADD3_INT(html, " rows=", lines, "");
		if(columns > 0) DYNBUF_ADD3_INT(html, " cols=", columns, "");
		DYNBUF_ADD_STR(html, ">");
		if(val && *val) DYNBUF_ADD(html, val, sz_val, TO_XML);
		DYNBUF_ADD_STR(html, "</textarea>");
	}

	if(ctrl_autofocus(cntxt, name, sz_name, !val || !*val || !sz_val)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_text_value
** Description : output HTML code for text input with given value
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_text_value"
#define ERR_CLEANUP if(!name1) M_FREE(name)
int ctrl_add_text_value(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,	 				/* in : control to process */
	unsigned long i_val,			/* in : value index in ctrl->val */
	char *txt, size_t len,			/* in : text to use for value - if NULL use ctrl->val[i] */
	DynBuffer *name1 				/* in : CGI input name - if NULL build control value name */
){
	EVA_form *form = cntxt->form;
	DynBuffer *name = name1 ? name1 : NULL;
	DynTableCell *lblunit = CTRL_ATTR_CELL(LABELUNIT);
	DynTableCell *val = dyntab_cell(&ctrl->val, i_val, 0);
	char *editmode = CTRL_ATTR_VAL(HTML_EDITOR);

	/* Set text if not given */
	if(!txt && val)
	{
		txt = val->txt;
		len = val->len;
	}

	/* If edit mode : add HTML text input */
	if(form->step == HtmlEdit)
	{
		if(!name) CTRL_CGINAMEVAL(&name, i_val);
		if(put_html_text_input(cntxt, 
				DYNBUF_VAL_SZ(name),
				txt, len,
				CTRL_ATTR_VAL(PASSWORD)[0] == '1', 
				!*editmode ? 0 : !strcmp(editmode, "_EVA_SIMPLE") ? 1 : !strcmp(editmode, "_EVA_NORMAL") ? 2 : 3,
				ctrl->LINES,
				ctrl->COLUMNS,
				atoi(CTRL_ATTR_VAL(MAXLENGTH))) ||
			ctrl_put_hidden_old(cntxt, ctrl, i_val, name, txt, len))
		STACK_ERROR;
	}
	else
	{
		/* Else : add text value */
		if(CTRL_ATTR_VAL(PASSWORD)[0] == '1') DYNBUF_ADD_STR(form->html, "******")
		else if(*editmode)
			DYNBUF_ADD(form->html, txt, len, NO_CONV)
		else
			DYNBUF_ADD(form->html, txt, len, TO_HTML)
	}
	if(lblunit && (*ctrl->LABEL_NOSEL || ctrl->val.nbrows || form->step == HtmlEdit)) 
		DYNBUF_ADD3_CELLP(form->html, " ", lblunit, TO_HTML, "");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_text
** Description : handles INPUT controls of type TEXT
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_text"
#define ERR_CLEANUP
int ctrl_add_text(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i, k;
	int b_multiple = ctrl->MULTIPLE[0] != 0 && strcmp("No", ctrl->MULTIPLE);
	
	switch(form->step)
	{
	case InputCheck:
		{
			/* Handle input mask */
			char *_inputmask = CTRL_ATTR_VAL(TEXTINPUTMASK), *inputmask = _inputmask + 5;

			/* Format each value */
			if(*_inputmask) for(i = 0; i < ctrl->val.nbrows; i++)
			{
				DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
				char *in = c->txt, *out = in, *old;
				if(!c->txt && *c->txt) continue;
				M_STRDUP(old, c->txt, c->len);
				if(!strcmp(inputmask, "UCASE"))
					setcase(1, c->txt, 0);
				else if(!strcmp(inputmask, "NAMECASE"))
				{
					while(*in)
					{
						setcase((in == c->txt || strchr(" -", *(in - 1))), in, 1);
						in++;
					}
				}
				else if(!strcmp(inputmask, "PHRASE"))
				{
					setcase(1, c->txt, 1);
					setcase(0, c->txt + 1, 0);
				}
				else if(!strcmp(inputmask, "PHONE"))
				{
					while(*in)
					{
						*in = (char)((in == c->txt && *in == '+' || isdigit(*in)) ? *in : ' ');
						in++;
					}
				}

				/* Remove duplicate spaces */
				in = c->txt, out = in;
				while(*in)
				{
					if(!isspace(*in) || out > c->txt && !isspace(*(out - 1)))
					{
						*out = *in; 
						out++;
					}
					in++;
				}
				while(out < c->txt + c->len) *out++ = ' ';
				if(strcmp(old, c->txt))
				{
					c->b_modified = 1;
					ctrl->b_modified = 1;
				}
				M_FREE(old);
			}
		}
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		/* Add HTML code for control label & position */
		if(!form->html) RETURN_OK;
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle empty value in view mode */
		if(form->step != HtmlEdit && !ctrl->val.nbrows && *ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);

		/* Add HTML code for each value */
		k = ctrl->val.nbrows + ((form->step == HtmlEdit && b_multiple) ? 1 : 0);
		for(i = 0; !i || i < k; i++)
		{
			/* Separate multiple values with a line break */
			if(i) DYNBUF_ADD_STR(form->html, "<br>");

			/* Add text control */
			if(ctrl_add_text_value(cntxt, ctrl, i, NULL, 0, NULL)) STACK_ERROR;
		}

		/* Add HTML code for the control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_number
** Description : handles INPUT controls of type NUMBER
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_number"
#define ERR_CLEANUP
int ctrl_add_number(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i, k;
	int b_multiple = ctrl->MULTIPLE[0] != 0 && strcmp("No", ctrl->MULTIPLE);

	switch(form->step)
	{
	case CtrlRead:
		/* Format each value */
		if(!*ctrl->NOBR) ctrl->NOBR = "1";
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			char *val = dyntab_val(&ctrl->val, i, 0);
			char *num = input_to_number(val);
			if(strcmp(num, val)) 
			{
				DYNTAB_ADD(&ctrl->val, i, 0, num, strlen(num), NO_CONV);
				dyntab_cell(&ctrl->val, i, 0)->b_modified = 1;
				ctrl->b_modified = 1;
			}
		}
		break;

	case InputCheck:
		/* Check each value */
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			double num = atof(input_to_number(dyntab_val(&ctrl->val, i, 0)));
			DynTableCell *cmin = CTRL_ATTR_CELL(MIN_VALUE);
			double vmin = cmin ? atof(cmin->txt) : 0;
			DynTableCell *cmax = CTRL_ATTR_CELL(MAX_VALUE);
			double vmax = cmax ? atof(cmax->txt) : 0;
			if(cmin && num < vmin || cmax && num > vmax)
			{
				ctrl->error = 2;
				if(ctrl->errmsg) DYNBUF_ADD_STR(&ctrl->errmsg, " - ");
				if(cmin && cmax)
				{
					DYNBUF_ADD3_CELLP(&ctrl->errmsg, "Valeur entre ", cmin, NO_CONV, " et ");
					DYNBUF_ADD_CELLP(&ctrl->errmsg, cmax, NO_CONV);
				}
				else if(cmin)
					DYNBUF_ADD3_CELLP(&ctrl->errmsg, "Valeur supérieure à ", cmin, NO_CONV, "")
				else
					DYNBUF_ADD3_CELLP(&ctrl->errmsg, "Valeur inférieure à ", cmax, NO_CONV, "")
			}
		}
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		/* Add HTML code for control label & position */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle empty value in view mode */
		if(form->step != HtmlEdit && !ctrl->val.nbrows && *ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);

		/* Add HTML code for each value */
		k = ctrl->val.nbrows + ((form->step == HtmlEdit && b_multiple) ? 1 : 0);
		for(i = 0; !i || i < k; i++)
		{
			/* Separate multiple values with a line break */
			if(i) DYNBUF_ADD_STR(form->html, "<br>");

			/* Add text control */
			if(form->step == HtmlEdit)
			{
				if(ctrl_add_text_value(cntxt, ctrl, i, DYNTAB_VAL_SZ(&ctrl->val, i, 0), NULL)) STACK_ERROR;
			}
			else
			{
				/* Calc number value with given format */
				char *fmtval = number_to_format(dyntab_val(&ctrl->val, i, 0), CTRL_ATTR_VAL(FORMAT));
				
				if(ctrl_add_text_value(cntxt, ctrl, i, fmtval, strlen(fmtval), NULL)) STACK_ERROR;
			}
		}

		/* Add HTML code for the control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	default:
		/* Handle as text control */
		if(ctrl_add_text(cntxt, i_ctrl)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_html_chkbox
** Description : output HTML code for a chkbox input
*********************************************************************/
#define ERR_FUNCTION "put_html_chkbox"
#define ERR_CLEANUP 
int put_html_chkbox(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name, size_t sz_name, 		/* in : input name */
	char *value, size_t sz_value, 		/* in : input value */
	int b_status,				 		/* in : bitmask :
											bit 0 : checked if set
											bit 1 : radio button if set */
	int b_submit						/* in : use OnClick JavaScript if 1 */
){
	CHECK_HTML_STATUS;

	/* Output HTML code for input */
	if(b_status & 2) 
		DYNBUF_ADD_STR(html, "<input type=radio")
	else 
		DYNBUF_ADD_STR(html, "<input type=checkbox");
	if(name) DYNBUF_ADD3(html, " name='", name, sz_name, NO_CONV, "'");
	if(value) DYNBUF_ADD3(html, " value='", value, sz_value, HTML_NO_QUOTE, "'");
	if(b_status & 1) DYNBUF_ADD_STR(html, " checked");
	if(b_submit && cntxt->jsenabled) DYNBUF_ADD3(html, " onClick='cb(\"", name, sz_name, NO_CONV, "\");'");
	DYNBUF_ADD_STR(html, ">");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
