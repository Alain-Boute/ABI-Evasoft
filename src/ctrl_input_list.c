/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_list.c
** Description : handling functions for input controls of type LIST
**      Author : Alain BOUTE
**     Created : Dec 28 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : put_html_list
** Description : output HTML code for a list input
*********************************************************************/
#define ERR_FUNCTION "put_html_list"
#define ERR_CLEANUP
int put_html_list(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer *name,			 		/* in : input name */
	DynTable *options, 					/* in : options list (actual & displayed values) */
	DynTable *values, 					/* in : selected values */
	int lines, int columns,			 	/* in : # of lines & columns of input */
	int b_multiple,					 	/* in : multiple selection allowed if 1 */
	int b_submitonchange,			 	/* in : use JavaScript OnChange handler if 1 */
	int b_ignoreunlisted,			 	/* in : do not list values not in options if 1 */
	char *emptyval					 	/* in : add this option at 1st position with empty value */
){
	unsigned long i, k;
	CHECK_HTML_STATUS;

	/* Force multiple if more than one value */
	if(values && values->nbrows > 1) b_multiple = 1;
	if(b_multiple && lines < 2) lines = 2;
	
	/* HTML code for input */
	DYNBUF_ADD_STR(html, "<select size=");
	DYNBUF_ADD_INT(html, lines);
	if(name) DYNBUF_ADD3_BUF(html, " name='", name, NO_CONV, "'");
	if(b_multiple) DYNBUF_ADD_STR(html, " multiple");
	if(b_submitonchange) DYNBUF_ADD3_BUF(html, " onChange='cb(\"", name, NO_CONV, "\");'");
	DYNBUF_ADD_STR(html, ">\n");

	/* Init col member of values : used to return the index of the selected option */
	if(values) for(i = 0; i < values->nbrows; i++) dyntab_cell(values, i, 0)->col = 0;

	/* Add empty value at beginning of list if specified */
	DYNBUF_ADD_STR(html, "<option value=''></option>\n");
	if(emptyval && *emptyval)
	{
		DYNBUF_ADD_STR(html, "<option value=''");
		if(values && (!values->nbrows || !values->cell->txt || !*values->cell->txt)) DYNBUF_ADD_STR(html, " selected");
		DYNBUF_ADD3(html, ">", emptyval, 0, TO_HTML, "</option>\n");
	}

	/* HTML code for list options */
	if(options)
	{
		int j = (options && options->nbcols > 1) ? 1 : 0;
		for(i = 0; i < options->nbrows; i++)
		{
			DYNBUF_ADD3_CELL(html, "<option value='", options, i, 0, HTML_NO_QUOTE, "'");

			/* Search option in values */
			for(k = 0; k < (values ? values->nbrows : 1); k++)
			{
				if(!dyntab_cmp(options, i, 0, values, k, 0))
				{
					/* Add 'selected' attribute & mark value if found */
					DynTableCell *val = dyntab_cell(values, k, 0);
					DYNBUF_ADD_STR(html, " selected");
					dyntab_cell(options, i, 0)->col = 1;
					if(val) val->col = i + 1;
					break;
				}
			}
			DYNBUF_ADD3_CELL(html, ">", options, i, j, TO_HTML, "</option>\n");
		}
	}

	/* Add HTML code for unlisted values */
	if(values && !b_ignoreunlisted) for(i = 0; i < values->nbrows; i++) 
	{
		DynTableCell *val = dyntab_cell(values, i, 0);
		if(!val->col && val->len && val->txt && val->txt[0])
		{
			DYNBUF_ADD3_CELLP(html, "<option value='", val, HTML_NO_QUOTE, "'");
			DYNBUF_ADD3_CELLP(html, " selected>", val, TO_HTML, "</option>\n");
		}
	}

	/* Add empty value at end of list to set list width */
	if(columns)
	{
		DYNBUF_ADD_STR(html, "<option value=''>");
		while(--columns > 0) DYNBUF_ADD_STR(html, "&nbsp;");
		DYNBUF_ADD_STR(html, "</option>");
	}

	DYNBUF_ADD_STR(html, "</select>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_selected_options
** Description : processes selected options for a controls of type INPUT/LIST
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_selected_options"
#define ERR_CLEANUP
int ctrl_add_selected_options(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	DynTable *options				/* in : list of options */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i, j, i_opt;

	/* Process empty option if applicable */
	if(ctrl->b_use_val && ctrl->i_empty && (!ctrl->val.nbrows || 
		form->step == CtrlRead && ctrl->cgival.nbrows > ctrl->val.nbrows))
	{
		form->ctrl[ctrl->i_empty].b_selected = 1;
		CTRL_PRI_HDLR(ctrl->i_empty)
		ctrl = form->ctrl + i_ctrl;
	}

	/* For each control value */
	for(j = 0; j < ctrl->val.nbrows; j++)
	{

		/* Search for corresponding options */
		for(i = 0; i < options->nbrows; i++)
		{
			DynTableCell *val = dyntab_cell(&ctrl->val, j, 0);
			i_opt = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(options, i, 0));
			if(i_opt)
			{
				/* Read option label & value */
				DynTableCell *label = ATTR_CELL(form->ctrl + i_opt, LABEL);
				DynTableCell *value = ATTR_CELL(form->ctrl + i_opt, VALUE);
				DynTableCell *optval = ctrl->b_use_val ? value : label;

				/* If not already selected in CtrlRead mode (avoid multiple reads) */
				if((form->step != CtrlRead || !form->ctrl[i_opt].b_selected) &&
					!strcmp(optval ? optval->txt : "", (val && val->txt) ? val->txt : ""))
				{
					/* Option value is same as control value : process option */
					form->ctrl[i_opt].b_selected = 1;
					CTRL_PRI_HDLR(i_opt)
					ctrl = form->ctrl + i_ctrl;
					break;
				}
			}
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_put_html_input_list
** Description : output HTML input list for a control
*********************************************************************/
#define ERR_FUNCTION "ctrl_put_html_input_list"
#define ERR_CLEANUP DYNTAB_FREE(ctrlval); \
					M_FREE(name)
int ctrl_put_html_input_list(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	DynTable *optlist,				/* in : options list (actual & displayed values) */
	int mode						/* in : output mode - bit mask
										bit 0 : use OnChange Javascript handler if set
										bit 1 : output relation open button if set */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable ctrlval = { 0 };
	DynBuffer *name = NULL;
	unsigned long i, k;
	int b_multiple = ctrl->MULTIPLE[0] != 0 && strcmp("No", ctrl->MULTIPLE);
	unsigned long lines = ctrl->LINES;
	char *onchange = CTRL_ATTR_VAL(ONCHANGE);
	int b_submit = mode & 1;
	int b_empty = (strcmp("No", ctrl->ALLOWEMPTY) || !*CTRL_ATTR_VAL(NOEMPTYOPTION)) ? 1 : 0;
	int b_openbtn = mode & 2 && strcmp(CTRL_ATTR_VAL(OPENBUTTON), "_EVA_NONE");
	char *helptext  = CTRL_ATTR_VAL(HELPTEXT);
	char *emptylabel = ctrl->i_empty ? NULL : ctrl->LABEL_NOSEL;

	/* Handle ONCHANGE attribute */
	if(*onchange && strcmp(onchange, "_EVA_NONE")) b_submit = 1;
	if(!strcmp(onchange, "_EVA_UPDATE"))
	{
		/* TODO - Handle Update on change : check if value has changed  */
	}

	/* Handle special case for HTML list input with multiple selection */
	if(lines > 1)
	{
		/* Put HTML list input */
		CTRL_CGINAMEVAL(&name, 0);
		if(put_html_list(cntxt, 
						name,
						optlist, &ctrl->val,
						lines,
						ctrl->COLUMNS ? ctrl->COLUMNS : b_empty,
						b_multiple, 
						b_submit, 0, emptylabel))
			STACK_ERROR;

		/* Add hidden inputs for unchanged values */
		for(i = 0; i < (ctrl->val.nbrows ? ctrl->val.nbrows : 1); i++)
			if(ctrl_put_hidden_old(cntxt, ctrl, i, name, NULL, 0)) STACK_ERROR;
	}
	else
	{
		/* Add HTML list input for each value */
		k = ctrl->val.nbrows + (b_multiple ? 1 : 0);
		for(i = 0; !i || i < k; i++)
		{
			unsigned long idobj = DYNTAB_TOULRC(&ctrl->val, i, 0);
			unsigned long i_opt;

			/* Separate multiple values with a line break */
			if(i) DYNBUF_ADD_STR(form->html, "<br>");

			/* Put HTML list input */
			CTRL_CGINAMEVAL(&name, i);
			DYNTAB_SET_CELL(&ctrlval, 0, 0, &ctrl->val, i, 0);
			if(put_html_list(cntxt, 
							name,
							optlist, (i && i == ctrl->val.nbrows) ? NULL : &ctrlval,
							lines,
							ctrl->COLUMNS ? ctrl->COLUMNS : b_empty,
							0, 
							b_submit || b_multiple, 0, emptylabel) ||
				ctrl_put_hidden_old(cntxt, ctrl, i, name, NULL, 0))
				STACK_ERROR;
			i_opt = ctrlval.cell->col;

			/* Output open button if applicable */
			if(b_openbtn && idobj)
			{
				char buf[128] = {0};
				snprintf(add_sz_str(buf), "B%s$%s#.OPENOBJ=%lu",
								dyntab_val(&form->id_form, 0, 0),
								dyntab_val(&form->id_obj, 0, 0),
								idobj);
				M_FREE(name);
				if(qry_obj_idfield(cntxt, &ctrlval, idobj, 0) ||
					qry_obj_label(cntxt, &name, NULL, &name, NULL, &name, NULL, NULL, NULL, 0, &ctrlval, 0) ||
					put_html_button(cntxt, buf, NULL, "_eva_open_small.gif", "_eva_open_small_s.gif", name->data, 0, 0))
					STACK_ERROR;
				DYNTAB_FREE(ctrlval);
			}

			/* Output option help text if applicable */
			if((!strcmp(helptext, "_EVA_ALWAYS") || !strcmp(helptext, "_EVA_EDIT") && cntxt->form->step == HtmlEdit) && i_opt)
			{
				if(qry_obj_field(cntxt, &ctrlval, dyntab_cell(optlist, i_opt - 1, 0)->IdObj, "_EVA_NOTES")) STACK_ERROR;
				if(ctrlval.nbrows)
					DYNBUF_ADD3_CELL(cntxt->form->html, "<div class=EVAOptionNotes>", &ctrlval, 0, 0, TO_HTML, "</div>");
				DYNTAB_FREE(ctrlval);
			}
		}
	}

	if(!ctrl->val.nbrows)
	{
		CTRL_CGINAMEVAL(&name, 0);
		DYNBUF_ADD3_BUF(cntxt->form->html, "<input type=hidden name='", name, NO_CONV, "'>");
		if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(cntxt->form->html, "\n");
		if(ctrl_put_hidden_old(cntxt, ctrl, 0, name, NULL, 0)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_input_list_view
** Description : handles INPUT controls of type LIST in view / print mode
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_input_list_view"
#define ERR_CLEANUP
int ctrl_add_input_list_view(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	DynTable *options				/* in : list of options */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i, j, i_opt;

	/* Add HTML code for control label & position */
	if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

	/* If no value : output empty value label if any */
	if(!ctrl->val.nbrows)
	{
		int b_done = 0;
		if(ctrl->b_use_val && ctrl->i_empty)
		{
			EVA_ctrl *ctl = form->ctrl + ctrl->i_empty;
			if(ctrl_check_access(cntxt, ctl)) STACK_ERROR;
			if(ctl->access != AccessNone)
			{
				b_done = 1;
				DYNBUF_ADD_CELLP(form->html, ATTR_CELL(ctl, LABEL), TO_HTML);
			}
		}
		if(!b_done && *ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);
	}
	
	/* For each value */
	for(i = 0; i < ctrl->val.nbrows; i++)
	{
		DynTableCell *val = dyntab_cell(&ctrl->val, i, 0);
		char *v = "";

		/* Search value in options values */
		j = options->nbrows;	i_opt = 0;
		if(ctrl->b_use_val) for(j = 0; j < options->nbrows; j++)
		{
			i_opt = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(options, j, 0));
			if(!i_opt) continue;
			v = ATTR_VAL(form->ctrl + i_opt, VALUE);
			if(*v && !strcmp(v, val->txt)) break;
		}

		/* Add value */
		if(i) DYNBUF_ADD_STR(form->html, "<br>");
		DYNBUF_ADD_CELLP(form->html,
						(j < options->nbrows ? 
							ATTR_CELL(form->ctrl + i_opt, LABEL) :
							dyntab_cell(&ctrl->val, i, 0)),
						TO_HTML);
	}

	/* Add HTML code for control footer */
	if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_input_list
** Description : handles INPUT controls of type LIST
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_input_list"
#define ERR_CLEANUP DYNTAB_FREE(optlist); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(options); \
					M_FREE(name)
int ctrl_add_input_list(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable optlist = { 0 };
	DynTable data = { 0 };
	DynTable options = { 0 };
	DynBuffer *name = NULL;
	unsigned long i, j, i_opt;
	int b_submit = 0;
	int b_multiple = ctrl->MULTIPLE[0] != 0 && strcmp("No", ctrl->MULTIPLE);
	char *inputmode, *allow_other;
	DynTableCell *expr;
	int empty_val = 0;

	/* Read control attributes */
	CTRL_ATTR(options, CTRLTREE);

	switch(form->step)
	{
	case CtrlRead:
		/* Add list options */
		CTRL_ADD_CHILD(i_ctrl, &options);
		ctrl = form->ctrl + i_ctrl;

		/* Count empty values (used to decide the strategy for values usage) */
		for(i = 0; i < options.nbrows; i++)
		{
			i_opt = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(&options, i, 0));
			if(i_opt && !ATTR_CELL(form->ctrl + i_opt, VALUE) && strcmp(form->ctrl[i_opt].CONTROL, "_EVA_CALC"))
			{
				ctrl->i_empty = empty_val ? 0 : i_opt;
				empty_val++;
			}
		}
		ctrl->b_use_val = !empty_val || empty_val == 1 &&  options.nbrows > 1;
		break;

	case HtmlEdit:
		if(!form->html) break;

		/* Handle no edit mode */
		if(CTRL_ATTR_CELL(NOEDIT) || CTRL_ATTR_CELL(NO_ACCESS_INHERIT) && !(ctrl->access & AccessEdit))
		{
			if(ctrl_add_input_list_view(cntxt, i_ctrl, &options)) STACK_ERROR;
			for(i = 0; i < ctrl->val.nbrows; i++)
				if(ctrl_put_hidden(cntxt, ctrl, i)) STACK_ERROR;
			break;
		}

		/* Handle empty value if applicable */
		if(!ctrl->i_empty && *ctrl->LABEL_NOSEL)
		{
			DYNTAB_ADD(&optlist, 0, 1, ctrl->LABEL_NOSEL, 0, NO_CONV);
			ctrl->b_use_val = 1;
		}

		/* Build list of options values & label */
		for(i = 0; i < options.nbrows; i++)
		{
			i_opt = ctrl_from_id(cntxt, i_ctrl, DYNTAB_VAL_SZ(&options, i, 0));
			if(i_opt)
			{
				/* Check access to option */
				int b_calc = !strcmp(form->ctrl[i_opt].CONTROL, "_EVA_CALC");
				if(ctrl_check_access(cntxt, form->ctrl + i_opt)) STACK_ERROR;
				if((form->ctrl[i_opt].access & AccessView) && (form->ctrl[i_opt].b_selected || b_calc) ||
					(form->ctrl[i_opt].access & AccessEdit))
				{
					j = optlist.nbrows;
					/* Handle calc-ed options from calc control */
					if(b_calc)
					{
						if(ctrl_output_exprval(cntxt, form->ctrl + i_opt, 3)) STACK_ERROR;
						ctrl->b_use_val |= form->ctrl[i_opt].val.nbcols > 1 || optlist.nbrows == 2 && !dyntab_sz(&optlist,0, 1);
						for(j = 0; j < form->ctrl[i_opt].val.nbrows; j++)
						{
							DYNTAB_SET_CELL(&optlist, optlist.nbrows, 0, &form->ctrl[i_opt].val, j, 0);
							if(ctrl->b_use_val && form->ctrl[i_opt].val.nbcols > 1)
								DYNTAB_SET_CELL(&optlist, optlist.nbrows - 1, 1, &form->ctrl[i_opt].val, j, 1)
							else if(ctrl->b_use_val)
								DYNTAB_SET_CELL(&optlist, optlist.nbrows - 1, 1, &form->ctrl[i_opt].val, j, 0);
						}
					}
					else
					{
						DynTableCell *l = ATTR_CELL(form->ctrl + i_opt, LABEL), *v = ATTR_CELL(form->ctrl + i_opt, VALUE);
						if(ctrl->b_use_val) 
						{
							DYNTAB_SET_CELLP(&optlist, j, 1, l);
							DYNTAB_SET_CELLP(&optlist, j, 0, (v || i_opt == ctrl->i_empty ? v : l));
						}
						else
							DYNTAB_SET_CELLP(&optlist, j, 0, l);
						dyntab_cell(&optlist, j, 0)->row = i_opt;
					}
				}
			}
		}

		/* Add calc-ed options */
		expr = CTRL_ATTR_CELL(CALCOPTIONS);
		if(expr)
		{
			if(ctrl_eval_fieldexpr(cntxt, ctrl, &data, expr->txt)) STACK_ERROR;
			dyntab_sort(&data, qsort_col0);
			for(i = 0; i < data.nbrows; i++)
				if(!i || dyntab_cmp(&data, i - 1, 0, &data, i, 0)) 
					DYNTAB_ADD_CELL(&optlist, optlist.nbrows, 0, &data, i, 0);
		}

		/* Add control header */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle input mode */
		allow_other = CTRL_ATTR_VAL(ALLOW_OTHER);
		inputmode = CTRL_ATTR_VAL(INPUTMODE);
		if(!strcmp(inputmode, "_EVA_OPTIONS") || !strcmp(inputmode, "_EVA_LIST_OPTIONS"))
		{
			/* Options list : build CGI name for options */
			int b_empty = 0;
			DynTableCell *val;
			char *onchange = CTRL_ATTR_VAL(ONCHANGE);
			if(ctrl_cgi_name(cntxt, ctrl, NULL, 0, &name, 0, NULL, 0)) STACK_ERROR; 
			ctrl->b_selected = 1;
			if(ctrl->val.nbrows > 1) b_multiple = 1;

			/* Handle submit on change  */
			b_submit = *onchange && strcmp(onchange, "_EVA_NONE");
			if(!b_multiple) for(i = ctrl->i_child; !b_submit && i; i = form->ctrl[i].i_brother)
				b_submit |= ATTR_CELL(form->ctrl + i, CTRLTREE) != 0;

			/* Output empty option if applicable */
			DYNBUF_ADD_STR(form->html, "<font size=-1></b>");
			for(i = 0; i < optlist.nbrows && dyntab_sz(&optlist, i, 0); i++);
			if(!ctrl->b_use_val && i == optlist.nbrows && !b_multiple && !*ctrl->ALLOWEMPTY)
			{
				b_empty = 1;
				if(put_html_chkbox(cntxt, DYNBUF_VAL_SZ(name), "", 0,
					(ctrl->val.nbrows ? 0 : 1) + (b_multiple ? 0 : 2), b_submit)) STACK_ERROR;
				DYNBUF_ADD_STR(form->html, "<span style=\"cursor:pointer\" onClick=\"ChkLblClick('");
				DYNBUF_ADD_BUF(form->html, name, NO_CONV);
				DYNBUF_ADD3_INT(form->html, "',0,", b_submit || b_multiple, "");
				DYNBUF_ADD3_INT(form->html, ",", b_multiple, ")\"><nobr>");
				if(!ctrl->val.nbrows) DYNBUF_ADD_STR(form->html, "<b>");
				DYNBUF_ADD_STR(form->html, "(non renseigné)</b></span>");
			}

			/* Output each option */
			for(i = 0; i < optlist.nbrows; i++)
			{
				unsigned long k = dyntab_cell(&optlist, i, 0)->row;
				EVA_ctrl *optctrl = form->ctrl + k;
				int b_optsubmit = b_submit || b_multiple && DYNTAB_FIELD_CELL(&optctrl->attr, CTRLTREE);
				int b_checked;
				
				/* Search option in values to determine selected status */
				for(j = 0; j < ctrl->val.nbrows && dyntab_cmp(&optlist, i, 0, &ctrl->val, j, 0); j++);
 				val = dyntab_cell(&ctrl->val, j, 0);
				if(val) val->col  = 1;
				b_checked = (val || !ctrl->val.nbrows && ctrl->b_use_val && !dyntab_sz(&optlist, i, 0)) ? 1 : 0;

				/* Options separator : spacer or line break */
				if(i || b_empty)
				{
					if(!ctrl->COLUMNS || !((i + (b_empty ? 1 : 0)) % ctrl->COLUMNS))
						DYNBUF_ADD_STR(form->html, "<br>")
					else
						DYNBUF_ADD_STR(form->html, " ");
				}

				/* Output option : radio or chexkbox depending on b_multiple setting */
				DYNBUF_ADD_STR(form->html, "<nobr>");
				if(put_html_chkbox(cntxt, DYNBUF_VAL_SZ(name), DYNTAB_VAL_SZ(&optlist, i, 0),
					b_checked | (b_multiple ? 0 : 2), b_optsubmit)) STACK_ERROR;
				if(val && ctrl_put_hidden_old(cntxt, ctrl, j, name, NULL, 0)) STACK_ERROR;

				/* Output option label & help button */
				DYNBUF_ADD_STR(form->html, "<span style=\"cursor:pointer\" onClick=\"ChkLblClick('");
				DYNBUF_ADD_BUF(form->html, name, NO_CONV);
				DYNBUF_ADD3_INT(form->html, "',", (i + b_empty), ",");
				DYNBUF_ADD_INT(form->html, b_optsubmit);
				DYNBUF_ADD3_INT(form->html, ",", b_multiple, ")\">");
				if(b_checked) DYNBUF_ADD_STR(form->html, "<b>");
				DYNBUF_ADD_CELL(form->html, &optlist, i, ctrl->b_use_val ? 1 : 0, TO_HTML);
				DYNBUF_ADD_STR(form->html, "</span>");
				if(b_checked) DYNBUF_ADD_STR(form->html, "</b>");
				if(k && optctrl->NOTES && optctrl->NOTES[0] && ctrl_add_opt_btn(cntxt, optctrl)) STACK_ERROR;
				DYNBUF_ADD_STR(form->html, "</nobr>");
			}
		
			/* Add HTML code for unlisted values */
			for(i = 0; i < ctrl->val.nbrows; i++) 
			{
				DynTableCell *val = dyntab_cell(&ctrl->val, i, 0);
				if(!val->col && val->len && val->txt && val->txt[0])
				{
					/* Output checkbox & option label */
					DYNBUF_ADD_STR(form->html, "<br>");
					if(put_html_chkbox(cntxt, DYNBUF_VAL_SZ(name), DYNTAB_VAL_SZ(&ctrl->val, i, 0), b_multiple ? 1 : 3, b_submit)) STACK_ERROR;
					if(ctrl_put_hidden_old(cntxt, ctrl, i, name, NULL, 0)) STACK_ERROR;
					DYNBUF_ADD_CELL(form->html, &ctrl->val, i, 0, TO_HTML);
					DYNBUF_ADD_STR(form->html, "<br>");
				}
			}

			/* Add hidden input for empty value */
			if(!ctrl->val.nbrows)
			{
				if(ctrl_put_hidden_old(cntxt, ctrl, i, name, NULL, 0)) STACK_ERROR;
			}
			DYNBUF_ADD_STR(form->html, "</font>");
		}
		else
		{
			/* Standard list input */
			b_submit = b_multiple;
			for(i = ctrl->i_child; !b_submit && i; i = form->ctrl[i].i_brother)
				b_submit = ATTR_CELL(form->ctrl + i, CTRLTREE) != 0;
			if(ctrl_put_html_input_list(cntxt, i_ctrl, &optlist, b_submit)) STACK_ERROR;
		}

		/* Handle free user input if allowed */
		if(*allow_other && (b_multiple || !ctrl->val.nbrows))
		{
			CTRL_CGINAMEVAL(&name, ctrl->val.nbrows);
			DYNBUF_ADD_STR(&name, ".0");
			DYNBUF_ADD_STR(form->html, "<br>");
			if(put_html_text_input(cntxt, 
								DYNBUF_VAL_SZ(name),
								NULL, 0, 0, 0, 1, ctrl->COLUMNS * 2 / 3,
								atoi(CTRL_ATTR_VAL(MAXLENGTH)))) STACK_ERROR;

		}

		/* Add control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	case HtmlPrint:
	case HtmlView:
		if(!form->html) break;
		if(ctrl_add_input_list_view(cntxt, i_ctrl, &options)) STACK_ERROR;
		break;
	}

	/* Handle controls under selected options */
	if(ctrl_add_selected_options(cntxt, i_ctrl, &options)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
