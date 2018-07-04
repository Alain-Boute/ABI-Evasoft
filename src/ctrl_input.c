/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input.c
** Description : general handling functions for all input controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_check_changes
** Description : check control values modifications
*********************************************************************/
void ctrl_check_changes(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	int b_ignorenew					/* in : do not check unmodified new values if not null */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i, j;
	DynTableCell *dbval, *val;

	if(form->b_newobj) return;
	ctrl->b_modified = 0;
	if(!ctrl->storage) return;

	/* Mark new values */
	for(i = 0; i < ctrl->val.nbrows; i++)
	{
		val = dyntab_cell(&ctrl->val, i, 0);
		if(b_ignorenew && form->b_newobj && !val->b_modified) continue;
		val->b_modified = 1;
		for(j = 0; j < ctrl->dbval.nbrows; j++)
		{
			dbval = dyntab_cell(&ctrl->dbval, j, 0);
			if(!DYNTAB_SAMEVAL(val, dbval)) continue;
			val->b_modified = 0;
			break;
		}
		ctrl->b_modified |= val->b_modified;
	}

	/* Mark deleted values */
	for(j = 0; j < ctrl->dbval.nbrows; j++)
	{
		dbval = dyntab_cell(&ctrl->dbval, j, 0);
		dbval->b_modified = 1;
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			val = dyntab_cell(&ctrl->val, i, 0);
			if(!DYNTAB_SAMEVAL(val, dbval)) continue;
			dbval->b_modified = 0;
			break;
		}
		ctrl->b_modified |= dbval->b_modified;
	}
}

/*********************************************************************
** Function : ctrl_default_value
** Description : set control default value where applicable
*********************************************************************/
#define ERR_FUNCTION "ctrl_default_value"
#define ERR_CLEANUP DYNTAB_FREE(defaultval)
int ctrl_default_value(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable defaultval = {0};
	DynTable listobj = {0};
	unsigned long i;

	/* Use default value if applicable */
	if(!ctrl->val.nbrows)
	{
		char *defaultwhen = (ctrl->storage || ctrl->cgival.nbrows) ? CTRL_ATTR_VAL(DEFAULTWHEN) : "_EVA_ALWAYS";
		char *defaulttype = CTRL_ATTR_VAL(DEFAULTVALUETYPE);

		/* Check if applicable */
		if(!strcmp(defaulttype, "_EVA_NONE")) RETURN_OK;
		CTRL_ATTR(defaultval, DEFAULTVALUE);
		if((defaultval.nbrows || *defaulttype) && (
			!strcmp(defaultwhen, "_EVA_ALWAYS") ||
			!strcmp(defaultwhen, "_EVA_OPEN") && !form->ctrl->cgival.nbrows ||
			!strcmp(defaultwhen, "_EVA_NEWOBJ") && form->b_newobj ||
			form->b_newobj && !form->ctrl->cgival.nbrows))
		{
			if(*defaulttype)
			{
				/* Expression : evaluate & set ctrl->val */
				int sql_trace = cntxt->sql_trace;
				CTRL_ATTR(listobj, INIT_OBJ);
				if(CTRL_ATTR_CELL(DEBUG_SQL_DEFVAL)) cntxt->sql_trace = DEBUG_SQL_RES;
				if(ctrl_eval_valtyp(cntxt, ctrl, &ctrl->val, defaulttype, &listobj, &defaultval)) CLEAR_ERROR;
				cntxt->sql_trace = sql_trace;
			}
			else
				/* Fixed value : set ctrl->val */
				DYNTAB_ADD_CELL(&ctrl->val, 0, 0, &defaultval, 0, 0);

			/* Delete empty values */
			for(i = 1; i <= ctrl->val.nbrows; i++)
				if(!dyntab_sz(&ctrl->val, ctrl->val.nbrows - i, 0)) 
					dyntab_del_rows(&ctrl->val, ctrl->val.nbrows - i, 1);

			/* If no default value was added : free control values */
			if(!dyntab_sz(&ctrl->val, 0, 0))
				dyntab_free(&ctrl->val);
			
			/* Else if existing object */
			else if(!dyntab_sz(&ctrl->val, 0, 0) && !form->b_newobj)
			{
				/* Mark default values & ctrl as modified */
				for(i = 0; i < ctrl->val.nbrows; i++) dyntab_cell(&ctrl->val, i, 0)->b_modified = 1;
				ctrl->b_modified = 1;
			}
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_read_values
** Description : read input control values in ctrl->val
*********************************************************************/
#define ERR_FUNCTION "ctrl_read_values"
#define ERR_CLEANUP
int ctrl_read_values(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i;
	unsigned long idform = DYNTAB_TOUL(&form->id_form);
	unsigned long idobj = DYNTAB_TOUL(&form->id_obj);

	/* Call handler & return if no field info */
	if(!ctrl->FIELD[0])
	{
		ctrl->storage = 0;
		CTRL_SEC_HDLR(i_ctrl);
		RETURN_OK;
	}

	/* Read control values & status in CGI data */
	if(cgi_filter_values(cntxt, &ctrl->cgival, 'D', ~0UL, idobj ? ~0UL : idform, idobj, ctrl->FIELD, "", 0, 0))
		STACK_ERROR; 

	/* Process CGI values if any */
	if(ctrl->cgival.nbrows) for(i = 0; i < ctrl->cgival.nbrows; i++)
	{
		DynTableCell *val = dyntab_cell(&ctrl->cgival, i, 0);
		CGIData *cgi = val->i_cgi ? cntxt->cgi + val->i_cgi : NULL;
		int b_modified = ctrl->storage && cgi &&
						(form->b_newobj && !cgi->i_old && val->len||
							STRCMPNUL(cgi->value, cntxt->cgi[cgi->i_old].value));

		/* Add non empty CGI value to control values table */
		if(val->len)
		{
			DYNTAB_ADD_CELL(&ctrl->val, ctrl->val.nbrows, 0, &ctrl->cgival, i, 0);
			val = dyntab_cell(&ctrl->val, ctrl->val.nbrows - 1, 0);
			val->b_modified = (char)b_modified;
			form->has_data |= ctrl->storage;
		}
		ctrl->b_modified |= b_modified;
	}
	else
		/* Read control values from DB if no CGI */
		if(!form->b_newobj && 
			dyntab_filter_field(&ctrl->val, 0, &form->objdata, ctrl->FIELD, ~0UL, NULL)) RETURN_ERR_MEMORY;

	/* Read DB values if applicable */
	if(!form->b_newobj && 
		dyntab_filter_field(&ctrl->dbval, 0, &form->objdata, ctrl->FIELD, ~0UL, NULL)) RETURN_ERR_MEMORY;

	/* Handle default values */
	if((form->nextstep == HtmlEdit || 
		form->nextstep == HtmlView && !ctrl->storage) &&
		ctrl_default_value(cntxt, i_ctrl)) STACK_ERROR;

	/* Call control handler */
	CTRL_SEC_HDLR(i_ctrl);
	ctrl = form->ctrl + i_ctrl;

	/* Check values changes */
	ctrl_check_changes(cntxt, i_ctrl, 1);
	form->b_modified |= ctrl->b_modified;
 
	/* Clear CGI values if control unchanged (to optimize size of transmitted data) */
	if(!form->b_newobj && ctrl->storage && !ctrl->b_modified && form->prevstep != HtmlSaveDlg)
	{
		CGI_VALUES_DONTKEEP(&ctrl->cgival);
		ctrl->b_cgicleared = 1;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_filter_tables
** Description : filter a values table for current line & num index
*********************************************************************/
#define ERR_FUNCTION "ctrl_filter_tables"
#define ERR_CLEANUP 
int ctrl_filter_tables(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data
											Line & Num members are used to filter values */
	DynTable *dest,					/* in : destination table */
	DynTable *src					/* in : source table */
){
	EVA_form *form = cntxt->form;
	unsigned long i, j, num = 0, line = 0;

	/* Check line / number index mismatch (handle Line/Num index switching) */
	for(i = 0; i < src->nbrows; i++)
	{
		DynTableCell *val = dyntab_cell(src, i, 0);
		if(num < val->Num) num = val->Num;
		if(line < val->Line) line = val->Line;
	}
	if(form->Line && !line && num || form->Num && !num && line)
		for(i = 0; i < src->nbrows; i++)
		{
			DynTableCell *val = dyntab_cell(src, i, 0);
			if(form->Line) { val->Line = val->Num; val->Num = 0; }
			else { val->Num = val->Line; val->Line = 0; }
		}

	/* For each value */
	for(i = 0; i < src->nbrows; i++)
	{
		DynTableCell *val = dyntab_cell(src, i, 0);

		/* If value matches form Line & Num indexes : add to control values */
		if((!form->Line || form->Line == val->Line) && (!form->Num || form->Num == val->Num))
		{
			j = dest->nbrows;
			val->Line = form->Line;
			DYNTAB_SET_CELLP(dest, j, 0, val);
			val = dyntab_cell(dest, j, 0);
			val->row = i + 1;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_filter_values
** Description : filter control values for current line & num index
*********************************************************************/
#define ERR_FUNCTION "ctrl_filter_values"
#define ERR_CLEANUP 
int ctrl_filter_values(				/* return : 0 on success, other on error
											original values are placed in ctrl->allval
											filtered valued are placed in ctrl->val */
	EVA_context *cntxt,				/* in : execution context data
											Line & Num members are used to filter values */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;

	/* If control is already filtered - free previous filtered values */
	if(ctrl->allval.nbrows || ctrl->alldbval.nbrows)
	{
		dyntab_free(&ctrl->val);
		dyntab_free(&ctrl->dbval);
	}

	/* Else transfer values to ctrl->allval */
	else
	{
		dyntab_moveto(&ctrl->allval, &ctrl->val);
		dyntab_moveto(&ctrl->alldbval, &ctrl->dbval);
	}

	/* Filter value to ctrl->val */
	if(ctrl_filter_tables(cntxt, &ctrl->val, &ctrl->allval)) STACK_ERROR;
	if(ctrl_filter_tables(cntxt, &ctrl->dbval, &ctrl->alldbval)) STACK_ERROR;
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_filter_check_values
** Description : Filter & check control values for current line & num index
*********************************************************************/
#define ERR_FUNCTION "ctrl_filter_check_values"
#define ERR_CLEANUP 
int ctrl_filter_check_values(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	int step = form->step, b_noctrltree = form->b_noctrltree;

	/* Return if no filter active */
	if(!form->Line && !form->Num) RETURN_OK;

	/* Call control handler for InputCheck */
	form->step = InputCheck;
	form->b_noctrltree = 1;
	if(ctrl_add_input(cntxt, i_ctrl)) STACK_ERROR;
	form->step = step;
	form->b_noctrltree = b_noctrltree;
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_dontkeep_cgi_input
** Description : mark CGI input of a control for no reoutput
*********************************************************************/
void ctrl_dontkeep_cgi_input(
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i;
	if(form->html) for(i = 0; i < ctrl->cgival.nbrows; i++) 
	{
		DynTableCell *val = dyntab_cell(&ctrl->cgival, i, 0);
		if((!form->Num || val->Num == form->Num) && (!form->Line || !val->Line || val->Line == form->Line))
			cgi_value_setkeep(cntxt, &ctrl->cgival, i, 1);
	}
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_renumber_values
** Description : renumber values indexes of a control
*********************************************************************/
void ctrl_renumber_values(
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i, line, maxnum, maxline = 0;
	DynTable *ctlval = (ctrl->allval.nbrows || ctrl->alldbval.nbrows) ? &ctrl->allval : & ctrl->val;
	DynTableCell *val;
	int b_keepold = form->step == CtrlRead || CTRL_ATTR_CELL(MULTIPLE_AUTO_ORDER) || form->Num;

	if(!ctrl || !ctlval->nbrows) return;

	/* Handle duplicate values if applicable */
	if(strcmp(ctrl->MULTIPLE, "Yes"))
		for(i = 0; i < ctlval->nbrows; i++)
			for(line = i + 1; line < ctlval->nbrows; line ++)
			{
				DynTableCell *v1 = dyntab_cell(ctlval, i, 0);
				DynTableCell *v2 = dyntab_cell(ctlval, line, 0);
				if(v1->Line == v2->Line && !STRCMPNUL(v1->txt, v2->txt))
					dyntab_del_rows(ctlval, line--, 1);
			}

	/* Look for maximum line number */
	for(i = 0; i < ctlval->nbrows; i++)
	{
		val = dyntab_cell(ctlval, i, 0);
		if(val->Line > maxline) maxline = val->Line;
	}

	/* Process each line */
	for(line = 0; line <= maxline; line ++)
	{
		/* Look for maximum index numbers */
		maxnum = 0;
		if(b_keepold) for(i = 0; i < ctlval->nbrows; i++)
		{
			val = dyntab_cell(ctlval, i, 0);
			if(val->Line != line) continue;
			if(val->Num > maxnum) maxnum = val->Num;
		}

		/* Renumber values */
		for(i = 0; i < ctlval->nbrows; i ++) 
		{
			val = dyntab_cell(ctlval, i, 0);
			if(val->Line != line) continue;
			if(!b_keepold || !val->Num)
			{
				maxnum++;
				val->Num = maxnum;
			}
		}
	}

	/* Sort values on Line / Num */
	dyntab_sort(ctlval, qsort_ctrlval);

	return;
}

/*********************************************************************
** Function : ctrl_add_input
** Description : handles INPUT controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_input"
#define ERR_CLEANUP M_FREE(htmlbuf); \
					M_FREE(name); \
					form->step = step
int ctrl_add_input(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *htmlbuf = NULL;
	DynBuffer *name = NULL;
	DynBuffer **html = form->html;
	DynTableCell *field, *val;
	unsigned long i;
	int step = form->step, error = 0;

	/* Unsaved controls are always in edit mode if allowed */
	if(step == HtmlView && !ctrl->storage && ctrl->access & AccessEdit) form->step = HtmlEdit;

	switch(form->step)
	{
	case CtrlRead:
		/* Set CGI base name */
		ctrl->cginame->data[0] = 'D';
		{
			char *storage = CTRL_ATTR_VAL(STORAGE);
			ctrl->storage = !strcmp("_EVA_NONE", storage) ? 0 :
							!strcmp("_EVA_USER_PARAM", storage) ? 0 : 1;
		}

		/* Read control values & renumber */
		if(ctrl_read_values(cntxt, i_ctrl)) STACK_ERROR;
		ctrl_renumber_values(cntxt, i_ctrl);
		break;

	case InputCheck:
		/* Renumber values / handle duplicates */
		ctrl_renumber_values(cntxt, i_ctrl);

		/* Filter control values for current line & num index */
		if(ctrl_filter_values(cntxt, i_ctrl)) STACK_ERROR;

		/* Call control secondary handler */
		ctrl->error = 0;
		M_FREE(ctrl->errmsg);
		CTRL_SEC_HDLR(i_ctrl);
		error = ctrl->error;

		/* Check error status */
		if(ctrl_check_error_status(cntxt, ctrl)) STACK_ERROR;
		ctrl->error |= error;
		form->error |= ctrl->error;

		/* Check for changes in values */
		if(!ctrl->storage) ctrl->b_modified = 0;
		else ctrl_check_changes(cntxt, i_ctrl, 1);
		form->b_modified |= ctrl->b_modified;
		break;

	case HtmlSaveDlg:
		/* Ignore if input not stored */
		if(!ctrl->storage || !form->html) break;

		/* Filter control values for current line & num index */
		if(ctrl_filter_check_values(cntxt, i_ctrl)) STACK_ERROR;

		/* Check if control was already output */
		for(i = 1; i < i_ctrl && (strcmp(ctrl->FIELD, form->ctrl[i].FIELD) ||
									form->ctrl[i].outmode != HtmlEdit); i++);

		/* If control is elegible for output */
		if(i == i_ctrl && ((form->savedlg_outmode & 8 && ctrl->val.nbrows) ||
			(ctrl->error & 2 && form->savedlg_outmode & 2) ||
			(ctrl->error & 1 && form->savedlg_outmode & 1) ||
			(ctrl->b_modified && form->savedlg_outmode & 4)))
		{
			/* Set output format suitable for dialog */
			form->step = (ctrl->access & AccessEdit && (ctrl->error || ctrl->b_modified)) ? HtmlEdit : HtmlView;
			form->b_noctrltree = 1;
			ctrl->outmode = form->step;
			ctrl->POSITION = "_EVA_SameCell";
			ctrl->ALIGN = "";
			ctrl->BACKGROUND = "";
			ctrl->FONTCOLOR = "";
			ctrl->FONTSIZE = "";
			ctrl->COLSPAN = 0;
			ctrl->ROWSPAN = 0;
			if(ctrl->LINES > 5) ctrl->LINES = 5;
			if(ctrl->COLUMNS > 50) ctrl->COLUMNS = 50;
			if(!ctrl->FONTFACE[0]) ctrl->FONTFACE = "Arial";
			ctrl->LABELPOS = "_EVA_SameCell";
			ctrl->LABELBOLD = "";
			ctrl->LABELFONTCOLOR = "";
			ctrl->LABELFONTSIZE = "";
			ctrl->OPTIONBUTTON = "_EVA_ALWAYS";
			ctrl->CELL_STYLE = "";
			ctrl->LABEL_STYLE = "";

			/* Output control label & message if applicable */
			DYNBUF_ADD3(html, 
				"<td colspan=2><hr></td></tr><tr>"
				"<td align=right valign=top><font face='", ctrl->FONTFACE, 0, NO_CONV, "'>");
			if(ctrl_put_label(cntxt, ctrl, "_EVA_SameCell")) STACK_ERROR; 
			if(form->step == HtmlEdit || ctrl->error > 1)
			{
				DYNBUF_ADD_STR(form->html, "<br><font size=-1>");
				if(ctrl->b_modified) DYNBUF_ADD_STR(html, "Champ modifié");
				if(ctrl->errmsg)
				{
					if(form->step != HtmlEdit) DYNBUF_ADD_STR(html, " Vous n'avec pas accès à ce champ en modification");
					if(ctrl->b_modified || form->step != HtmlEdit) DYNBUF_ADD_STR(html, "<br>");
					DYNBUF_ADD3_BUF(html, "<font color=#FF0000><b>", ctrl->errmsg, TO_HTML, "</font></b>");
				}
				DYNBUF_ADD_STR(html, "</font>\n");
			}
			DYNBUF_ADD3(html, "</td><td valign=top><font face='", ctrl->FONTFACE, 0, NO_CONV, "'>");

			/* If edit mode : handle hidden inputs */
			if(form->step == HtmlEdit)
			{
				/* If control has cleared CGI input : restore values */
				DynTable *dbval = ctrl->alldbval.nbrows ? &ctrl->alldbval  : &ctrl->dbval;
				if(ctrl->b_cgicleared && ctrl->cgival.nbrows)
				{
					for(i = 0; i < ctrl->cgival.nbrows; i++) cgi_value_setkeep(cntxt, &ctrl->cgival, i, 0);
					ctrl->b_cgicleared = 0;
				}
				/* If control has values but no CGI input : Add hidden input for db values */
				else if(dbval->nbrows && !ctrl->cgival.nbrows)
					for(i = 0; i < dbval->nbrows; i++) 
					{
						unsigned long i_cgi = 0;
						val = dyntab_cell(dbval, i, 0);
						if(ctrl_cgi_name(cntxt, ctrl, val, 1, &name, 0, NULL, 0)) STACK_ERROR; 
						if(cgi_add_input(cntxt, &i_cgi, name->data, name->cnt, val->txt, val->len)) STACK_ERROR;
						DYNTAB_SET_CELLP(&ctrl->cgival, i, 0, val);
						val = dyntab_cell(&ctrl->cgival, i, 0);
						val->i_cgi = i_cgi;
					}

				/* Clear out CGI values from input for the values in ctrl->val */
				ctrl_dontkeep_cgi_input(cntxt, i_ctrl);

				/* Handle selection mode for relations */
				if(ctrl->objtbl && ctrl->error && strcmp(ctrl->TYPE, "_EVA_FILEUPLOAD"))
					ctrl->objtbl->status = ctrl->val.nbrows ? 
								ctrl->objtbl->status & ~(TblCtrl_opensearch | TblCtrl_opensel) :
								ctrl->objtbl->status | TblCtrl_opensearch | TblCtrl_opensel;
			}

			/* Output control */
			ctrl->LABELPOS = "_EVA_NONE";
			CTRL_SEC_HDLR(i_ctrl);

			/* Output help text if applicable */
			if((form->step == HtmlEdit || ctrl->error > 1) && ctrl->NOTES && *ctrl->NOTES)
				DYNBUF_ADD3(form->html, "\n<br><font size=-1>", ctrl->NOTES, 0, TO_HTML, "</font>");
			DYNBUF_ADD_STR(html, "</td>\n</tr><tr>\n");

			/* Restore context */
			if(ctrl_set_common_attr(cntxt, ctrl)) STACK_ERROR; 
			form->step = HtmlSaveDlg;
			form->b_noctrltree = 0;
		}

		/* Call control secondary handler to process child controls */
		CTRL_SEC_HDLR(i_ctrl);
		break;

	case FormSave:
		/* Ignore if input not stored */
		if(!ctrl->storage) break;

		/* Restore unfiltered control values */
		if(ctrl->allval.nbrows || ctrl->alldbval.nbrows)
		{
			dyntab_moveto(&ctrl->val, &ctrl->allval);
			dyntab_moveto(&ctrl->dbval, &ctrl->alldbval);
		}

		/* Check for changes against original DB values */
		ctrl_check_changes(cntxt, i_ctrl, 0);

		/* Archive old values - set delete time & user */
		for(i = 0; i < ctrl->dbval.nbrows; i++)
		{
 			char sql[512];
			val = dyntab_cell(&ctrl->dbval, i, 0);
			if(!val->b_modified) continue;
			sprintf(sql, "UPDATE TLink SET DateDel='%s', IdWhoDel=%s WHERE Pkey=%lu", 
				cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0), val->Pkey);
			if(sql_exec_query(cntxt , sql)) STACK_ERROR;
		}

		/* For each value */
		field = CTRL_ATTR_CELL(FIELD);
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			/* Create new object if needed */
			if(!dyntab_sz(&form->id_obj, 0, 0) &&
				qry_add_new_obj(cntxt, &form->id_obj, form->id_formsave)) STACK_ERROR;

			/* Read value & set object id & field */
			val = dyntab_cell(&ctrl->val, i, 0);
			val->IdObj = DYNTAB_TOUL(&form->id_obj);
			val->IdField = field->IdValue;
			if(val->txt) val->len = strlen(val->txt);

			/* If modified or new : add value in db */
			if(ctrl->storage == 3) val->b_relation = 1;
			if(val->len && (val->b_modified || !form->objdata.nbrows) && 
				qry_add_val(cntxt, val, 2, NULL)) STACK_ERROR;
		}

		/* Call control secondary handler */
		CTRL_SEC_HDLR(i_ctrl);

		/* Clear modified status */
		for(i = 0; i < ctrl->val.nbrows; i++)
			dyntab_cell(&ctrl->val, i, 0)->b_modified = 0;
		ctrl->b_modified = 0;
		break;

	case HtmlPrint:
	case HtmlView:
		/* Filter control values & Call control secondary handler */
		if(ctrl_filter_values(cntxt, i_ctrl)) STACK_ERROR;
		CTRL_SEC_HDLR(i_ctrl);
		break;

	case HtmlEdit:
		/* Filter + check control values & Call control secondary handler */
		if(ctrl_filter_check_values(cntxt, i_ctrl)) STACK_ERROR;
		CTRL_SEC_HDLR(i_ctrl);

		/* Mark corresponding CGI inputs for no reoutput */
		ctrl_dontkeep_cgi_input(cntxt, i_ctrl);
		break;

	default:
		/* Call control secondary handler */
		CTRL_SEC_HDLR(i_ctrl);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
