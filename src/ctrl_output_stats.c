/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_output_stats.c
** Description : handling functions for output controls of type LIST/STATS (tables of statistics)
**      Author : Alain BOUTE
**     Created : April 7 2004
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : ctrl_calc_output_fieldval_tooltip
** Description : output field value of a pivot table field displayed in values tooltip
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_fieldval_tooltip"
#define ERR_CLEANUP	M_FREE(tmp)
int ctrl_calc_output_fieldval_tooltip(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	PivotTableField *pvf,			/* in : pivot table field */
	unsigned long i					/* in : value index	in pvf->labels */
){
	DynBuffer *tmp = NULL;
	DynBuffer **html = cntxt->form->html;

	DYNBUF_ADD_STR(html, "[");
	DYNBUF_ADD(html, pvf->label, 0, HTML_TOOLTIP);
	DYNBUF_ADD_STR(html, "=");
	if(dyntab_sz(&pvf->labels, i, 0))
	{
		if(put_value_fmt(cntxt, &tmp, DYNTAB_VAL_SZ(&pvf->labels, i, 0), pvf->dispfmt)) STACK_ERROR;
		DYNBUF_ADD_BUF(html, tmp, HTML_TOOLTIP);
	}
	else
		DYNBUF_ADD_CELL(html, &pvf->emptyval, 0, 0, HTML_TOOLTIP);
	DYNBUF_ADD_STR(html, "]");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_pvt_val
** Description : output a cell value in a pivot table
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_pvt_val"
#define ERR_CLEANUP	M_FREE(name)
int ctrl_calc_output_pvt_val(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv,					/* in : pivot table data */
	unsigned long i,				/* in : cell index in pv->row->labels */
	unsigned long j					/* in : cell index in pv->col->labels */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *name = NULL;
	DynTableCell *c = dyntab_cell(pv->res, i, j);
	DynBuffer **html = form->html;
	int b_vallinks = pv->clicmode[0] &&	c && c->IdObj > 0 && !pv->exportparams;
	int b_row = i < ROWCNT;
	int b_col = j < COLCNT;
	char *fmt = pv->data ? pv->data->dispfmt : "";
	int always_list = DYNTAB_FIELD_CELL(pv->srcdata, PVTLIST_ALWAYS) != NULL;

	DYNBUF_ADD_STR(html, "&nbsp;");
	if(b_vallinks) 
	{

		/* Prepare title for link to objects list */
		DYNBUF_ADD3_INT(html, "<a title='", c->IdObj, " fiche");
		if(c->IdObj > 1) DYNBUF_ADD_STR(html, "s");
		DYNBUF_ADD_STR(html, " ");
		if(b_row && ctrl_calc_output_fieldval_tooltip(cntxt, pv->row, i)) STACK_ERROR;
		if(b_row && b_col) DYNBUF_ADD_STR(html, " ¤ ");
		if(b_col && ctrl_calc_output_fieldval_tooltip(cntxt, pv->col, j)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "\n");
		if(c->IdObj == 1&& !always_list)
		{
			DYNBUF_ADD_STR(html, "Cliquez pour ouvrir la fiche");
		}
		else
			DYNBUF_ADD_STR(html, "Cliquez pour voir la liste des fiches");
		DYNBUF_ADD_STR(html, "'");
		if(put_showhelp(cntxt, html)) STACK_ERROR;

		/* Prepare button name for link to objects list */
		if(c->IdObj > 1 || always_list)
		{
			char rc[64];
			if(ctrl_cgi_name(cntxt, ctrl, NULL, 1, &name, 'B',
					rc, sprintf(rc, "PVTLIST=%lu,%lu,%lu,%lu,%lu,%lu",
									pv->srcdata->cell->IdObj, 
									pv->data ? pv->data - pv->datas : 0,
									pv->row ? pv->row - pv->rows : 0,
									pv->col ? pv->col - pv->cols : 0,
									i, j)))
				STACK_ERROR;
		}
		else
			DYNBUF_ADD3_INT(&name, "B$#.OPENOBJ=", c->IdValObj, "");
		DYNBUF_ADD3_BUF(html, " href='javascript:cb(\"", name, NO_CONV, "\");'>");
	}

	/* Output cell value & footer */
	if(!strcmp(fmt, "_EVA_FILESIZE"))
	{
		DYNBUF_ADD(html, human_filesize(strtoul(dyntab_val(pv->res, i, j), NULL, 10)), 0, TO_HTML);
	}
	else if(pv->exportparams ||
			pv->data && !strcmp(dyntab_val(&pv->data->percent, 0, 0), "_EVA_SINGLE"))
	{
		DYNBUF_ADD_CELL(html, pv->res, i, j, TO_HTML);
	}
	else 
		DYNBUF_ADD(html, number_space_thousands(dyntab_val(pv->res, i, j), pv->data ? pv->data->decimals : 0), 0, TO_HTML);
	if(b_vallinks) DYNBUF_ADD_STR(html, "</a>");
	DYNBUF_ADD_STR(html, "&nbsp;");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_title
** Description : output title of a pivot table
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_title"
#define ERR_CLEANUP M_FREE(name)	
int ctrl_calc_output_title(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv					/* in : pivot table data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer **html = form->html;
	DynBuffer *name = NULL;
	if(pv->srcdata->nbrows && !pv->exportparams)
	{
		char tmp[32];
		sprintf(tmp, "EXPORTPVT=%lu", pv->srcdata->cell->IdObj);
		if(ctrl_cgi_name(cntxt, ctrl, NULL, 1, &name, 'B', tmp, strlen(tmp))) STACK_ERROR;
		DYNBUF_ADD_STR(html, "<table width=100%><tr><td class=NoBorder>");
		if(ctrl_add_symbol_btn(cntxt, ctrl, NULL, pv->srcdata, 0, NULL, "LABEL+OBJNOTES"))
			STACK_ERROR;
		DYNBUF_ADD_STR(html, "</td><td align=right class=NoBorder>");
		if(!pv->rowcolclic && put_html_button(cntxt, name->data, "Exporter", "_eva_excel_icon_small.gif", "_eva_excel_icon_small_s.gif", 
			"Cliquez pour obtenir le tableau de statistiques au format Excel", 0, 0)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</td></tr></table>");
	}
	else 
		DYNBUF_ADD(html, pv->label, 0, TO_HTML);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_fieldval_objlist
** Description : output field value of a pivot table field displayed as objects list
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_fieldval_objlist"
#define ERR_CLEANUP	M_FREE(tmp)
int ctrl_calc_output_fieldval_objlist(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	PivotTableField *pvf,			/* in : pivot table field */
	unsigned long i					/* in : value index	in pvf->labels */
){
	DynBuffer *tmp = NULL;
	DynBuffer **html = cntxt->form->html;

	DYNBUF_ADD_STR(html, "<u>");
	DYNBUF_ADD(html, pvf->label, 0, TO_HTML);
	DYNBUF_ADD_STR(html, "=<b>");
	if(dyntab_sz(&pvf->labels, i, 0))
	{
		if(put_value_fmt(cntxt, &tmp, DYNTAB_VAL_SZ(&pvf->labels, i, 0), pvf->dispfmt)) STACK_ERROR;
		DYNBUF_ADD_BUF(html, tmp, TO_HTML);
	}
	else
		DYNBUF_ADD_CELL(html, &pvf->emptyval, 0, 0, TO_HTML);
	DYNBUF_ADD_STR(html, "</b></u>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_prepare_objlist
** Description : prepare list definition to display pivot table objects
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_prepare_objlist"
#define ERR_CLEANUP	M_FREE(expr)
int ctrl_calc_prepare_objlist(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynTable *disptbldata,				/* in/out : table definition data */
	unsigned long *col,					/* in/out : column index to add */
	PivotTableField *pvf				/* in : pivot table field */
){
	DynBuffer *expr = NULL;
	if(!pvf || !dyntab_sz(&pvf->expr, 0, 0)) RETURN_OK;

	/* Handle relation to base table */
	/* TODO use FROM clause if relation to SQL expression */
	switch(pvf->relmode) {
	case RelReverse:
		DYNBUF_ADD_STR(&expr, "<-");
	case RelDirect:
		DYNBUF_ADD_CELL(&expr, pvf->relfields, 0, 0, NO_CONV);
		DYNBUF_ADD_STR(&expr, "->");
	}

	/* Add column for field in display table definition - TODO use FROM clause if relation to SQL expression */
	if(qry_eval_sqlexpr_var(cntxt, &expr, dyntab_val(&pvf->expr, 0, 0), &pvf->srcdata, NULL)) CLEAR_ERROR;
	DYNTAB_ADD_FIELD_NL(disptbldata, disptbldata->nbrows, 0, 
		(pvf->relmode && strchr(dyntab_val(&pvf->expr, 0, 0), '[')) ?
			"***" : expr->data, 0, "_EVA_DISPLAYFIELDS", 0, 1, 1, *col);
	DYNTAB_ADD_FIELD_NL(disptbldata, disptbldata->nbrows, 0, pvf->label, 0, "_EVA_DISPLAYLABELS", 0, 1, 1, *col);
	if(pvf->dispfmt && pvf->dispfmt[0])
		DYNTAB_ADD_FIELD_NL(disptbldata, disptbldata->nbrows, 0, pvf->dispfmt, 0, "_EVA_DISPLAYFORMAT", 0, 1, 1, *col);

	(*col)++;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_pvtcell_objlist
** Description : output list of object for of a pivot table cell
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_pvtcell_objlist"
#define ERR_CLEANUP	DYNTAB_FREE(disptbldata); \
					M_FREE(sql); \
					M_FREE(name); \
					sql_drop_table(cntxt, "IdList")
int ctrl_calc_output_pvtcell_objlist(	/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	PivotTable *pv						/* in : pivot table data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable disptbldata = {0};
	DynBuffer *sql = NULL;
	DynBuffer *name = NULL;
	DynBuffer **html = form->html;
	unsigned long i, irow = 0, icol = 0, idata = 0, clrow = 0, clcol = 0;

	/* Build clicked pivot table */
	sscanf(pv->rowcolclic, "%lu,%lu,%lu,%lu,%lu,%lu", &i, &idata, &irow, &icol, &clrow, &clcol);
	pv->row = pv->nbrow ? pv->rows + irow : NULL;
	pv->col = pv->nbcol ? pv->cols + icol : NULL;
	pv->data = pv->nbdata ? pv->datas + idata : NULL;
	pv->res = &pv->restbl[idata][irow][icol];
	if(pivottable_select(cntxt, pv)) STACK_ERROR;
	if(pivottable_calc_single(cntxt, pv)) STACK_ERROR;

	/* Build clicked value objects list */
	DYNBUF_ADD_STR(&sql,
		"-- ctrl_calc_stats - Build clicked value objects list\n"
		"CREATE TEMPORARY TABLE IdList TYPE=HEAP\n");
	if(pv->basetable) 
	{
		DYNBUF_ADD3(&sql,
			"SELECT DISTINCT ", pv->basetable, 0, NO_CONV, ".IdObj ");
		DYNBUF_ADD3(&sql, "FROM ", pv->basetable, 0, NO_CONV, "\n");
		DYNBUF_ADD_BUF(&sql, pv->join, NO_CONV);
		if(pv->row && clrow < ROWCNT)
		{
			if(pv->where) DYNBUF_ADD_STR(&pv->where, " AND ");
			DYNBUF_ADD3(&pv->where, "", pv->row->datatable, 0, NO_CONV, ".Val=");
			if(strcmp(pv->row->dispfmt, "_EVA_RELATION"))
				DYNBUF_ADD3_CELL(&pv->where, "'", &pv->row->labels, clrow, 0, SQL_NO_QUOTE, "'")
			else
				DYNBUF_ADD_INT(&pv->where, dyntab_cell(&pv->row->labels, clrow, 0)->IdObj);
		}
		if(pv->col && clcol < COLCNT)
		{
			if(pv->where) DYNBUF_ADD_STR(&pv->where, " AND ");
			DYNBUF_ADD3(&pv->where, "", pv->col->datatable, 0, NO_CONV, ".Val=");
			if(strcmp(pv->col->dispfmt, "_EVA_RELATION"))
				DYNBUF_ADD3_CELL(&pv->where, "'", &pv->col->labels, clcol, 0, SQL_NO_QUOTE, "'")
			else
				DYNBUF_ADD_INT(&pv->where, dyntab_cell(&pv->col->labels, clcol, 0)->IdObj);
		}
		if(pv->where) DYNBUF_ADD3_BUF(&sql, "WHERE ", pv->where, NO_CONV, "");
	}
	else
		DYNBUF_ADD_STR(&sql, "(IdObj INT)");
	if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;

	/* Output title & 'Back to table' button */
	DYNBUF_ADD_STR(html, "<table border=1 rules=rows width=100% cellspacing=0 bgcolor=#FFFFFF><tr><td>");
	if(ctrl_calc_output_title(cntxt, i_ctrl, pv)) STACK_ERROR;
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("PVTLISTOFF"));
	DYNBUF_ADD_STR(html, "</td><td align=right>");
	if(put_html_button(cntxt, name->data, NULL, "_eva_btn_gobacktable_fr.gif", "_eva_btn_gobacktable_fr_s.gif", 
		"Affiche le tableau de statistiques complet\n"
		"Vous pouvez aussi utiliser le bouton 'Page précédente' de votre navigateur (plus rapide)" , 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td></tr><tr>");

	/* Output row / column values */
	DYNBUF_ADD_STR(html, "<td colspan=2 align=center>");
	if(pv->row && clrow < ROWCNT && ctrl_calc_output_fieldval_objlist(cntxt, pv->row, clrow)) STACK_ERROR;
	if(pv->row && clrow < ROWCNT && pv->col && clcol < COLCNT) DYNBUF_ADD_STR(html, " --- ");
	if(pv->col && clcol < COLCNT && ctrl_calc_output_fieldval_objlist(cntxt, pv->col, clcol)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td></tr></table>");

	/* Display list of matched objects */
	if(!strcmp(pv->clicmode, "_EVA_AUTO_LIST"))
	{
		unsigned long col = 1;
		DYNTAB_ADD_FIELD_NL(&disptbldata, disptbldata.nbrows, 0, "_EVA_SYMBOL_LABEL", 0, "_EVA_OPENBUTTON", 0, 1, 1, 0);
		DYNTAB_ADD_FIELD_NL(&disptbldata, disptbldata.nbrows, 0, "_EVA_SEARCHONLOAD", 0, "_EVA_TABLESEARCH", 0, 1, 1, 0);
		DYNTAB_ADD_FIELD_NL(&disptbldata, disptbldata.nbrows, 0, "1", 0, "_EVA_EXPORTLIST", 0, 1, 1, 0);
		DYNTAB_ADD_FIELD_NL(&disptbldata, disptbldata.nbrows, 0, "1", 0, "_EVA_TABLE_NOTITLESHRINK", 0, 1, 1, 0);
		if(ctrl_calc_prepare_objlist(cntxt, &disptbldata, &col, pv->row) ||
			ctrl_calc_prepare_objlist(cntxt, &disptbldata, &col, pv->col) ||
			ctrl_calc_prepare_objlist(cntxt, &disptbldata, &col, pv->data)) STACK_ERROR;
	}
	else
		if(qry_obj_field(cntxt, &disptbldata, pv->objlist, NULL) || ctrl_read_baseobj(cntxt, &disptbldata)) STACK_ERROR;
	if(table_read_controls(cntxt, i_ctrl, disptbldata.nbrows ? &disptbldata : NULL) ) STACK_ERROR;
	ctrl->objtbl->from_idlist = 1;
	ctrl->objtbl->lines = 40;
	ctrl->LABEL = pv->label;
	if(table_process_controls(cntxt, i_ctrl)) STACK_ERROR;
	if(!cntxt->b_terminate && table_put_html_obj_list(cntxt, i_ctrl)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_field_label
** Description : output a pivot table field label
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_field_label"
#define ERR_CLEANUP	M_FREE(objnotes); \
					DYNTAB_FREE(objdata)
int ctrl_calc_output_field_label(	/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	PivotTable *pv,					/* in : pivot table data */
	PivotTableField *pvf,			/* in : pivot field parameters */
	unsigned long i					/* in : row index in field labels */
){
	DynBuffer *objnotes = NULL;
	DynBuffer **html = cntxt->form->html;
	DynTable objdata = {0};
	char printbuf[1024];
	DynTableCell *c = dyntab_cell(&pvf->labels, i, 0);
	unsigned long id_obj = c ? c->IdObj : 0;
	if(!c || !c->len) c = dyntab_cell(&pvf->emptyval, 0, 0);
	DYNBUF_ADD_STR(html, "<font size=-1>");
	if(pvf->dispfmt && !strcmp(pvf->dispfmt, "_EVA_RELATION") && id_obj && !pv->exportparams)
	{
		if(qry_obj_field(cntxt, &objdata, id_obj, NULL)) STACK_ERROR;
		/* TODO - use code from ctrl_add_symbol_btn to handle multiple formstamps */
		if(qry_obj_label(cntxt, &objnotes, NULL, &objnotes, NULL, &objnotes, NULL, NULL, NULL, 0, &objdata, 0)) STACK_ERROR;
		dynbuf_print3(html, "<a href='javascript:ol(%lu,0,%lu,%lu);'",
						dyntab_cell(&pvf->labels, i, 0)->IdObj,
						DYNTAB_TOUL(&cntxt->form->id_form),
						DYNTAB_TOUL(&cntxt->form->id_obj));
		DYNBUF_ADD3_BUF(html, " title='", objnotes, HTML_TOOLTIP, "'");
		if(put_showhelp(cntxt, html)) STACK_ERROR;
		DYNBUF_ADD3_CELL(html, ">", &pvf->labels, i, 0, TO_HTML, "</a>");
	}
	else if(c && c->txt)
	{
		char *fmt = pvf->dispfmt;
		DYNBUF_ADD_STR(html, "<b>");
		if(fmt && !strcmp(fmt, "_EVA_DATE"))
		{
			char *unit = pvf->dispunit ? pvf->dispunit : "";
			if(!strcmp(unit, "_EVA_DAY"))
				fmt = (i && !strncmp(c->txt, dyntab_val(&pvf->labels, i - 1, 0), 6)) ? "_EVA_DayLong" : "_EVA_LongDay";
			else if(!strcmp(unit, "_EVA_HOUR"))
				fmt = (i && !strncmp(c->txt, dyntab_val(&pvf->labels, i - 1, 0), 8)) ? "_EVA_Time" : "_EVA_LongTime";
			datetxt_to_format(cntxt, printbuf, c->txt, fmt);
			DYNBUF_ADD(html, printbuf, 0, TO_HTML);
		}
		else if(put_value_fmt(cntxt, html, c->txt, c->len, fmt)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</b>");
	}
	DYNBUF_ADD_STR(html, "</font></td>");
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_field_title
** Description : output a pivot table field title
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_field_title"
#define ERR_CLEANUP	DYNTAB_FREE(options); \
					DYNTAB_FREE(values); \
					M_FREE(name)
int ctrl_calc_output_field_title(	/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv,					/* in : pivot table data */
	PivotTableField *pvf			/* in : pivot field parameters */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer **html = form->html;
	DynBuffer *name = NULL;
	DynTable options = {0};
	DynTable values = {0};

	/* Output field label */
	if(!pvf || pvf == pv->data && (!pv->row && pv->nbdata > 1)) 
	{
		/* No field : empty label */
		DYNBUF_ADD_STR(html, "&nbsp;");
		RETURN_OK;
	}
	if(!DYNTAB_FIELD_CELL(&pvf->srcdata, MASK_ITEM))
	{
		if(pvf->srcdata.nbrows && !pv->exportparams)
		{
			/* Indicator / control : output label as open object link if not masked */
			if(ctrl_add_symbol_btn(cntxt, ctrl, NULL, &pvf->srcdata, 0, NULL, "LABEL+OBJNOTES"))
				STACK_ERROR;
		}
		else 
			/* No description data for indicator : output label */
			DYNBUF_ADD(html, pvf->label, 0, TO_HTML);
	}

	/* Output calendar control if applicable */
	if(pvf->b_calendar == 2 && !pv->exportparams)
	{
		char idindic[16] = {0};
		size_t cnt = ctrl->cginame->cnt;
		if(pvf && pvf->srcdata.cell) snprintf(add_sz_str(idindic), "%lu", pvf->srcdata.cell->IdObj);
		if(*idindic) DYNBUF_ADD(&ctrl->cginame, idindic, 0, NO_CONV);
		DYNBUF_ADD_STR(html, " <font size=-1>sur <nobr>");
		ctrl->FIELD = idindic;
		if(ctrl_add_calendar_input(cntxt, i_ctrl,
			pvf->startdate[0] ? pvf->startdate : dyntab_val(&pvf->labels, 0, 0),
			pvf->dispunit, pvf->maxlabels)) STACK_ERROR;
		ctrl->FIELD = "";
		ctrl->cginame->cnt = cnt;
		ctrl->cginame->data[cnt] = 0;
		DYNBUF_ADD_STR(html, "</nobr></font>");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_pvtcell
** Description : output a pivot table value cell
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_pvtcell"
#define ERR_CLEANUP	
int ctrl_calc_output_pvtcell(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv,					/* in : pivot table data */
	unsigned long i,				/* in : row index in results table */
	unsigned long j					/* in : col index in results table */
){
	EVA_form *form = cntxt->form;
	DynBuffer **html = form->html;
	int b_multi_data = pv->nbdata > 1 && !pv->row;
	int b_last_data = pv->data == &pv->datas[pv->nbdata - 1];

	/* Output cell header */
	DYNBUF_ADD_STR(html, "<td");
	DYNBUF_ADD(html, (pv->data && pv->data->decimals > 0) ? " align=right" : " align=center", 0, NO_CONV);
	if((b_multi_data ? !b_last_data : i < ROWCNT) && j < COLCNT)
	{
		/* Output values */
		DYNBUF_ADD_STR(html, "><nobr><font size=-1>");
		if(dyntab_sz(pv->res, i, j))
		{
			if(ctrl_calc_output_pvt_val(cntxt, i_ctrl, pv, i, j)) STACK_ERROR;
		}
		else
			DYNBUF_ADD_STR(html, "&nbsp;"); 
		DYNBUF_ADD_STR(html, "</font>");
	}
	else
	{
		/* Output totals */
		if(b_multi_data ? b_last_data : i >= ROWCNT) DYNBUF_ADD_STR(html, " bgcolor=#EEEEEE");
		DYNBUF_ADD_STR(html, "><nobr><b>");
		if((b_multi_data ? !b_last_data : i != ROWCNT) || j != COLCNT) DYNBUF_ADD_STR(html, "<font size=-1>");
		if(i <= ROWCNT && j <= COLCNT)
		{
			if(ctrl_calc_output_pvt_val(cntxt, i_ctrl, pv, i, j)) STACK_ERROR;
		}
		else
			DYNBUF_ADD_CELL(html, pv->res, i, j, NO_CONV);
		if(i != ROWCNT || j != COLCNT) DYNBUF_ADD_STR(html, "</font>");
		DYNBUF_ADD_STR(html, "</b>");
	}
	DYNBUF_ADD_STR(html, "</td>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_coltitle
** Description : output title row(s) for all column fields
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_coltitle"
#define ERR_CLEANUP	
int ctrl_calc_output_coltitle(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv					/* in : pivot table data */
){
	EVA_form *form = cntxt->form;
	unsigned long i, j, icol;
	char *bgcolor = "DDDDDD";
	DynBuffer **html = form->html;
	int b_coltitle = pv->row == pv->rows || !pv->b_stackcols;
	char printbuf[1024];

	/* Output row header */
	dynbuf_print(html, "<tr><td valign=bottom rowspan=%lu><b>", b_coltitle ? pv->maxcols + 1 : 1);
	if(ctrl_calc_output_field_title(cntxt, i_ctrl, pv, pv->row)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</b></td>");

	/* Output empty line & return if no column titles (multiple lines with stacked columns) */
	if(!b_coltitle)
	{
		for(i = 0; i < pv->totcols - 1; i++) DYNBUF_ADD_STR(html, "<td>&nbsp;</td>");
		DYNBUF_ADD_STR(html, "</tr>");
		RETURN_OK;
	}

	/* Handle stacked labels : output one column label per row */
	if(pv->b_stackcols)
	{
		for(i = 0; i <= pv->maxcols; i++)
		{
			/* Output title row for each column field */
			for(icol = 0; icol < pv->nbcol; icol++)
			{
				pv->col = pv->cols + icol;
				pv->res = &pv->restbl[0][0][icol];

				/* Output unstacked label on last row only */
				if(!pv->col->b_stackcols && i == pv->maxcols)
				{
					unsigned long j;
					for(j = 0; j < COLCNT; j++)
					{
						DYNBUF_ADD3(html, "<td align=center bgcolor=#", bgcolor, 0, NO_CONV, ">");
						if(ctrl_calc_output_field_label(cntxt, pv, pv->col, j)) STACK_ERROR;
					}
				}

				/* Output stacked labels or totals */
				if(pv->col->b_stackcols && i >= pv->maxcols - COLCNT && i < pv->maxcols)
				{
					/* Output stacked label for this title row */
					unsigned long j = i + COLCNT - pv->maxcols;
					if(j) DYNBUF_ADD3_INT(html, "<td width=30 bgcolor=#EEEEEE rowspan=", pv->maxcols - i + 1, ">&nbsp;</td>");
					DYNBUF_ADD3(html, "<td bgcolor=#", bgcolor, 0, NO_CONV, "");
					DYNBUF_ADD3_INT(html, " colspan=", pv->res->nbcols - j, ">");
					if(ctrl_calc_output_field_label(cntxt, pv, pv->col, j)) STACK_ERROR;
				}
				else if(i >= pv->maxcols)
				{
					/* Output field total labels */
					if(pv->col->b_stackcols) DYNBUF_ADD_STR(html, "<td>&nbsp;</td>");
					for(j = 0; !j || pv->row && j < pv->row->totals.nbrows; j++)
					{
						DYNBUF_ADD3(html, "<td align=center bgcolor=#", "EEEEEE", 0, NO_CONV, ">");
						DYNBUF_ADD3(html, "<font size=-1><b>", 
								(pv->row && dyntab_sz(&pv->row->totals, j, 0 )) ? 
									dyntab_val(&pv->row->totals, j, 0 ) : "Total", 0,
								TO_HTML, "</b></font></td>");
					}
				}
			}
			DYNBUF_ADD_STR(html, "</tr>");

			/* Prepare next row if applicable */
			if(i < pv->maxcols)
				DYNBUF_ADD_STR(html, "<tr bgcolor=#EEEEEE>");
		}
	}
	else
	{
		/* Standard format : output column labels in one row */
		for(icol = 0; (!icol && pv->row) || icol < pv->nbcol; icol++)
		{
			/* Output field labels */
			if(pv->nbcol)
			{
				pv->col = pv->cols + icol;
				for(i = 0; i < COLCNT; i++)
				{
					DYNBUF_ADD3(html, "<td align=center bgcolor=#", bgcolor, 0, NO_CONV, ">");
					if(ctrl_calc_output_field_label(cntxt, pv, pv->col, i)) STACK_ERROR;
				}
			}

			/* Output field total labels */
			for(i = 0; !i || pv->row && i < pv->row->totals.nbrows; i++)
			{
				DYNBUF_ADD3(html, "<td align=center bgcolor=#", "EEEEEE", 0, NO_CONV, ">");
				DYNBUF_ADD3(html,
					"<font size=-1><b>",
						(pv->row && dyntab_sz(&pv->row->totals, i, 0 )) ? 
							dyntab_val(&pv->row->totals, i, 0 ) : "Total", 0, TO_HTML,
					"</b></font></td>");
			}
		}
		DYNBUF_ADD_STR(html, "</tr>");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_rowfield
** Description : output rows for a row field & all columns fields
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_rowfield"
#define ERR_CLEANUP	
int ctrl_calc_output_rowfield(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv					/* in : pivot table data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long irow = pv->row ? pv->row - pv->rows : 0;
	unsigned long idata = pv->data ? pv->data - pv->datas : 0;
	unsigned long i, j, icol;
	DynBuffer **html = form->html;
	char printbuf[1024];
	PivotTableField *pvf, *pvg;

	/* Return if no object selected */
	if(pv->b_empty)
	{
		DYNBUF_ADD_STR(html, "<tr><td>-</td><td align=center>0</td>");
		RETURN_OK;
	} 

	/* Get min / max values for each column field */
	if(pv->row) for(icol = 0; !icol || icol < pv->nbcol; icol++)
	{
		pv->col = pv->nbcol ? pv->cols + icol : NULL;
		pvf = pv->col ? pv->col : pv->row;
		pvf->resmin = 0; pvf->resmax = 0;
		for(j = 0; j < ROWCNT; j++)
		{
			double val = atof(dyntab_val(&pv->restbl[idata][irow][icol], j, COLCNT));
			if(val > pvf->resmax) pvf->resmax = val; 
			else if(val < pvf->resmin) pvf->resmin = val;
		}
	}

	/* Output rows */
	for(i = 0; i < pv->maxrows; i++)
	{
		char *bgcolor = i < ROWCNT ? table_row_bgcolor(cntxt, ctrl->objtbl, i, NULL) : "EEEEEE";
		DYNBUF_ADD_STR(html, "<tr");
		if(*bgcolor) DYNBUF_ADD3(html, " bgcolor=#", bgcolor, 0, NO_CONV, "");
		DYNBUF_ADD_STR(html, ">");

		/* Output columns for each field */
		for(icol = 0; !icol || icol < pv->nbcol; icol++)
		{
			pv->col = pv->nbcol ? pv->cols + icol : NULL;
			pv->res = &pv->restbl[idata][irow][icol];

			/* Output row label if first column field */
			if(!icol)
			{
				if(i < ROWCNT)
				{
					DYNBUF_ADD_STR(html, "<td align=right><font size=-1>");
					if(ctrl_calc_output_field_label(cntxt, pv, pv->row, i)) STACK_ERROR;
				}
				else
				{
					DYNBUF_ADD_STR(html, "<td align=right><font size=-1>");
					if(i == ROWCNT && pv->row && pv->row->labels.nbrows > 2 && pv->tblnotes != TblNote_minimal)
						DYNBUF_ADD3_INT(html, "", pv->row->labels.nbrows, " lignes - ");
					DYNBUF_ADD3(html, "<b>",
								(pv->col && dyntab_sz(&pv->col->totals, i - ROWCNT, 0 )) ? 
								dyntab_val(&pv->col->totals, i - ROWCNT, 0 ) : (i - ROWCNT) ? "%" : "Total", 0, NO_CONV,
							"</b></font></td>");
				}
			}

			/* Output cells */
			for(j = 0; j < pv->res->nbcols; j++)
				if(ctrl_calc_output_pvtcell(cntxt, i_ctrl, pv, i, j)) STACK_ERROR;
 
			/* Set params for in-table graph */
			pvf = pv->col ? pv->col : pv->row;
			pvg = (pv->row && pv->row->graph_pos) ? pv->row : pv->data;
			if(pvg && !pvg->graph_color) pvg->graph_color = "8080FF";
			if(pvg && pvg->graph_size < 10) pvg->graph_size = 40;

			/* Output in-table graph for row values */
			if((icol + 1 == pv->nbcol || !pv->nbcol) && !pv->exportparams && pvg && pvg->graph_pos &&
				!strcmp(pvg->graph_pos, "_EVA_TOTAL") && pvf && pvf->resmax > pvf->resmin)
			{
				double val = atof(dyntab_val(pv->res, i, COLCNT));
				unsigned long w = (unsigned long)(pvg->graph_size * (val - pvf->resmin) / (pvf->resmax - pvf->resmin));
				DYNBUF_ADD_STR(html, "<td>");
				if( i < ROWCNT && w)
				{
					dynbuf_print3(html,
						"<table noborder height=16 cellpadding=0 cellspacing=0 bgcolor=#%s width=%lu><tr>"
						"<td><font size=-1 color=%s></font></td></tr></table>", pvg->graph_color, w, pvg->graph_color);
				}
				DYNBUF_ADD_STR(html, "</td>");
			}
			
		}
		DYNBUF_ADD_STR(html, "</tr>");
	}

	/* Output in-table graph for each column field */
	if(!pv->exportparams && pv->nbcol && (!pv->nbrow || irow + 1 == pv->nbrow))
	{
		DYNBUF_ADD_STR(html, "<tr><td></td>");
		for(icol = 0; icol < pv->nbcol; icol++)
		{
			int b_graph = 0;
			pv->col = pv->cols + icol;
			pv->col->resmin = 0; pv->col->resmax = 0;
			pvg = (pv->col->graph_pos) ? pv->col : pv->data;
			if(pvg && !pvg->graph_color) pvg->graph_color = "8080FF";
			if(pvg && pvg->graph_size < 10) pvg->graph_size = 40;

			/* Get maximum value for column field totals */
			pv->res = &pv->restbl[idata][irow][icol];
			for(j = 0; j < COLCNT; j++)
			{
				double val = atof(dyntab_val(pv->res, ROWCNT, j));
				if(val > pv->col->resmax) pv->col->resmax = val; 
				else if(val < pv->col->resmin) pv->col->resmin = val;
			}
			b_graph = pvg && pvg->graph_pos && !strcmp(pvg->graph_pos, "_EVA_TOTAL") && pv->col->resmax > pv->col->resmin;

			/* Output graph for column field */
			for(j = 0; j < pv->res->nbcols; j++)
			{
				double val = atof(dyntab_val(pv->res, ROWCNT, j));
				unsigned long h = b_graph ? (unsigned long)(pvg->graph_size * (val - pv->col->resmin) / (pv->col->resmax - pv->col->resmin)) : 0;
				DYNBUF_ADD_STR(html, "<td valign=bottom>");
				if(h && j < COLCNT)
				{
					dynbuf_print2(html,
						"<table noborder cellpadding=0 cellspacing=0 width=100%%><tr>"
						"<td height=%lu bgcolor=#%s></td></tr></table>", h, pvg->graph_color);
				}
				DYNBUF_ADD_STR(html, "</td>");
			}

		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_pvt_lblnote
** Description : output LABEL/NOTES fields
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_pvt_lblnote"
#define ERR_CLEANUP
int ctrl_output_pvt_lblnote(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	int options,					/* in : display options
										bit 0 : output notes title
										bit 1 : output obj label
										bit 2 : output obj notes */
	DynTable *data					/* in : object data to read notes from */
){
	DynBuffer **html = cntxt->form->html;
	if(!html) return 0;

	if(options & 2)
	{
		DYNBUF_ADD3_CELLP(html, "<b>", DYNTAB_FIELD_CELL(data, LABEL), TO_HTML, "</b> ");
		if(options & 1) DYNBUF_ADD_STR(html, "<br>")
	}

	if(options & 4)
	{
		DynTableCell *c = DYNTAB_FIELD_CELL(data, NOTES);
		if(c)
		{
			if(options & 1) DYNBUF_ADD_STR(html, "<u>Notes</u> : ")
			else if(options & 2) DYNBUF_ADD_STR(html, " : ");
			DYNBUF_ADD3_CELLP(html, "", c, TO_HTML, "<br>");
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_listobj_lblnote
** Description : output LABEL/NOTES fields for multiple objects
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_listobj_lblnote"
#define ERR_CLEANUP	DYNTAB_FREE(valdata)
int ctrl_output_listobj_lblnote(	/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *title,					/* in : list title */
	DynTable *val					/* in : list of objects to output */
){
	DynBuffer **html = cntxt->form->html;
	DynTable valdata = {0};
	unsigned long i;
	if(!html || !val || !val->nbrows || !val->cell || !val->cell->txt) return 0;

	if(title) DYNBUF_ADD3(html, "<br><u>", title, 0, TO_HTML, "</u>");
	DYNBUF_ADD_STR(html, "<ul>");
	for(i = 0; i < val->nbrows; i++)
	{
		DYNBUF_ADD_STR(html, "<li>");
		if(qry_obj_field(cntxt, &valdata, DYNTAB_TOULRC(val, i, 0), "_EVA_LABEL,_EVA_NOTES") ||
			ctrl_output_pvt_lblnote(cntxt, 6, &valdata)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</li>");
	}
	DYNBUF_ADD_STR(html, "</ul>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_pvt_notes
** Description : output notes for a pivot table field
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_pvt_notes"
#define ERR_CLEANUP	DYNTAB_FREE(val)
int ctrl_output_pvt_notes(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *title,					/* in : notes title */
	int options,					/* in : display options
										bit 0 : output detail
										bit 1 : output trailing empty line */
	DynTable *data					/* in : object data to read from */
){
	DynBuffer **html = cntxt->form->html;
	DynTable val = {0};
	if(!html) return 0;

	/* Output detail if applicable */
	if(options & 1)
	{
		char *expr = DYNTAB_FIELD_VAL(data, FIELD);

		/* Output label & notes */
		DYNBUF_ADD_STR(html, "<br><u><b>");
		if(title) DYNBUF_ADD(html, title, 0, TO_HTML);
		DYNBUF_ADD3_CELLP(html, " [", DYNTAB_FIELD_CELL(data, LABEL), TO_HTML, "]</b></u>");
		if(ctrl_output_pvt_lblnote(cntxt, 4, data)) STACK_ERROR;

		/* Output fields label or formula */
		DYNTAB_FIELD(&val, data, SELCTRL);
		if(val.nbrows && val.cell->txt)
		{
			if(ctrl_output_listobj_lblnote(cntxt, "Champ de saisie", &val)) STACK_ERROR;
		}
		else if(*expr)
			DYNBUF_ADD3(html, "<br><u>Valeur calculee</u> : <font face='Courier new'>", expr, 0, TO_HTML, "</font>");

		/* Output forms label */
		DYNTAB_FIELD(&val, data, FORMS);
		if(ctrl_output_listobj_lblnote(cntxt, "Fiches sélectionnées", &val)) STACK_ERROR;

		/* Output filters label */
		DYNTAB_FIELD(&val, data, FILTERS);
		if(ctrl_output_listobj_lblnote(cntxt, "Condition de sélection", &val)) STACK_ERROR;
	}
	else if(DYNTAB_FIELD_CELL(data, NOTES))
	{
		/* Output notes */
		if(ctrl_output_pvt_lblnote(cntxt, 6, data)) STACK_ERROR;
	}

	if(options & 2) DYNBUF_ADD_STR(html, "<br>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_calc_output_pvtres
** Description : output results of a pivot table
*********************************************************************/
#define ERR_FUNCTION "ctrl_calc_output_pvtres"
#define ERR_CLEANUP
int ctrl_calc_output_pvtres(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv					/* in : pivot table data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i;
	unsigned long irow = 0, icol, idata = 0;
	DynBuffer **html = form->html;
	char printbuf[1024];

	/* Scan column labels for each column field to determine stacking mode */
	pv->totcols = (pv->nbrow ? 1 : 0);
	for(icol = 0; !icol || icol < pv->nbcol; icol++)
	{
		pv->col = pv->nbcol ? pv->cols + icol : NULL;
		pv->res = &pv->restbl[idata][irow][icol];

		/* Compute total # of columns for the field */
		pv->totcols += pv->res->nbcols;
		if(!pv->col || pv->exportparams) continue;

		/* Check column labels size & set columns stack mode */
		if(!pv->col->b_stackcols && pv->col->stackcols_limit)
			for(i = 0; i < pv->col->labels.nbrows; i++)
				if(dyntab_sz(&pv->col->labels, i, 0) > pv->col->stackcols_limit)
					pv->col->b_stackcols = 1;
		if(pv->col->b_stackcols && pv->col->labels.nbrows > pv->maxcols) pv->maxcols = pv->col->labels.nbrows;
		pv->b_stackcols |= pv->col->b_stackcols;
	}
	if(pv->exportparams) pv->b_stackcols = 0;

	/* Output table header */
	if(!(ctrl->TABLE_STYLE && *ctrl->TABLE_STYLE)) ctrl->BORDER = "1";
	if(table_read_controls(cntxt, i_ctrl, NULL) ||
		ctrl_put_table_header(cntxt, ctrl)) STACK_ERROR;

	/* Output title */
	DYNBUF_ADD3_INT(html, "</tr><tr bgcolor=#FFFFFF><td colspan=", pv->totcols + 2, ">");
	if(pv->tblnotes != TblNote_minimal && ctrl_calc_output_title(cntxt, i_ctrl, pv)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td></tr>");

	/* Loop on data fields */
	for(idata = 0; !idata || idata < pv->nbdata; idata++)
	{
		/* Loop on row fields */
		pv->data = pv->nbdata ? pv->datas + idata : NULL;
		for(irow = 0; !irow || irow < pv->nbrow; irow++)
		{
			/* Output data & column headers if applicable */
			pv->row = pv->nbrow ? pv->rows + irow : NULL;
			if((pv->data ? !idata : (pv->exportparams || !irow)) && (pv->nbdata || pv->nbcol))
			{
				if(pv->exportparams && irow) DYNBUF_ADD3_INT(html, "<tr><td colspan=", pv->maxcols, ">&nbsp;</tr>");
				DYNBUF_ADD_STR(html, "<tr bgcolor=#EEEEEE><td align=center valign=middle><font size=-1>");
				if(pv->tblnotes != TblNote_minimal)
				{
					if(ctrl_calc_output_field_title(cntxt, i_ctrl, pv, pv->data)) STACK_ERROR;
					DYNBUF_ADD_STR(html, "&nbsp;</font></td>");
				}
				for(icol = 0; icol < pv->nbcol; icol++)
				{
					pv->col = pv->cols + icol;
					pv->res = &pv->restbl[idata][irow][icol];
					dynbuf_print2(html, "<td align=center valign=bottom rowspan=%lu colspan=%lu><b>",
						(!pv->b_stackcols || irow) ? 1 : 
						pv->col->b_stackcols ? pv->maxcols - COLCNT + 1 : pv->b_stackcols ? pv->maxcols + 1 : 1,
						pv->res->nbcols);
					if(ctrl_calc_output_field_title(cntxt, i_ctrl, pv, pv->col)) STACK_ERROR;
					DYNBUF_ADD_STR(html, "</b></td>");
				}
				DYNBUF_ADD_STR(html, "</tr>");
			}

			/* Output columns titles if rows present */
			if(!idata || pv->row)
			{
				/* Output columns labels */
				pv->row = pv->nbrow ? pv->rows + irow : NULL;
				if(ctrl_calc_output_coltitle(cntxt, i_ctrl, pv)) STACK_ERROR;

				/* Output columns fields for the row field */
				pv->maxrows = pv->restbl[idata][irow][0].nbrows;
				for(icol = 1; icol < pv->nbcol; icol++)
				{
					pv->res = &pv->restbl[idata][irow][icol];
					if(pv->res->nbrows > pv->maxrows) pv->maxrows = pv->res->nbrows;
				}
			}

			/* If no rows : output data totals for each column field */
			if(!pv->row && pv->nbdata > 1)
			{
				/* Output line header & data label */
				DYNBUF_ADD3(html, "<tr bgcolor=#", table_row_bgcolor(cntxt, ctrl->objtbl, idata, NULL), 0, NO_CONV, "><td align=right><font size=-1>");
				if(idata == pv->nbdata - 1)	DYNBUF_ADD_STR(html, "<b>");
				if(pv->data->srcdata.nbrows && !pv->exportparams)
				{
					if(ctrl_add_symbol_btn(cntxt, ctrl, NULL, &pv->data->srcdata, 0, NULL, "LABEL+OBJNOTES")) STACK_ERROR;
				}
				else 
					DYNBUF_ADD(html, pv->data->label, 0, TO_HTML);
				if(idata == pv->nbdata - 1)	DYNBUF_ADD_STR(html, "</b>");
				DYNBUF_ADD_STR(html, "</td>");

				/* Output cells */
				for(icol = 0; !icol || icol < pv->nbcol; icol++)
				{
					pv->res = &pv->restbl[idata][irow][icol];
					for(i = 0; i < pv->res->nbcols; i++)
						if(ctrl_calc_output_pvtcell(cntxt, i_ctrl, pv, 0, i)) STACK_ERROR;
				}
			}
			else
				/* Output rows fields for the data field */
				if(ctrl_calc_output_rowfield(cntxt, i_ctrl, pv)) STACK_ERROR;

			/* Output export params if applicable */
			if(pv->exportparams) for(icol = 0; !icol || icol < pv->nbcol; icol++)
			{
				pv->col = pv->nbcol ? pv->cols + icol : NULL;
				pv->res = &pv->restbl[idata][irow][icol];
				dynbuf_print6(&pv->exportparams, "Table\t%lu\t%s\t%s\t%s\t%lu\t%lu",	irow,
					pv->row ? pv->row->label : "", 
					pv->row ? (pv->row->basefmt && *pv->row->basefmt) ? pv->row->basefmt : pv->row->dispfmt : "",
					pv->row ? pv->row->graphtyp : "", pv->row ? pv->row->labels.nbrows : 0, pv->res->nbrows);
				dynbuf_print6(&pv->exportparams, "\t%lu\t%s\t%s\t%s\t%lu\t%lu",	icol,
					pv->col ? pv->col->label : "", 
					pv->col ? (pv->col->basefmt && *pv->col->basefmt) ? pv->col->basefmt : pv->col->dispfmt : "",
					pv->col ? pv->col->graphtyp : "", pv->col ? pv->col->labels.nbrows : 0, pv->res->nbcols);
				dynbuf_print3(&pv->exportparams, "\t%lu\t%s\t%d\n",
					idata, pv->data ? pv->data->label : "", pv->data ? pv->data->decimals : 0);
			}
		}
	}
	DYNBUF_ADD_STR(html, "</table>");

	/* Output table comments */
	if(pv->tblnotes)
	{
		char *function = (pv->data && pv->data->function.cell && pv->data->function.cell->txt && pv->data->function.cell->txt[0]) ?
							pv->data->function.cell->txt : pv->function;
		if(!function) function = "";
		DYNBUF_ADD3_INT(html, "<table><tr><td bgcolor=#FFFFFF colspan=", pv->totcols, ">");

		/* Output dynamic label */
		if(pv->tblnotes & TblNote_dynamic)
		{
			EVA_ctrl ct = {0};
			for(i = 0; i < pv->dynnotes.nbrows; i++)
			{
				if(qry_obj_field(cntxt, &ct.attr, DYNTAB_TOULRC(&pv->dynnotes, i, 0), NULL) ||
					ctrl_read_baseobj(cntxt, &ct.attr) ||
					ctrl_set_format_attr(cntxt, &ct)) STACK_ERROR;
				ct.LABELPOS = "_EVA_NONE";
				ct.POSITION = "_EVA_SameCell";
				if(ctrl_format_pos(cntxt, &ct, 1) ||
					ctrl_output_exprval(cntxt, &ct, 1) ||
					ctrl_format_pos(cntxt, &ct, 0)) STACK_ERROR;
				DYNTAB_FREE(ct.attr);
				DYNTAB_FREE(ct.val);
				DYNTAB_FREE(ct.allval);
			}
			DYNBUF_ADD_STR(html, "<br>");
		}

		/* Output table contents */
		DYNBUF_ADD_STR(html, "<font size=-1>");
		if(pv->tblnotes & (TblNote_note | TblNote_detail)) 
		{
			ctrl_output_pvt_notes(cntxt, "Détail du tableau", pv->tblnotes & TblNote_detail ? 1 : 0,pv->srcdata);
			if(pv->tblnotes & TblNote_detail)
				dynbuf_print2(html, "<br><u>Valeurs affichées</u> : <b>%s de [%s]</b><br>", 
									!strcmp("VALCOUNT", function) ? "Nombre de valeurs" :
									!strcmp("VALCOUNT1", function) ? "Nombre de valeurs distinctes" :
									!strcmp("SUM", function) ? "Somme des valeurs" :
									!strcmp("AVG", function) ? "Moyenne des valeurs" :
									!strcmp("MIN", function) ? "Plus petite valeur" :
									!strcmp("MAX", function) ? "Plus grande valeur" :
																	"Nombre de fiches",
									pv->data ? pv->data->label : pv->row ? pv->row->label : pv->col ? pv->col->label : "tout");
		}
		if(pv->row && pv->tblnotes & (TblNote_rownote | TblNote_rowdetail)) 
			ctrl_output_pvt_notes(cntxt, "Lignes", pv->tblnotes & TblNote_rowdetail ? 3 : 0,&pv->row->srcdata);
		if(pv->col && pv->tblnotes & (TblNote_colnote | TblNote_coldetail)) 
			ctrl_output_pvt_notes(cntxt, "Colonnes", pv->tblnotes & TblNote_coldetail ? 3 : 0,&pv->col->srcdata);
		if(pv->data && pv->tblnotes & (TblNote_datanote | TblNote_datadetail)) 
			ctrl_output_pvt_notes(cntxt, "Données", pv->tblnotes & TblNote_datadetail ? 3 : 0,&pv->data->srcdata);
		DYNBUF_ADD_STR(html, "</font></td></tr></table>");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : get_cgi_obj_data
** Description : obtain edited data for a given object
*********************************************************************/
#define ERR_FUNCTION "get_cgi_obj_data"
#define ERR_CLEANUP
int get_cgi_obj_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *objdata,				/* out : object data - empty if multiple objects */
	unsigned long idobj,			/* in : desired object Id */
	unsigned long idform,			/* in : cgi search form Id */
	DynTable *dbdata				/* in/out : database object data - read from db if empty */
){
	/* Read cgi data for object / form */
	dyntab_free(objdata);
	if(cgi_filter_values(cntxt, objdata, 'D', ~0UL, idform, idobj, NULL, "", 0, 0)) STACK_ERROR;

	/* Check for new object */
	if(!idobj)
	{
		/* New object : add formstamp for correct title display */
		char tmp[32];
		sprintf(tmp, "%lu", idform);
		DYNTAB_ADD_FIELD(objdata, objdata->nbrows, 0, tmp, strlen(tmp),
						"_EVA_FORMSTAMP", cntxt->val_FORMSTAMP, idobj);
	}
	else
	{
		/* Existing object : complete with DB data for unchanged fields */
		if(!dbdata->nbrows && qry_obj_field(cntxt, dbdata, idobj, NULL)) STACK_ERROR;
		if(qry_complete_data(cntxt, objdata, dbdata, NULL, NULL)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : get_current_obj_field_data
** Description : obtain data from current obj or current obj field
*********************************************************************/
#define ERR_FUNCTION "get_current_obj_field_data"
#define ERR_CLEANUP
int get_current_obj_field_data(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *field,					/* in : field to use in current object to obtain data */
	DynTable *objdata,				/* out : object data - empty if multiple objects */
	DynTable *idlist				/* out : list of objects Ids if multiple objects */
){
	EVA_form *form = cntxt->form;

	dyntab_free(objdata);
	if(field && *field)
	{
		/* Field is given : read field value from CGI / DB data (multiple IdObj) */
		if(form_get_field_values(cntxt, idlist, field, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj)))
			STACK_ERROR;

		/* Read object data if unique object */
		if(idlist->nbrows == 1 && qry_obj_data(cntxt, objdata, idlist, NULL)) STACK_ERROR;
	}
	else
	{
		/* Field is not given : use CGI data of current object */
		unsigned long i;
		if(cgi_filter_values(cntxt, objdata, 'D', ~0UL,
				DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), NULL, "", 0, 0)) STACK_ERROR;
		for(i = objdata->nbrows; i > 0; i--)
			if(!dyntab_sz(objdata, i - 1, 0))
				dyntab_del_rows(objdata, i - 1, 1);

		/* Check for new object */
		if(!form->objdata.nbrows)
		{
			/* New object : add formstamp for correct title display */
			DynTableCell *c = dyntab_cell(&form->id_form, 0, 0);
			DYNTAB_ADD_FIELD(objdata, objdata->nbrows, 0, c->txt, c->len,
							"_EVA_FORMSTAMP", cntxt->val_FORMSTAMP, DYNTAB_TOUL(&form->id_obj));
		}
		else
			/* Existing object : complete with DB data for unchanged fields */
			if(qry_complete_data(cntxt, objdata, &form->objdata, NULL, NULL)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_add_sr_options
** Description : add a list of options to pivot field search & replace table
*********************************************************************/
#define ERR_FUNCTION "pivottable_add_sr_options"
#define ERR_CLEANUP	DYNTAB_FREE(values)
int pivottable_add_sr_options(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	PivotTableField *pvf,				/* in/out : field definition */
	DynTable *options					/* in : options list to read from */
){							
	DynTable values = {0};
	unsigned long i, j, cnt = 0;
	DynTableCell *empty = NULL;

	/* Read options labels & values in a single table (default order is used : label & value for each option) */
	if(qry_listobj_field(cntxt, &values, options, "_EVA_LABEL,_EVA_VALUE")) STACK_ERROR;

	for(i = 0; i < values.nbrows; i++)
	{
		DynTableCell *c = dyntab_cell(&values, i, 0);
		DynTableCell *cprev = i ? c - 1 : NULL;
		DynTableCell *cnext = (i + 1 < values.nbrows) ? c + 1 : NULL;

		/* If label / value pair found */
		if(cnext && c->IdObj == cnext->IdObj)
		{
			/* Add to search & replace table */
			DynTableCell *l = c, *v = cnext;
			j = pvf->sr_src.nbrows > pvf->sr_dest.nbrows ? pvf->sr_src.nbrows : pvf->sr_dest.nbrows;
			DYNTAB_ADD_CELLP(&pvf->sr_src, j, 0, v);
			DYNTAB_ADD_CELLP(&pvf->sr_dest, j, 0, l);
		}
		/* Count options without value */
		else if((!cprev || c->IdObj != cprev->IdObj) && (!cnext || c->IdObj != cnext->IdObj))
		{
			cnt ++;
			empty = c;
		}
	}

	/* Use option with empty value if applicable */
	if(!pvf->emptyval.nbrows && pvf->sr_src.nbrows > 1 && empty && cnt == 1)
		DYNTAB_ADD_CELLP(&pvf->emptyval, 0, 0, empty);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_set_indic_fmt_from_ctrl
** Description : set indicator format based on control type & format
*********************************************************************/
#define ERR_FUNCTION "pivottable_set_indic_fmt_from_ctrl"
#define ERR_CLEANUP	DYNTAB_FREE(options)
int pivottable_set_indic_fmt_from_ctrl(	/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	PivotTableField *pvf,				/* in/out : field definition */
	DynTable *attr						/* in : control data to read from */
){							
	DynTable options = {0};
	char *ctrltyp = DYNTAB_FIELD_VAL(attr, TYPE);
	char *ctrlfmt = DYNTAB_FIELD_VAL(attr, FORMAT);

	if((!pvf->dispfmt || !pvf->dispfmt[0]) && (
		!strcmp(ctrltyp, "_EVA_RELATION") ||
		!strcmp(ctrltyp, "_EVA_FILEUPLOAD")))
	{
		pvf->dispfmt = "_EVA_RELATION";
	}
	else if((!pvf->dispfmt || !pvf->dispfmt[0]) && !strcmp(ctrltyp, "_EVA_NUMBER"))
	{
		pvf->dispfmt = "_EVA_NUMBER";
		if(!stricmp(ctrlfmt, "_EVA_MONEY")) pvf->decimals = 2;
		if(!pvf->sortmode || !pvf->sortmode[0]) pvf->sortmode = "_EVA_NUMBER";
	}
	else if((!pvf->dispfmt || !pvf->dispfmt[0]) && !strcmp(ctrltyp, "_EVA_DATE"))
	{
		pvf->dispfmt = "_EVA_DATE";
		pvf->b_calendar = 2;
		pvf->maxlabels = 12;
	}
	else if(!strcmp(ctrltyp, "_EVA_LIST") && !pvf->optsrc.nbrows && pvf->datatable[0] != 'D')
	{
		/* List : use options label/value as transform table */
		DYNTAB_FIELD(&options, attr, CTRLTREE);
		if((!pvf->dispfmt || !pvf->dispfmt[0]) && pivottable_add_sr_options(cntxt, pvf, &options)) STACK_ERROR;

		/* Add list options to labels if show no match */
		if(pvf->b_shownomatch && form_eval_fieldexpr(cntxt, &pvf->labels, 0, attr->cell->IdObj, add_sz_str("_EVA_CTRLTREE->_EVA_LABEL"), attr, 0)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_get_indic_definition
** Description : read an indicator definition (control or indicator)
*********************************************************************/
#define ERR_FUNCTION "pivottable_get_indic_definition"
#define ERR_CLEANUP	DYNTAB_FREE(cgival); \
					DYNTAB_FREE(data)
int pivottable_get_indic_definition(	/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl	,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv,						/* in : pivot table definition */
	PivotTableField *pvf				/* in/out : field definition */
){							
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = {0};
	DynTable data = {0};
	DynTableCell *c;
	char *txt;
	int b_ctrl;

	/* Check if field is a control */
	DYNTAB_FIELD(&data, &pvf->srcdata, FORMSTAMP);
	b_ctrl = data.nbrows == 1 && DYNTAB_TOUL(&data) == OBJ_FORM_CONTROL;

	/* Read indicator label */
	pvf->label = DYNTAB_FIELD_VAL(&pvf->srcdata, LABEL);

	/* Read control / indicator data */
	if(b_ctrl)
	{
		/* Set display format depending on control type */
		if(pivottable_set_indic_fmt_from_ctrl(cntxt, pvf, &pvf->srcdata)) STACK_ERROR;
		pvf->b_showemptyval = 1;

		/* Read displayed data contents & totals */
		DYNTAB_ADD_STR(&pvf->percent, 0, 0, "_EVA_TOTAL");
		pvf->graph_pos = "_EVA_TOTAL";
	}
	else
	{
		/* Read filter & field control */
		DYNTAB_FIELD(&pvf->forms, &pvf->srcdata, FORMS);
		DYNTAB_FIELD(&pvf->filters, &pvf->srcdata, FILTERS);
		DYNTAB_FIELD(&pvf->selctrl, &pvf->srcdata, SELCTRL);
		if(!dyntab_sz(&pvf->forms, 0, 0)) dyntab_free(&pvf->forms);
		if(!dyntab_sz(&pvf->filters, 0, 0)) dyntab_free(&pvf->filters);
		if(!dyntab_sz(&pvf->selctrl, 0, 0)) dyntab_free(&pvf->selctrl);

		/* Read displayed data contents & totals */
		DYNTAB_FIELD_TAB(&pvf->totlabel, &pvf->srcdata, TOT_LABEL);
		DYNTAB_FIELD_TAB(&pvf->function, &pvf->srcdata, FUNCTION);
		DYNTAB_FIELD_TAB(&pvf->percent, &pvf->srcdata, PERCENT);

		/* Read display options */
		pvf->clicmode = DYNTAB_FIELD_VAL(&pvf->srcdata, CLICMODE);
		if(!strcmp(pvf->clicmode, "_EVA_NONE")) pvf->clicmode = "";
		pvf->objlist = strtoul(DYNTAB_FIELD_VAL(&pvf->srcdata, PVTLIST_CTRL), NULL, 10);
		pvf->dispfmt = DYNTAB_FIELD_VAL(&pvf->srcdata, FORMAT);
		pvf->dispunit = DYNTAB_FIELD_VAL(&pvf->srcdata, DATEUNIT);
		pvf->b_overrun = DYNTAB_FIELD_VAL(&pvf->srcdata, OVERRUN)[0] != 0;
		c = DYNTAB_FIELD_CELL(&pvf->srcdata, DATEREF);
		if(c)
		{
			if(!ctrl_eval_fieldexpr(cntxt, ctrl, &data, c->txt))
				snprintf(add_sz_str(pvf->dateref), "%s", dyntab_val(&data, 0, 0));
			else
				CLEAR_ERROR;
		}
		if(!*pvf->dateref) snprintf(add_sz_str(pvf->dateref), "%s", cntxt->timestamp);
		pvf->trunc = DYNTAB_FIELD_VAL(&pvf->srcdata, TRUNC);
		pvf->sortmode = DYNTAB_FIELD_VAL(&pvf->srcdata, SORTMODE);
		pvf->b_sortdesc = atoi(DYNTAB_FIELD_VAL(&pvf->srcdata, SORTDESC)) != 0;
		DYNTAB_FIELD(&pvf->emptyval, &pvf->srcdata, EMPTYLABEL);
		pvf->b_showemptyval = DYNTAB_FIELD_VAL(&pvf->srcdata, NOEMPTYVAL)[0] == 0;
		pvf->b_shownomatch = DYNTAB_FIELD_VAL(&pvf->srcdata, SHOWNOMATCH)[0] != 0;
		pvf->decimals = atoi(DYNTAB_FIELD_VAL(&pvf->srcdata, DECIMALS));
		c = DYNTAB_FIELD_CELL(&pvf->srcdata, SLICES);
		if(c && dyntab_from_list(&pvf->slices, c->txt, c->len, "/", 0, 2)) RETURN_ERR_MEMORY;
		DYNTAB_FIELD_TAB(&pvf->sr_src, &pvf->srcdata, SUBST_SRC);
		DYNTAB_FIELD_TAB(&pvf->sr_dest, &pvf->srcdata, SUBST_DEST);
		DYNTAB_FIELD_TAB(&pvf->sr_pos, &pvf->srcdata, SUBST_POS);
		pvf->graphtyp = DYNTAB_FIELD_VAL(&pvf->srcdata, GRAPH_TYPE);
		if(!strcmp(pvf->graphtyp, "_EVA_GRAPH_PERSO"))
		{
			if(form_eval_fieldexpr(cntxt, &pvf->graphfile, 0, pvf->srcdata.cell->IdObj, add_sz_str("_EVA_GRAPH_PERSO->_EVA_FILE_SERVER_NAME"), &pvf->srcdata, 0)) STACK_ERROR;
			pvf->graphtyp = dyntab_val(&pvf->graphfile, 0, 0);
		}
		pvf->maxlabels = atoi(DYNTAB_FIELD_VAL(&pvf->srcdata, PERIODS));
		pvf->timeway = DYNTAB_FIELD_VAL(&pvf->srcdata, TIMEWAY);
		pvf->b_calendar = (pvf->maxlabels && !strcmp(pvf->dispfmt, "_EVA_DATE")) ? 
									DYNTAB_FIELD_CELL(&pvf->srcdata, CALENDAR) ? 2 : 1 : 0;

		/* Handle list options */
		DYNTAB_FIELD(&pvf->optsrc, &pvf->srcdata, OPTIONS_LIST);
		if(pvf->optsrc.nbrows)
		{
			/* If list options present - check control type */
			if(qry_listobj_field(cntxt, &data, &pvf->optsrc, "_EVA_CONTROL")) STACK_ERROR;
			if(!dyntab_txt_cmp(add_sz_str("_EVA_CALC"), &data, 0, 0))
			{
				/* Calc-ed value : add result to labels */
				if(form_eval_valtype(cntxt, &pvf->labels, "_EVA_EXPRVAL", &pvf->optsrc, NULL)) CLEAR_ERROR;
			}
			else
			{
				/* Other : get control tree */
				if(qry_listobj_field(cntxt, &data, &pvf->optsrc, "_EVA_CTRLTREE")) STACK_ERROR;

				/* Add list options to labels if show no match */
				if(pvf->b_shownomatch && qry_listobj_field(cntxt, &pvf->labels, &data, "_EVA_LABEL")) STACK_ERROR;

				/* Add list options to search & replace table */
				if(pivottable_add_sr_options(cntxt, pvf, &data)) STACK_ERROR;
			}
			dyntab_group(&pvf->labels, "DISTINCT");
		}

		/* Read labels stacking option for columns */
		txt = DYNTAB_FIELD_VAL(&pvf->srcdata, STACKLABELS);
		if(!strcmp(txt, "_EVA_SINGLE") || pv->exportparams)
			pvf->b_stackcols = 0;
		else if(!strcmp(txt, "_EVA_STACK"))
			pvf->b_stackcols = 1;
		else
		{
			pvf->stackcols_limit = strtoul(DYNTAB_FIELD_VAL(&pvf->srcdata, STACK_THRESHOLD), NULL, 10);
			if(!pvf->stackcols_limit) pvf->stackcols_limit = 10;
		}

		/* Read graph options */
		pvf->graph_pos = DYNTAB_FIELD_VAL(&pvf->srcdata, HTML_GRAPH_POS);
		if(!*pvf->graph_pos) pvf->graph_pos = NULL;
		pvf->graph_color = DYNTAB_FIELD_VAL(&pvf->srcdata, GRAPH_COLOR);
		if(!*pvf->graph_color) pvf->graph_color = NULL;
		pvf->graph_size = strtoul(DYNTAB_FIELD_VAL(&pvf->srcdata, GRAPH_MAX_SIZE), NULL, 10);
	}
	/* Read calendar inputs if applicable */
	if(pvf->b_calendar == 2)
	{
		char idindic[16] = {0};
		size_t cnt = ctrl->cginame->cnt;
		if(pvf && pvf->srcdata.cell) snprintf(add_sz_str(idindic), "%lu", pvf->srcdata.cell->IdObj);
		if(*idindic) DYNBUF_ADD(&ctrl->cginame, idindic, 0, NO_CONV);
		ctrl->FIELD = idindic;
		if(ctrl_get_calendar_input(cntxt, i_ctrl, CtlDateAll, pvf->startdate, &pvf->dispunit, &pvf->maxlabels))
			STACK_ERROR;
		ctrl->FIELD = "";
		ctrl->cginame->cnt = cnt;
		ctrl->cginame->data[cnt] = 0;
	}

	/* Read indicator value expression */
	if(pvf->selctrl.nbrows)
	{
		if(qry_listobj_field(cntxt, &pvf->expr, &pvf->selctrl, "_EVA_FIELD,_EVA_SRCFIELD")) STACK_ERROR;
		if(qry_listobj_field(cntxt, &data, &pvf->selctrl, NULL)) STACK_ERROR;

		/* Set default format from control */
		if(pivottable_set_indic_fmt_from_ctrl(cntxt, pvf, &data)) STACK_ERROR;
	}
	else
		DYNTAB_FIELD(&pvf->expr, &pvf->srcdata, FIELD);
	if(!pvf->expr.nbrows) DYNTAB_ADD_STR(&pvf->expr, 0, 0, "");
	if(!pvf->emptyval.nbrows) DYNTAB_ADD_STR(&pvf->emptyval, 0, 0, "(non renseigné)");

	/* Store base format */
	pvf->basefmt = pvf->dispfmt;
	if(!pvf->basefmt) pvf->basefmt = "";

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP


/*********************************************************************
** Function : pivottable_get_field_definition
** Description : read a field definition for pivot table
*********************************************************************/
#define ERR_FUNCTION "pivottable_get_field_definition"
#define ERR_CLEANUP	DYNTAB_FREE(idlist)
int pivottable_get_field_definition(	/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv,						/* in/out : pivot table definition */
	char *pos							/* in : field position to read (ROW | COL | DATA) */
){							
	char field[64];
	char *srctype;
	DynTable idlist = {0};
	PivotTableField *pvftbl = *pos == 'R' ? pv->rows : *pos == 'C' ? pv->cols : pv->datas;
	unsigned long *pvfnb = *pos == 'R' ? &pv->nbrow : *pos == 'C' ? &pv->nbcol : &pv->nbdata;
	PivotTableField **dest = *pos == 'R' ? &pv->row : *pos == 'C' ? &pv->col : &pv->data;
	PivotTableField *pvf = pvftbl + *pvfnb;
	unsigned long maxnb =  *pos == 'R' ? MAX_PVT_ROWFIELDS : *pos == 'C' ? MAX_PVT_COLFIELDS : MAX_PVT_DATAFIELDS;

	/* Read source type & basic field expression*/
	sprintf(field, "_EVA_SEL%s", pos);
	srctype = dyntab_field_val(pv->srcdata, field, ~0UL, 0);
	if(!strcmp(srctype, "_EVA_SIMPLE"))
	{
		/* Simple source type : read prefixed label & expression */
		pvf->pos = pos;
		sprintf(field, "_EVA_%sEXPR", pos);
		if(dyntab_filter_field(&pvf->expr, 0, pv->srcdata, field, ~0UL, NULL)) RETURN_ERR_MEMORY;
		sprintf(field, "_EVA_%sLABEL", pos);
		pvf->label = dyntab_field_val(pv->srcdata, field, ~0UL, 0);
		if(!strcmp(pos, "DATA"))
			pvf->decimals = atoi(DYNTAB_FIELD_VAL(pv->srcdata, DECIMALS));
		else
		{
			sprintf(field, "_EVA_%sTRUNC", pos);
			pvf->trunc = dyntab_field_val(pv->srcdata, field, ~0UL, 0);
		}
		pvf->stackcols_limit = 10;
		if(*pos == 'D') DYNTAB_FIELD(&pvf->function, pv->srcdata, FUNCTION);
		snprintf(add_sz_str(pvf->datatable), "%s0", pos);
	}
	else if(!strcmp(srctype, "_EVA_CURRENTOBJ"))
	{
		/* Current object : read corresponding object data - return if empty */
		sprintf(field, "_EVA_SEL%s_FIELD", pos);
		if(get_current_obj_field_data(cntxt, dyntab_field_val(pv->srcdata, field, ~0UL, 0),
											&pvf->srcdata, &idlist)) STACK_ERROR;
		if(!pvf->srcdata.nbrows) RETURN_OK;
		pvf->pos = pos;
		snprintf(add_sz_str(pvf->datatable), "%s0", pos);
		if(pivottable_get_indic_definition(cntxt, i_ctrl, pv, pvf)) STACK_ERROR;
	}
	else
	{
		/* Existing indicator : read corresponding object data - return if empty */
		unsigned long i;
		sprintf(field, "_EVA_SEL%s_OBJ", pos);
		if(dyntab_filter_field(&idlist, 0, pv->srcdata, field, ~0UL, NULL)) RETURN_ERR_MEMORY;
		if(!dyntab_sz(&idlist, 0, 0)) dyntab_free(&idlist);
		if(!pvf->srcdata.nbrows && !idlist.nbrows) RETURN_OK;

		/* Read field data for indicators */
		for(i = 0; i < idlist.nbrows && i < maxnb; i++)
		{
			/* Initialize pivot field data */
			PivotTableField *pvf1 = pvftbl + *pvfnb + i;
			pvf1->pos = pos;
			snprintf(add_sz_str(pvf1->datatable), "%s%lu", pos, i);
			if(!pvf1->srcdata.nbrows && 
				qry_obj_field(cntxt, &pvf1->srcdata, DYNTAB_TOULRC(&idlist, i, 0), NULL) ||
				pivottable_get_indic_definition(cntxt, i_ctrl, pv, pvf1)) STACK_ERROR;
		}
		*pvfnb += i - 1;
	}

	/* Set initial field for computation */
	*dest = pvf;
	(*pvfnb)++;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pvt_eval_relfield
** Description : replace expression by its value in the given table
*********************************************************************/
#define ERR_FUNCTION "pvt_eval_relfield"
#define ERR_CLEANUP	M_FREE(expr)
int pvt_eval_relfield(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *tab					/* in/out : table containing relfield definition */
){
	DynBuffer *expr = NULL;
	DynTableCell *c = dyntab_cell(tab, 0, 0);
	if(!c || !c->txt || !c->len) RETURN_OK;
	DYNBUF_ADD_CELLP(&expr, c, NO_CONV);
	if(dyntab_from_list(tab, expr->data, expr->cnt, ",", 0, 2)) STACK_ERROR;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pvt_load_definition
** Description : fill PivotTable structure from given attributes
*********************************************************************/
#define ERR_FUNCTION "pvt_load_definition"
#define ERR_CLEANUP	DYNTAB_FREE(data)
int pvt_load_definition(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	PivotTable *pv,					/* in : pivot table data */
	DynTable *pvtdata				/* in : pivot table definition data */
){
	DynTable data = {0};
	char *srctype = DYNTAB_FIELD_VAL(pvtdata, SELFILTER);
	unsigned long srcobj = strtoul(DYNTAB_FIELD_VAL(pvtdata, BASE_OBJ), NULL, 10);

	/* Use template object data if applicable */
	if(srcobj)
	{
		if(qry_cache_idobj(&data, srcobj)) STACK_ERROR;
		if(qry_complete_data(cntxt, pvtdata, &data, NULL, NULL)) STACK_ERROR;
	}

	/* Initialize pivot table structure */
	pv->srcdata = pvtdata;
	pv->label = DYNTAB_FIELD_VAL(pvtdata, LABEL);

	/* Read notes contents */
	DYNTAB_FIELD(&pv->dynnotes, pvtdata, LABEL_CALC);
	if(!pv->dynnotes.nbrows) DYNTAB_FIELD(&pv->dynnotes, &cntxt->form->objdata, LABEL_CALC);
	{
		char *notes = DYNTAB_FIELD_VAL(pvtdata, EXTRA_NOTES);
		pv->tblnotes = (pv->dynnotes.nbrows ? TblNote_dynamic : 0) |
					   (!strcmp(notes, "_EVA_NOTES") ? TblNote_allnote :
						!strcmp(notes, "_EVA_DETAIL") ? TblNote_alldetail :
						!strcmp(notes, "_EVA_MINIMAL") ? TblNote_minimal :
						!strcmp(notes, "_EVA_NONE") ? 0 : TblNote_note);
	}

	/* Read initial filter definition */
	if(!strcmp(srctype, "_EVA_SELFILTER_OBJ"))
	{
		/* Existing conditional expression */
		DYNTAB_FIELD(&pv->filters, pvtdata, FILTERS);
	}
	else if(!strcmp(srctype, "_EVA_CURRENTOBJ"))
	{
		/* Current object : get data from given field */
		DYNTAB_FIELD(&data, pvtdata, SELFILTER_FIELD);
		if(form_eval_valtype(cntxt, &pv->filters, srctype, NULL, &data)) 
		{
			CLEAR_ERROR;
			DYNTAB_ADD_INT(&pv->filters, 0, 0, 0);
		}
	}
	else
	{
		/* Simple : read filter params from pvtdata */
		DYNTAB_FIELD(&pv->forms, pvtdata, FORMS);
		DYNTAB_FIELD(&pv->filters, pvtdata, FILTERS);
		pv->condexpr = DYNTAB_FIELD_VAL(pvtdata, CONDEXPR);
	}
	if(!dyntab_sz(&pv->forms, 0, 0)) dyntab_free(&pv->forms);
	if(!dyntab_sz(&pv->filters, 0, 0)) dyntab_free(&pv->filters);

	/* Read fields definition & relations */
	if(pivottable_get_field_definition(cntxt, i_ctrl, pv, "DATA") ||
		pivottable_get_field_definition(cntxt, i_ctrl, pv, "ROW") ||
		pivottable_get_field_definition(cntxt, i_ctrl, pv, "COL")) STACK_ERROR;

	/* Read relations definition */
	pv->rowdata_relmode = DYNTAB_FIELD_VAL(pvtdata, ROWDATA_RELMODE);
	DYNTAB_FIELD(&pv->rowdata_relfield, pvtdata, ROWDATA_RELFIELD);
	if(pvt_eval_relfield(cntxt, &pv->rowdata_relfield)) STACK_ERROR;
	pv->coldata_relmode = DYNTAB_FIELD_VAL(pvtdata, COLDATA_RELMODE);
	DYNTAB_FIELD(&pv->coldata_relfield, pvtdata, COLDATA_RELFIELD);
	if(pvt_eval_relfield(cntxt, &pv->coldata_relfield)) STACK_ERROR;
	pv->rowcol_relmode = DYNTAB_FIELD_VAL(pvtdata, ROWCOL_RELMODE);
	DYNTAB_FIELD(&pv->rowcol_relfield, pvtdata, ROWCOL_RELFIELD);
	if(pvt_eval_relfield(cntxt, &pv->rowcol_relfield)) STACK_ERROR;

	/* Set list to use for values clic */
	pv->clicmode = DYNTAB_FIELD_VAL(pvtdata, CLICMODE);
	pv->objlist =strtoul(DYNTAB_FIELD_VAL(pvtdata, PVTLIST_CTRL), NULL, 10);
	if(!pv->clicmode[0])
	{
		PivotTableField *pvf = (pv->data && pv->data->clicmode && pv->data->clicmode[0]) ? pv->data : 
								(pv->row && pv->row->clicmode && pv->row->clicmode[0]) ? pv->row :
								(pv->col && pv->col->clicmode && pv->col->clicmode[0]) ? pv->col : NULL;
		if(pvf)
		{
			pv->clicmode = pvf->clicmode;
			pv->objlist = pvf->objlist;
		}
	}
	if(!strcmp(pv->clicmode, "_EVA_NONE")) pv->clicmode = "";

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_pivottable
** Description : output a statistics pivot table
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_pivottable"
#define ERR_CLEANUP	DYNTAB_FREE(cgival); \
					M_FREE(name); \
					pivottable_free(cntxt, pv); \
					cntxt->sql_trace = sql_trace
int ctrl_output_pivottable(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	DynTable *pvtdata				/* in : pivot table definition data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer **html = form->html;
	DynTable cgival = { 0 };
	DynBuffer *name = NULL;
	char *btn = i_ctrl == form->i_ctrl_clic ? CGI_CLICK_BTN_SUBFIELD : "";
	int sql_trace = cntxt->sql_trace;

	/* Initialize pivot table */
	PivotTable _pv = {0}, *pv = &_pv;

	/* Handle empty data */
	if(!pvtdata || !pvtdata->nbrows)
	{
		DYNBUF_ADD_STR(html, "No data");
		RETURN_OK;
	}

	/* Read pivot table definition */
	if(pvt_load_definition(cntxt, i_ctrl, pv, pvtdata)) STACK_ERROR;

	/* Handle debug */
	if(form_get_field_values(cntxt, &cgival, "_EVA_DEBUG_SQL", 0, 0)) STACK_ERROR; 
	if(*dyntab_val(&cgival, 0, 0) == '1' || *DYNTAB_FIELD_VAL(pvtdata, DEBUG_SQL) == '1')
		cntxt->sql_trace = DEBUG_SQL_RES;

	/* Read values list status & handle values clic */
	if(!strncmp(btn, add_sz_str("PVTLIST=")))
	{
		/* Value clic : store clic info in CGI data */
		pv->rowcolclic = btn + 8;
		CTRL_PUT_CGI_SUBFIELD_VAL("PVTLIST", pv->rowcolclic, 0);
	}
	else if(!strcmp(btn, "PVTLISTOFF"))
	{
		/* Click on 'Return to table' button - clear values list CGI data */
		CTRL_GET_CGI_SUBFIELD("PVTLIST");
		CTRL_GET_CGI_SUBFIELD("STATUS");
	}
	else
	{
		/* Other cases : check for values list status */
		CTRL_GET_CGI_SUBFIELD1("PVTLIST");
		if(cgival.nbrows && DYNTAB_TOUL(&cgival) == pvtdata->cell->IdObj)
			pv->rowcolclic = dyntab_val(&cgival, 0, 0);
	}

	/* Values list is displayed : output list if pivot table is the one clicked */
	if(pv->rowcolclic)
	{
		if(strtoul(pv->rowcolclic, NULL, 10) == pvtdata->cell->IdObj &&
			ctrl_calc_output_pvtcell_objlist(cntxt, i_ctrl, pv)) STACK_ERROR;
		RETURN_OK;
	}

	/* Calc pivot table - return on error */
	CTRL_GET_CGI_SUBFIELD("PVTLIST");
	if(pivottable_calc(cntxt, pv))
	{
		DYNBUF_ADD(html, cntxt->err.text, 0, TO_HTML);
		CLEAR_ERROR;
		RETURN_OK;
	}

	/* Output pivot table */
	if(ctrl_calc_output_pvtres(cntxt, i_ctrl, pv)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_output_stats_fields
** Description : output statistics on selected objects
*********************************************************************/
#define ERR_FUNCTION "ctrl_output_stats_fields"
#define ERR_CLEANUP	
int ctrl_output_stats_fields(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ctrl = 0;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_pivottable_selector
** Description : output control using multiple tables selector
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_pivottable_selector"
#define ERR_CLEANUP	DYNTAB_FREE(cgival); \
					DYNTAB_FREE(optlist); \
					M_FREE(name); \
					M_FREE(buf)
int ctrl_add_pivottable_selector(		/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	DynTable *idlist					/* in : list of pivot tables object Ids */
){							
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = {0};
	DynTable optlist = {0};
	DynBuffer *name = NULL;
	DynBuffer *buf = NULL;
	unsigned long i, i_sel = 0, objsel;
	unsigned long idlbl = get_id_value(cntxt, add_sz_str("_EVA_LABEL"));
	DynBuffer **html = form->html;

	/* Get current selection */
	CTRL_GET_CGI_SUBFIELD("PVTSELECT");
	objsel = DYNTAB_TOUL(&cgival);

	/* For each pivot table */
	for(i = 0; i < idlist->nbrows; i++)
	{
		/* Read pivot table label & build selector list options */
		unsigned long idobj = DYNTAB_TOULRC(idlist, i, 0);
		if(qry_obj_idfield(cntxt, &cgival, idobj, idlbl)) STACK_ERROR;
		DYNTAB_ADD_CELL(&optlist, i, 0, idlist, i, 0);
		DYNBUF_ADD_INT(&name, i + 1);
		DYNBUF_ADD3_CELL(&name, " - ", &cgival, 0, 0, NO_CONV, "");
		DYNTAB_ADD_BUF(&optlist, i, 1, name);
		M_FREE(name);
		if(objsel == idobj) i_sel = i;
	}

	/* Handle buttons click */
	{
		char *btn = i_ctrl == form->i_ctrl_clic ? CGI_CLICK_BTN_SUBFIELD : "";
		if(!strcmp(btn, "PVTNEXT")) i_sel++;
		else if(!strcmp(btn, "PVTPREV")) i_sel--;
	}
	if(i_sel >= optlist.nbrows) i_sel = 0;

	/* Output enclosing table header */
	DYNBUF_ADD_STR(form->html, "<table><tr><td>");

	/* Output selector list */
	DYNTAB_FREE(cgival);
	DYNTAB_ADD_CELL(&cgival, 0, 0, &optlist, i_sel, 0);
	CTRL_CGINAMESUBFIELD(&name, NULL, "PVTSELECT");
	if(ctrl->LINES > optlist.nbrows) ctrl->LINES = optlist.nbrows;
	else if(!ctrl->LINES) ctrl->LINES = optlist.nbrows < 5 ? optlist.nbrows : 5;
	if(put_html_list(cntxt, name, &optlist, &cgival, ctrl->LINES, 0, 0, 1, 1, NULL)) STACK_ERROR;

	/* Output page buttons */
	DYNBUF_ADD_STR(html, "</td><td>");
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("PVTPREV"));
	if(put_html_button(cntxt, name->data, "Page précédente", "arrow_left_yellow.gif", "arrow_left_yellow_s.gif",
										"Passe au tableau précédent", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, " ");
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("PVTNEXT"));
	if(put_html_button(cntxt, name->data, "Page suivante", "arrow_right_yellow.gif", "arrow_right_yellow_s.gif",
										"Passe au tableau suivant", 0, 0)) STACK_ERROR;

	/* Output export button */
	DYNBUF_ADD_STR(form->html, "</td><td>");
	DYNBUF_ADD_STR(&buf, "EXPORTPVT=");
	dyntab_to_dynbuf(idlist, &buf, 0, 0, ",", 1, NO_CONV);
	CTRL_CGINAMEBTN(&name, NULL, DYNBUF_VAL_SZ(buf));
	if(put_html_button(cntxt, name->data, "Rapport", "btn_book.gif", "btn_book_s.gif",
						"Produit l'ensemble des tableaux dans un document Excel et Word", 0, 12)) STACK_ERROR;
	DYNBUF_ADD_STR(form->html, "</td></tr></table>");
 
	/* Read table definition data */
	if(qry_obj_idfield(cntxt, &cgival, DYNTAB_TOULRC(&optlist, i_sel, 0), 0)) STACK_ERROR;

	/* Handle table type */
	if(!strcmp(DYNTAB_FIELD_VAL(&cgival, TYPE), "_EVA_LIST_SIMPLE"))
	{
		/* Output list table */
		DynTable tmp = {0};
		memcpy(&tmp, &ctrl->attr, sizeof(tmp));
		memcpy(&ctrl->attr, &cgival, sizeof(tmp));
		if(ctrl_add_symbol_btn(cntxt, ctrl, NULL, &cgival, 0, NULL, "LABEL+SYMBOL+OBJNOTES") ||
			table_read_controls(cntxt, i_ctrl, &cgival) ||
			table_process_controls(cntxt, i_ctrl) ||
			table_put_html_obj_list(cntxt, i_ctrl)) CLEAR_ERROR;
		memcpy(&ctrl->attr, &tmp, sizeof(tmp));
	}
	else
		/* Call pivot table display function */
		if(ctrl_output_pivottable(cntxt, i_ctrl, &cgival)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_export_pivottable
** Description : output pivot table data as html & parameters file
*********************************************************************/
#define ERR_FUNCTION "ctrl_export_pivottable"
#define ERR_CLEANUP	M_FREE(export); \
					DYNTAB_FREE(data)
int ctrl_export_pivottable(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *outfile,					/* out: output data file name */
	PivotTable *pv,					/* in/out : pivot table data */
	DynTable *pvtdata				/* in : pivot table definition data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *export = NULL;
	FILE *f;
	char filename[4096];
	DynTable data = { 0 };
	DynTableCell *c;

	/* Build file name */
	snprintf(add_sz_str(filename), "%s.htm", pv->label ? pv->label : "Statistiques");
	file_compatible_name(filename);

	/* Output pivot table params to buffer */
	DYNBUF_ADD3(&pv->exportparams, "DataFile\t", filename, 0, NO_CONV, "\t");
	DYNBUF_ADD_INT(&pv->exportparams, pvtdata->cell->IdObj);
	DYNBUF_ADD3_CELLP(&pv->exportparams, "\t", DYNTAB_FIELD_CELL(pvtdata, GRAPH_SEL), NO_CONV, "\t");
	c = DYNTAB_FIELD_CELL(pvtdata, 2DGRAPH_TYPE);
	if(c && !strcmp(c->txt, "_EVA_GRAPH_PERSO"))
	{
		if(form_eval_fieldexpr(cntxt, &data, 0, pvtdata->cell->IdObj, add_sz_str("_EVA_GRAPH_PERSO->_EVA_FILE_SERVER_NAME"), pvtdata, 0)) STACK_ERROR;
 		DYNBUF_ADD_CELL(&pv->exportparams, &data, 0, 0, NO_CONV);
	}
	else
		DYNBUF_ADD_CELLP(&pv->exportparams, c, NO_CONV);
	DYNBUF_ADD3_CELLP(&pv->exportparams, "\t", DYNTAB_FIELD_CELL(pvtdata, GRAPH_WORD_FMT), NO_CONV, "\n");

	/* Output results to buffer */
	form->html = &export;
	if(pivottable_calc(cntxt, pv)) CLEAR_ERROR_RETURN;
	if(ctrl_calc_output_pvtres(cntxt, i_ctrl, pv)) STACK_ERROR;
	form->html = NULL;

	/* Add display parameters */
	DYNBUF_ADD3_CELLP(&pv->exportparams, "DisplayParams\t", DYNTAB_FIELD_CELL(pv->srcdata, XL_LBL_WIDTH), NO_CONV, "\t");
	DYNBUF_ADD3_CELLP(&pv->exportparams, "", DYNTAB_FIELD_CELL(pv->srcdata, XL_COL_WIDTH), NO_CONV, "\t");
	DYNBUF_ADD3_CELLP(&pv->exportparams, "", DYNTAB_FIELD_CELL(pv->srcdata, XL_COL_ORIENT), NO_CONV, "\t");
	DYNBUF_ADD3_CELLP(&pv->exportparams, "", DYNTAB_FIELD_CELL(pv->srcdata, XL_COL_HEIGHT), NO_CONV, "\n");

	/* Output buffer to HTML file */
	f = fopen(filename, "w");
	if(!f) RETURN_ERR_DIRECTORY(ERR_PUT_TXT("Export stats : ", filename, 0));
	fprintf(f, "<html><body><font face=%s>%s</font></body></html>",
				ctrl->FONTFACE, export ? export->data : "");
	fclose(f);

	/* Output parameters file */
	f = fopen("dumpfmt.txt", "a");
	if(!f) RETURN_ERR_DIRECTORY(ERR_PUT_TXT("Export stats : dumpfmt.txt", "", 0));
	fputs(pv->exportparams->data, f);
	fputs("\n", f);
	fclose(f);
	if(outfile) strcpy(outfile, filename);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_output_stats
** Description : handles OUTPUT controls of type STATLIST
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_output_stats"
#define ERR_CLEANUP DYNTAB_FREE(idlist); \
					DYNTAB_FREE(objdata); \
					pivottable_free(cntxt, &pv); \
					cntxt->sql_trace = sql_trace
int ctrl_add_output_stats(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable idlist = { 0 };
	DynTable objdata = { 0 };
	PivotTable pv = {0};
	char *stats_type = CTRL_ATTR_VAL(STATS_TYPE);
	int sql_trace = cntxt->sql_trace;
	char *btn = i_ctrl == form->i_ctrl_clic ? CGI_CLICK_BTN_SUBFIELD : "";

	switch(form->step)
	{
	case CtrlRead:
		/* Ignore field name */
		ctrl->FIELD = "";
		if(cgi_build_basename(cntxt, &ctrl->cginame, i_ctrl, 'D')) STACK_ERROR;

		/* Handle click on 'Export' buttons */
		if(!strncmp(btn, add_sz_str("EXPORTPVT=")))
		{
			char fname[4096] = {0}; size_t sz_fname;
			char filename[4096] = {0}; size_t sz_filename;
			char htmname[4096] = {0};
			struct stat fs = {0};
			unsigned long i;
			int b_err;
			char *lbl = DYNTAB_FIELD_VAL(&form->objdata, LABEL);

			/* Read list of tables to output */
			if(chdir_user_doc(cntxt)) STACK_ERROR;
			remove("dumpfmt.txt");
			btn += 10;
			if(dyntab_from_list(&idlist, btn, strlen(btn), ",", 0, 0)) RETURN_ERR_MEMORY;

			/* Output page header */
			put_html_page_header(cntxt, NULL, NULL, NULL, 3);
			printf(
				"<font face=Arial><hr><center>"
				"<b>Export de données statistiques - %s</b></center><hr>", lbl);

			/* Loop on each table to output */
			for(i = 0; i < idlist.nbrows; i++)
			{
				/* Read pivot table definition */
				unsigned long idobj = DYNTAB_TOULRC(&idlist, i, 0);
				if(idobj == DYNTAB_TOUL(&form->id_obj))
				{
					if(get_current_obj_field_data(cntxt, NULL, &objdata, NULL)) STACK_ERROR;
				}
				else
				{
					if(qry_obj_field(cntxt, &objdata, idobj, NULL)) STACK_ERROR;
				}

				/* Output status message */
				printf("<li>%lu / %lu : %s</li>\n", i + 1, idlist.nbrows, DYNTAB_FIELD_VAL(&objdata, LABEL)); fflush(stdout);

				/* Handle table type */
				if(!strcmp(DYNTAB_FIELD_VAL(&objdata, TYPE), "_EVA_LIST_SIMPLE"))
				{
					/* Export list contents */
					DynTable tmp = {0};
					char filename[64];
					snprintf(add_sz_str(filename), "%lu.txt", idobj);
					memcpy(&tmp, &ctrl->attr, sizeof(tmp));
					memcpy(&ctrl->attr, &objdata, sizeof(tmp));
					if(table_read_controls(cntxt, i_ctrl, NULL) ) STACK_ERROR;
					if(table_init_obj_list(cntxt, i_ctrl, NULL)) STACK_ERROR;
					ctrl->objtbl->line = 0;
					ctrl->objtbl->lines = ctrl->objtbl->totlines;
					if(table_read_obj_list(cntxt, i_ctrl, 0)) STACK_ERROR;
					if(table_prepare_rows(cntxt, i_ctrl, "")) STACK_ERROR;
					if(file_write_tabrc(cntxt, &ctrl->objtbl->cellval, "dump.txt")) CLEAR_ERROR;
					memcpy(&ctrl->attr, &tmp, sizeof(tmp));
					table_free(ctrl->objtbl);
					M_FREE(ctrl->objtbl);
					DYNBUF_ADD3(&pv.exportparams, "DataFile\t", filename, 0, NO_CONV, "\t");
					DYNBUF_ADD3_INT(&pv.exportparams, "", pv.srcdata->cell->IdObj, "\n");
				}
				else
				{
					/* Export pivot table contents */
					pivottable_free(cntxt, &pv);
					if(pvt_load_definition(cntxt, i_ctrl, &pv, &objdata)) STACK_ERROR;
					if(ctrl_export_pivottable(cntxt, i_ctrl, htmname, &pv, &objdata)) STACK_ERROR;
				}
			}

			/* Launch excel file preparation */
			printf("<li>Mise au format Office</li>\n"); fflush(stdout);
			if(office_launchproc(cntxt, "_EVA_EXCEL", "TableStats.xls")) CLEAR_ERROR;

			/* Prepare file name & cleanup temp files */
			b_err = stat("TableStatsRes.xls", &fs);
			remove("TableStats.xls");
			remove("dumpfmt.txt");
			if(idlist.nbrows > 1)
				sz_fname = snprintf(add_sz_str(fname), "%s-%s", cntxt->dbname, *lbl ? lbl : ctrl->LABEL[0] ? ctrl->LABEL : "Rapport statistique");
			else
				sz_fname = snprintf(add_sz_str(fname), "%s-%s", cntxt->dbname, pv.label ? pv.label : "Tableau statistique");

			DYNBUF_ADD_STR(&cntxt->html, "<hr>");

			/* Output error message if applicable */
			if(b_err)
			{
				DYNBUF_ADD_STR(&cntxt->html,
					"Erreur dans Microsoft Excel - le fichier n'a pas pu être produit au format Office<br><br>"
					"Vous pouvez télécharger le fichier au format HTML et l'ouvrir avec un tableur sur votre poste<br><br>");
				rename(htmname, "TableStatsRes.xls");
			}
			remove(htmname);

			/* Output download dialog for word file if applicable */
			if(idlist.nbrows > 1)
			{
				sprintf(filename, "%s.doc", fname);
				sz_filename = file_compatible_name(filename);
				remove(filename);
				rename("TableStatsRes.doc", filename);
				if(file_output_link(cntxt, &cntxt->html,
							add_sz_str("<b><u>Fichier document</u></b> : "),
							fname, sz_fname, filename, sz_filename, "#user", NULL, 0, NULL, 0, 3)) STACK_ERROR;
			}

			/* Output download dialog for excel file */
			sprintf(filename, "%s.xls", fname);
			sz_filename = file_compatible_name(filename);
			remove(filename);
			rename("TableStatsRes.xls", filename);
			if(file_output_link(cntxt, &cntxt->html,
						add_sz_str("<b><u>Fichier tableur</u></b> : "),
						fname, sz_fname, filename, sz_filename, "#user", NULL, 0, NULL, 0,
						(b_err ? 7 : 6) | (form->b_modified ? 0 : 8))) STACK_ERROR;
			DYNBUF_ADD_STR(&cntxt->html, "<br><br>");
			cntxt->b_terminate = 15;
		}
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		if(!form->html) break;
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		if(*CTRL_ATTR_VAL(DEBUG_SQL) == '1') cntxt->sql_trace = DEBUG_SQL_RES;

		/* Call apropriate function for statistics type */
		if(!strcmp(stats_type, "_EVA_STATS_FORM"))
		{
			if(ctrl_output_stats_fields(cntxt, i_ctrl)) STACK_ERROR;
		}
		else if(!strcmp(stats_type, "_EVA_PVT_LIST"))
		{
			/* Pivot tables list : output tables */
			unsigned long i;
			CTRL_ATTR(idlist, PVT_LIST);

			/* Handle display mode */
			if(CTRL_ATTR_CELL(DISPMODE) && idlist.nbrows > 1)
			{
				/* Selector : call selector display function */
				if(ctrl_add_pivottable_selector(cntxt, i_ctrl, &idlist)) STACK_ERROR
			}
			else
			{
				/* No selector : read each pivot table and call display function */
				for(i = 0; i < idlist.nbrows; i++)
					if(qry_obj_idfield(cntxt, &objdata, DYNTAB_TOULRC(&idlist, i, 0), 0) ||
						ctrl_output_pivottable(cntxt, i_ctrl, &objdata)) STACK_ERROR;
			}
		}
		else if(!strcmp(stats_type, "_EVA_CURRENTOBJ"))
		{
			/* Pivot table defined with current object : check for source field */
			DynTableCell *c = CTRL_ATTR_CELL(SELOBJ_FIELD);
			if(c && strpbrk(c->txt, ".-([") != NULL)
			{
				if(form_eval_fieldexpr(cntxt, &idlist, 0, 0, c->txt, c->len, NULL, 0)) STACK_ERROR;
				if(ctrl_add_pivottable_selector(cntxt, i_ctrl, &idlist)) STACK_ERROR;
			}
			else
			{
				/* Current object : call display function with read data */
				if(get_current_obj_field_data(cntxt, c ? c->txt : "", &objdata, &idlist)) STACK_ERROR;
				if(idlist.nbrows > 1)
				{
					/* Multiple tables : handle table selection */
					if(ctrl_add_pivottable_selector(cntxt, i_ctrl, &idlist)) STACK_ERROR;
				}
				else
				{
					/* Single table : call display function with read data */
					if(ctrl_output_pivottable(cntxt, i_ctrl, &objdata)) STACK_ERROR;
				}
			}
		}
		else
		{
			/* Pivot table defined with current control : call display function with control attributes */
			if(ctrl_output_pivottable(cntxt, i_ctrl, &ctrl->attr)) STACK_ERROR;
		}

		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
