/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : qry_utils.c
** Description : utility functions for building SQL queries
**      Author : Alain BOUTE
**     Created : Feb 26 2002
**********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : qry_recursive_relation
** Description : get multiple levels related objects
*********************************************************************/
#define ERR_FUNCTION "qry_recursive_relation"
#define ERR_CLEANUP	DYNTAB_FREE(sqlres); \
					M_FREE(forms); \
					M_FREE(fields); \
					M_FREE(qry)
int qry_recursive_relation(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result : list of IdObj */
	DynTable *srcidobj,				/* in : list of base objects */
	DynTable *relfields				/* in : list of relation fields (all if NULL) */
){
	DynTable sqlres = {0};
	DynBuffer *forms = NULL;
	DynBuffer *fields = NULL;
	DynBuffer *qry = NULL;
	unsigned long i, j, level = 0, nb_cur = srcidobj ? srcidobj->nbrows : 0, nb_prev = 0;
	if(!nb_cur) RETURN_OK;

	/* Build list of forms */
	DYNBUF_ADD3_INT(&qry, 
		"-- qry_recursive_relation : build forms list\n"
		"SELECT TLink.IdObj FROM TLink \n"
		"INNER JOIN TLink AS TLink0 ON TLink.IdObj=TLink0.IdObj \n"
		"WHERE TLink.DateDel IS NULL AND TLink.IdField=", IDVAL("_EVA_FORMSTAMP"), "");
	DYNBUF_ADD3_INT(&qry, " AND TLink.IdRelObj=", OBJ_FORM_CONTROL, " \n");
	DYNBUF_ADD3_INT(&qry, 
		"AND TLink0.DateDel IS NULL AND TLink0.IdField=", IDVAL("_EVA_CONTROL"), "");
	DYNBUF_ADD3_INT(&qry, " AND TLink0.IdValue=", IDVAL("_EVA_FORM"), "  \n"
		"GROUP BY IdObj");
	if(sql_exec_query(cntxt, qry->data) || sql_get_table(cntxt, &sqlres, 2) ||
		qry_values_list(cntxt, &sqlres, 0, &forms)) STACK_ERROR;
	M_FREE(qry);

	/* Create a temporary table & insert base objects */
	if(sql_exec_query(cntxt, "-- qry_recursive_relation : create objects Ids table\n"
		"CREATE TEMPORARY TABLE RecursRel (IdObj INT NOT NULL,Lvl INT,Ord TEXT, PRIMARY KEY (IdObj))"))
		STACK_ERROR;
	DYNBUF_ADD3_CELL(&qry, 
		"INSERT INTO RecursRel (IdObj,Lvl,Ord) VALUES (", srcidobj, 0, 0, NO_CONV, ",0,'0')");
	for(i = 1; i < nb_cur; i++) DYNBUF_ADD3_CELL(&qry, ",(", srcidobj, i, 0, NO_CONV, ",0,'0')");
	if(sql_exec_query(cntxt, qry->data)) STACK_ERROR;
	M_FREE(qry);

	/* Build list of objects with given relation */
	if(qry_values_list(cntxt, relfields, 2, &fields)) STACK_ERROR;
	while(nb_cur > nb_prev)
	{
		level++;
		DYNBUF_ADD3_INT(&qry, 
			"-- qry_recursive_relation : get child ids - level=", level, "\n"); 
		DYNBUF_ADD3_INT(&qry, 
			"CREATE TEMPORARY TABLE RecursRelTmp \n" 
			"SELECT DISTINCT TLink.IdRelObj As IdObj,"
					"CONCAT(RecursRel.Ord,"
						"RIGHT(CONCAT('00',CONV(IFNULL(TLink.Line,0),10,36)),3),"
						"RIGHT(CONCAT('00',CONV(TLink.Num,10,36)),3)) AS Ord\n" 
			"FROM TLink \n"
			"INNER JOIN RecursRel ON TLink.IdObj=RecursRel.IdObj\n"
			"WHERE RecursRel.Lvl=", level - 1, " AND TLink.DateDel IS NULL AND TLink.IdRelObj IS NOT NULL \n"); 
		DYNBUF_ADD3_BUF(&qry, 
			"AND TLink.IdField IN (", fields, NO_CONV,") \n");
		DYNBUF_ADD3_BUF(&qry, 
			"AND TLink.IdRelObj NOT IN (", forms, NO_CONV,") \n" 
			"ORDER BY Ord,IdObj");
		if(sql_exec_query(cntxt, qry->data)) STACK_ERROR;
		M_FREE(qry);
		DYNBUF_ADD3_INT(&qry, 
			"INSERT IGNORE RecursRel (IdObj,Lvl,Ord) \n"
			"SELECT IdObj,", level, " AS Lvl,Ord FROM RecursRelTmp");
		if(sql_exec_query(cntxt, qry->data) ||
			sql_exec_query(cntxt, "DROP TABLE RecursRelTmp") || 
			sql_exec_query(cntxt, "SELECT COUNT(*) FROM RecursRel") || 
			sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;
		nb_prev = nb_cur;
		nb_cur = atoi(dyntab_val(&sqlres, 0, 0));
		M_FREE(qry);
	}

	/* Build list of objects & inner relations */
	M_FREE(forms);
	if(sql_exec_query(cntxt, "SELECT IdObj FROM RecursRel ORDER BY Ord") ||
		sql_get_table(cntxt, res, 2) ||
		qry_values_list(cntxt, res, 0, &forms)) STACK_ERROR;
	DYNBUF_ADD3_BUF(&qry, 
		"-- qry_recursive_relation : build list of inner relations\n"
		"SELECT DISTINCT RecursRel.IdObj,TLink.IdObj FROM RecursRel\n"
		"LEFT JOIN TLink ON TLink.IdRelObj=RecursRel.IdObj\n"
		"WHERE TLink.DateDel IS NULL\n"
		"AND TLink.IdField IN(", fields, NO_CONV,")\n");
	DYNBUF_ADD3_BUF(&qry, 
		"AND TLink.IdObj IN (", forms, NO_CONV,")\n"
		"ORDER BY RecursRel.Ord,RecursRel.IdObj");
	if(sql_exec_query(cntxt, qry->data) || sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;
	if(sql_exec_query(cntxt, "DROP TABLE RecursRel")) STACK_ERROR;

	/* Add references to objects */
	M_FREE(fields);
	for(i = 0, j = 0; i < sqlres.nbrows && j < res->nbrows; i++)
	{
		if(i && dyntab_cmp(&sqlres, i, 0, &sqlres, i - 1, 0)) 
		{
			DYNTAB_ADD_BUF(res, j, 1, fields);
			M_FREE(fields);
		}
		while(j < res->nbrows && dyntab_cmp(&sqlres, i, 0, res, j, 0)) j++;
		if(fields) DYNBUF_ADD_STR(&fields, ",");
		DYNBUF_ADD_CELL(&fields, &sqlres, i, 1, NO_CONV);
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_add_table_col
** Description : evaluate a field expression based on table 1st col & place result in 1st available col
*********************************************************************/
#define ERR_FUNCTION "qry_add_table_col"
#define ERR_CLEANUP qry_build_free(&flt); \
					M_FREE(val); \
					DYNTAB_FREE(data)
int qry_add_table_col(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* in/out : table to add values to : col 0 contains base IdObj */
	char *expr, size_t expr_sz					/* in : field expression to evaluate */
){
	DynTable data = {0};
	int b_err = !expr || !*expr;
	DynBuffer *val = NULL;
	QryBuild flt = { 0 };
	unsigned long i, row = res->nbrows, col = res->nbcols;
	static DynTable *prevres = NULL;

	/* Prepare column 0 if first column added */
	if(!row) RETURN_OK;
	if(!res->cell->IdObj) for(i = 0; i < row; i++) dyntab_cell(res, i, 0)->IdObj = DYNTAB_TOULRC(res, i, 0);

	/* Add a column to the table */
	DYNTAB_SET(res, 0, col, "");

	/* Return if empty expression */
	if(!expr || !expr_sz) RETURN_OK;

	/* Read column data */
	if(strchr(expr, '['))
	{
		if(prevres != res)
		{
			sql_drop_table(cntxt, "TmpTableCol");
			if(qry_make_idlist(cntxt, res, "TmpTableCol")) STACK_ERROR;
			prevres = res;
		}
		b_err = b_err ||
			qry_eval_fieldexpr(cntxt, &data, expr, "TmpTableCol", NULL);
	}
	else
	{
		flt.b_recdata = 1;
		b_err = b_err ||
				qry_parse_fieldexpr(cntxt, expr, expr_sz, &flt) ||
				flt.b_oper ? sql_get_listcol(cntxt, &data, res, &flt, 0) :
							qry_listobj_field(cntxt, &data, res, expr);
	}

	/* Place data in table rows corresponding to IdObj in 1st col of res */
	if(!b_err && data.nbrows)
	{
		unsigned long idobj = 0;
		for(i = 0; i < data.nbrows; i++)
		{
			DynTableCell *c = dyntab_cell(&data, i, 0);
			if(c->IdObj != idobj)
			{
				if(val && row < res->nbrows) DYNTAB_ADD_BUF(res, row, col, val);
				idobj = c->IdObj;
				for(row = 0; row < res->nbrows && idobj != dyntab_cell(res, row, 0)->IdObj; row++);
				M_FREE(val);
			}
			if(row == res->nbrows) continue;
			if(val) DYNBUF_ADD_STR(&val, ", ");
			DYNBUF_ADD_CELLP(&val, c, NO_CONV);
		}
		if(val && row < res->nbrows) dyntab_add(res, row, col, DYNBUF_VAL_SZ(val), NO_CONV);
	}
	else
		CLEAR_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_complete_data
** Description : complete object data with values from given object
*********************************************************************/
#define ERR_FUNCTION "qry_complete_data"
#define ERR_CLEANUP
int qry_complete_data(		/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in : execution context data */
	DynTable *objdata,		/* in/out : table to complete */
	DynTable *srcdata,		/* in : table holding source data for transfer */
	char *fields,			/* in : comma separated list of fields to transfer
								NULL : all fields not in objdata are transfered
								not NULL : given fields are transfered (TODO) 
											if values are already present,
											they are added with new Line index if
											other values with Line>0 are found
											or with new Num index else */
	char *tables			/* in : comma/newline separated list of fields
									to handle as tables (TODO) */
){
	unsigned long i, nbobj = objdata ? objdata->nbrows : 0;
	DynTableCell *obj;

	/* Check params */
	if(!objdata || !srcdata || !srcdata->nbrows) RETURN_OK;

	/* TODO : handle field tables */
	tables = fields;

	/* For each source value */
	for(i = 0; i < srcdata->nbrows; i++)
	{
		/* Search for values for that field in objdata */
		DynTableCell *src = dyntab_cell(srcdata, i, 0);
		unsigned long pos = dyntab_next_field(objdata, src->field, ~0UL, 0, 1, 0);
		unsigned long nb = objdata->nbrows;

		/* If value found - continue */
		if(pos < objdata->nbrows) continue;

		/* Transfer source field values to objdata */
		pos = i;
		if(dyntab_filter_field(objdata, 1, srcdata, src->field, ~0UL, &pos)) RETURN_ERR_MEMORY;
		if(objdata->nbrows > nb) i += objdata->nbrows - nb - 1;
	}

	/* Set IdObj member of copied values */
	obj = dyntab_cell(objdata, 0, 0);
	if(obj)
		for(i = nbobj; i < objdata->nbrows; i++)
			dyntab_cell(objdata, i, 0)->IdObj = obj->IdObj;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_debug_table
** Description : print a short view of a table in the debug zone
*********************************************************************/
#define ERR_FUNCTION "qry_debug_table"
#define ERR_CLEANUP
int qry_debug_table(		/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in : execution context data */
	DynTable *tab, 			/* in : table to print */
	unsigned long beg,		/* in : first row to print */
	unsigned long end		/* in : first row to stop printing (0 for none) */
){
	unsigned long i;

	if(!tab || !tab->nbrows || !(cntxt->sql_trace & 2)) RETURN_OK;
	if(!end || end > tab->nbrows) end = tab->nbrows;

	DYNBUF_ADD_STR(&cntxt->debug_msg, "\n");
	for(i = beg; i < end; i++)
	{
		DYNBUF_PRINTF(&cntxt->debug_msg, 16, "%05lu\t", i, NO_CONV);
		DYNBUF_ADD_INT(&cntxt->debug_msg, tab->cell[i].IdObj);
		if(tab->cell[i].IdValObj) DYNBUF_ADD3_INT(&cntxt->debug_msg, ".", tab->cell[i].IdValObj, "");
		DYNBUF_ADD_STR(&cntxt->debug_msg, " : ");
		DYNBUF_ADD(&cntxt->debug_msg, tab->cell[i].field, 0, NO_CONV);
		if(tab->cell[i].Num > 0) DYNBUF_ADD3_INT(&cntxt->debug_msg, ".", tab->cell[i].Num, "");
		if(tab->cell[i].Line > 0) DYNBUF_ADD3_INT(&cntxt->debug_msg, ".", tab->cell[i].Line, "");
		if(tab->cell[i].Lang > 0) DYNBUF_ADD3_INT(&cntxt->debug_msg, ".", tab->cell[i].Lang, "");
		DYNBUF_ADD3_CELL(&cntxt->debug_msg, "=", tab, i, 0, NO_CONV, "\n");
		if(i> beg+50 && i < end-50 && end - beg > 200)
		{
			DYNBUF_ADD_STR(&cntxt->debug_msg, "...\n");
			i = end - 50;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_values_list
** Description : build a comma separated list of values suitable for IN() SQL statement
*********************************************************************/
#define ERR_FUNCTION "qry_values_list"
#define ERR_CLEANUP DYNTAB_FREE(sqlres)
int qry_values_list(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynTable *tab,						/* in : table of ids or values */
	int mode,							/* in : handling of tab elements 
											0 : list of Ids
											1 : list of like values - find Ids 
											2 : list of values - find Ids 
											3 : list of values - find Ids in cell->IdValue
											4 : list of contain values - find Ids
											5 : list of comma separated text values
											6 : list of comma separated text values - escape generic search chars */
	DynBuffer **ret						/* in/out : buffer receiving the list of Ids */
){
	unsigned long i, j, id;
	DynTable sqlres = { 0 };

	if(!tab) RETURN_OK;
	if(!tab->nbrows)
	{
		if(mode >= 5) DYNBUF_ADD_STR(ret, "''")
		else DYNBUF_ADD_STR(ret, "0");
		RETURN_OK;
	}

	for(i = 0; i < tab->nbrows; i++)
	{
		/* Add value separator if needed */
		if(i) DYNBUF_ADD_STR(ret, ",");

		/* Process depending on mode */
		switch(mode)
		{
		case 0:	/* List of Ids : Add id to buffer */
			if(dyntab_from_list(&sqlres, DYNTAB_VAL_SZ(tab, i, 0), ",", 0, 2)) STACK_ERROR;
			for(j = 0; !j || j < sqlres.nbrows; j++) 
			{
				if(j) DYNBUF_ADD_STR(ret, ",");
				DYNBUF_ADD_INT(ret, DYNTAB_TOULRC(&sqlres, j, 0));
			}
			break;
			
		case 1:	/* List of like / contain values Ids - find id in TVal & add to buffer */
		case 4:
			if(sql_id_values(cntxt, &sqlres, DYNTAB_VAL_SZ(tab, i, 0), mode == 1 ? Like : Contain, 0)) STACK_ERROR;
			for(j = 0; j < (sqlres.nbrows ? sqlres.nbrows : 1); j++)
			{
				if(j) DYNBUF_ADD_STR(ret, ",");
				DYNBUF_ADD_INT(ret, atoi(dyntab_val(&sqlres, j, 0)));
			}
			break;

		case 2:	/* List of value Ids - find id in TVal & add to buffer */
			if(sql_id_value(cntxt, DYNTAB_VAL_SZ(tab, i, 0), &id)) STACK_ERROR;
			DYNBUF_ADD_INT(ret, id);
			break;

		case 3:	/* List of value Ids - find id in cell data & add to buffer */
			id = tab->cell[DYNTAB_INDEX(tab, i, 0)].IdValue;
			if(!id && sql_id_value(cntxt, DYNTAB_VAL_SZ(tab, i, 0), &id)) STACK_ERROR;
			DYNBUF_ADD_INT(ret, id);
			break;

		case 5:
			DYNBUF_ADD3_CELL(ret, "'", tab, i, 0, SQL_NO_QUOTE, "'");
			break;

		case 6:
			DYNBUF_ADD3_CELL(ret, "'", tab, i, 0, SQL_NO_LIKE_OPS, "'");
			break;

		case 7:
			DYNBUF_ADD_CELL(ret, tab, i, 0, NO_CONV);
			break;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_field_value
** Description : exec a query with field/value criteria
*********************************************************************/
#define ERR_FUNCTION "qry_field_value"
#define ERR_CLEANUP M_FREE(sql); \
					M_FREE(l_srchval); \
					M_FREE(l_srchform)
int qry_field_value(		/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	DynTable *res,			/* out : list of matching objects Ids */
	DynBuffer *srchfld,		/* in : search fields name */
	MatchMode matchmode,	/* in : how is searched value ? */
	DynTable *srchval,		/* in : values searched for fields listed in srchfld */
	DynTable *srchform		/* in : select only objects with form stamp in the list */
){
 	DynBuffer *sql = NULL;
	DynBuffer *l_srchval = NULL;
 	DynBuffer *l_srchform = NULL;

	/* Convert tables to lists for SQL IN () operator */
	if(qry_values_list(cntxt, srchval, 
		(matchmode == Like || matchmode == NotLike) ? 1 : 
		(matchmode == Exact || matchmode == Different) ? 2 : 
		(matchmode == Contain || matchmode == NotContain) ? 4 : 
		0, &l_srchval)) STACK_ERROR;
 	if(qry_values_list(cntxt, srchform, 0, &l_srchform)) STACK_ERROR;
	
	/* Build SELECT statement depending on outmode & relfld */
	DYNBUF_ADD_STR(&sql, 
		"-- qry_field_value : read IdObj \n"
		"SELECT DISTINCTROW TLink.IdObj AS IdObj \n"
		"FROM TLink \n");
	if(srchform) DYNBUF_ADD_STR(&sql, 
		"INNER JOIN TLink AS TLink_1 ON TLink.IdObj = TLink_1.IdObj \n");
	
	/* Build WHERE statement */
	DYNBUF_ADD_STR(&sql, "WHERE TLink.DateDel IS NULL \n");
	if(srchval)
	{
		switch(matchmode) {
		case RelList:
			DYNBUF_ADD3(&sql, 
				"AND TLink.IdRelObj IN ( ", l_srchval->data, l_srchval->cnt, NO_CONV, " ) \n");
			break;
		case NotContain:
		case NotLike:
		case Different:
			DYNBUF_ADD3(&sql, "AND TLink.IdValue NOT IN (", l_srchval->data, l_srchval->cnt, NO_CONV, ") \n");
			break;
		default:
			DYNBUF_ADD3(&sql, "AND TLink.IdValue IN (", l_srchval->data, l_srchval->cnt, NO_CONV, ") \n");
		}
	}
	if(srchfld) DYNBUF_ADD3(&sql, 
		"AND TLink.IdField IN (", srchfld->data, srchfld->cnt, NO_CONV, ") \n");
	if(srchform) DYNBUF_ADD3(&sql,
		"AND TLink_1.IdField=1 AND TLink_1.IdRelObj IN (", l_srchform->data, l_srchform->cnt, NO_CONV, ") \n");

	/* Build GROUP and ORDER statements */
	DYNBUF_ADD_STR(&sql,
		"GROUP BY TLink.IdObj ORDER BY TLink.DateCr DESC")
	
	if(sql_exec_query(cntxt , sql->data) || sql_get_table(cntxt, res, 2)) STACK_ERROR;
	M_FREE(sql);
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_make_idlist
** Description : transfer a list of ids to a table
*********************************************************************/
#define ERR_FUNCTION "qry_make_idlist"
#define ERR_CLEANUP M_FREE(sql)
int qry_make_idlist(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_list,				/* in : list of objects ids to put in table */
	char *table						/* in : name of table to create */
){
	DynBuffer *sql = NULL;
	unsigned long i, nb = 0;

	/* Create table */
	DYNBUF_ADD3(&sql, "CREATE TEMPORARY TABLE ", table, 0, NO_CONV, " (IdObj INT, Num INT) TYPE=HEAP")
	if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
	M_FREE(sql);

	/* Add objects Ids */
	DYNBUF_ADD3(&sql, "INSERT INTO ", table, 0, NO_CONV, " VALUES ");
	if(id_list) for(i = 0; i < id_list->nbrows; i++)
	{
		/* Keep only numeric */
		unsigned long idobj = DYNTAB_TOULRC(id_list, i, 0);
		if(idobj)
		{
			if(nb) DYNBUF_ADD_STR(&sql, ",");
			DYNBUF_ADD3_INT(&sql, "(", idobj, "");
			DYNBUF_ADD3_INT(&sql, ",", i, ")");
			nb++;
		}
	}
	if(nb && sql_exec_query(cntxt, sql->data)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_get_obj_data
** Description : return a table of object data with values description
*********************************************************************/
#define ERR_FUNCTION "qry_get_obj_data"
#define ERR_CLEANUP sql_free_result(cntxt);	\
					sql_free_row(&row)
int qry_get_obj_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	char *where						/* in : where clause for select statement */
){
	EVA_sql_row row = {0};
	unsigned long irow = 0;
 	DynBuffer *sql = NULL;
	DynTableCell cell;
	int sqltrace = cntxt->sql_trace;

	/* Cleanup result table */
	dyntab_free(res);

	/* Build SQL Query */
	DYNBUF_ADD_STR(&sql,
		"-- qry_get_obj_data : read data\n"
		"SELECT DISTINCT TLink.IdObj,TField.TxtValue,\n" \
			"IF(TVal.TxtValue IS NULL,TLink.IdRelObj,TVal.TxtValue),\n" \
			"TLink.IdRelObj,TLink.Num,TLink.Line,TLink.Lang,TLink.IdField,TLink.IdValue,TLink.Pkey \n" 
		"FROM TLink \n"
		"INNER JOIN TVal as TField ON TLink.IdField=TField.IdValue \n"
		"LEFT JOIN TVal ON TLink.IdValue=TVal.IdValue \n");
	if(where) DYNBUF_ADD3(&sql, "WHERE ", where, 0, NO_CONV, " ");
	DYNBUF_ADD_STR(&sql, "ORDER BY TLink.IdObj,TField.TxtValue,TLink.Num,TLink.Line,TLink.Lang,TLink.Pkey \n");

	/* Exec Query */
	cntxt->sql_trace = 0;
	if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
	cntxt->sql_trace = sqltrace;

	/* Add result rows to table */
	if(sql_result_next_row(cntxt, &row)) STACK_ERROR;
	while(row.value)
	{
		memset(&cell, 0, sizeof(cell));
		if(row.sz[0]) cell.IdObj = strtoul(row.value[0], NULL, 10);
		M_STRDUP(cell.field, row.value[1], (size_t)row.sz[1]);
		cell.len = row.sz[2];
		M_STRDUP(cell.txt, row.value[2], cell.len);
		if(row.sz[3] && row.value[3][0]) cell.b_relation = 1;
		if(row.sz[4]) cell.Num = strtoul(row.value[4], NULL, 10);
		if(row.sz[5]) cell.Line = strtoul(row.value[5], NULL, 10);
		if(row.sz[6]) cell.Lang = strtoul(row.value[6], NULL, 10);
		if(row.sz[7]) cell.IdField = strtoul(row.value[7], NULL, 10);
		if(row.sz[8]) cell.IdValue = strtoul(row.value[8], NULL, 10);
		if(row.sz[9]) cell.Pkey = strtoul(row.value[9], NULL, 10);
		if(dyntab_set(res, irow, 0, &cell)) RETURN_ERR_MEMORY;
		irow++;
		if(sql_result_next_row(cntxt, &row)) STACK_ERROR;
	}

	qry_debug_table(cntxt, res, 0, 0);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_obj_idfield
** Description : return single field data for single object
*********************************************************************/
#define ERR_FUNCTION "qry_obj_idfield"
#define ERR_CLEANUP	
int qry_obj_idfield(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	unsigned long id_obj,			/* in : object id to read */
	unsigned long id_field			/* in : field to read 
											if 0 : all data is read */
){
	char where[128];
	size_t sz = sprintf(where, "IdObj=%lu AND DateDel IS NULL", id_obj);
	if(id_field) sz += sprintf(where + sz, " AND IdField=%lu", id_field);
	if(qry_get_obj_data(cntxt, res, where)) STACK_ERROR;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_obj_field
** Description : return field data for single object
*********************************************************************/
#define ERR_FUNCTION "qry_obj_field"
#define ERR_CLEANUP	DYNTAB_FREE(obj); \
					DYNTAB_FREE(fld)
int qry_obj_field(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	unsigned long id_obj,			/* in : object id to read */
	char *field						/* in : fields to read (comma separated list)
											if NULL : all data is read */
){
	DynTable obj = {0};
	DynTable fld = {0};
	if(!id_obj) RETURN_OK;
	DYNTAB_ADD_INT(&obj, 0, 0, id_obj);
	if(field && *field && dyntab_from_list(&fld, field, strlen(field), ",", 0, 0)) STACK_ERROR;
	if(qry_obj_data(cntxt, res, &obj, &fld)) STACK_ERROR; 
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_listobj_field
** Description : return single field data for mutliple objects
*********************************************************************/
#define ERR_FUNCTION "qry_listobj_field"
#define ERR_CLEANUP	DYNTAB_FREE(fld)
int qry_listobj_field(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	DynTable *obj,					/* in : list of objects id to read */
	char *field						/* in : fields to read (comma separated list)
											if NULL : all data is read */
){
	DynTable fld = {0};
	if(field && *field && dyntab_from_list(&fld, field, strlen(field), ",", 0, 0)) STACK_ERROR;
	if(qry_obj_data(cntxt, res, obj, &fld)) STACK_ERROR; 
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_obj_data
** Description : return fields data for a list of objects
*********************************************************************/
#define ERR_FUNCTION "qry_obj_data"
#define ERR_CLEANUP
int qry_obj_data(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	DynTable *id_obj,				/* in : list of objects to read (in column 0) */
	DynTable *fields				/* in : list of fields to read (in column 0)
											if NULL : all data is read */
){
 	DynBuffer *sql = NULL;
	unsigned long i, j;
	if(!id_obj || !id_obj->nbrows || !res) RETURN_OK;

	/* Build SQL WHERE clause for fields if applicable */
	DYNBUF_ADD_STR(&sql, "TLink.DateDel IS NULL");
	if(fields && fields->nbrows)
	{
		DYNBUF_ADD_STR(&sql, " AND IdField IN (");
		if(qry_values_list(cntxt, fields, 3, &sql)) STACK_ERROR;
		DYNBUF_ADD_STR(&sql, ")");
	}

	/* Build SQL WHERE clause for object ids */
	DYNBUF_ADD_STR(&sql, " AND IdObj IN(");
	for(j = 0; j < id_obj->nbrows; j++) 
	{
		unsigned long idobj = DYNTAB_TOULRC(id_obj, j, 0);
		dyntab_cell(id_obj, j, 0)->IdObj = idobj;
		if(j) DYNBUF_ADD_STR(&sql, ",");
		DYNBUF_ADD_INT(&sql, idobj);
	}
	DYNBUF_ADD_STR(&sql, ") \n");

	/* Read objects data */
	if(qry_get_obj_data(cntxt, res, sql->data)) STACK_ERROR;

	/* Set col member of values with object position in id_obj */
	for(i = 0; i < res->nbrows; i++)
	{
		DynTableCell *val = dyntab_cell(res, i, 0);
		for(j = 0; j < id_obj->nbrows && val->IdObj != dyntab_cell(id_obj, j, 0)->IdObj; j++);
		val->col = j;
	}

	/* Sort values  & clear col member */
	dyntab_sort(res, qsort_colobjdata);
	for(i = 0; i < res->nbrows; i++) dyntab_cell(res, i, 0)->col = 0;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_add_new_idobj
** Description : add a new object in db
*********************************************************************/
#define ERR_FUNCTION "qry_add_new_idobj"
#define ERR_CLEANUP	DYNTAB_FREE(idobj)
int qry_add_new_idobj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long *id_obj,			/* out : new object id */
	unsigned long formstamp			/* in : form stamp for object in cell(0,0) */
){
	DynTable idobj = {0};
	if(qry_add_new_obj(cntxt, &idobj, formstamp)) STACK_ERROR;
	*id_obj = DYNTAB_TOUL(&idobj);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_add_new_obj
** Description : add a new object in db
*********************************************************************/
#define ERR_FUNCTION "qry_add_new_obj"
#define ERR_CLEANUP	M_FREE(sql); \
					DYNTAB_FREE(sqlres)
int qry_add_new_obj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_obj,				/* out : new object id */
	unsigned long formstamp			/* in : form stamp for object in cell(0,0) */
){
	DynBuffer *sql = NULL;
	DynTable sqlres = {0};
	unsigned long pkey = 0, i;
	char qry[128];

	if(!id_obj)  RETURN_OK;
	/* Read next object Id */
	if(sql_exec_query(cntxt, "SELECT MAX(IdObj)+1 FROM TLink") ||
		sql_get_table(cntxt, id_obj, 2)) STACK_ERROR;
	if(!id_obj->nbrows) DYNTAB_SET(id_obj, 0, 0, "1");

	/* Build query for new record with next object number */
	DYNBUF_ADD3_CELL(&sql,
		"-- qry_add_new_obj : Create new object\n"
		"INSERT TLink SET IdObj=", id_obj, 0, 0, NO_CONV, "");

	/* Add formstamp if given */
	if(formstamp) 
	{
		DYNBUF_ADD3_INT(&sql, ",IdField=",  cntxt->val_FORMSTAMP, "");
		DYNBUF_ADD3_INT(&sql, ",IdRelObj=", formstamp, ",Num=1");
	}

	/* Add stamp & exec query to insert new record */
	SQL_STAMP_RECORD(&sql);
	if(sql_exec_query(cntxt, sql->data)) STACK_ERROR; 
	pkey = sql_insert_id(cntxt);

	/* Check created object unicity */
	sprintf(qry, "SELECT Pkey FROM TLink WHERE IdObj=%s", dyntab_val(id_obj, 0, 0));
	if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;
	if(!sqlres.nbrows) RETURN_ERROR("Impossible de créer une nouvelle fiche", NULL);

	/* Loop until unique object Id could be created or max # of trials reached */
	for(i = 0; i < 50 && sqlres.nbrows != 1; i++)
	{
		/* Wait random time and check again object unicity (other process may have released it) */
		sql_control(cntxt, 10);
		sprintf(qry, "SELECT Pkey FROM TLink WHERE IdObj=%s", dyntab_val(id_obj, 0, 0));
		if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;
		if(sqlres.nbrows == 1) break;

		/* Try new Id */
		if(sql_exec_query(cntxt, "SELECT MAX(IdObj)+1 FROM TLink") ||
			sql_get_table(cntxt, id_obj, 2)) STACK_ERROR;
		sprintf(qry, "UPDATE TLink SET IdObj=%s WHERE Pkey=%lu", dyntab_val(id_obj, 0, 0), pkey);
		if(sql_exec_query(cntxt, qry)) STACK_ERROR; 
	}

	/* Delete created record & return error if duplicates found */
	if(sqlres.nbrows != 1)
	{
		sprintf(qry, "DELETE FROM TLink WHERE Pkey=%lu", pkey);
		sql_exec_query(cntxt, qry); 
		RETURN_ERROR("Impossible de créer une nouvelle fiche", NULL);
	}

	/* Store created record Pkey */
	dyntab_cell(id_obj, 0, 0)->Pkey = pkey;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_update_idobj_idfield
** Description : update field values for an object in DB
*********************************************************************/
#define ERR_FUNCTION "qry_update_idobj_idfield"
#define ERR_CLEANUP DYNTAB_FREE(dbval)
int qry_update_idobj_idfield(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long id_obj,			/* in : object id to update */
	unsigned long id_field,			/* in : field to update */
	DynTable *values,				/* in : values to add to DB */
	int mode						/* in : update mode
										0 = add all values (add values without check - suitable for creation only)
										1 = add new values - do not check Num nor Line
										2 = add new values - check Num & Line
										3 = replace existing values
										4 = add new values - check Line, not Num
										5 = remove values - do not check Num nor Line */
){
	unsigned long i, j;
	DynTable dbval = {0};
	DynTableCell *val, *valdb;

	/* Read object values for field if check requested */
	qry_uncache_idobj(cntxt, id_obj);
	if(mode && qry_obj_idfield(cntxt, &dbval, id_obj, id_field)) STACK_ERROR;

	/* Mark new values */
	if(mode < 5) for(i = 0; i < values->nbrows; i++)
	{
		val = dyntab_cell(values, i, 0);
		if(!val->Num) val->Num = 1;
		val->b_modified = 1;
		if(mode) for(j = 0; j < dbval.nbrows; j++)
		{
			valdb = dyntab_cell(&dbval, j, 0);
			if(!DYNTAB_SAMEVAL_MODE(val, valdb, mode)) continue;
			val->b_modified = 0;
			break;
		}
	}

	/* Mark values to remove */
	if(mode > 1) for(j = 0; j < dbval.nbrows; j++)
	{
		valdb = dyntab_cell(&dbval, j, 0);
		valdb->b_modified = (char)(mode != 5);
		for(i = 0; i < values->nbrows; i++)
		{
			val = dyntab_cell(values, i, 0);
			if(!DYNTAB_SAMEVAL_MODE(val, valdb, mode)) continue;
			valdb->b_modified = (char)(mode == 5);
		}
	}

	/* Archive old values - set delete time & user */
	for(i = 0; i < dbval.nbrows; i++)
	{
 		char sql[512];
		val = dyntab_cell(&dbval, i, 0);
		if(!val->b_modified) continue;
		sprintf(sql, "UPDATE TLink SET DateDel='%s', IdWhoDel=%s WHERE Pkey=%lu", 
			cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0), val->Pkey);
		if(sql_exec_query(cntxt , sql)) STACK_ERROR;
	}

	/* Add each value */
	for(i = 0; i < values->nbrows; i++)
	{
		/* Read value & set object id & field */
		val = dyntab_cell(values, i, 0);
		if(!val->len || !val->b_modified) continue;
		val->IdObj = id_obj;
		val->IdField = id_field;
		val->Pkey = 0;

		/* Add value in db */
		if(qry_add_val(cntxt, val, 0, &val->Pkey)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_check_idobj
** Description : check IdObj exists with formstamp in DB
*********************************************************************/
int qry_check_idobj(				/* return : 1 if object exists, 0 else */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idobj				/* in : object id to check */
){
 	char sql[128];
	DynTable res = { 0 };
	int b_ex;

	/* Build & exec query for object formstamp */
	sprintf(sql, "SELECT Pkey FROM TLink WHERE IdObj=%lu AND IdField=%lu AND DateDel IS NULL", idobj, cntxt->val_FORMSTAMP);
	if(!sql_exec_query(cntxt, sql)) sql_get_table(cntxt, &res, 2);

	/* Return result */
	b_ex = res.nbrows > 0;
	DYNTAB_FREE(res);
	return b_ex;
}

/*********************************************************************
** Function : qry_add_val
** Description : add a value in DB
*********************************************************************/
#define ERR_FUNCTION "qry_add_val"
#define ERR_CLEANUP DYNTAB_FREE(sqlres); \
					M_FREE(sql)
int qry_add_val(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTableCell *val,				/* in : value to add to DB 
										note : IdObj, field or IdField, IdValue or txt & sz or IdRelObj,
												Num & Line attributes must be set properly */
	int check,						/* in : check if value already exists before adding
											0 = no check
											1 = check value & field
											2 = check value, field, Num & Line
											3 = check field	- remove other values
											4 = check field - add if empty */
	unsigned long *pkey				/* in : record to set value to / create new record if 0
									  out : Pkey of set record / 0 if not set */
){
 	DynBuffer *sql = NULL;
	DynTable sqlres = { 0 };
	int storage;
	char *txt;
	size_t sz;
	unsigned long i;

	/* Check params */
	if(!val || !val->IdObj || !(val->field || val->IdField)) RETURN_OK;

	/* Strip spaces around the value */
	txt = val->txt;
	sz = val->len;
	if(!sz && txt) sz = strlen(txt);
	STRIP_SPACES_AROUND(txt, sz);
	if(txt && val->b_relation && !val->IdValue) val->IdValue = strtoul(txt, NULL, 10);
	if(!(txt && *txt && sz && !val->b_relation || val->IdValue) && check < 3) RETURN_OK;

	/* Determine storage type */
	storage = (val->b_relation && val->IdValue) ? 3 : 1;

	/* Search & Create field name value if needed */
	if(!val->IdField && 
		sql_add_value(cntxt, val->field, strlen(val->field), &val->IdField)) STACK_ERROR;

	/* Return if no field */
	if(!val->IdField) RETURN_OK;

	/* Search & Create value if needed */
	if(storage == 1 && !val->IdValue &&
		sql_add_value(cntxt, txt, sz, &val->IdValue)) STACK_ERROR;

	/* Check if record exists */
	if(check)
	{
		int b_exist = 0;

		/* Build query for existing values */
		DYNBUF_ADD3_INT(&sql, 
			"-- qry_add_val : Check if record exists\n"
			"SELECT Pkey,IdRelObj,IdValue FROM TLink WHERE DateDel Is NULL AND IdObj=", val->IdObj, "");
		DYNBUF_ADD3_INT(&sql, " AND IdField=", val->IdField, "");
		if(check == 2)
		{
			if(val->Num) DYNBUF_ADD3_INT(&sql, " AND Num=", val->Num, NULL);
			if(val->Line) DYNBUF_ADD3_INT(&sql, " AND Line=", val->Line, NULL);
		}
		if(check < 3) switch(storage)
		{
		case 3: DYNBUF_ADD3_INT(&sql, " AND IdRelObj=", val->IdValue, ""); break;
		default: DYNBUF_ADD3_INT(&sql, " AND IdValue=", val->IdValue, "");
		}
		if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &sqlres, 0)) STACK_ERROR;
		M_FREE(sql);

		/* Return if record exists */
		if(sqlres.nbrows && check != 3) 
		{
			if(pkey) *pkey = 0;
			RETURN_OK;
		}

		/* Delete old values where applicable */
		if(check == 3) 
		{
			for(i = 0; i < sqlres.nbrows; i++)
			{
				if(val->IdValue && (val->b_relation ?
					val->IdValue == DYNTAB_TOULRC(&sqlres, i, 1) :
					val->IdValue == DYNTAB_TOULRC(&sqlres, i, 2))) b_exist = 1;
				else
				{
					char qry[128];
					snprintf(qry, sizeof(qry), 
							"UPDATE TLink SET DateDel='%s', IdWhoDel=%s WHERE Pkey=%s",
						cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0), dyntab_val(&sqlres, i, 0));
					if(sql_exec_query(cntxt, qry)) STACK_ERROR;
				}
			}
			if(b_exist) RETURN_OK;
		}
	}

	/* Return if no value */
	if(!val->IdValue) RETURN_OK;

	/* UPDATE / INSERT where applicable */
	if(!val->Num) val->Num = 1;
	if(pkey && *pkey)
		DYNBUF_ADD_STR(&sql, "-- Update TLink record\nUPDATE")
	else
		DYNBUF_ADD_STR(&sql, "-- Insert TLink record\nINSERT");


	/* Prepare query with new values */
	DYNBUF_ADD3_INT(&sql, " TLink SET IdObj=", val->IdObj, ", IdField=");
	DYNBUF_ADD_INT(&sql, val->IdField);
	switch(storage)
	{
	case 3: DYNBUF_ADD3_INT(&sql, ",IdRelObj=", val->IdValue, ""); break;
	default: DYNBUF_ADD3_INT(&sql, ",IdValue=", val->IdValue, "");
	}
	DYNBUF_ADD3_INT(&sql, ", Num=", val->Num, "");
	if(val->Line) DYNBUF_ADD3_INT(&sql, ", Line=", val->Line, "");
	SQL_STAMP_RECORD(&sql)

	/* Select record to update if applicable */
	if(pkey && *pkey) DYNBUF_ADD3_INT(&sql," WHERE Pkey=", *pkey, "");

	
	/* Exec query & return new Pkey if applicable */
	if(sql_exec_query(cntxt , sql->data)) STACK_ERROR;
	if(pkey && !*pkey) *pkey = sql_insert_id(cntxt);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_update_idobj_idfield_1
** Description : update an object single value in db
*********************************************************************/
#define ERR_FUNCTION "qry_update_idobj_idfield_1"
#define ERR_CLEANUP
int qry_update_idobj_idfield_1(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idobj,			/* in : object id */
	unsigned long idfield,			/* in : field id */
	char *txt, size_t len			/* in : value text */
){
	DynTableCell val = {0};

	/* Fill value attributes */
	val.IdObj = idobj;
	val.IdField = idfield;
	val.txt = txt;
	val.len = len;
	val.Num = 1;

	/* Add new value in db */
	if(qry_add_val(cntxt, &val, 3, NULL)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_add_idobj_field_val
** Description : add an object value in db
*********************************************************************/
#define ERR_FUNCTION "qry_add_idobj_field_val"
#define ERR_CLEANUP
int qry_add_idobj_field_val(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long id_obj,			/* in : object id */
	char *field,					/* in : field name */
	char *txt, size_t sz,			/* in : value text */
	unsigned long num,				/* in : value index */
	unsigned long line,				/* in : line index */
	int check						/* in : check if value exist - see qry_add_val for details */
){
	DynTableCell val = {0};

	/* Fill value attributes */
	val.IdObj = id_obj;
	val.field = field;
	val.txt = txt;
	val.len = sz;
	val.Num = num ? num : 1;
	val.Line = line;

	/* Add new value in db */
	if(qry_add_val(cntxt, &val, check, NULL)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_add_idobj_field_rel
** Description : add an object relation in db
*********************************************************************/
#define ERR_FUNCTION "qry_add_idobj_field_rel"
#define ERR_CLEANUP
int qry_add_idobj_field_rel(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long id_obj,			/* in : object id */
	char *field,					/* in : field name */
	unsigned long idrelobj,			/* in : related object id */
	unsigned long num,				/* in : value index */
	unsigned long line,				/* in : line index */
	int check						/* in : check if value exist - see qry_add_val for details */
){
	DynTableCell val = {0};
	char txt[16];

	/* Check parameters */
	if(!idrelobj) RETURN_OK;

	/* Fill value attributes */
	val.IdObj = id_obj;
	val.field = field;
	val.txt = txt;
	val.len = snprintf(add_sz_str(txt), "%lu", idrelobj);
	val.b_relation = 1;
	val.Num = num ? num : 1;
	val.Line = line;

	/* Add new value in db */
	if(qry_add_val(cntxt, &val, check, NULL)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_add_obj_field_val
** Description : add an object value in db
*********************************************************************/
#define ERR_FUNCTION "qry_add_obj_field_val"
#define ERR_CLEANUP
int qry_add_obj_field_val(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_obj,				/* in : object id */
	char *field,					/* in : field name */
	char *txt, size_t sz,			/* in : value text */
	char b_relation,				/* in : store value as relation if not 0 - txt must be a valid number then */
	unsigned long numval			/* in : value index */
){
	DynTableCell val = {0};

	/* Fill value attributes */
	val.IdObj = DYNTAB_TOULRC(id_obj, 0, 0);
	val.field = field;
	val.txt = txt;
	val.len = sz;
	val.b_relation = b_relation;
	val.Num = numval ? numval : 1;

	/* Add new value in db */
	if(qry_add_val(cntxt, &val, numval ? 2 : 1, NULL)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_add_obj_data
** Description : add object values in db
*********************************************************************/
#define ERR_FUNCTION "qry_add_obj_data"
#define ERR_CLEANUP
int qry_add_obj_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_obj,				/* in/out : object id */
	DynTable *data,					/* in : object data : Field, Value, Storage (V/R), Num, Line */
	DynTable *formstamp,			/* in : form id for object formstamp */
	int b_clearprev					/* in : clear previous values if not 0 */
){
	unsigned long i, j, idobj;
	DynTable res = {0};
	DynBuffer *sql = NULL;
	DynTableCell val;

	if(!data || !id_obj || !(dyntab_sz(id_obj, 0, 0) ||dyntab_sz(formstamp, 0, 0))) RETURN_OK;

	/* Create new object number if current is null or new */
	if(!dyntab_sz(id_obj, 0, 0) && qry_add_new_obj(cntxt, id_obj, DYNTAB_TOUL(formstamp))) STACK_ERROR;
	idobj = DYNTAB_TOULRC(id_obj, 0, 0);

	/* For each value to add */
	for(i = 0; i < data->nbrows; i++)
	{
		/* Fill value attributes */
		memset(&val, 0, sizeof(val));
		val.IdObj = idobj;
		val.field = dyntab_val(data, i, 0);
		if(!*val.field) continue;
		val.txt = dyntab_val(data, i, 1);
		val.len = dyntab_sz(data, i, 1);
		val.b_relation = (char)(*dyntab_val(data, i, 2) == 'R');
		val.Num = atoi(dyntab_val(data, i, 3));
		val.Line = atoi(dyntab_val(data, i, 4));

		/* Search for previous values */
		DYNTAB_FREE(res);
		if(b_clearprev && qry_obj_field(cntxt, &res, idobj, val.field)) STACK_ERROR;
		if(res.nbrows)
		{
			/* Clear previous values */
			M_FREE(sql);
			DYNBUF_ADD3_CELL(&sql, "UPDATE TLink SET IdWhoDel=", &cntxt->id_user, 0, 0, NO_CONV, ", DateDel='");
			DYNBUF_ADD(&sql, cntxt->timestamp, 14, NO_CONV);
			DYNBUF_ADD_STR(&sql, "' WHERE Pkey IN (");
			for(j = 0; j < res.nbrows; j++)
			{
				if(j) DYNBUF_ADD_STR(&sql, ",");
				DYNBUF_ADD_INT(&sql, dyntab_cell(&res, j, 0)->Pkey);
			}
			DYNBUF_ADD_STR(&sql, ")");
			if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
		}

		/* Add new value in db */
		if(qry_add_val(cntxt, &val, 2, NULL)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_list_data
** Description : return data for objects with Ids in table IdList
*********************************************************************/
#define ERR_FUNCTION "qry_list_data"
#define ERR_CLEANUP	M_FREE(sql); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(tmp); \
					qry_build_free(&flt); \
					cntxt->sql_trace = sql_trace
int qry_list_data(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	ObjTableFormat *tbl				/* in : list parameters */
){
 	DynBuffer *sql = NULL;
	DynTable data = {0};
	DynTable tmp = {0};
	unsigned long i, j, k, row0;
	QryBuild flt = { 0 };
	int sql_trace = cntxt->sql_trace;

	/* Initialize list with objects data (all fields are read to be able to display links tooltips - this may be optimizeed) */
	dyntab_free(&tbl->data);
	if(!tbl->idobj.nbrows) RETURN_OK;
	if(qry_obj_data(cntxt, &tbl->data, &tbl->idobj, NULL)) STACK_ERROR;
	row0 = tbl->data.nbrows;
	for(i = 0; i < row0; i++)
		dyntab_cell(&tbl->data, i, 0)->col = tbl->field.nbrows;

	/* Read columns data */
	for(i = 0; i < tbl->field.nbrows; i++)
	{
		unsigned long beg = 0, end = row0;
		DynTableCell *c = dyntab_cell(&tbl->field, i, 0);
		if(!c || !c->txt || !c->len) continue;

		/* Handle expression debug */
		if(*dyntab_val(&tbl->debug, i, 0) == '1') cntxt->sql_trace = DEBUG_SQL_RES;

		/* Build query for column : check for expression type */
		if(!strncmp(c->txt, add_sz_str("SELECT ")))
		{
			/* Handle raw SQL SELECT */
			unsigned long r = tbl->data.nbrows;
			if(form_eval_fieldexpr(cntxt, &data, 0, 0, c->txt, c->len, tbl->attr, 0)) STACK_ERROR;
			if(dyntab_resize(&tbl->data, tbl->data.nbrows + data.nbrows - 1, tbl->data.nbcols - 1)) RETURN_ERR_MEMORY;
			for(k = 0; k < data.nbrows; k++)
			{
				DynTableCell *s = dyntab_cell(&data, k, 0);
				DynTableCell *d = dyntab_cell(&tbl->data, r + k, 0);
				memcpy(d, s, sizeof(d[0]));
				memset(s, 0, sizeof(d[0]));
				d->col = i;
			}
		}
		else if(strchr(c->txt, '['))
		{
			/* Handle SQL/relationnal expression */
			int b_notable = strstr(c->txt, "[$") != 0;
			M_FREE(sql);

			if(b_notable || *dyntab_val(&tbl->notable, i, 0) == '1')
			{
				/* Evaluate SQL expression without tables - loop on objects */
				if(qry_eval_sqlexpr_var(cntxt, &sql, c->txt, tbl->attr, NULL)) STACK_ERROR;
				beg = 0;
				for(j = 0; j < tbl->idobj.nbrows; j++)
				{
					/* Retrieve object data in temp location */
					unsigned long r = tbl->data.nbrows;
					unsigned long idobj = DYNTAB_TOULRC(&tbl->idobj, j, 0);
					while(beg < r && dyntab_cell(&tbl->data, beg, 0)->IdObj != idobj) beg++;
					DYNTAB_FREE(tmp);
					for(k = beg; k < r; k++)
					{
						DynTableCell *s = dyntab_cell(&tbl->data, k, 0);
						if(s->IdObj != idobj) break;
						DYNTAB_SET_CELLP(&tmp, tmp.nbrows, 0, s);
						dyntab_cell(&tmp, tmp.nbrows - 1, 0)->col = 0;
					}

					/* Evaluate expression on object & transfer to list data */
					if(form_eval_fieldexpr(cntxt, &data, 0, idobj, sql->data, sql->cnt, &tmp, 0)) STACK_ERROR;
					if(dyntab_resize(&tbl->data, tbl->data.nbrows + data.nbrows - 1, tbl->data.nbcols - 1)) RETURN_ERR_MEMORY;
					for(k = 0; k < data.nbrows; k++)
					{
						DynTableCell *s = dyntab_cell(&data, k, 0);
						DynTableCell *d = dyntab_cell(&tbl->data, r + k, 0);
						memcpy(d, s, sizeof(d[0]));
						memset(s, 0, sizeof(d[0]));
						d->col = i;
						d->IdObj = idobj;
					}
				}
			}
			else
			{
				/* Evaluate SQL expression with tables */
				EVA_sql_row row = {0};

				/* Build table of source objects */
				if(!tbl->objtbl[0])
				{
					sprintf(tbl->objtbl, "Tbl%lu", cntxt->sql_cnt);
					if(qry_make_idlist(cntxt, &tbl->idobj, tbl->objtbl)) STACK_ERROR;
				}

				/* Evaluate expression */
				if(qry_eval_sqlexpr_var(cntxt, &sql, c->txt, tbl->attr, tbl->objtbl)) STACK_ERROR;
				if(qry_eval_sql_fieldexpr(cntxt, "TmpListData", sql ? sql->data : NULL, tbl->objtbl, tbl->attr) ||
					sql_exec_query(cntxt, "SELECT * FROM TmpListData")) STACK_ERROR;

				/* Read result & add columns values */
				do 
				{
					if(sql_result_next_row(cntxt, &row)) STACK_ERROR;
					if(!row.sz || !row.sz[1]) continue;
					beg = tbl->data.nbrows;
					if(dyntab_add(&tbl->data, beg, 0, row.value[1], row.sz[1], NO_CONV)) RETURN_ERR_MEMORY;
					c = dyntab_cell(&tbl->data, beg, 0);
					if(row.value[0]) c->IdObj = strtoul(row.value[0], NULL, 10);
					if(row.value[2]) c->Line = strtoul(row.value[2], NULL, 10);
					c->col = i;
				}
				while(row.sz);
				sql_drop_table(cntxt, "TmpListData");
		}
		}
		else
		{
			/* Handle field or relation expression : parse expression */
			qry_build_free(&flt);
			flt.b_recdata = 2;
			if(qry_parse_fieldexpr(cntxt, c->txt, c->len, &flt)) CLEAR_ERROR_CONT;

			/* If not a trivial field : exec query & add values */
			if(flt.b_oper)
			{
				beg = tbl->data.nbrows;
				if(sql_get_listcol(cntxt, &tbl->data, &tbl->idobj, &flt, i)) CLEAR_ERROR_CONT;
				end = tbl->data.nbrows;
			}

			/* Mark value column */
			for(j = beg; j < end; j++)
			{
				DynTableCell *val = dyntab_cell(&tbl->data, j, 0);
				QryBuilNode *lastnode = flt.node + flt.nbnode - (flt.nbnode ? 1 : 0);

				/* Search field in fields columns (handle multiple fields ordering) */
				k = 0;
				if(val->field) for(k = 0; k < lastnode->field.nbrows && strcmp(dyntab_val(&lastnode->field, k, 0), val->field); k++);
				if(flt.b_oper || k < lastnode->field.nbrows)
				{
					val->col = i;
					val->row = k;
					if(flt.b_oper) M_FREE(val->field);
				}
			}
		}
	}

	/* Sort on Id, col, Field, Line, Num */
	dyntab_sort(&tbl->data, qsort_colval);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : sql_get_table
** Description : return values of a SQL query as a table for each column
*********************************************************************/
#define ERR_FUNCTION "sql_get_table"
#define ERR_CLEANUP M_FREE(fields);	\
					sql_free_result(cntxt);	\
					sql_free_row(&row)
int sql_get_table				/* return : 0 on success, other on error */
(
	EVA_context *cntxt,			/* in : execution data */
	DynTable *tab, 				/* in/out : table to add rows to */
	int options					/* in : table output format flags :
									bit 0 : set first line with field names
									bit 1 : clear table before processing
									bit 2 : handle field names columns */
){
	EVA_sql_row row = {0};
	unsigned long i, j, irow = (tab && !options) ? tab->nbrows : 0, icol;
	int *fields = NULL;
	char *fld[] = { "IdObj", "Pkey", "Num", "Line", "IdValue", "IdField", "IdValObj" };

	if(!cntxt->sql_result || !tab) RETURN_OK;

	/* Initialize table */
	if(options) dyntab_free(tab);

	/* Get first row */
	if(sql_result_next_row(cntxt, &row)) STACK_ERROR;

	/* Handle fields names if applicable */
	if(options & 4)
	{
		C_ALLOC(fields, sizeof(fields[0]), row.nbcols);
		for(i = 0; i < row.nbcols; i++)
			for(j = 0; j < sizeof(fld)/sizeof(fld[0]); j++)
				if(!strcmp(row.name[i], fld[j]))
				{ 
					fields[i] = j + 1;
					break;
				}
	}

	/* Set first line with field names */
	if(options & 1)
	{
		for(i = 0; i < row.nbcols; i++) 
			DYNTAB_ADD(tab, 0, i, row.name[i], 0, NO_CONV);
		irow = 1;
	}

	/* Add result rows to table */
	while(row.value)
	{
		DynTableCell *c = NULL;
		icol = 0;
		for(i = 0; i < row.nbcols; i++)
			if(!i || !fields || !fields[i])
			{
				DYNTAB_ADD(tab, irow, icol++, row.value[i], row.sz[i], NO_CONV);
				if(!i) c = dyntab_cell(tab, irow, 0);
			}
			else
			{
				unsigned long id = row.value[i] ? strtoul(row.value[i], NULL, 10) : 0;
				if(id) switch(fields[i])
				{
				case 1:	c->IdObj = id; break;
				case 2:	c->Pkey = id; break;
				case 3:	c->Num = id; break;
				case 4:	c->Line = id; break;
				case 5:	c->IdValue = id; break;
				case 6:	c->IdField = id; break;
				case 7:	c->IdValObj = id; break;
				}
			}
		irow++;
		if(sql_result_next_row(cntxt, &row)) STACK_ERROR;
	}

	if(cntxt->sql_trace & DEBUG_SQL_RES)
		err_print_dyntab(&cntxt->debug_msg, tab, 10, NO_CONV);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : sql_get_buf
** Description : return values of a SQL query as a list of values
*********************************************************************/
#define ERR_FUNCTION "sql_get_buf"
#define ERR_CLEANUP sql_free_result(cntxt);	\
					sql_free_row(&row)
int sql_get_buf					/* return : 0 on success, other on error */
(
	EVA_context *cntxt,			/* in : execution data */
	DynBuffer **res,			/* in/out : values list */
	int options					/* in : table output format flags :
									bit 0 : quote values
									bit 1 : clear res before processing */
){
	EVA_sql_row row = {0};
	unsigned long i;

	if(!cntxt->sql_result || !res) RETURN_OK;

	/* Initialize result buffer */
	if(options & 2) M_FREE(*res);

	/* Get first row */
	if(sql_result_next_row(cntxt, &row)) STACK_ERROR;

	/* Add result rows to list */
	while(row.value)
	{
		for(i = 0; i < row.nbcols; i++)
		{
			if(*res) DYNBUF_ADD_STR(res, ",");
			DYNBUF_ADD(res, row.value[i], row.sz[i], NO_CONV);
		}
		if(sql_result_next_row(cntxt, &row)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : sql_get_listcol
** Description : read values corresponding to a filter in table column format
*********************************************************************/
#define ERR_FUNCTION "sql_get_listcol"
#define ERR_CLEANUP sql_free_result(cntxt);	\
					sql_free_row(&row); \
					M_FREE(sql); \
					cntxt->sql_trace = sql_trace; \
					if(cntxt->err.text) err_print_filter(&cntxt->err.detail, flt)
int sql_get_listcol(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing result - if NULL, output to table ValList */
	DynTable *idlist,				/* in : list of objects Ids - if NULL, read from table IdList */
	QryBuild *flt,					/* in : query build data */
	unsigned long col				/* in : column to store in cell->col */
){
	EVA_sql_row row = {0};
	unsigned long irow = res ? res->nbrows : 0, irow0 = irow;
 	DynBuffer *sql = NULL;
	DynTableCell cell;
	int sql_trace = cntxt->sql_trace;

	if(!flt->select || idlist && !idlist->nbrows) RETURN_OK;

	/* Build query for field data */
	if(res)
		DYNBUF_ADD_STR(&sql, 
			"-- sql_get_listcol : read field data\n")
	else
		DYNBUF_ADD_STR(&sql, 
			"-- sql_get_listcol : create ValList with field data\n"
			"CREATE TEMPORARY TABLE ValList ");

	/* Build & exec Query - return on error */
	if(flt->debug) cntxt->sql_trace = DEBUG_SQL_RES;
	if(qry_build_flt_select(cntxt, &sql, idlist, flt, 0, 0)) STACK_ERROR;
	if(qry_exec_filter(cntxt, flt, sql->data)) STACK_ERROR;	

	/* Output to DynTable : add result rows to table */
	if(res)
	{
		if(sql_result_next_row(cntxt, &row)) STACK_ERROR;
		while(row.value)
		{
			/* Read IdObj & value in cell */
			memset(&cell, 0, sizeof(cell));
			if(row.value[0]) cell.IdObj = strtoul(row.value[0], NULL, 10);
			cell.len = row.sz[1];
			M_STRDUP(cell.txt, row.value[1], cell.len);
			cell.col = col;

			/* Read optional columns */
			if(row.nbcols > 2 && row.value[2]) cell.IdValObj = strtoul(row.value[2], NULL, 10);
			if(row.nbcols > 3)
			{
				if(row.value[3]) cell.Num = strtoul(row.value[3], NULL, 10);
				if(row.value[4]) cell.Line = strtoul(row.value[4], NULL, 10);
				if(row.nbcols > 5)
				{
					if(row.value[5]) M_STRDUP(cell.field, row.value[5], row.sz[5]);
					if(row.value[6]) cell.IdField = strtoul(row.value[6], NULL, 10);
					if(row.value[7]) cell.IdValue = strtoul(row.value[7], NULL, 10);
					cell.b_relation = (char)(row.value[8] ? row.value[8][0] == '1' : 0);
				}
			}

			/* Add cell to results table & read next row */
			if(dyntab_set(res, irow, 0, &cell)) RETURN_ERR_MEMORY;
			irow++;
			if(sql_result_next_row(cntxt, &row)) STACK_ERROR;
		}

		qry_debug_table(cntxt, res, irow0, irow);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_exec_filter
** Description : execute query for a filter
*********************************************************************/
#define ERR_FUNCTION "qry_exec_filter"
#define ERR_CLEANUP
int qry_exec_filter(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	QryBuild *flt,					/* in : filter data */
	char *qry						/* in : SQL query to exec */
){
	int sqltrace = cntxt->sql_trace; 
	//flt->debug = 1;
	if(flt->debug) cntxt->sql_trace = DEBUG_SQL_RES; 
	if(cntxt->sql_trace & DEBUG_SQL_RES) err_print_filter(&cntxt->debug_msg, flt); 
	if(sql_exec_query(cntxt, qry)) STACK_ERROR; 
	if((cntxt->debug & DEBUG_SQL_SLOW && cntxt->sql_restime > DEBUG_SQL_SLOW_TH) && !(cntxt->sql_trace & DEBUG_SQL_RES))
		err_print_filter(&cntxt->debug_msg, flt); 
	cntxt->sql_trace = sqltrace; 

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : sql_drop_table
** Description : drop a temporary SQL table
*********************************************************************/
void sql_drop_table(
	EVA_context *cntxt,						/* in/out : execution context data */
	char *table								/* in : table name */
){
	char buf[1024];
	if(!table || !*table) return;
	snprintf(add_sz_str(buf), "DROP TABLE IF EXISTS %s", table);
	sql_exec_query(cntxt, buf);
}
