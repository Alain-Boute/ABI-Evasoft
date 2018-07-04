/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_exprval.c
** Description : handling functions for computed value controls 
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : html_put_value_fmt
** Description : output a formated cell for a table of objects
*********************************************************************/
#define ERR_FUNCTION "html_put_value_fmt"
#define ERR_CLEANUP DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(objdata); \
					M_FREE(stripval) 
int html_put_value_fmt(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	EVA_ctrl *ctrl,						/* in : output control */
	DynTable *table,					/* in : values table */
	unsigned long row,					/* in : index of value to process in table */
	char *fmt,							/* in : format specification */
	size_t strip,						/* in : max length for output values */
	size_t *tot_char,					/* in/out : output values char counter */
	int *b_stripped						/* out : 1 if value was stripped, 0 else */
){
	EVA_form *form = cntxt->form;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynTable id_obj = { 0 };
	DynTable objdata = { 0 };
	DynTableCell *val = dyntab_cell(table, row, 0);
	DynBuffer *stripval = NULL;
	int tooltip_mode = atoi(CTRL_ATTR_VAL(TOOLTIP_MODE));
	CHECK_HTML_STATUS;
#define HANDLE_STRIP(code) DYNBUF_HANDLE_STRIP(html, strip, *tot_char, *b_stripped, code)

	/* Return if no value */
	if(!val || !val->txt || !val->len) RETURN_OK;

	/* Init stripping */
	*b_stripped = 0;

	/* Compute best format if none given and value not empty */
	if((!fmt || !*fmt) && val->txt && val->len)
	{
		if(val->b_relation)
			fmt = "_EVA_RELATION";
		else if(!strncmp(val->txt, add_sz_str("http://")) ||
				!strncmp(val->txt, add_sz_str("ftp://")) ||
				!strncmp(val->txt, add_sz_str("www.")))
			fmt = "_EVA_HTTPLINK";
	}

	/* Handle value format */
	if(!strncmp(fmt, add_sz_str("_EVA_RELATION")) || !strcmp(fmt, "_EVA_TXTLINK") && form->step != HtmlPrint)
	{
		/* Relation to other object : check if given value is a relation */
		char *idend = NULL;
		unsigned long idrelobj = val->txt ? strtoul(val->txt, &idend, 10) : 0;
		int b_button = strstr(fmt, "BTN") != NULL;
		int b_formlb = strstr(fmt, "FORM") != NULL;
		int b_label = strcmp(fmt, "_EVA_RELATION_BTN0");
		if(idend && *idend) idrelobj = 0;

		/* Set value object if value is not a relation */
		if(!strcmp(fmt, "_EVA_TXTLINK"))
		{
			idrelobj = val->IdValObj ? val->IdValObj : val->IdObj;
			val->b_relation = 0;
		}
		else if(!val->IdValObj) val->IdValObj = idrelobj;
		
		if(!idrelobj)
		{
			DYNBUF_ADD_CELL(html, table, row, 0, TO_HTML);
		}
		else 
		{
			/* Output open button */
			char symtyp[64] = {0};
			unsigned long symlen = 0;
			if(b_button) symlen += snprintf(symtyp + symlen, sizeof(symtyp) - symlen, "%sSYMBOL", *symtyp ? "+" : "");
			if(b_formlb) symlen += snprintf(symtyp + symlen, sizeof(symtyp) - symlen, "%sNAME", *symtyp ? "+" : "");
			if(b_label) symlen += snprintf(symtyp + symlen, sizeof(symtyp) - symlen, "%sLABEL", *symtyp ? "+" : "");
			if(!tbl || (tooltip_mode & 1)) symlen += snprintf(symtyp + symlen, sizeof(symtyp) - symlen, "%sOBJNOTES", *symtyp ? "+" : "");
			if(b_button) DYNBUF_ADD_STR(html, "<table noborder cellpadding=0 cellspacing=0><tr>");
			if(qry_cache_idobj(&objdata, idrelobj) ||
				ctrl_add_symbol_btn(cntxt, ctrl, val, &objdata, 0, b_button ? "" : NULL, symtyp))
				STACK_ERROR;
			if(b_button) DYNBUF_ADD_STR(html, "</tr></table>");
		}
		
		/* Handle striping */
		(*tot_char) ++;
	}
	else if(!strcmp(fmt, "_EVA_EMAIL"))
	{
		/* Email adress */
		DYNBUF_ADD3_CELL(html, "<a href='mailto:", table, row, 0, HTML_NO_QUOTE, "'>");
		HANDLE_STRIP({
			if(put_html_bold_substring(cntxt, html, DYNTAB_VAL_SZ(table, row, 0), tbl ? &tbl->srchwords : NULL))
				STACK_ERROR;
		})
		DYNBUF_ADD_STR(html, "</a>");
	}
	else if(!strcmp(fmt, "_EVA_HTTPLINK"))
	{
		/* HTTP link */
		size_t sz = dyntab_sz(table, row, 0);
		char *url = dyntab_val(table, row, 0);
		char *adr = strstr(url, "://");
		char *urlstrip = sz < 200 ? NULL : strchr(url + 30, '/');
		DYNBUF_ADD_STR(html, "<a href='");
		if(!adr && *url != '.') DYNBUF_ADD_STR(html, "http://");
		DYNBUF_ADD(html, url, sz, HTML_NO_QUOTE);
		DYNBUF_ADD_STR(html, "' target=_blank>");
		adr = adr ? adr + 3 : url;
		sz = urlstrip ? urlstrip - adr + 1 : strlen(adr);
		HANDLE_STRIP({
			if(put_html_bold_substring(cntxt, html, adr, sz, tbl ? &tbl->srchwords : NULL))
				STACK_ERROR;
			if(urlstrip) DYNBUF_ADD_STR(html, "...");
		})
		DYNBUF_ADD_STR(html, "</a>");
	}
	else if(!strcmp(fmt, "_EVA_DOCUMENT"))
	{
		/* Link to document */
		char *filename;
		DynTableCell *name, *path;

		/* Read linked object */
		DYNTAB_SET_CELL(&id_obj, 0, 0, table, row, 0);
		if(qry_obj_data(cntxt, &objdata, &id_obj, NULL)) STACK_ERROR;

		/* Add link to file */
		path = DYNTAB_FIELD_CELL(&objdata, FILE_SERVER_NAME);
		if(path)
		{
			DYNBUF_ADD3(html, "<a href='../" DIRECTORY_DOCS "/", cntxt->dbname, 0, NO_CONV, "");
			DYNBUF_ADD3_CELLP(html, "/", path, HTML_NO_QUOTE, "'");
			DYNBUF_ADD3_CELLP(html, " target='W", path, HTML_NO_QUOTE, "'>");
		}
		name = DYNTAB_FIELD_CELL(&objdata, FILE_CLIENT_PATH);
		filename = (name && name->len) ? basename(name->txt, name->len) : "???";
		HANDLE_STRIP({
			if(put_html_bold_substring(cntxt, html, filename, 0, tbl ? &tbl->srchwords : NULL))
				STACK_ERROR;
		})
		if(path) DYNBUF_ADD_STR(html, "</a>");
	}
	else if(!strcmp(fmt, "_EVA_HTML"))
	{
		/* HTML : output raw value */
		DYNBUF_ADD_CELL(html, table, row, 0, NO_CONV);
	}
	else if(!strcmp(fmt, "_EVA_IMAGE"))
	{
		/* Image : output <img> tag of appropriate size */
		DynTableCell *c = dyntab_cell(table, row, 0);
		unsigned long obj = DYNTAB_TOUL(&form->id_obj);
		unsigned long idobj = (c->IdObj != obj) ? c->IdObj : (c->IdValObj != obj) ? c->IdValObj : 0;
		char htxt[16], *w = CTRL_ATTR_VAL(IMG_WIDTH), *h = CTRL_ATTR_VAL(IMG_HEIGHT);

		/* Handle default image size */
		if(!*w && !*h)
		{
			w = ctrl->WIDTH;
			h = ctrl->HEIGHT;
		}

		/* Handle image in list field */
		if(tbl)
		{
			DynTableCell *fmt = dyntab_cell(&tbl->fontsize, c->col, 0);
			int hnum = (fmt && fmt->txt) ? 100 + 40 * atoi(fmt->txt) : 0;
			char *dh = (!fmt || !fmt->txt) ? "50" : *fmt->txt == '0' ? NULL : isdigit(*fmt->txt) ? fmt->txt : itoa(hnum, htxt, 10);
			if(dh && *dh != '0')
			{
				h = dh;
				w = NULL;
			}
		}

		/* Output image or thumbnail */
		if(put_html_image_dir(cntxt, NULL, CTRL_ATTR_VAL(IMAGE_DIR), c->txt, NULL, w, h, 0, idobj, NULL, NULL)) STACK_ERROR;
	}
	else if(!strcmp(fmt, "_EVA_AGE") ||
			!strcmp(fmt, "_EVA_FILESIZE") ||
			!strcmp(fmt, "_EVA_NUMBER") ||
			!stricmp(fmt, "_EVA_MONEY") ||
			!strcmp(fmt, "_EVA_TELEPHONE"))
	{
		/* Age, file size, number, money */
		HANDLE_STRIP({
			if(put_value_fmt(cntxt, &stripval, DYNTAB_VAL_SZ(table, row, 0), fmt) ||
				put_html_bold_substring(cntxt, html, DYNBUF_VAL_SZ(stripval), tbl ? &tbl->srchwords : NULL)) STACK_ERROR;
		});
	}
	else if(!strncmp(fmt, add_sz_str("_EVA_")))
	{
		/* Date : use put_value_fmt */
		HANDLE_STRIP(
			{ if(put_value_fmt(cntxt, html, DYNTAB_VAL_SZ(table, row, 0), fmt)) STACK_ERROR; });
	}
	else 
	{
		/* Output data with bolded search string if applicable */
		if(tbl && tbl->input && tbl->input[0] && (tbl->srchcol >= tbl->field.nbrows || 
				tbl->srchcol & (1 << val->col)) && (tbl->status & TblCtrl_opensearch))
		{
			HANDLE_STRIP(
			{
				DYNBUF_ADD_CELL(&stripval, table, row, 0, NO_CONV);
				if(put_html_bold_substring(cntxt, html, DYNBUF_VAL_SZ(stripval), tbl ? &tbl->srchwords : NULL))
					STACK_ERROR;
			})
		}
		else
			HANDLE_STRIP(
				DYNBUF_ADD_CELL(html, table, row, 0, TO_HTML));
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_build_joins
** Description : compute values of CALC controls
*********************************************************************/
#define ERR_FUNCTION "qry_build_joins"
#define ERR_CLEANUP	DYNTAB_FREE(typ); \
					DYNTAB_FREE(field); \
					DYNTAB_FREE(src); \
					DYNTAB_FREE(left); \
					DYNTAB_FREE(datedel); \
					DYNTAB_FREE(fld); \
					DYNTAB_FREE(data); \
					M_FREE(bef); \
					M_FREE(aft); \
					M_FREE(cond); \
					M_FREE(join)
int qry_build_joins(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **sql,				/* out : SELECT, JOIN & WHERE clauses */
	char *select,					/* in : SELECT clauses */
	char *from,						/* in : FROM clauses */
	DynTable *joins,				/* in : joins definition */
	char *where,					/* in : WHERE clauses */
	char *other						/* in : strings to search for TVal/TField references */
){
	DynTable typ = {0};
	DynTable field = {0};
	DynTable src = {0};
	DynTable left = {0};
	DynTable datedel = {0};
	DynTable fld = {0};
	DynTable data = {0};
	DynBuffer *bef = sql ? *sql : NULL;
	DynBuffer *aft = NULL;
	DynBuffer *join = NULL;
	DynBuffer *cond = NULL;
	unsigned long i, k = 0;

	/* Return if no select */
	if(!sql) RETURN_OK;
	*sql = NULL;
	if(!(select && *select)) RETURN_OK;

	/* Read joins params */
	DYNTAB_FIELD_TABK(&typ, joins, JOIN_TYPE);
	DYNTAB_FIELD_TABK(&field, joins, JOIN_FIELD);
	DYNTAB_FIELD_TABK(&src, joins, JOIN_SRC);
	DYNTAB_FIELD_TABK(&datedel, joins, JOIN_DATEDEL);
	DYNTAB_FIELD_TABK(&left, joins, JOIN_LEFT);

	/* Handle parameters in SELECT & FROM clauses */
	DYNBUF_ADD_STR(&bef, "SELECT ");
	if(qry_eval_sqlexpr_var(cntxt, &bef, select, joins, NULL)) STACK_ERROR;
	DYNBUF_ADD_STR(&bef, "\nFROM ");
	if(qry_eval_sqlexpr_var(cntxt, &bef, from, joins, NULL)) STACK_ERROR;

	/* Handle parameters in other clauses */
	if(where && *where)
	{
		if(qry_eval_sqlexpr_var(cntxt, &aft, where, joins, NULL)) STACK_ERROR;
		DYNBUF_ADD_STR(&aft, ")");
	}
	if(other && *other)
	{
		DYNBUF_ADD_STR(&aft, "\n");
		if(qry_eval_sqlexpr_var(cntxt, &aft, other, joins, NULL)) STACK_ERROR;
	}

	/* Loop on joins */
	for(i = 0; i < k; i++)
	{
		char *t = dyntab_val(&typ, i, 0);
		char *dtdel = dyntab_val(&datedel, i, 0);
		int b_left = dyntab_sz(&left, i, 0);
		char *tsrc = dyntab_val(&src, i, 0);
		char src[16], *end;
		unsigned long n = strtoul(tsrc, &end, 10);
		char printbuf[128];

		/* Detect references to TVal or TField */
		char tval[16], tfield[16];
		int b_val = 0, b_field = 0;
		sprintf(tval, "TVal%lu.", i + 1);
		sprintf(tfield, "TField%lu.", i + 1);
		b_val = bef && strstr(bef->data, tval) || aft && strstr(aft->data, tval);
		b_field = bef && strstr(bef->data, tfield) || aft && strstr(aft->data, tfield);

		/* Build source table name */
		if(!*tsrc || end > tsrc)
		{
			sprintf(src, "TLink%lu", end > tsrc ? n : i ? i : (from && *from) ? 0 : 1); 
			tsrc = src;
		}

		/* Handle TLink join */
		if(!i && !(from && *from) && tsrc == src)
		{
			dynbuf_print(&join, "TLink AS %s ", src);
		}
		else
		{
			dynbuf_print3(&join, "\n%s JOIN TLink AS TLink%lu ON TLink%lu.", b_left ? "LEFT" : "INNER", i + 1, i + 1);
			if(!strcmp(t, "_EVA_DIRECT"))
			{
				dynbuf_print(&join, "IdObj=%s.IdRelObj", tsrc);
			}
			else if(!strcmp(t, "_EVA_REVERSE"))
			{
				dynbuf_print(&join, "IdRelObj=%s.IdObj", tsrc);
			}
			else if(!strcmp(t, "_EVA_DUAL"))
			{
				dynbuf_print(&join, "IdRelObj=%s.IdRelObj", tsrc);
			}
			else if(!strcmp(t, "_EVA_LINE"))
			{
				dynbuf_print3(&join, "IdObj=%s.IdObj AND TLink%lu.Line=%s.Line", tsrc, i + 1, tsrc);
			}
			else if(strchr(t, '='))
			{
				char *e = strchr(t, '=');
				dynbuf_print4(&join, "%.*s%s.%s", e - t + 1, t, tsrc, e + 1);
			}
			else
			{
				dynbuf_print(&join, "IdObj=%s.IdObj", tsrc);
			}
		}

		/* Handle TVal & TField joins */
		if(b_val && *tsrc)
			dynbuf_print4(&join, "\n%s JOIN TVal AS TVal%lu ON TVal%lu.IdValue=TLink%lu.IdValue", b_left ? "LEFT" : "INNER", i + 1, i + 1, i + 1);
		if(b_field && *tsrc)
			dynbuf_print4(&join, "\n%s JOIN TVal AS TField%lu ON TField%lu.IdValue=TLink%lu.IdField", b_left ? "LEFT" : "INNER", i + 1, i + 1, i + 1);

		/* Handle field & date condition */
		if(dyntab_sz(&field, i, 0))
		{
			if(dyntab_from_tab(&fld, 0, 0, &field, i, 0, i + 1, field.nbcols, 17)) RETURN_ERR_MEMORY;
			if(qry_listobj_field(cntxt, &data, &fld, "_EVA_FIELD")) STACK_ERROR;
			dynbuf_print2(&cond, "%sTLink%lu.IdField IN(", cond ? "\nAND " : "", i + 1);
			if(qry_values_list(cntxt, &data, 3, &cond)) STACK_ERROR;
			DYNBUF_ADD_STR(&cond, ")");
		}
		if(!*dtdel)
			dynbuf_print2(&cond, "%sTLink%lu.DateDel IS NULL", cond ? " AND " : "", i + 1);
	}

	/* Build SQL expression */
	if(!join) RETURN_OK;
	DYNBUF_ADD_BUF(sql, bef, NO_CONV);
	DYNBUF_ADD_BUF(sql, join, NO_CONV);
	if(where && *where || cond) DYNBUF_ADD_STR(sql, "\nWHERE (");
	if(cond) DYNBUF_ADD3_BUF(sql, "", cond, NO_CONV, ")");
	if(where && *where && cond) DYNBUF_ADD_STR(sql, "\nAND (");
	if(aft) DYNBUF_ADD_BUF(sql, aft, NO_CONV);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : subform_output
** Description : compute values of CALC controls
*********************************************************************/
#define ERR_FUNCTION "subform_output"
#define ERR_CLEANUP	M_FREE(buf); \
					form_free(cntxt); \
					cntxt->form = form0
int subform_output(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : container control */
	DynTable *srcobj,				/* in : list of objects to output */
	DynTable *srcfield,				/* in : list of controls to output for each object */
	int mode						/* in : output mode - bitmask
										bit 0 : hide labels
										bit 1 : ignore position (always newline) */
){
	EVA_form *form = NULL, *form0 = cntxt->form;
	unsigned long i, j;
	DynBuffer *buf = NULL;
	char *layout = CTRL_ATTR_VAL(LAYOUT_MODE);

	/* Load new form based on current form */
	M_ALLOC(cntxt->form, sizeof(EVA_form));
	form = cntxt->form;
	DYNTAB_ADD_CELL(&form->id_form, 0, 0, &form0->id_form, 0, 0);
	M_REALLOC(EVA_ctrl, form->ctrl, form->nb_ctrl);
	if(dyntab_from_tab(&form->ctrl->attr, 0, 0, &form0->ctrl->attr, 0, 0, form0->ctrl->attr.nbrows, 1, 7)) RETURN_ERR_MEMORY;
	if(ctrl_set_common_attr(cntxt, form->ctrl)) STACK_ERROR;

	/* Load selected controls in form */
	for(i = 0; i < srcfield->nbrows; i++)
	{
		if(ctrl_add_new(cntxt, DYNTAB_TOULRC(srcfield, i, 0), &j)) STACK_ERROR;
		if(mode & 1) form->ctrl[j].LABELPOS = "_EVA_NONE";
		if(mode & 2) form->ctrl[j].POSITION = "_EVA_NewLine";
	}

	/* Read & output each object */
	form->b_noctrltree = 1;
	form->html = form0->html;
	if(ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;
	for(i = 0; i < srcobj->nbrows; i++)
	{
		DYNTAB_SET_CELL(&form->id_obj, 0, 0, srcobj, i, 0);
		if(qry_obj_data(cntxt, &form->objdata, &form->id_obj, NULL)) STACK_ERROR;
		for(j = 1; j <= srcfield->nbrows; j++)
		{
			unsigned long cnt = (*form->html) ? (*form->html)->cnt : 0;
			if(cgi_build_basename(cntxt, &form->ctrl[j].cginame, j, 'C')) STACK_ERROR;
			form->step = CtrlRead;
			CTRL_PRI_HDLR(j);
			if(*layout)
			{
				EVA_ctrl *c = form->ctrl + j;
				c->POSITION = "";
				c->ALIGN = "";
				c->VALIGN = "";
				c->LABELPOS= "";
				c->LABELALIGN = "";
				c->LABELVALIGN = "";
			}
			form->step = HtmlView;
			CTRL_PRI_HDLR(j);
			if(!form->ctrl[j].val.nbrows)
				(*form->html)->cnt = cnt;
			DYNTAB_FREE(form->ctrl[j].val);
		}
	}
	if(ctrl_put_table_footer(cntxt, ctrl)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_exprval
** Description : compute values of CALC controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_exprval"
#define ERR_CLEANUP	DYNTAB_FREE(srcobj); \
					DYNTAB_FREE(srcfield); \
					qry_build_free(&flt); \
					M_FREE(expr); \
					cntxt->sql_trace = sql_trace
int ctrl_output_exprval(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : control to output */
	unsigned long options			/* in : output options
										0 : HTML values using tables
										1 : HTML values in standard text
										2 : output to column 1 of ctrl->val
										3 : output to column 0 of ctrl->val */
){
	DynTable srcobj = {0};
	DynTable srcfield = {0};
	QryBuild flt = {0};
	DynBuffer *expr = NULL;
	char *type = CTRL_ATTR_VAL(TYPE);
	DynBuffer **html = cntxt->form->html;
	int sql_trace = cntxt->sql_trace;

	/* Handle debug mode */
	if(*CTRL_ATTR_VAL(DEBUG_SQL_EXPR) == '1') cntxt->sql_trace = DEBUG_SQL_RES;

	/* Add control value depending on value type */
	if(!strcmp("_EVA_USERNAME", type))
	{
		if(cntxt->b_identified)
		{
			char opname[64];
			unsigned long idform;
			DYNTAB_FIELD(&srcobj, &cntxt->user_data, FORMSTAMP);
			if(dyntab_group(&srcobj, "NMIN")) RETURN_ERR_MEMORY;
			idform = DYNTAB_TOUL(&srcobj);
			if(qry_obj_label(cntxt, &expr, NULL, NULL, &expr, &expr, NULL, NULL, NULL, idform, &cntxt->user_data, 0)) CLEAR_ERROR;
			build_open_btn_name(cntxt, opname, idform, cntxt->user_data.cell->IdObj, 0, 0);
			if(put_html_button_sz(cntxt, opname, DYNTAB_FIELD_VAL(&cntxt->user_data, USERNAME),
									NULL, NULL, expr->data, NULL, 0, 0, 0, 32)) STACK_ERROR;
		}
		else
		{
			char *uname = CTRL_ATTR_VAL(NOT_IDENTIFIED);
			if(!*uname) uname = "Utilisateur non identifié";
			DYNBUF_ADD(html, uname, 0, NO_CONV);
		}
		if(options == 2) DYNTAB_ADD_BUF(&ctrl->val, 0, 1, expr);
	}
	else if(!strcmp("_EVA_SYSTEMTIME", type))
	{
		char txt[32];
		datetxt_to_format(cntxt, txt, cntxt->timestamp, NULL);
		DYNBUF_ADD(html, txt, 0, NO_CONV);
		if(options == 2) DYNTAB_ADD_BUF(&ctrl->val, 0, 1, expr);
	}
	else if(!strcmp("_EVA_CTRLVAL", type))
	{
		/* List of controls : get source objects */
		char *srctyp = CTRL_ATTR_VAL(SRC_TYPE);
		if(!strcmp(srctyp, "_EVA_SRC_CTRL")) CTRL_ATTR(srcfield, SRC_CTRL)
		else 
		{
			srctyp = "_EVA_FILTER";
			CTRL_ATTR(srcfield, FILTERS);
		}
		if(!*srctyp || !strcmp(srctyp, "_EVA_FILTER") && !srcfield.nbrows)
			DYNTAB_ADD_CELL(&srcobj, 0, 0, &cntxt->form->id_obj, 0, 0)
		else
			if(form_eval_valtype(cntxt, &srcobj, srctyp, &srcfield, NULL)) STACK_ERROR;

		/* Read list of controls & output each matched object */
		CTRL_ATTR(srcfield, CTRLTREE);
		if(subform_output(cntxt, ctrl, &srcobj, &srcfield, 0)) STACK_ERROR;
	}
	else
	{
		/* Field / expression value : output result values */
		unsigned long i;
		size_t tot_char = 0;
		int b_stripped;
		char *fmt = CTRL_ATTR_VAL(DISPLAYFORMAT);
		char *vmult = CTRL_ATTR_VAL(MULTIPLE_HANDLER);
		DynTableCell *lblunit = CTRL_ATTR_CELL(LABELUNIT);
		char *separ = ", ";

		/* Handle SQL query */
		if(!strcmp("_EVA_SQL_QUERY", type))
		{
			char *qry = 0;
			int b_end = 1;

			/* Evaluate variables in SQL expression */
			if(qry_eval_sqlexpr_var(cntxt, &expr, CTRL_ATTR_VAL(SQL_SCRIPT), &ctrl->attr, NULL)) STACK_ERROR;
			if(expr)
			{
				b_end = 0;
				qry = expr->data;
			}

			/* Process queries separated with ";\n" */
			while(!b_end)
			{
				char *end = strstr(qry, ";\n");
				if(!end) { end = expr->data + expr->cnt - 1; b_end = 1; }
				if(!b_end) *end = 0;
				if(sql_exec_query(cntxt, qry)) CLEAR_ERROR_RETURN;
				qry = end + 2;
			}
			if(sql_get_table(cntxt, &ctrl->val, 4)) CLEAR_ERROR;
		}
		/* Handle advanced SQL expression */
		else if(!strcmp("_EVA_SQLEXPR", type))
		{
			DYNBUF_ADD3(&expr, "-- SQL value [", ctrl->LABEL, 0, TO_SYSCMD, "]\n");
			if(qry_build_joins(cntxt, &expr, 
								CTRL_ATTR_VAL(SELECT_CLAUSE), 
								CTRL_ATTR_VAL(FROM_CLAUSE), 
								&ctrl->attr, 
								CTRL_ATTR_VAL(WHERE_CLAUSE),
								CTRL_ATTR_VAL(OTHER_CLAUSE))) CLEAR_ERROR;
			if(expr && (
				sql_exec_query(cntxt, expr->data) || sql_get_table(cntxt, &ctrl->val, 4))) CLEAR_ERROR;
		}
		/* Handle control values */
		else if(!strcmp("_EVA_CTRL_VAL", type))
		{
			unsigned long i;
			CTRL_ATTR(srcobj, CTRLTREE);
			for(i = 0; i < srcobj.nbrows; i++)
			{
				char *field;
				if(qry_cache_idobj_field(cntxt, &srcfield, DYNTAB_TOULRC(&srcobj, i, 0), NULL, 2) ||
					ctrl_read_baseobj(cntxt, &srcfield)) STACK_ERROR;
				field = dyntab_field_val(&srcfield, "_EVA_FIELD", ~0UL, 0);
				dyntab_free(&srcfield);
				if(*field && form_get_field_values(cntxt, &srcfield, field, 0, 0)) STACK_ERROR;
				if(dyntab_from_tab(&ctrl->val, ctrl->val.nbrows, 0, &srcfield, 0, 0, srcfield.nbrows, 1, 33)) RETURN_ERR_MEMORY;
			}
		}
		/* Handle SQL/relational expression */
		else
		{
			char *valtyp = CTRL_ATTR_VAL(SRC_OBJ);
			char *meth = CTRL_ATTR_VAL(EVAL_METHOD);
			char *m = !strncmp("_EVA_", vmult, 5) ? vmult + 5 : "";
			if(!*valtyp) valtyp = "_EVA_CURRENTOBJ";
			CTRL_ATTR(srcobj, LISTOBJ);
			CTRL_ATTR(srcfield, SRCFIELD);
			if(!strcmp(meth, "_EVA_DB") && !strcmp(valtyp, "_EVA_FILTER") && srcfield.nbrows)
			{
				char objtbl[16];
				snprintf(objtbl, sizeof(objtbl), "EV%s", (ctrl->id.cell && ctrl->id.cell->txt) ? ctrl->id.cell->txt : "0");
				if(qry_add_filter_forms(cntxt, &flt, NULL, &srcobj) || qry_filter_table(cntxt, objtbl, &flt, 0)) STACK_ERROR;
				if(qry_eval_sqlexpr_var(cntxt, &expr, srcfield.cell->txt, &ctrl->attr, objtbl)) STACK_ERROR;
				if(strcmp(m, "MIN") && strcmp(m, "MAX") && strcmp(m, "SUM") && strcmp(m, "AVG"))
				{
					if(qry_eval_fieldexpr(cntxt, &ctrl->val, expr ? expr->data : NULL, objtbl, &ctrl->attr)) STACK_ERROR;
				}
				else
				{
					char qry[64];
					snprintf(qry, sizeof(qry), "SELECT %s(Val) FROM EVRes", m);
					if(qry_eval_sql_fieldexpr(cntxt, "EVRes", expr ? expr->data : NULL, objtbl, &ctrl->attr) ||
						sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &ctrl->val, 0)) STACK_ERROR;
					sql_drop_table(cntxt, "EVRes");
					vmult = "";

				}
				sql_drop_table(cntxt, objtbl);
			}
			else
			{
				if(ctrl_eval_valtyp(cntxt, ctrl, &ctrl->val, valtyp, &srcobj, &srcfield)) CLEAR_ERROR;
			}
		}

		/* Handle multiple values */
		ctrl->MULTIPLE = "";
		if(!strcmp(vmult, "_EVA_LINE"))
		{
			ctrl->MULTIPLE = "Yes";
			separ = "<br>";
		}
		else if(!strcmp(vmult, "_EVA_COMMA"))
		{
			ctrl->MULTIPLE = "Yes";
		}
		else if(!strcmp(vmult, "_EVA_SPACE"))
		{
			if(dyntab_group(&ctrl->val, "DISTINCT")) RETURN_ERR_MEMORY;
			ctrl->MULTIPLE = "Yes";
			separ = " ";
		}
		else if(!strncmp(vmult, add_sz_str("_EVA_")))
		{
			if(dyntab_group(&ctrl->val, vmult + 5)) RETURN_ERR_MEMORY;
		}
		else
		{
			ctrl->MULTIPLE = "Unique";
			separ = "<br>";
		}

		/* Output values */
		if(options < 3) for(i = 0; i < ctrl->val.nbrows; i++)
		{
			/* Output formated value */
			if(options)
			{
				M_FREE(expr);
				if(put_value_fmt(cntxt, &expr, DYNTAB_VAL_SZ(&ctrl->val, i, 0), fmt)) CLEAR_ERROR;
				if(options < 2)
				{
					DYNBUF_ADD_BUF(html, expr, TO_HTML);
					DYNBUF_ADD(html, separ, 0, NO_CONV);
				}
				else
					DYNTAB_ADD_BUF(&ctrl->val, i, 1, expr);
			}
			else
			{
				if(i && html && *html && (*html)->cnt > 8 && strcmp("</table>", (*html)->data + (*html)->cnt - 8))
					DYNBUF_ADD(html, separ, 0, NO_CONV);
				if(html_put_value_fmt(cntxt, ctrl, &ctrl->val, i, fmt, 0, &tot_char, &b_stripped))
					CLEAR_ERROR;
			}
			if(lblunit) DYNBUF_ADD3_CELLP(html, " ", lblunit, TO_HTML, "");

			/* Add value to list of values if in a table */
			if(cntxt->form->Line) DYNTAB_ADD_CELL(&ctrl->allval, ctrl->allval.nbrows, 0, &ctrl->val, i, 0);
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_exprval
** Description : handles CALC controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_exprval"
#define ERR_CLEANUP	
int ctrl_add_exprval(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl		/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	CHECK_HTML_STATUS;

	if(form->step == HtmlView || form->step == HtmlPrint || form->step == HtmlEdit)
	{
		/* Handle auto nobr for specific formats */
		char *fmt = CTRL_ATTR_VAL(DISPLAYFORMAT);
		if(!(ctrl->NOBR && *ctrl->NOBR) && (
				!strcmp(fmt, "_EVA_FILESIZE") ||
				!strcmp(fmt, "_EVA_NUMBER") ||
				!stricmp(fmt, "_EVA_MONEY") ||
				!strcmp(fmt, "_EVA_TELEPHONE")))
			ctrl->NOBR  = "1";

		/* Add HTML code for the text control label & position */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Add control value depending on value type */
		if(ctrl_output_exprval(cntxt, ctrl, 0)) STACK_ERROR;

		/* Add HTML code for the control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
