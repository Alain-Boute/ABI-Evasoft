/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : table_export.c
** Description : functions to prepare & export table data  
**      Author : Alain BOUTE
**     Created : Oct 22 2003
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : table_free
** Description : free ObjTableFormat structure data
*********************************************************************/
void table_free(ObjTableFormat *tbl)
{
	DYNTAB_FREE(tbl->cgiinput);
	DYNTAB_FREE(tbl->srchwords);
	DYNTAB_FREE(tbl->bgcol);
	DYNTAB_FREE(tbl->bgimg);
	DYNTAB_FREE(tbl->idobj);
	DYNTAB_FREE(tbl->data);
	DYNTAB_FREE(tbl->cellval);
	DYNTAB_FREE(tbl->altfilter);
	DYNTAB_FREE(tbl->typfilter);
	DYNTAB_FREE(tbl->condfilter);
	DYNTAB_FREE(tbl->groupfilter);
	DYNTAB_FREE(tbl->selfilter);
	DYNTAB_FREE(tbl->label);
	DYNTAB_FREE(tbl->field);
	DYNTAB_FREE(tbl->format);
	DYNTAB_FREE(tbl->notes);
	DYNTAB_FREE(tbl->fontsize);
	DYNTAB_FREE(tbl->align);
	DYNTAB_FREE(tbl->bold);
	DYNTAB_FREE(tbl->underline);
	DYNTAB_FREE(tbl->strip);
	DYNTAB_FREE(tbl->lblstyle);
	DYNTAB_FREE(tbl->celstyle);
	DYNTAB_FREE(tbl->notable);
	DYNTAB_FREE(tbl->distinctval);
	DYNTAB_FREE(tbl->lblunit);
	DYNTAB_FREE(tbl->debug);
}

/*********************************************************************
** Function : table_insert_fieldlist
** Description : handle fields indirection (format _EVA_FIELDLIST) in list of of objects
*********************************************************************/
#define ERR_FUNCTION "table_insert_fieldlist"
#define ERR_CLEANUP	DYNTAB_FREE(fldlst)
int table_insert_fieldlist(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	unsigned long ifld,					/* in : field definition index in tbl->field */
	int *offset							/* out : # of added columns in table */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynTable fldlst = { 0 };
	unsigned long i;

	/* Read inserted fields list */
	*offset = -1;
	if(ctrl_eval_fieldexpr(cntxt, ctrl, &fldlst, dyntab_val(&tbl->field, ifld, 0))) CLEAR_ERROR;
	if(qry_add_table_col(cntxt, &fldlst, add_sz_str("_EVA_LABEL")) ||
		qry_add_table_col(cntxt, &fldlst, add_sz_str("_EVA_FIELD")) ||
		qry_add_table_col(cntxt, &fldlst, add_sz_str("_EVA_TYPE"))) STACK_ERROR;
	for(i = 0; i < fldlst.nbrows; i++)
		if(!dyntab_sz(&fldlst, i, 1) || !dyntab_sz(&fldlst, i, 1))
			dyntab_del_rows(&fldlst, i--, 1);

	/* Insert rows in table data */
	for(i = 0; i < 15; i++)
	{
		DynTable *tab = &tbl->label + i;
		if(dyntab_ins_rows(tab, ifld + 1, fldlst.nbrows)) STACK_ERROR;
	}

	/* Insert fields in table */
	for(i = 0; i < fldlst.nbrows; i++)
	{
		DYNTAB_ADD_CELL(&tbl->label, ifld + i + 1, 0, &fldlst, i, 1);
		DYNTAB_ADD_CELL(&tbl->field, ifld + i + 1, 0, &fldlst, i, 2);
		DYNTAB_ADD_CELL(&tbl->lblstyle, ifld + i + 1, 0, &tbl->lblstyle, ifld, 0);
		DYNTAB_ADD_CELL(&tbl->celstyle, ifld + i + 1, 0, &tbl->celstyle, ifld, 0);
		DYNTAB_ADD_CELL(&tbl->fontsize, ifld + i + 1, 0, &tbl->fontsize, ifld, 0);
		DYNTAB_ADD_CELL(&tbl->align, ifld + i + 1, 0, &tbl->align, ifld, 0);
		DYNTAB_ADD_CELL(&tbl->bold, ifld + i + 1, 0, &tbl->bold, ifld, 0);
	}

	/* Delete original row */
	for(i = 0; i < 15; i++)
	{
		DynTable *tab = &tbl->label + i;
		dyntab_del_rows(tab, ifld, 1);
	}

	*offset = fldlst.nbrows - 1;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_read_control
** Description : read column definition from a control for list & relations tables
*********************************************************************/
#define ERR_FUNCTION "table_read_control"
#define ERR_CLEANUP DYNTAB_FREE(attr); \
					DYNTAB_FREE(attr1); \
					DYNTAB_FREE(idobj); \
					M_FREE(buf)
int table_read_control(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long id,					/* in : control Id */
	ObjTableFormat *tbl,				/* in/out : table format data */
	unsigned long i, unsigned long j	/* in : destination cell in tbl attributes */
){
	DynTable attr = { 0 };
	DynTable attr1 = { 0 };
	DynTable idobj = { 0 };
	DynBuffer *buf = NULL;
	char *typ, *ctl, *fmt, *lblpos, *hmult, *meth;
	DynTableCell *strip;
	if(!id) RETURN_OK;

	/* Read control definition */
	if(qry_cache_idobj(&attr, id) || ctrl_read_baseobj(cntxt, &attr)) STACK_ERROR;
	ctl = DYNTAB_FIELD_VAL(&attr, CONTROL);

	/* Handle column format */
	typ = DYNTAB_FIELD_VAL(&attr, TYPE);
	fmt = strcmp(ctl, "_EVA_INPUT") ? DYNTAB_FIELD_VAL(&attr, DISPLAYFORMAT) : DYNTAB_FIELD_VAL(&attr, FORMAT);
	if(!*fmt && !strcmp(ctl, "_EVA_INPUT"))
		fmt = (!strcmp(typ, "_EVA_DATE") || !strcmp(typ, "_EVA_NUMBER") || !strcmp(typ, "_EVA_EMAIL") ||
				!strcmp(typ, "_EVA_RELATION") || !strcmp(typ, "_EVA_TELEPHONE")) ? typ :
				!strcmp(typ, "_EVA_FILEUPLOAD") ? "_EVA_DOCUMENT" : "";

	/*  Handle other column attributes */
	lblpos = DYNTAB_FIELD_VAL(&attr, LABELPOS);
	if(strcmp(lblpos, "_EVA_NONE")) DYNTAB_ADD_CELLP(&tbl->label, i, j, DYNTAB_FIELD_CELL(&attr, LABEL));
	DYNTAB_ADD_CELLP(&tbl->align, i, j, DYNTAB_FIELD_CELL(&attr, ALIGN));
	DYNTAB_ADD_CELLP(&tbl->bold, i, j, DYNTAB_FIELD_CELL(&attr, BOLD));
	DYNTAB_ADD_CELLP(&tbl->fontsize, i, j, DYNTAB_FIELD_CELL(&attr, FONTSIZE));
	DYNTAB_ADD_CELLP(&tbl->underline, i, j, DYNTAB_FIELD_CELL(&attr, UNDERLINE));
	DYNTAB_ADD_CELLP(&tbl->notes, i, j, DYNTAB_FIELD_CELL(&attr, NOTES));
	DYNTAB_ADD_CELLP(&tbl->celstyle, i, j, DYNTAB_FIELD_CELL(&attr, CELL_STYLE));
	DYNTAB_ADD_CELLP(&tbl->lblstyle, i, j, DYNTAB_FIELD_CELL(&attr, LABEL_STYLE));
	DYNTAB_ADD_CELLP(&tbl->lblunit, i, j, DYNTAB_FIELD_CELL(&attr, LABELUNIT));
	meth = DYNTAB_FIELD_VAL(&attr, EVAL_METHOD);
	if(!strcmp(meth, "_EVA_VALUES")) DYNTAB_ADD_STR(&tbl->notable, i, j, "1");
	DYNTAB_ADD_CELLP(&tbl->debug, i, j, DYNTAB_FIELD_CELL(&attr, DEBUG_SQL_EXPR));
	hmult = DYNTAB_FIELD_VAL(&attr, MULTIPLE_HANDLER);
	if(!strcmp(hmult, "_EVA_DISTINCT") || !strcmp(hmult, "_EVA_SPACE")) DYNTAB_ADD_STR(&tbl->distinctval, i, j, "1");
	if(!strcmp(hmult, "_EVA_LINE")) DYNTAB_ADD_STR(&tbl->align, i, j, "newline")
	else if(!strcmp(hmult, "_EVA_SPACE")) DYNTAB_ADD_STR(&tbl->align, i, j, "space")
	else if(!strcmp(hmult, "_EVA_COMMA")) DYNTAB_ADD_STR(&tbl->align, i, j, "index");

	/* Handle stripping */
	strip = DYNTAB_FIELD_CELL(&attr, DISPLAY_STRIP);
	if(strip)
	{
		DYNTAB_ADD_CELLP(&tbl->strip, i, j, strip);
	}
	else if(*DYNTAB_FIELD_VAL(&attr, NOBR) == '1' || !strcmp(fmt, "_EVA_NUMBER") || !strcmp(fmt, "_EVA_MONEY"))
		DYNTAB_ADD_STR(&tbl->strip, i, j, "0");

	/* Handle control type for field definition */
	if(!strcmp(typ, "_EVA_CTRL_VAL"))
	{
		/* Calc-ed controls values : use fields names */
		unsigned long k;
		DYNTAB_FIELD(&idobj, &attr, CTRLTREE);
		for(k = 0; k < idobj.nbrows; k++)
		{
			if(qry_cache_idobj(&attr1, DYNTAB_TOULRC(&idobj, k, 0)) || ctrl_read_baseobj(cntxt, &attr1)) STACK_ERROR;
			if(buf) DYNBUF_ADD_STR(&buf, ",");
			DYNBUF_ADD_CELLP(&buf, DYNTAB_FIELD_CELL(&attr1, FIELD), NO_CONV);
			if(!k && !*fmt)
			{
				typ = DYNTAB_FIELD_VAL(&attr1, TYPE);
				fmt = DYNTAB_FIELD_VAL(&attr1, FORMAT);
				if(!*fmt)
					fmt = (!strcmp(typ, "_EVA_DATE") || !strcmp(typ, "_EVA_NUMBER") || !strcmp(typ, "_EVA_RELATION") || !strcmp(typ, "_EVA_EMAIL")) ? typ :
							!strcmp(typ, "_EVA_FILEUPLOAD") ? "_EVA_DOCUMENT" : "";
			}
		}
		DYNTAB_ADD_BUF(&tbl->field, i, j, buf);
	}
	else if(!strcmp(ctl, "_EVA_BUTTON"))
	{
		/* Button : use object itself */
		DYNTAB_ADD_CELLP(&tbl->field, i, j, dyntab_field_cell(&attr, "_EVA_CONTROL", ~0UL, 0));
	}
	else if(!strcmp(ctl, "_EVA_INPUT"))
	{
		/* Input : use field name */
		DYNTAB_ADD_CELLP(&tbl->field, i, j, dyntab_field_cell(&attr, "_EVA_FIELD", ~0UL, 0));
	}
	else
	{
		/* Calc expression : evaluate variables */
		if(qry_eval_sqlexpr_var(cntxt, &buf, DYNTAB_FIELD_VAL(&attr, SRCFIELD), &attr, NULL)) CLEAR_ERROR_RETURN;
		DYNTAB_ADD_BUF(&tbl->field, i, j, buf);
	}

	/* Store column format */
	DYNTAB_ADD(&tbl->format, i, j, fmt, 0, NO_CONV);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_read_controls
** Description : read & intialize data for list & relations tables
*********************************************************************/
#define ERR_FUNCTION "table_read_controls"
#define ERR_CLEANUP DYNTAB_FREE(cgival); \
					DYNTAB_FREE(listctrl)
int table_read_controls(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	DynTable *data						/* in : table format data (from a list control) */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = { 0 };
	DynTable listctrl = { 0 };
	char *tablesearch;
	ObjTableFormat *tbl = NULL;
	unsigned long i, j;
	int offset = 0, b_sts = 0;
	char *coldef;
	char *insert = CTRL_ATTR_VAL(TABLEINSERT);

	/* Alloc && initialize table data */
	M_ALLOC(ctrl->objtbl, sizeof(ctrl->objtbl[0]));
	tbl = ctrl->objtbl;

	/* Handle basic case : base form & no fields */
	if(CTRL_ATTR_CELL(BASE_FORMS) && !CTRL_ATTR_CELL(COLDEF) && !CTRL_ATTR_CELL(DISPLAYFIELDS) &&
		(CTRL_ATTR_CELL(INPUTMODE) || strcmp(ctrl->CONTROL, "_EVA_INPUT")) && !data)
	{
		data = &tbl->tattr;
		if(dyntab_from_tab(data, 0, 0, &ctrl->attr, 0, 0, ctrl->attr.nbrows, ctrl->attr.nbcols, 19)) RETURN_ERR_MEMORY;
		if(qry_cache_idobj(&listctrl, strtoul(CTRL_ATTR_VAL(BASE_FORMS), NULL, 10)) ||
			ctrl_read_baseobj(cntxt, &listctrl) ||
			qry_complete_data(cntxt, data, &listctrl, NULL, NULL)) STACK_ERROR;
	}

	/* Handle data from control attributes (pointer may change) */
	if(!data)
	{
		data = &ctrl->attr;
		tbl->b_ctrl = 1;
	}
	tbl->attr = data;
	coldef = DYNTAB_FIELD_VAL(data, COLDEF);

	/* Handle columns definition method */
	if(!strcmp(coldef, "_EVA_CTRL_LIST"))
	{
		/* Controls list : read columns definition from given controls */
		DYNTAB_FIELD(&listctrl, tbl->attr, CTRLTREE);
		for(i = 0; i < listctrl.nbrows; i++)
			if(table_read_control(cntxt, DYNTAB_TOULRC(&listctrl, i, 0), tbl, i, 0)) STACK_ERROR;
	}
	else if(!strcmp(coldef, "_EVA_CTRL_TABLE"))
	{
		/* Read columns data from given controls */
		DYNTAB_FIELD_TAB(&listctrl, tbl->attr, CTRLTREE);
		for(i = 0; i < listctrl.nbrows; i++)
			for(j = 0; j < listctrl.nbcols; j++)
				if(table_read_control(cntxt, DYNTAB_TOULRC(&listctrl, i, j), tbl, i, j)) STACK_ERROR;
	}
	else
	{
		/* Expression list : read columns data & format from current control */
		DYNTAB_FIELD_TAB(&tbl->field, tbl->attr, DISPLAYFIELDS);
		DYNTAB_FIELD_TAB(&tbl->format, tbl->attr, DISPLAYFORMAT);
		DYNTAB_FIELD_TAB(&tbl->label, tbl->attr, DISPLAYLABELS);
		DYNTAB_FIELD_TAB(&tbl->align, tbl->attr, DISPLAYALIGN);
		DYNTAB_FIELD_TAB(&tbl->bold, tbl->attr, DISPLAYBOLD);
		DYNTAB_FIELD_TAB(&tbl->fontsize, tbl->attr, DISPLAYFONTSIZE);
		DYNTAB_FIELD_TAB(&tbl->underline, tbl->attr, DISPLAY_UNDERLINE);
		DYNTAB_FIELD_TAB(&tbl->strip, tbl->attr, DISPLAY_STRIP);
		DYNTAB_FIELD_TAB(&tbl->notes, tbl->attr, DISPLAYNOTES);
		DYNTAB_FIELD_TAB(&tbl->distinctval, tbl->attr, DISTINCT_VAL);
		DYNTAB_FIELD_TAB(&tbl->debug, tbl->attr, DEBUG_SQL_EXPR);
		DYNTAB_FIELD_TAB(&tbl->lblstyle, tbl->attr, DISPLAY_LBLSTYLE);
		DYNTAB_FIELD_TAB(&tbl->celstyle, tbl->attr, DISPLAY_CELLSTYLE);
		DYNTAB_FIELD_TAB(&tbl->notable, tbl->attr, NO_TMPTABLE);
	}
	tbl->minsearchlen = strtoul(DYNTAB_FIELD_VAL(tbl->attr, MINSEARCHLEN), NULL, 10);
	tbl->colbrk = strtoul(DYNTAB_FIELD_VAL(tbl->attr, COLUMN_LIMIT), NULL, 10);
	if(tbl->colbrk > tbl->field.nbrows) tbl->colbrk = 0;
	if(qry_eval_var_expr_table(cntxt, &tbl->field, tbl->attr)) CLEAR_ERROR;
	
	/* Read extra filters definition */
	DYNTAB_FIELD_TAB(&tbl->altfilter, tbl->attr, ALTFILTER);
	DYNTAB_FIELD_TAB(&tbl->condfilter, tbl->attr, ALTFILTER_FILTER);
	DYNTAB_FIELD_TAB(&tbl->typfilter, tbl->attr, ALTFILTER_TYPE);
	DYNTAB_FIELD_TAB(&tbl->groupfilter, tbl->attr, ALTFILTER_GROUP);

	/* Handle field indirection */
	for(i = 0; i < tbl->field.nbrows; i++)
		if(!strcmp(dyntab_val(&tbl->format, i, 0), "_EVA_FIELDLIST"))
		{
			if(table_insert_fieldlist(cntxt, i_ctrl, i, &offset)) STACK_ERROR;
			i += offset;
		}

	/* Retrieve list tools status */
	if(cgi_get_subfield(cntxt, &tbl->selfilter, ctrl, "SELFILTER", 1, 0, 0)) STACK_ERROR;
	if(dyntab_order_lines(&tbl->cgiinput)) RETURN_ERR_MEMORY;
	CTRL_GET_CGI_SUBFIELD1("STATUS");
	b_sts = cgival.nbrows != 0;
	if(b_sts)
		sscanf(dyntab_val(&cgival, 0, 0), "%lx,%lu,%lu,%lu,%lu,%lu,%lu,%u", 
			&tbl->status, &tbl->line, &tbl->lines, &tbl->sortcol, &tbl->b_sortdesc, &tbl->srchcol, &tbl->ctrlline, &tbl->srchmode);

	/* Handle search function */
	tablesearch = DYNTAB_FIELD_VAL(data, TABLESEARCH);
	if(!strcmp(tablesearch, "_EVA_EXTCTRL"))
	{
		if(cgi_filter_values(cntxt, &tbl->cgiinput, 'D', ~0UL, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), "_EVA_SEARCHINPUT", "", 0, 0)) STACK_ERROR;
		dyntab_sort(&tbl->cgiinput, qsort_col0desc);
		tbl->status |= TblCtrl_extsearch | TblCtrl_opensearch;
		if(cgi_filter_values(cntxt, &cgival, 'D', ~0UL, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), "_EVA_SEARCHMODE", "", 0, 0)) STACK_ERROR;
		dyntab_group(&cgival, "MAX");
		if(cgival.nbrows && cgival.cell->txt)
		{
			tbl->srchmode =  !strcmp(cgival.cell->txt, "_EVA_BEGIN") ? Begin :
							!strcmp(cgival.cell->txt, "_EVA_EXACT") ? Exact : Contain;
		}
	}
	else
		if(cgi_get_subfield(cntxt, &tbl->cgiinput, ctrl, "INPUT", 1, 0, 0)) STACK_ERROR;
	
	/* Enable functions depending on control attributes */
	tbl->inputwidth = ctrl->COLUMNS ? ctrl->COLUMNS : 15;
	tbl->status &= ~(TblCtrl_addnew|TblCtrl_openbtn|TblCtrl_search|TblCtrl_sel);
	if(*insert && strcmp(insert, "0") && (DYNTAB_FIELD_CELL(tbl->attr, LIST_CREATE_FORM) || DYNTAB_FIELD_CELL(tbl->attr, BASE_FORMS)))
		tbl->status |= TblCtrl_addnew;
	if(strcmp(DYNTAB_FIELD_VAL(tbl->attr, OPENBUTTON), "_EVA_NONE"))
		tbl->status |= TblCtrl_openbtn;
	if(*tablesearch ? strcmp(tablesearch, "_EVA_NONE") : strcmp(CTRL_ATTR_VAL(PAGE_CTRL), "0")) 
		tbl->status |= TblCtrl_search;
	if(!strcmp(ctrl->CONTROL, "_EVA_INPUT"))
	{
		tbl->status |= TblCtrl_sel;
		if(!strcmp(tablesearch, "_EVA_NONE") && !strcmp(ctrl->TYPE, "_EVA_RELATION")) tbl->status |= TblCtrl_opensel;
	}
	tbl->b_sortsel = CTRL_ATTR_CELL(MULTIPLE_AUTO_ORDER) != 0;
	if(*CTRL_ATTR_VAL(NOTABLESORT) != '1') tbl->status |= TblCtrl_sort;

	/* Initialize status if first call */
	if(!b_sts)
	{
		DynTableCell *sortcol = DYNTAB_FIELD_CELL(tbl->attr, SORTCOL);
		DynTableCell *ae = DYNTAB_FIELD_CELL(tbl->attr, ALLOWEMPTY);
		char *initsearch = DYNTAB_FIELD_VAL(tbl->attr, INIT_SEARCH_MODE);
		tbl->b_first = 1;
		tbl->srchmode = !strcmp(initsearch, "_EVA_BEGIN") ? Begin :
						!strcmp(initsearch, "_EVA_EXACT") ? Exact : Contain;
		if(sortcol)
		{
			if(sortcol->txt) tbl->sortcol = atoi(sortcol->txt);
			if(tbl->sortcol) tbl->sortcol--;
			else tbl->sortcol = ~0UL;
		}
		if(!strcmp("_EVA_SEARCHONLOAD", tablesearch) ||
			!strcmp("_EVA_SEARCHONLOAD_NEW", tablesearch) && form->b_newobj ||
			!strcmp("_EVA_SEARCHONLOAD_EMPTY", tablesearch) && !ctrl->val.nbrows)
			tbl->status |= TblCtrl_opensearch;
		if(tbl->status & TblCtrl_sel && strcmp(ctrl->TYPE, "_EVA_FILEUPLOAD") && (
				tbl->status & TblCtrl_opensearch || ae && strcmp(ae->txt, "Yes") && strcmp(ae->txt, "_EVA_CONDITION") && !ctrl->val.nbrows))
		{
			tbl->status |= TblCtrl_opensel;
			if(tbl->status & TblCtrl_search) tbl->status |= TblCtrl_opensearch;
		}
		tbl->lines = strtoul(DYNTAB_FIELD_VAL(tbl->attr, LINES), NULL, 10);
		if(!tbl->lines) tbl->lines = ctrl->LINES ? ctrl->LINES : 5;
		tbl->b_sortdesc = atoi(DYNTAB_FIELD_VAL(tbl->attr, SORTDESC));
	}
	tbl->srchfmt = "";

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_read_obj_list
** Description : read table list of of objects
*********************************************************************/
#define ERR_FUNCTION "table_read_obj_list"
#define ERR_CLEANUP
int table_read_obj_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int b_selobj						/* in : use control values if not 0 */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	unsigned long i;

	if(!tbl->idobj.nbrows && tbl->totlines)
	{
		if(b_selobj)
		{
			/* Copy selected objects in display list */
			for(i = 0; i < tbl->lines && i + tbl->line < ctrl->val.nbrows; i++) 
				DYNTAB_SET_CELL(&tbl->idobj, i, 0, &ctrl->val, i + tbl->line, 0);
		}
		else
		{
			/* Default sort - build query for displayed objects in IdObj (creation) order */
			char qry[256] = {0};
			snprintf(qry, sizeof(qry)-1, 
				"SELECT DISTINCT IdObj FROM IdList ORDER BY IdObj %s LIMIT %lu,%lu",
				tbl->b_sortdesc ? "DESC" : "", tbl->line, tbl->lines);
			if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &tbl->idobj, 2)) CLEAR_ERROR;
		}
	}

	/* Drop temporary tables */
	if(sql_exec_query(cntxt, "DROP TABLE IF EXISTS IdList,ValList,IdListMatch")) CLEAR_ERROR;

	/* Read displayed objects data */
	if(qry_list_data(cntxt, tbl)) CLEAR_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_prepare_rows
** Description : prepare cell values for a table of objects
*********************************************************************/
#define ERR_FUNCTION "table_prepare_rows"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					M_FREE(outval); \
					M_FREE(val)
int table_prepare_rows(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char *pfx							/* in : values prefix */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynTable data = {0};
	DynBuffer *outval = NULL;
	DynBuffer *val = NULL;
	unsigned long i, j, k;
	unsigned long rowstart;
	DynTableCell *cell;
	size_t pfx_sz = pfx ? strlen(pfx) : 0;

	/* Prepare title line */
	DYNTAB_SET(&tbl->cellval, 0, 0, "IdObj");
	for(j = 0; j < tbl->field.nbrows; j++) 
	{
		DYNTAB_SET_CELL(&tbl->cellval, 0, j + 1, &tbl->label, j, 0);
	}

	/* Process rows */
	for(i = 0; i < tbl->idobj.nbrows; i++)
	{
		char *idend;
		unsigned long idobj = strtoul(dyntab_val(&tbl->idobj, i, 0), &idend, 10);

		/* First colummn contains object Id */
		DYNTAB_ADD_CELL(&tbl->cellval, i + 1, 0, &tbl->idobj, i, 0);

		/* If object is not valid */
		if(!idobj || *idend)
		{
			DYNTAB_SET(&tbl->cellval, i + 1, 1, "#NAR#");
		}
		else
		{
			/* Find object in objects data */
			for(j = 0; j < tbl->data.nbrows && dyntab_cell(&tbl->data, j, 0)->IdObj != idobj; j++);
			rowstart = j;

			/* Output line data in tbl->field order */
			for(j = 0; j < tbl->field.nbrows; j++) 
			{
				char *fmt = dyntab_val(&tbl->format, j, 0);
				char *separ = ", ";
				int b_index = 0;
				unsigned long line = 1;
				unsigned long i0;
				int b_distinct = dyntab_sz(&tbl->distinctval, j, 0);
				char *align = dyntab_val(&tbl->align, j, 0);

				/* Set alignment & value separator */
				if(!strcmp(align, "newline")) { align = ""; separ = "\n"; }
				else if(!strcmp(align, "space")) { align = ""; separ = " "; }
				else if(!strcmp(align, "index")) { align = ""; b_index = 1; }

				/* Concat formated cell values */
				M_FREE(outval);
				if(pfx_sz) DYNBUF_ADD(&outval, pfx, pfx_sz, NO_CONV);
				for(k = 0; rowstart < tbl->data.nbrows; k++, rowstart++)
				{
					/* Break if column changes */
					int b_found = 0;
					unsigned long idrelobj;
					char *idend = NULL;
					cell = dyntab_cell(&tbl->data, rowstart, 0);
					if(cell->col != j) break;

					/* Distinct values required : search for same previous value - ignore if found */
					if(b_distinct) for(i0 = 0; i0 < k && !b_found; i0++)
						if(!dyntab_cmp(&tbl->data, rowstart, 0, &tbl->data, rowstart - i0 - 1, 0))
							b_found = 1;
					if(b_found) continue;

					/* Output value separator if applicable */
					if(b_index || cell->Line && !dyntab_sz(&tbl->align, j, 0))
					{
						if(line >= cell->Line && k) DYNBUF_ADD(&outval, separ, 0, NO_CONV)
						else while(line < cell->Line) { DYNBUF_ADD_STR(&outval, "\n"); line++; }
					}
					else if(outval && outval->cnt > pfx_sz) DYNBUF_ADD(&outval, separ, 0, NO_CONV);

					/* Use relation format if applicable */
					idrelobj = cell->txt ? strtoul(cell->txt, &idend, 10) : 0;
					if(idend && *idend) idrelobj = 0;
					if(idrelobj && (!strncmp(fmt, add_sz_str("_EVA_RELATION")) || cell->b_relation && !*fmt))
					{
						/* Relation formats */
						if(qry_obj_idfield(cntxt, &data, idrelobj, 0) ||
							qry_obj_label(cntxt, NULL, NULL, NULL, &val, NULL, NULL, NULL, NULL, 0, &data, 0)) STACK_ERROR;
						DYNBUF_ADD_BUF(&outval, val, NO_CONV);
					}

					/* Add formated value */
					else if(put_value_fmt(cntxt, &outval, DYNTAB_VAL_SZ(&tbl->data, rowstart, 0), dyntab_val(&tbl->format, j, 0)))
						STACK_ERROR;
				}
				if(outval && outval->cnt > pfx_sz) DYNTAB_ADD_BUF(&tbl->cellval, i + 1, j + 1, outval);
			}
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
