/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : pivottable.c
** Description : handling functions for pivot tables & statistics
**      Author : Alain BOUTE
**     Created : April 2 2004
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : pivottablefield_free
** Description : free data alloc-ed for a pivot table
*********************************************************************/
void pivottablefield_free(
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTableField *pvf					/* in/out : pivot table data */
){
	if(!pvf) return;
	dyntab_free(&pvf->expr);
	dyntab_free(&pvf->labels);
	dyntab_free(&pvf->totals);
	dyntab_free(&pvf->forms);
	dyntab_free(&pvf->filters);
	dyntab_free(&pvf->selctrl);
	dyntab_free(&pvf->function);
	dyntab_free(&pvf->totlabel);
	dyntab_free(&pvf->function);
	dyntab_free(&pvf->percent);
	dyntab_free(&pvf->srcdata);
	dyntab_free(&pvf->graphfile);
	dyntab_free(&pvf->slices);
	dyntab_free(&pvf->sr_src);
	dyntab_free(&pvf->sr_dest);
	dyntab_free(&pvf->sr_pos);
	dyntab_free(&pvf->optsrc);
	dyntab_free(&pvf->emptyval);
	M_FREE(pvf->where);
	sql_drop_table(cntxt, pvf->datatable);
	sql_drop_table(cntxt, pvf->objtable);
	M_FREE(pvf->vmin);
	M_FREE(pvf->vmax);
	memset(pvf, 0, sizeof(*pvf));
}

/*********************************************************************
** Function : pivottable_free
** Description : free data alloc-ed for a pivot table
*********************************************************************/
void pivottable_free(
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv							/* in/out : pivot table data */
){
	unsigned long irow, icol, idata;
	if(!pv) return;
	dyntab_free(&pv->forms);
	dyntab_free(&pv->filters);
	for(idata = 0; idata < MAX_PVT_DATAFIELDS; idata++)	pivottablefield_free(cntxt, &pv->datas[idata]);
	for(irow = 0; !irow || irow < pv->nbrow; irow++) pivottablefield_free(cntxt, &pv->rows[irow]);
	for(icol = 0; !icol || icol < pv->nbcol; icol++) pivottablefield_free(cntxt, &pv->cols[icol]);
	for(idata = 0; idata < MAX_PVT_DATAFIELDS; idata++)
		for(irow = 0; !irow || irow < pv->nbrow; irow++)
			for(icol = 0; !icol || icol < pv->nbcol; icol++) dyntab_free(&pv->restbl[idata][irow][icol]);
	sql_drop_table(cntxt, pv->objtable);
	M_FREE(pv->exportparams);
	memset(pv, 0, sizeof(*pv));
}

/*********************************************************************
** Function : get_relmode_oneway
** Description : get relation mode & field betwen tables of a pivot table
*********************************************************************/
RelationMode get_relmode_oneway(				/* return : relation mode */
	PivotTable *pv,							/* in : pivot table data */
	DynTable **field,						/* out : relation field */
	char *ltable,							/* in : source table */
	char *rtable							/* in : destination table */
){
	char *relmode = "";
	DynTable *relfield = NULL;
	if(!ltable || !rtable) return RelNone;
	if(*rtable == 'D')
	{
		if(*ltable == 'R')
		{
			relmode = pv->rowdata_relmode;
			relfield = &pv->rowdata_relfield;
		}
		else if(*ltable == 'C')
		{
			relmode = pv->coldata_relmode;
			relfield = &pv->coldata_relfield;
		}
	}
	else if(*rtable == 'R' && *ltable == 'C')
	{
		relmode = pv->rowcol_relmode;
		relfield = &pv->rowcol_relfield;
	}
	*field = relfield;
	return !*relmode ? RelNone : !strcmp("_EVA_DIRECT", relmode) ? RelDirect : RelReverse;
}

/*********************************************************************
** Function : get_relmode
** Description : get relation mod & field betwen tables of a pivot table
*********************************************************************/
RelationMode get_relmode(					/* return : relation mode */
	PivotTable *pv,							/* in : pivot table data */
	DynTable **field,						/* out : relation field */
	char *ltable,							/* in : source table */
	char *rtable							/* in : destination table */
){
	RelationMode relmode = get_relmode_oneway(pv, field, ltable, rtable);
	if(*field) return relmode;
	relmode = get_relmode_oneway(pv, field, rtable, ltable);
	return relmode == RelDirect ? RelReverse : relmode == RelReverse ? RelDirect : relmode;
}

/*********************************************************************
** Function : auto_date_unit
** Description : set automatic unit for date
*********************************************************************/
void auto_date_unit(
	PivotTableField *pvf					/* in/out : pivot table data */
){
	char fmt = 0;
	int d = datetxt_to_age(pvf->vmin, pvf->vmax, &fmt);
	if(pvf->dispunit && *pvf->dispunit) return;
	if(d > 12 && fmt == 'M') fmt = 'Y';
	else if(d > 60 && fmt == 'D') fmt = 'M';
	else if(d > 14 && fmt == 'D') fmt = 'W';
	else if(d > 48 && fmt == 'h') fmt = 'D';
	pvf->dispunit = fmt == 'm' ? "_EVA_MINUTE" :
					fmt == 'h' ? "_EVA_HOUR" :
					fmt == 'D' ? "_EVA_DAY" :
					fmt == 'W' ? "_EVA_WEEK" :
					fmt == 'M' ? "_EVA_MONTH" :
					fmt == 'Y' ? "_EVA_YEAR" :
					"_EVA_YEAR";
}

/*********************************************************************
** Function : pivottable_select
** Description : build selection table with objects matching form & SQL condition
*********************************************************************/
#define ERR_FUNCTION "pivottable_select"
#define ERR_CLEANUP DYNTAB_FREE(id_obj); \
					M_FREE(sql); \
					qry_build_free(&flt)
int pivottable_select(						/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv							/* in/out : pivot table data */
){
	DynTable id_obj = {0};
	DynBuffer *sql = NULL;
	QryBuild flt = {0};

	/* Handle formstamp & filter condition */
	if(pv->forms.nbrows || pv->filters.nbrows )
	{
		if(qry_add_filter_forms(cntxt, &flt, &pv->forms, &pv->filters)) STACK_ERROR;

		/* Build table with objects matching formstamp & filter */
		strcpy(pv->objtable, "DTmpPVT");
		DYNBUF_ADD3(&sql, 
			"-- pivottable_select - select objects matching formstamp\n"
			"CREATE TEMPORARY TABLE ", pv->objtable, 0, NO_CONV, " TYPE=HEAP\n");
		if(qry_build_clauses(cntxt, &flt, 1)) STACK_ERROR;
		if(qry_build_flt_select(cntxt, &sql, &id_obj, &flt, 0, 0)) STACK_ERROR;
		if(qry_exec_filter(cntxt, &flt, sql->data)) STACK_ERROR;	
		pv->nbobj = cntxt->sql_nbrows;
		pv->b_empty = !pv->nbobj;
		if(sql_exec_query(cntxt, "ALTER TABLE DTmpPVT ADD INDEX(IdObj)")) STACK_ERROR;
	}

	/* Handle litteral condition if not empty */
	if(pv->condexpr && *pv->condexpr)
	{
		/* Evaluate given condition */
		if(qry_filter_sql_fieldexpr(cntxt, &id_obj, pv->condexpr, pv->objtable, NULL)) CLEAR_ERROR;

		/* Delete unmatched objects */
		if(pv->nbobj > id_obj.nbrows)
		{
			M_FREE(sql);
			DYNBUF_ADD_STR(&sql, 
				"-- pivottable_select - remove objects not matching condition\n"
				"DELETE FROM DTmpPVT WHERE IdObj NOT IN (");
			if(qry_values_list(cntxt, &id_obj, 0, &sql)) STACK_ERROR;
			DYNBUF_ADD_STR(&sql, ")");
			if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
		}
		pv->nbobj = (id_obj.nbrows == 1 && !DYNTAB_TOUL(&id_obj)) ? 0 : id_obj.nbrows;
		pv->b_empty = !pv->nbobj;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_get_field_data
** Description : build datatable for the given pivot table field
*********************************************************************/
#define ERR_FUNCTION "pivottable_get_field_data"
#define ERR_CLEANUP DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(data); \
					M_FREE(sql); \
					M_FREE(tmp); \
					M_FREE(expr); \
					qry_build_free(&flt)
int pivottable_get_field_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv,							/* in/out : pivot table data */
	PivotTableField *pvf					/* in : pivot field parameters */
){
	DynTable id_obj = {0};
	DynTable data = {0};
	DynBuffer *sql = NULL;
	DynBuffer *tmp = NULL;
	DynBuffer *expr = NULL;
	QryBuild flt = {0};
	int b_linkonrelobj = 0;
	char printbuf[1024];
	int b_cal_filter = 0;

	/* Handle link to data table */
	pvf->relmode = get_relmode(pv, &pvf->relfields, pvf->srctable, pvf->datatable);
	if(pvf->relmode == RelReverse)
	{
		if(qry_add_filter(cntxt, &flt, RelReverse, NULL, NoMatch, NULL)) STACK_ERROR;
		if(qry_add_filter(cntxt, &flt, RelDirect, pvf->relfields, NoMatch, NULL)) STACK_ERROR;
		flt.srcjoin = 1;
		b_linkonrelobj = 1;
	}
	else if(pvf->relmode == RelDirect)
	{
		if(qry_add_filter(cntxt, &flt, RelDirect, pvf->relfields, NoMatch, NULL)) STACK_ERROR;
		flt.srcjoin = 0;
		b_linkonrelobj = 1;
	}

	/* Handle formstamp & filter condition */
	flt.srctable = pvf->srctable;
	if(qry_add_filter_forms(cntxt, &flt, &pvf->forms, &pvf->filters)) STACK_ERROR;

	/* Build temporary list of matching & related objects if applicable */
	if(flt.nbnode || flt.node->idobjmatch.nbrows)
	{
		snprintf(add_sz_str(pvf->objtable), "%cTmp%s", *pvf->pos, pvf->datatable);
		DYNBUF_ADD3(&sql, 
			"-- pivottable_add_field - Create field idobj table\n"
			"CREATE TEMPORARY TABLE ", pvf->objtable, 0, NO_CONV, " TYPE=HEAP\n");
		cntxt->sql_nbrows = 0;
		if(flt.nbnode)
		{
			if(qry_build_clauses(cntxt, &flt, pvf->relmode == RelNone ? 1 : b_linkonrelobj ? 3 : 2)) STACK_ERROR;
			if(qry_build_flt_select(cntxt, &sql, &id_obj, &flt, 0, 0)) STACK_ERROR;
			if(qry_exec_filter(cntxt, &flt, sql->data)) STACK_ERROR;	
		}
		else
		{
			DYNBUF_ADD_STR(&sql, "SELECT DISTINCT IdObj FROM TLink WHERE IdObj IN (");
			if(dyntab_to_dynbuf(&flt.node->idobjmatch, &sql, 0, 0, ",", 1, NO_CONV)) STACK_ERROR;
			DYNBUF_ADD_STR(&sql, ")");
			if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;	
		}
		pvf->nbsrcobj = cntxt->sql_nbrows;

		/* Add index to list of matching object table */
		snprintf(add_sz_str(printbuf), "ALTER TABLE %s ADD INDEX(IdObj)", pvf->objtable);
		if(sql_exec_query(cntxt, printbuf)) STACK_ERROR;
	}

	/* Prepare query for field values table */
	sql_drop_table(cntxt, pvf->datatable);
	M_FREE(sql);
	DYNBUF_ADD3(&sql, 
		"-- pivottable_add_field - Create field values table\n"
		"CREATE TEMPORARY TABLE ", pvf->datatable, 0, NO_CONV, "\n");

	/* Handle multiple fields : concat fields */
	if(pvf->expr.nbrows > 1)
	{
		if(dyntab_to_dynbuf(&pvf->expr, &expr, NULL, 0, ",", 1, NO_CONV)) RETURN_ERR_MEMORY; 
		dyntab_free(&pvf->expr);
		DYNTAB_ADD_BUF(&pvf->expr, 0, 0, expr);
	}

	/* Handle SQL expression */
	if(strchr(dyntab_val(&pvf->expr, 0, 0), '[') || !strncmp(dyntab_val(&pvf->expr, 0, 0), "SELECT ", 7))
	{
		/* Build table of base objects if applicable */
		char *srcobj = "TmpExpr";
		char *dsttbl = pvf->datatable;
		if(!*pvf->objtable && pvf->srctable)
		{
			snprintf(add_sz_str(pvf->objtable), "%cTmp%s", *pvf->pos, pvf->datatable);
			snprintf(add_sz_str(printbuf),
				"-- pivottable_add_field - Build table of base objects for SQL expression\n"
				"CREATE TEMPORARY TABLE %s TYPE=HEAP\n"
				"SELECT DISTINCT IdObj FROM %s", 
				pvf->objtable, pvf->srctable);
			if(sql_exec_query(cntxt, printbuf) || sql_get_table(cntxt, &id_obj, 2)) STACK_ERROR;
			pvf->nbsrcobj = cntxt->sql_nbrows;
		}
		if(pvf->objtable && pvf->objtable[0] && pvf->relmode != RelNone)
		{
			snprintf(add_sz_str(printbuf),
				"-- pivottable_add_field - Build table of base objects for SQL expression\n"
				"CREATE TEMPORARY TABLE TmpExpr TYPE=HEAP\n"
				"SELECT DISTINCT %s AS IdObj FROM %s",
				pvf->relmode == RelNone ? "IdObj" : "IdRelObj",
				pvf->objtable);
			if(sql_exec_query(cntxt, printbuf)) STACK_ERROR;
			dsttbl = "TmpValExpr";
		}
		else if(pvf->objtable && pvf->objtable[0]) srcobj = pvf->objtable;
		else if(!id_obj.nbrows) srcobj = NULL;
		else if(qry_make_idlist(cntxt, &id_obj, "TmpExpr")) STACK_ERROR;

		/* Evaluate SQL expression on base objects */
		if(qry_eval_sql_fieldexpr(cntxt, dsttbl, dyntab_val(&pvf->expr, 0, 0), srcobj, &pvf->srcdata)) STACK_ERROR;
		sql_drop_table(cntxt, "TmpExpr");

		/* Build values table if applicable */
		if(dsttbl != pvf->datatable)
		{
			snprintf(add_sz_str(printbuf),
				"-- pivottable_add_field - Build values table\n"
				"CREATE TEMPORARY TABLE %s TYPE=HEAP\n"
				"SELECT %s.IdObj,LEFT(TmpValExpr.Val,250) AS Val,TmpValExpr.Line AS Line FROM %s\n"
				"INNER JOIN TmpValExpr ON TmpValExpr.IdObj=%s.%s",
				pvf->datatable, pvf->objtable, pvf->objtable, pvf->objtable,
				pvf->relmode == RelNone ? "IdObj" : "IdRelObj");
			if(sql_exec_query(cntxt, printbuf)) STACK_ERROR;
			sql_drop_table(cntxt, "TmpValExpr");
		}
	}
	else
	{
		/* Parse field relational expression */
		qry_build_free(&flt);
		if(dyntab_sz(&pvf->expr, 0, 0))
		{
			/* Get unique idobj list from source table if applicable */
			if(!*pvf->objtable && pvf->srctable)
			{
				snprintf(add_sz_str(printbuf), "SELECT DISTINCT IdObj FROM %s", pvf->srctable);
				if(sql_exec_query(cntxt, printbuf) || sql_get_table(cntxt, &id_obj, 2)) STACK_ERROR;
				pvf->nbsrcobj = cntxt->sql_nbrows;
			}
			else
				flt.srctable = *pvf->objtable ? pvf->objtable : pvf->srctable;

			/* Parse field expression - Keep only 250 first chars */
			flt.b_recdata = 1;
			flt.b_linkonrelobj = b_linkonrelobj;
			if(qry_parse_fieldexpr(cntxt, DYNTAB_VAL_SZ(&pvf->expr, 0, 0), &flt)) STACK_ERROR;
			DYNBUF_ADD3_BUF(&tmp, "LEFT(", flt.select, NO_CONV, ",250)");
			M_FREE(flt.select);
			flt.select = tmp;
			tmp = NULL;
			flt.b_nodistinct = 1;
		}
		else
		{
			DYNBUF_ADD_STR(&flt.select, "''");
			DYNBUF_ADD_STR(&flt.optselect, "NULL AS Line");
			flt.srctable = *pvf->objtable ? pvf->objtable : pvf->srctable;
		}

		/* Handle date filter if calendar active */
		if(pvf->b_calendar) 
		{
			/* Set start date if applicable */
			size_t tr;
			if(!pvf->maxlabels) pvf->maxlabels = 12;
			if(!pvf->dispunit || strlen(pvf->dispunit) < 5) pvf->dispunit = "_EVA_YEAR";
			tr = pvf->dispunit && !strcmp(pvf->dispunit, "_EVA_YEAR") ? 4 : !strcmp(pvf->dispunit, "_EVA_MONTH") ? 6 : 8;
			if(!pvf->startdate[0])
			{
				if(!pvf->timeway) pvf->timeway = "";
				if(!*pvf->timeway) 
				{
					sprintf(printbuf, "SELECT DATE_SUB(CURDATE(),INTERVAL %ld %s)+0", 
						(!strcmp(pvf->dispunit, "_EVA_WEEK") ? 7 : 1) * (pvf->maxlabels - 1),
						!strcmp(pvf->dispunit, "_EVA_WEEK") ? "DAY" : pvf->dispunit + 5);
					if(sql_exec_query(cntxt, printbuf) || sql_get_table(cntxt, &data, 2)) STACK_ERROR;
					strncpy(pvf->startdate, dyntab_val(&data, 0, 0), tr);
				}
				else if(!strcmp(pvf->timeway, "_EVA_FUTURE"))
					strncpy(pvf->startdate, cntxt->timestamp, tr);
			}
			pvf->startdate[tr] = 0;

			b_cal_filter = pvf->startdate[0] && pvf->maxlabels && pvf->dispunit && strlen(pvf->dispunit) > 5;
			if(b_cal_filter)
			{
				char date[32];
				/* Handle align on monday if week format */
				if(!strcmp(pvf->dispunit, "_EVA_WEEK"))
				{
					time_t t;
					struct tm dt;
					if(!datetxt_to_time(pvf->startdate, &t, &dt) &&	dt.tm_wday != 1 && 
						!delay_to_datetxt(date, pvf->startdate, dt.tm_wday ? 1 - dt.tm_wday : 6, 'D'))
						strcpy(pvf->startdate, date);
					pvf->startdate[8] = 0;
				}
				datetxt_to_format(cntxt, date, pvf->startdate, "_EVA_DATE_SORT");
				sprintf(printbuf, "SELECT DATE_ADD('%s', INTERVAL %lu %s)+0", date,
					(!strcmp(pvf->dispunit, "_EVA_WEEK") ? 7 : 1) * pvf->maxlabels,
					!strcmp(pvf->dispunit, "_EVA_WEEK") ? "DAY" : pvf->dispunit + 5);
				if(sql_exec_query(cntxt, printbuf) || sql_get_table(cntxt, &data, 2)) STACK_ERROR;
				if(flt.where) DYNBUF_ADD_STR(&flt.where, "\nAND ");
				dynbuf_print5(&flt.where, "%s>='%s' AND %s<'%.*s'",
									flt.select->data, pvf->startdate, 
									flt.select->data, tr, dyntab_val(&data, 0, 0));
				pvf->b_showemptyval = 0;
			}
		}
		else 
			pvf->startdate[0] = 0;

		/* Create values table for the pivot field */
		if(qry_build_flt_select(cntxt, &sql, &id_obj, &flt, 0, 0)) STACK_ERROR;
		if(qry_exec_filter(cntxt, &flt, sql->data)) STACK_ERROR;	
	}

	/* Add IdObj index to values table */
	snprintf(add_sz_str(printbuf), "ALTER TABLE %s ADD INDEX(IdObj)", pvf->datatable);
	if(sql_exec_query(cntxt, printbuf)) STACK_ERROR;
	if(cntxt->sql_trace) put_debug_sqltable(cntxt, pvf->datatable);

	/* Adjust values type if applicable */
	if(pvf->sr_src.nbrows || pvf->slices.nbrows)
	{
		snprintf(add_sz_str(printbuf), "ALTER TABLE %s MODIFY Val VARCHAR(250)", pvf->datatable);
		if(sql_exec_query(cntxt, printbuf)) STACK_ERROR;
	}

	/* Get min/max values & check if field holds table (.Line) values */
	snprintf(add_sz_str(printbuf), "SELECT COUNT(DISTINCT IdObj),COUNT(IdObj),COUNT(DISTINCT Val),MIN(Val),MAX(Val)%s FROM %s",
		flt.groupfn ? "" : ",MAX(Line)", pvf->datatable);
	if(sql_exec_query(cntxt, printbuf) || sql_get_table(cntxt, &id_obj, 2)) STACK_ERROR;
	pvf->nbobj = DYNTAB_TOUL(&id_obj);
	pvf->nbval = DYNTAB_TOULRC(&id_obj, 0, 1);
	pvf->nbdistval = DYNTAB_TOULRC(&id_obj, 0, 2);
	pvf->sz_vmin = dyntab_sz(&id_obj, 0, 3);
	M_STRDUP(pvf->vmin, dyntab_val(&id_obj, 0, 3), pvf->sz_vmin);
	pvf->sz_vmax = dyntab_sz(&id_obj, 0, 4);
	M_STRDUP(pvf->vmax, dyntab_val(&id_obj, 0, 4), pvf->sz_vmax);
	pvf->b_lines = DYNTAB_TOULRC(&id_obj, 0, 5) > 1;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_add_empty_values
** Description : add empty values table for a pivot table field
*********************************************************************/
#define ERR_FUNCTION "pivottable_add_empty_values"
#define ERR_CLEANUP DYNTAB_FREE(id_obj); \
					M_FREE(sql)
int pivottable_add_empty_values(			/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv,							/* in/out : pivot table data */
	PivotTableField *pvf					/* in : pivot field parameters */
){
	DynTable id_obj = {0};
	DynBuffer *sql = NULL;
	char printbuf[1024];

	snprintf(add_sz_str(printbuf), 
		"-- pivottable_add_field - Add empty value for objects with no value\n"
		"SELECT DISTINCT IdObj FROM %s", pvf->datatable);
	if(sql_exec_query(cntxt, printbuf) || sql_get_table(cntxt, &id_obj, 2)) STACK_ERROR;
	M_FREE(sql);
	DYNBUF_ADD3(&sql, 
		"INSERT INTO ", pvf->datatable, 0, NO_CONV, " (IdObj,Val)\n");
	DYNBUF_ADD3(&sql,
		"SELECT DISTINCT IdObj,'' AS Val\n"
		"FROM ", pvf->srctable, 0, NO_CONV, "\n"
		"WHERE IdObj NOT IN (");
	if(qry_values_list(cntxt, &id_obj, 0, &sql)) STACK_ERROR;
	DYNBUF_ADD_STR(&sql, ")");
	if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
	pvf->nbobj = pv->nbobj;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_handle_field_format
** Description : format values table for a pivot table field
*********************************************************************/
#define ERR_FUNCTION "pivottable_handle_field_format"
#define ERR_CLEANUP M_FREE(sql)
int pivottable_handle_field_format(			/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTableField *pvf					/* in : pivot field parameters */
){
	DynBuffer *sql = NULL;
	char printbuf[1024];
	unsigned year, monthday;
	if(!pvf->dispfmt) pvf->dispfmt = "";
	if(!*pvf->dispfmt)
	{
		/* Auto format - use min & max values to select proper format */

		/* Check for date */
		if(pvf->sz_vmin > 5 && pvf->sz_vmax> 5 && 
			strcmp(pvf->vmin, "1900") > 0 && strcmp(pvf->vmax, "2100") < 0 &&
			!datetxt_invalid(pvf->vmin) && !datetxt_invalid(pvf->vmax))
		{
			pvf->dispfmt = "_EVA_DATE";
			auto_date_unit(pvf);

		}
	}
	if(!pvf->dispunit) pvf->dispunit = "";
	if(!strcmp(pvf->dispfmt, "_EVA_DATE") || !strcmp(pvf->dispfmt, "_EVA_AGE"))
	{
		/* Date or age format : read unit */
		auto_date_unit(pvf);
		pvf->ltrunc =	!strcmp(pvf->dispfmt, "_EVA_AGE") ? 14 :
						!strcmp(pvf->dispunit, "_EVA_MINUTE") ? 12 :
						!strcmp(pvf->dispunit, "_EVA_HOUR") ? 10 :
						!strcmp(pvf->dispunit, "_EVA_DAY") ? 8 :
						!strcmp(pvf->dispunit, "_EVA_WEEK") ? 8 :
						!strcmp(pvf->dispunit, "_EVA_MONTH") ? 6 :
						!strcmp(pvf->dispunit, "_EVA_YEAR") ? 4 : 14;
		if(!strcmp(pvf->dispfmt, "_EVA_DATE"))
		{
			if(pvf->b_overrun && pvf->ltrunc > 4)
			{
				pvf->ltrunc--;
				pvf->rtrunc = 2;
			}
		}
	}
	else if(!strcmp(pvf->dispfmt, "_EVA_TRUNC"))
	{
		sscanf(pvf->trunc, "%lu,%lu", &pvf->ltrunc, &pvf->rtrunc);
	}

	/* Truncate values as applicable */
	if(pvf->ltrunc || pvf->rtrunc)
	{
		dynbuf_print(&sql, 
			"-- pivottable_add_field - Truncate values\n"
			"UPDATE %s SET Val=", pvf->datatable);
		if(!pvf->ltrunc && pvf->rtrunc)
		{
			DYNBUF_ADD3_INT(&sql, "RIGHT(Val,", pvf->rtrunc, ")");
		}
		else if(pvf->ltrunc && pvf->rtrunc)
		{
			dynbuf_print2(&sql, "MID(Val,%lu,%lu)", pvf->ltrunc, pvf->rtrunc);
		}
		else
		{
			DYNBUF_ADD3_INT(&sql, "LEFT(Val,", pvf->ltrunc, ")");
		}
		if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
	}

	/* Transform to week if applicable */
	if(!strcmp(pvf->dispunit, "_EVA_WEEK") && !strcmp(pvf->dispfmt, "_EVA_DATE"))
	{
		M_FREE(sql);
		dynbuf_print(&sql, 
			"-- pivottable_add_field - Transform to week\n"
			"UPDATE %s SET Val=DATE_FORMAT(Val,'%%x S%%v')", pvf->datatable);
		if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
	}

	/* Transform to age if applicable */
	else if(!strcmp(pvf->dispfmt, "_EVA_AGE"))
	{
		/* Handle reference date for age calculation */
		sscanf(pvf->dateref, "%4u%4u", &year, &monthday);

		/* Transform to age */
		M_FREE(sql);
		dynbuf_print3(&sql, 
			"-- pivottable_add_field - Transform to age\n"
			"UPDATE %s SET Val=%u-LEFT(Val,4)-IF(MID(Val,5,4)<='%04u',0,1) WHERE Val<>''",
			pvf->datatable, year, monthday);
		if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
		pvf->dispfmt = "_EVA_NUMBER";
		if(!pvf->sortmode && !pvf->sortmode[0]) pvf->sortmode = "_EVA_NUMBER";
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_slice_values
** Description : format values table for a pivot table field
*********************************************************************/
#define ERR_FUNCTION "pivottable_slice_values"
#define ERR_CLEANUP M_FREE(sql)
int pivottable_slice_values(				/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTableField *pvf					/* in : pivot field parameters */
){
	DynBuffer *sql = NULL;
	char printbuf[1024];
	dynbuf_print(&sql, 
		"-- pivottable_add_field - Replace values\n"
		"UPDATE %s SET Val=", pvf->datatable);
	if(pvf->slices.nbrows == 1)
	{
		/* Only one slice : use as numerical slice width */
		unsigned long slice = DYNTAB_TOUL(&pvf->slices);
		if(slice > 1)
		{
			if(!strcmp(pvf->dispfmt, "_EVA_DATE") && pvf->b_overrun)
			{
				dynbuf_print5(&sql, 
					"CONCAT(LPAD(FLOOR((Val-1)/%lu)*%lu+1,2,'0'),' - ',LPAD(FLOOR((Val-1)/%lu)*%lu+%lu,2,'0')) WHERE Val<>''",
								slice, slice, slice, slice, slice);
				if(!*pvf->sortmode) pvf->sortmode = "_EVA_NUMBER";
				if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
				pvf->dispfmt = "";
			}
			else
			{
				dynbuf_print5(&sql, 
					"CONCAT(FLOOR(Val/%lu)*%lu,' ~ ',FLOOR(Val/%lu)*%lu+%lu) WHERE Val<>''",
								slice, slice, slice, slice, slice);
				if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
				pvf->dispfmt = "";
			}
		}
	}
	else if(pvf->slices.nbrows > 1)
	{
		unsigned long i;
		DynTableCell *s1 = dyntab_cell(&pvf->slices, 0, 0);
		DynTableCell *s2 = dyntab_cell(&pvf->slices, pvf->slices.nbrows - 1, 0);
		int n1 = s1->txt ? atoi(s1->txt) : 0;
		int n2 = s2->txt ? atoi(s2->txt) : 0;
		if(n1 || n2)
		{
			/* Numerical slices */
			dynbuf_print2(&sql, "CASE WHEN Val+0<%d THEN '%d ou -'", n1, n1 - 1);
			for(i = 1; i < pvf->slices.nbrows; i++)
			{
				n2 = atoi(dyntab_val(&pvf->slices, i, 0));
				if(n2 <= n1) continue;
				dynbuf_print4(&sql, n2 == n1 + 1 ? 
						" WHEN Val+0>=%d AND Val+0<%d THEN '%d'" :
						" WHEN Val+0>=%d AND Val+0<%d THEN '%d ~ %d'",
						n1, n2, n1, n2 - 1);
				n1 = n2;
			}
			dynbuf_print(&sql, " ELSE '%d ou +' END WHERE Val<>''", n1);
			if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
			pvf->dispfmt = "";
			if(!*pvf->sortmode) pvf->sortmode = "_EVA_NUMBER";
		}
		else if(!strcmp(pvf->dispfmt, "_EVA_DATE"))
		{
		}
		else
		{
			/* Text slices */
			DYNBUF_ADD3_CELLP(&sql, "CASE WHEN Val<'", s1, SQL_NO_QUOTE, "' THEN '");
			DYNBUF_ADD_CELLP(&sql, s1, SQL_NO_QUOTE);
			sql->data[sql->cnt - 1]--;
			DYNBUF_ADD_STR(&sql, " et -'");
			for(i = 1; i < pvf->slices.nbrows; i++)
			{
				s2 = dyntab_cell(&pvf->slices, i, 0);
				DYNBUF_ADD3_CELLP(&sql, " WHEN Val>='", s1, SQL_NO_QUOTE, "' AND Val<='");
				DYNBUF_ADD_CELLP(&sql, s2, SQL_NO_QUOTE);
				DYNBUF_ADD3_CELLP(&sql, "' THEN '", s1, SQL_NO_QUOTE, " à ");
				DYNBUF_ADD_CELLP(&sql, s2, SQL_NO_QUOTE);
				sql->data[sql->cnt - 1]--;
				DYNBUF_ADD_STR(&sql, "'");
			}
			DYNBUF_ADD3_CELLP(&sql, " ELSE '", s2, SQL_NO_QUOTE, " ou +' END WHERE Val<>''");
			if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_replace_values
** Description : apply substitution on values table for a pivot table field
*********************************************************************/
#define ERR_FUNCTION "pivottable_replace_values"
#define ERR_CLEANUP M_FREE(sql)
int pivottable_replace_values(				/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTableField *pvf					/* in : pivot field parameters */
){
	DynBuffer *sql = NULL;
	char printbuf[1024];
	unsigned long i, j, cnt = 0;

	if(!pvf->sr_src.nbrows) RETURN_OK;
	if(sql) sql->cnt = 0;
	dynbuf_print(&sql, 
		"-- pivottable_add_field - Replace values\n"
		"UPDATE %s SET Val=CASE Val ", pvf->datatable);
	for(i = 0; i < pvf->sr_src.nbrows; i++)
		for(j = 0; j < pvf->sr_src.nbcols && dyntab_sz(&pvf->sr_src, i, j); j++)
		{
			DYNBUF_ADD3_CELL(&sql, "WHEN '", &pvf->sr_src, i, j, SQL_NO_QUOTE, "' ");
			DYNBUF_ADD3_CELL(&sql, "THEN '", &pvf->sr_dest, i, 0, SQL_NO_QUOTE, "' ");
			cnt++;
		}
	DYNBUF_ADD_STR(&sql, "ELSE Val END");
	if(cnt && sql_exec_query(cntxt, sql->data)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_set_cellval
** Description : set cell value in pivot table result
*********************************************************************/
#define ERR_FUNCTION "pivottable_set_cellval"
#define ERR_CLEANUP 
int pivottable_set_cellval(					/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv,							/* in/out : pivot table data */
	unsigned long row,						/* in : result row index */
	unsigned long col,						/* in : result col index */
	DynTable *src,							/* in : source table */
	unsigned long i,						/* in : source row index */
	unsigned long j							/* in : source start col index */
){
	DynTableCell *c;
	DYNTAB_ADD_CELL(pv->res, row, col, src, i, j);
	c = dyntab_cell(pv->res, row, col);
	c->IdObj = DYNTAB_TOULRC(src, i, j + 1);
	c->IdValObj = DYNTAB_TOULRC(src, i, j + 2);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_add_field
** Description : build values table for a pivot table field
*********************************************************************/
#define ERR_FUNCTION "pivottable_add_field"
#define ERR_CLEANUP 
int pivottable_add_field(					/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv,							/* in/out : pivot table data */
	PivotTableField *pvf					/* in : pivot field parameters */
){
	char printbuf[1024];

	/* Check params & read field data */
	if(!pv || !pvf || !pvf->expr.nbrows) return 0;

	/* Read field data (base objects Ids) */
	if(!pvf->b_done && pivottable_get_field_data(cntxt, pv, pvf)) STACK_ERROR;

	/* Build SELECT & JOIN clauses for cross values table if not data field */
	if(*pvf->pos != 'D')
	{
		DYNBUF_ADD3(&pv->select, "", pvf->datatable, 0, NO_CONV, ".Val,");
		if(pv->basetable && strcmp(pvf->datatable, pv->basetable))
			dynbuf_print4(&pv->join, "%s JOIN %s ON %s.IdObj=%s.IdObj\n",
									pvf->b_showemptyval ? "LEFT" : "INNER",
									pvf->datatable, pv->basetable, pvf->datatable);

		/* Add WHERE clause to hide empty values */
		if(!pvf->b_showemptyval)
			dynbuf_print2(&pvf->where, "IF(%s.Val IS NULL,'',%s.Val)<>''", pvf->datatable, pvf->datatable);
	}

	/* Return if field already has values */
	if(pvf->b_done) RETURN_OK;
	pvf->b_done = 1;

	/* Handle field format */
	if(pivottable_handle_field_format(cntxt, pvf)) STACK_ERROR;

	/* Slice values as applicable */
	if(pivottable_slice_values(cntxt, pvf)) STACK_ERROR;

	/* Substitute values as applicable */
	if(pivottable_replace_values(cntxt, pvf)) STACK_ERROR;

	/* Handle empty values	*/
	if(!pvf->b_showemptyval)
	{
		snprintf(add_sz_str(printbuf), "DELETE FROM %s WHERE Val=''", pvf->datatable);
		if(sql_exec_query(cntxt, printbuf)) STACK_ERROR;
	}
	else
		if(pvf->srctable && pv->nbobj > pvf->nbobj && pvf != pv->data &&
				pivottable_add_empty_values(cntxt, pv, pvf)) STACK_ERROR;


	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_handle_relation_label
** Description : handle label with relation format
*********************************************************************/
#define ERR_FUNCTION "pivottable_handle_relation_label"
#define ERR_CLEANUP DYNTAB_FREE(objdata); \
					M_FREE(buf)
int pivottable_handle_relation_label(		/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTableField *pvf,					/* in/out : pivot field data */
	unsigned long i							/* in : index to process in pvf->labels */
){
	DynTable objdata = {0};
	DynBuffer *buf = NULL;
	unsigned long id_obj = DYNTAB_TOULRC(&pvf->labels, i, 0);
	if(id_obj)
	{
		dyntab_cell(&pvf->labels, i, 0)->IdObj = id_obj;
		if(qry_obj_field(cntxt, &objdata, id_obj, NULL)) STACK_ERROR;
		if(qry_obj_label(cntxt, NULL, NULL, &buf, NULL, NULL, NULL, NULL, NULL, 0, &objdata, 0)) STACK_ERROR;
		DYNTAB_ADD_BUF(&pvf->labels, i, 0, buf); 
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_build_labels
** Description : build labels for the given pivot table field
*********************************************************************/
#define ERR_FUNCTION "pivottable_build_labels"
#define ERR_CLEANUP DYNTAB_FREE(labels); \
					DYNTAB_FREE(objdata); \
					M_FREE(sql)
int pivottable_build_labels(				/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv,							/* in/out : pivot table data */
	PivotTableField *pvf					/* in/out : pivot field data */
){
	DynTable labels = {0};
	DynBuffer *sql = NULL;
	DynTable objdata = {0};
	char printbuf[1024], *end;
	DynTableCell *c, *c1;
	unsigned long i, j, k, id, id1 = 0;
	int b_first = pvf && pvf->labels.nbcols <= 1;
	int b_relation = pvf && pvf->dispfmt && !strcmp(pvf->dispfmt, "_EVA_RELATION");

	if(!pvf || !pv) RETURN_OK;

	/* Read all field labels if required */
	if(pvf->b_shownomatch && b_first)
	{
		snprintf(add_sz_str(printbuf), 
			"-- pivottable_build_labels - Read all field labels\n"
			"SELECT DISTINCT Val FROM %s", pvf->datatable);
		if(sql_exec_query(cntxt, printbuf) ||
			sql_get_table(cntxt, &pvf->labels, 0)) STACK_ERROR;
		dyntab_group(&pvf->labels, "DISTINCT");
	}

	/* Build SELECT clause for field labels & counts */
	dynbuf_print5(&sql, 
		"-- pivottable_build_labels - Read field labels & counts\n"
		"SELECT %s.Val,%s,COUNT(DISTINCT %s.IdObj),MAX(%s.IdObj)\nFROM %s\n",
		pvf->datatable, pv->datamember, pv->basetable, pv->basetable, pv->basetable);
	DYNBUF_ADD_BUF(&sql, pv->join, NO_CONV);
	if(pv->where) DYNBUF_ADD3_BUF(&sql, "WHERE ", pv->where, NO_CONV, "\n");
	dynbuf_print(&sql, "GROUP BY %s.Val", pvf->datatable);

	/* Handle existing labels - clear old totals */
	for(i = 0; i < pvf->labels.nbrows;i++) DYNTAB_SET_CELLP(&pvf->labels, i, 1, NULL);
	
	/* Exec query & get field labels */
	if(sql_exec_query(cntxt, sql->data) ||
		sql_get_table(cntxt, pvf->labels.nbrows ? &labels : &pvf->labels, 2)) STACK_ERROR;

	/* Handle existing labels - transfer new totals */
	for(i = 0; i < labels.nbrows;i++)
	{
		/* Search corresponding label in existing labels */
		c1 = dyntab_cell(&labels, i, 0);
		if(b_relation && c1 && c1->txt)
		{
			id1 = strtoul(c1->txt, &end, 10); 
			c1->b_relation = (char)(*end == 0);
		}
		for(j= 0; j < pvf->labels.nbrows; j++)
		{
			if(b_relation && c1 && c1->b_relation)
			{
				c = dyntab_cell(&pvf->labels, j, 0);
				id = c->IdValObj ? c->IdValObj : c->IdObj ? c->IdObj : c->txt ? strtoul(c->txt, NULL, 10) : 0;
				if(id == id1) break;
			}
			if(!dyntab_cmp(&labels, i, 0, &pvf->labels, j, 0)) break;
		}
		if(j == pvf->labels.nbrows)
		{
			/* Add new label if not found */
			DYNTAB_ADD_CELL(&pvf->labels, j, 0,	&labels, i, 0);
		}

		/* Transfer counts to labels */
		for(k = 1; k < labels.nbcols; k++) DYNTAB_ADD_CELL(&pvf->labels, j, k, &labels, i, k);
		pvf->labels.nbcols = labels.nbcols;
	}

	/* Handle relation format */
	if(b_relation)
		for(i = 0; i < pvf->labels.nbrows;i++)
		{
			DynTableCell *c = dyntab_cell(&pvf->labels, i, 0);
			char *s = "";
			unsigned long id_obj = c->txt ? strtoul(c->txt, &s, 10) : 0;
			if(c->col || !id_obj || *s) continue;
			c->IdObj = id_obj;
			c->col = 1;
			if(qry_obj_field(cntxt, &objdata, id_obj, NULL)) STACK_ERROR;
			if(qry_obj_label(cntxt, NULL, NULL, &sql, NULL, NULL, NULL, NULL, NULL, 0, &objdata, 0)) STACK_ERROR;
			DYNTAB_ADD_BUF(&pvf->labels, i, 0, sql); 
		}

	/* Sort labels on first field calculation */
	if(!pvf->sortmode) pvf->sortmode = "";
	if(b_first && strcmp(pvf->sortmode, "_EVA_NONE"))
	{
		/* Sort with given mode */
		dyntab_sort(&pvf->labels, 
			!strcmp(pvf->sortmode, "_EVA_NUMBER") ?
				pvf->b_sortdesc ? qsort_col0idesc : qsort_col0i :
			!strcmp(pvf->sortmode, "_EVA_TOTAL") ?
				pvf->b_sortdesc ? qsort_col1fdesc : qsort_col1f :
				pvf->b_sortdesc ? qsort_col0desc : qsort_col0);

		/* Handle labels sort position */
		if(pvf->sr_pos.nbrows) 
		{
			for(i = 0; i < pvf->labels.nbrows;i++)
			{
				DynTableCell *c = dyntab_cell(&pvf->labels, i, 0), *p;
				for(j = 0; j < pvf->sr_pos.nbrows && dyntab_cmp(&pvf->sr_dest, j, 0, &pvf->labels, i, 0); j++);
				p = dyntab_cell(&pvf->sr_pos, j, 0);
				c->col = (p && p->len && p->txt) ? atoi(p->txt) : i + 10000;
			}
			dyntab_sort(&pvf->labels, qsort_val_col);
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottablefield_add_total
** Description : ad percentage column to a pivot table field labels counts
*********************************************************************/
#define ERR_FUNCTION "pivottablefield_add_total"
#define ERR_CLEANUP DYNTAB_FREE(data)
int pivottablefield_add_total(			/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv,							/* in/out : pivot table data */
	PivotTableField *pvf,					/* in/out : pivot field data */
	double grandtot							/* in : grand total for percentage calculation */
){
	DynTable data = {0};
	char *percent = pv->data ? dyntab_val(&pv->data->percent, 0, 0) : "";
	int b_col = pvf == pv->col;
	int b_per, b_tot;
	unsigned long i, pos = 0, rowcol = b_col ? ROWCNT : COLCNT + pos;
	int b_done = pvf->totals.nbrows > 0;
	if(!*percent) percent = dyntab_val(&pvf->percent, 0, 0);
	b_per = *percent && strcmp(percent, "_EVA_NONE") && grandtot;
	b_tot = !b_per || strcmp(percent, "_EVA_SINGLE");

	/* Add total for each label */
	if(b_tot)
	{
		if(!b_done)
		{
			DynTableCell *c = dyntab_cell(pv->data ? &pv->data->totlabel : &pvf->totlabel, 0, 0);
			if(c && c->txt)
				DYNTAB_ADD_CELLP(&pvf->totals, pos, 0, c)
			else
				DYNTAB_ADD_STR(&pvf->totals, pos, 0, "Total");
		}
		for(i = 0; i < pvf->labels.nbrows; i++)
			if(pivottable_set_cellval(cntxt, pv, b_col ? rowcol : i, b_col ? i : rowcol, &pvf->labels, i, 1))
				STACK_ERROR;
		pos++;
		rowcol++;
	}

	/* Add percentage for each label */
	if(b_per)
	{
		DynTableCell c = {0};
		char percent[8];
		c.txt = percent;
		DYNTAB_SET_CELLP(&data, 0, 0, &c);
		if(!b_done) DYNTAB_ADD_STR(&pvf->totals, pos, 0, "%");
		for(i = 0; i < pvf->labels.nbrows; i++)
		{
			c.len = snprintf(add_sz_str(percent), "%1.1lf%%", 100.0 * atof(dyntab_val(&pvf->labels, i, 1)) / grandtot);
			DYNTAB_SET_CELL(&data, 1, 0, &pvf->labels, i, 1);
			DYNTAB_SET_CELL(&data, 2, 0, &pvf->labels, i, 2);
			if(pivottable_set_cellval(cntxt, pv, b_col ? rowcol : i, b_col ? i : rowcol, &data, 0, 0))
				STACK_ERROR;
		}
		pos++;
		rowcol++;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_calc_single
** Description : compute a pivot table with single indicator at each position
*********************************************************************/
#define ERR_FUNCTION "pivottable_calc_single"
#define ERR_CLEANUP DYNTAB_FREE(id_obj); \
					M_FREE(sql)
int pivottable_calc_single(					/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv							/* in/out : pivot table data */
){
	DynTable id_obj = {0};
	DynBuffer *sql = NULL;
	char *function = dyntab_val(pv->data ? &pv->data->function : NULL, 0, 0);
	unsigned long i, j;
	char printbuf[1024];

	/* Read data field */
	if(pv->data)
	{
		pv->data->srctable = *pv->objtable ? pv->objtable : NULL;
		if(pivottable_add_field(cntxt, pv, pv->data)) STACK_ERROR;
		if(pv->data->srctable || *pv->data->datatable)
		{
			pv->nbobj = pv->data->nbobj;
			if(!pv->nbobj)
			{
				pv->b_empty = 1;
				RETURN_OK;
			}
		}
		else
			pv->data = NULL;
	}

	/* Set idobj source table for row & column */
	if(*pv->objtable || pv->data && *pv->data->datatable)
	{
		/* There is a source table for IdObj : use it for both rows & columns */
		char *table = (*pv->objtable && !pv->data) ? pv->objtable : pv->data->datatable;
		if(pv->row) pv->row->srctable = table;
		if(pv->col) pv->col->srctable = table;
	}
	else
	{
		if(pv->row && pv->col) pv->col->srctable = pv->row->datatable;
	}

	/* Set base table */
	pv->basetable = pv->data ? pv->data->datatable :
					*pv->objtable ? pv->objtable :
					pv->row ? pv->row->datatable : 
					pv->col ? pv->col->datatable : NULL;
	if(!pv->basetable) RETURN_OK;

	/* Set data member */
	if(!strcmp(function, "SUM") || !strcmp(function, "AVG") ||
		!strcmp(function, "MIN") || !strcmp(function, "MAX") || !strcmp(function, "COUNT"))
	{
		if(!pv->data) RETURN_ERROR("NoDataExpression", NULL);
		snprintf(add_sz_str(pv->datamember), "%s(%s.Val)", function, pv->basetable);
	}
	else if(!strcmp(function, "VALCOUNT"))
		snprintf(add_sz_str(pv->datamember), "COUNT(%s.IdObj)", pv->basetable);
	else if(!strcmp(function, "VALCOUNT1") && (pv->row || pv->col))
		snprintf(add_sz_str(pv->datamember), "COUNT(DISTINCT %s.Val)", pv->basetable);
	else
		snprintf(add_sz_str(pv->datamember), "COUNT(DISTINCT %s.IdObj)", pv->basetable);

	/* Add row/column/data fields - return if empty */
	if(pivottable_add_field(cntxt, pv, pv->row)) STACK_ERROR;
	if(pv->row && !pv->nbobj) pv->nbobj = pv->row->nbobj;
	if(pivottable_add_field(cntxt, pv, pv->col)) STACK_ERROR;
	if(pv->col && !pv->nbobj) pv->nbobj = pv->col->nbobj;
	if(!pv->nbobj) RETURN_OK;

	/* Add WHERE clause for values Lines match if applicable */
	if(pv->data && pv->data->b_lines)
	{
		if(pv->row && pv->row->b_lines)
			dynbuf_print5(&pv->where, "%s (%s.Line=%s.Line OR %s.Line IS NULL OR %s.Line IS NULL)\n",
									pv->where ? "AND " : "", 
									pv->row->datatable, pv->data->datatable,
									pv->row->datatable, pv->data->datatable);
		if(pv->col && pv->col->b_lines)
			dynbuf_print5(&pv->where, "%s (%s.Line=%s.Line OR %s.Line IS NULL OR %s.Line IS NULL)\n",
									pv->where ? "AND " : "", 
									pv->col->datatable, pv->data->datatable,
									pv->col->datatable, pv->data->datatable);
	}
	else if(pv->row && pv->row->b_lines && pv->col && pv->col->b_lines)
		dynbuf_print5(&pv->where, "%s (%s.Line=%s.Line OR %s.Line IS NULL OR %s.Line IS NULL)\n",
								pv->where ? "AND " : "", 
								pv->row->datatable, pv->col->datatable,
								pv->row->datatable, pv->col->datatable);

	/* Build labels */
	if(pivottable_build_labels(cntxt, pv, pv->row)) STACK_ERROR;
	if(pivottable_build_labels(cntxt, pv, pv->col)) STACK_ERROR;
	
	/* Build GROUP clause */
	if(pv->row || pv->col) DYNBUF_ADD_STR(&pv->group, "GROUP BY ");
	if(pv->row) dynbuf_print(&pv->group, "%s.Val", pv->row->datatable);
	if(pv->row && pv->col) DYNBUF_ADD_STR(&pv->group, ",");
	if(pv->col) dynbuf_print(&pv->group, "%s.Val", pv->col->datatable);

	/* Return if building objects for list */
	if(pv->rowcolclic) RETURN_OK;

	/* Calc pivot table grand total */
	M_FREE(sql);
	DYNBUF_ADD3(&sql, 
		"-- pivottable_calc - grand total\n"
		"SELECT ", pv->datamember, 0, NO_CONV, "");
	DYNBUF_ADD3(&sql, ",COUNT(DISTINCT ", pv->basetable, 0, NO_CONV, ".IdObj) ");
	DYNBUF_ADD3(&sql, ",MAX(", pv->basetable, 0, NO_CONV, ".IdObj) ");
	DYNBUF_ADD3(&sql, "FROM ", pv->basetable, 0, NO_CONV, "\n");
	DYNBUF_ADD_BUF(&sql, pv->join, NO_CONV);
	if(pv->where) DYNBUF_ADD3_BUF(&sql, "WHERE ", pv->where, NO_CONV, "\n");
	if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &id_obj, 2)) STACK_ERROR;
	if(pivottable_set_cellval(cntxt, pv, ROWCNT, COLCNT, &id_obj, 0, 0)) STACK_ERROR;

	/* Build SELECT clause for cross values table if applicable */
	if(pv->row && pv->col) 
	{
		M_FREE(sql);
		dynbuf_print5(&sql, 
			"-- pivottable_calc - Build cross values table\n"
			"SELECT %s%s,COUNT(DISTINCT %s.IdObj),MAX(%s.IdObj)\nFROM %s\n",
			pv->select ? pv->select->data : "", pv->datamember, pv->basetable, pv->basetable, pv->basetable);
		DYNBUF_ADD_BUF(&sql, pv->join, NO_CONV);
		if(pv->where) DYNBUF_ADD3_BUF(&sql, "WHERE ", pv->where, NO_CONV, "\n");
		DYNBUF_ADD_BUF(&sql, pv->group, NO_CONV);
		
		/* Exec query & get cross values table */
		if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &id_obj, 2)) STACK_ERROR;

		/* Fill pivot table result */
		{
			unsigned long c_col = pv->row ? 1 : 0;
			unsigned long c_val = (pv->row ? 1 : 0) + (pv->col ? 1 : 0);
			int b_relrow = pv->row && !strcmp(pv->row->dispfmt, "_EVA_RELATION");
			int b_relcol = pv->col && !strcmp(pv->col->dispfmt, "_EVA_RELATION");
			for(i = 0; i < id_obj.nbrows; i++)
			{
				/* Search for row & column value in labels */
				unsigned long idrow = DYNTAB_TOULRC(&id_obj, i, 0);
				unsigned long idcol = DYNTAB_TOULRC(&id_obj, i, c_col);
				unsigned long irow = 0, icol = 0;
				if(pv->row) for(irow = 0; irow < ROWCNT; irow++)
				{
					if(b_relrow && idrow)
					{
						DynTableCell *c =  dyntab_cell(&pv->row->labels, irow, 0);
						if(idrow == (c->IdValObj ? c->IdValObj : c->IdObj)) break;
					}
					else if(!dyntab_cmp(&pv->row->labels, irow, 0, &id_obj, i, 0)) break;
				}
				if(pv->col) for(icol = 0; icol <  COLCNT; icol++)
				{
					if(b_relcol && idcol)
					{
						DynTableCell *c =  dyntab_cell(&pv->col->labels, icol, 0);
						if(idcol == (c->IdValObj ? c->IdValObj : c->IdObj)) break;
					}
					else if(!dyntab_cmp(&pv->col->labels, icol, 0, &id_obj, i, c_col)) break;
				}

				/* Set corresponding cell */
				if(pivottable_set_cellval(cntxt, pv, irow, icol, &id_obj, i, c_val)) STACK_ERROR;
			}
		}
	}

	/* Add pivot table column & row totals */
	{
		double grandtot = atof(dyntab_val(pv->res, ROWCNT, COLCNT));
		int b_pc_all = pv->data && !strcmp(dyntab_val(&pv->data->percent, 0, 0), "_EVA_SINGLE");
		int b_pc_row = pv->data && pv->row && !strcmp(dyntab_val(&pv->data->percent, 0, 0), "_EVA_ROW");
		int b_pc_col = pv->data && pv->col && !strcmp(dyntab_val(&pv->data->percent, 0, 0), "_EVA_COL");
		if(pv->row && pivottablefield_add_total(cntxt, pv, pv->row, grandtot)) STACK_ERROR;
		if(pv->col && pivottablefield_add_total(cntxt, pv, pv->col, grandtot)) STACK_ERROR;

		/* Percent displayed : replace values in result */
		if(b_pc_all || b_pc_row || b_pc_col)
		{
			for(i = 0; !i || i < ROWCNT; i++) 
				for(j = 0; !j || j < COLCNT; j++)
					if(dyntab_sz(pv->res, i, j))
					{
						double v = atof(dyntab_val(pv->res, i, j));
						double b = b_pc_all ? grandtot : b_pc_row ? atof(dyntab_val(&pv->row->labels, i, 1)) : atof(dyntab_val(&pv->col->labels, j, 1));
						char p[32];
						DYNTAB_ADD(pv->res, i, j, p, snprintf(add_sz_str(p), "%.1lf%%", 100.0 * v / b), NO_CONV);
					}
		}
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : pivottable_calc
** Description : compute a pivot table with multiple indicators at each position
*********************************************************************/
#define ERR_FUNCTION "pivottable_calc"
#define ERR_CLEANUP 
int pivottable_calc(						/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	PivotTable *pv							/* in/out : pivot table data */
){
	unsigned long irow, icol, idata, i;

	/* Check params */
	if(!pv) return 0;

	/* Build selected objects list */
	if(pivottable_select(cntxt, pv)) STACK_ERROR;

	/* Loop on data, rows & columns */
	if(!pv->b_empty) for(idata = 0; !idata || idata < pv->nbdata; idata++)
	{
		pv->data = pv->nbdata ? pv->datas + idata : NULL;
		for(irow = 0; !irow || irow < pv->nbrow; irow++)
		{
			pv->row = pv->nbrow ? pv->rows + irow : NULL;
			for(icol = 0; !icol || icol < pv->nbcol; icol++)
			{
				pv->col = pv->nbcol ? pv->cols + icol : NULL;
				pv->res = &pv->restbl[idata][irow][icol];

				/* Clean pivot table temp data & call calc function */
				M_FREE(pv->select);
				M_FREE(pv->join);
				M_FREE(pv->where);
				M_FREE(pv->group);
				if(pivottable_calc_single(cntxt, pv))
				{
					/* Display error in pivot table result */
					DYNTAB_ADD(pv->res, 0, 0, cntxt->err.text, 0, TO_HTML);
					CLEAR_ERROR;
				}
			}
		}

		/* Reset optimization status for row & col fields */
		for(i = 0; i < pv->nbcol; i++) pv->cols[i].b_done = 0;
		for(i = 0; i < pv->nbrow; i++) pv->rows[i].b_done = 0;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
