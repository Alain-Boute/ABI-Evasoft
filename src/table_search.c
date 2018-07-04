/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
** File : table_search.c
** Description : search functions used for tables & relations
** Author : Alain BOUTE
** Created : Aug 21 2003
**********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : table_search_set_node
** Description : build search clause for table search filter
*********************************************************************/
#define ERR_FUNCTION "table_search_set_node"
#define ERR_CLEANUP DYNTAB_FREE(data); \
					M_FREE(idlist)
int table_search_set_node(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	QryBuild *flt,						/* in/out : filter to add clauses to */
	unsigned long i_word				/* in : index of word to use in tbl->srchwords */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynTable data = {0};
	DynBuffer *idlist = NULL;

	if(flt->groupfn)
	{
		/* There is a group function : use HAVING clause */
		if(qry_build_clauses(cntxt, flt, 0)) STACK_ERROR;
		if(!strcmp(tbl->srchfmt, "_EVA_AGE") ||
			!strncmp(tbl->srchfmt, add_sz_str("_EVA_DATE")))
		{
			if(dyntab_sz(&tbl->srchwords, 0, 0))
			{
				if(flt->having) DYNBUF_ADD_STR(&flt->having, " AND ");
				DYNBUF_ADD3_BUF(&flt->having, "", flt->select, NO_CONV, ">='");
				DYNBUF_ADD3_CELL(&flt->having, "", &tbl->srchwords, 0, 0, SQL_NO_QUOTE, "'");
			}
			if(dyntab_sz(&tbl->srchwords, 1, 0))
			{
				if(flt->having) DYNBUF_ADD_STR(&flt->having, " AND ");
				DYNBUF_ADD3_BUF(&flt->having, "", flt->select, NO_CONV, "<'");
				DYNBUF_ADD3_CELL(&flt->having, "", &tbl->srchwords, 1, 0, SQL_NO_QUOTE, "'");
			}
		}
		else
		{
			if(flt->having) DYNBUF_ADD_STR(&flt->having, " AND ");
			if(tbl->srchmode == Exact)
			{
				DYNBUF_ADD3_BUF(&flt->having, "", flt->select, NO_CONV, "='");
				DYNBUF_ADD3_CELL(&flt->having, "", &tbl->srchwords, i_word, 0, SQL_NO_QUOTE, "'");
			}
			else
			{
				DYNBUF_ADD3_BUF(&flt->having, "", flt->select, NO_CONV, " LIKE '%");
				DYNBUF_ADD3_CELL(&flt->having, "", &tbl->srchwords, i_word, 0, SQL_NO_QUOTE, "%'");
			}
		}
	}
	else
	{
		/* No group function : set value node condition */
		QryBuilNode *node = flt->node + (flt->nbnode - 1);
		if(!flt->nbnode || node->rel != RelNone) node++;
		node->match =	!strcmp(tbl->srchfmt, "_EVA_AGE") ||
						!strncmp(tbl->srchfmt, add_sz_str("_EVA_DATE")) ? InBound : IdValList;

		/* Query values Id list if applicable */
		if(node->match == IdValList)
		{
			/* If query not executed (result is stored in column 1 of tbl->srchwords) */
			if(!dyntab_sz(&tbl->srchwords, i_word, 1))
			{
				/* Do not use IdValList if more than 1000 values matched */
				unsigned long limit = 1000;
				if(sql_id_values(cntxt, &data, DYNTAB_VAL_SZ(&tbl->srchwords, i_word, 0), tbl->srchmode, limit)) STACK_ERROR;
				if(data.nbrows < limit)
				{
					if(qry_values_list(cntxt, &data, 0, &idlist)) STACK_ERROR;
					DYNTAB_ADD_BUF(&tbl->srchwords, i_word, 1, idlist);
				}
				else
					DYNTAB_SET(&tbl->srchwords, i_word, 1, "0");
			}
			if(!strcmp(dyntab_val(&tbl->srchwords, i_word, 1), "0")) node->match = tbl->srchmode;
		}

		if(node->match == InBound)
		{
			DYNTAB_ADD_CELL(&node->value, 0, 0, &tbl->srchwords, 0, 0);
			DYNTAB_ADD_CELL(&node->value, 1, 0, &tbl->srchwords, 1, 0);
		}
		else if(node->match == IdValList)
		{
			DYNTAB_ADD_CELL(&node->value, 0, 0, &tbl->srchwords, i_word, 1);
		}
		else
		{
			DYNTAB_ADD_CELL(&node->value, 0, 0, &tbl->srchwords, i_word, 0);
		}
		if(node->modif == F_None) node->modif = F_Value;
		if(qry_build_clauses(cntxt, flt, 1)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_add_flt_idlist
** Description : insert objects matching filter in a IdObj table
*********************************************************************/
#define ERR_FUNCTION "table_add_flt_idlist"
#define ERR_CLEANUP M_FREE(sql)
int table_add_flt_idlist(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	QryBuild *flt,						/* in : filter nodes to process */
	int mode							/* in : query mode
											0 = output to IdList from all 
											1 = output to IdList from all - USE LIMIT
											2 = output to IdListMatch from IdList
											3 = filter IdList */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	DynBuffer *sql = NULL;
	DynTable empty = {0};
	M_FREE(flt->select);
	M_FREE(flt->optselect);
	M_FREE(flt->orderby);
	M_FREE(sql);
	if(mode == 3)
		DYNBUF_ADD_STR(&sql, "CREATE TEMPORARY TABLE IdListMatch TYPE=HEAP\n")
	else
		DYNBUF_ADD3(&sql, "INSERT INTO ", mode >= 2 ? "IdListMatch" : "IdList", 0, NO_CONV, " (IdObj)\n");
	if(	qry_build_flt_select(cntxt, &sql, mode >= 2 ? NULL : &empty, flt, 
								mode == 1 ? tbl->line : 0,
								mode == 1 ? tbl->lines : 0) ||
		sql_exec_query(cntxt, sql->data))
		STACK_ERROR_INFO(err_print_filter(&cntxt->debug_msg, flt));
	if(cntxt->debug & DEBUG_SQL_SLOW && cntxt->sql_restime > DEBUG_SQL_SLOW_TH) err_print_filter(&cntxt->debug_msg, flt);

	/* Store # of lines in table */
	tbl->totlines = cntxt->sql_nbrows; 

	/* Rename table if applicable */
	if(mode == 3 && (
		sql_exec_query(cntxt, "DROP TABLE IdList") ||
		sql_exec_query(cntxt, "CREATE TEMPORARY TABLE IdList TYPE=HEAP SELECT DISTINCT IdObj FROM IdListMatch") ||
		sql_exec_query(cntxt, "ALTER TABLE IdList ADD INDEX (IdObj)") ||
		sql_exec_query(cntxt, "DROP TABLE IdListMatch"))) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_search_filter
** Description : filter table records on search string
*********************************************************************/
#define ERR_FUNCTION "table_search_filter"
#define ERR_CLEANUP qry_build_free(&flt); \
					DYNTAB_FREE(fields); \
					M_FREE(expr); \
					M_FREE(sql)
int table_search_filter(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	unsigned long i_word				/* in : index of word to use in tbl->srchwords */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	QryBuild flt = { 0 };
	DynTable fields = {0};
	DynBuffer *expr = NULL;
	DynBuffer *sql = NULL;
	unsigned long i_col;
	
	/* Create temporary IdListMatch table */
	if(sql_exec_query(cntxt, "CREATE TEMPORARY TABLE IdListMatch (IdObj INT NOT NULL) TYPE=HEAP"))
		STACK_ERROR;
	if(!tbl->srchmode) tbl->srchmode = Contain;

	/* Build query clauses for non trivial field expression of selected search columns */
	if(tbl->srchwords.nbrows) 
		for(i_col = 0; i_col < tbl->field.nbrows; i_col++)
			if(tbl->srchcol & (1 << i_col))
	{
		DynTableCell *c = dyntab_cell(&tbl->field, i_col, 0);
		char *bracket = c && c->txt ? strchr(c->txt, '[') : NULL;
		if(bracket)
		{
			/* Handle SQL expression : check search format */
			M_FREE(sql);
			DYNBUF_ADD_STR(&sql, "INSERT INTO IdListMatch (IdObj) SELECT DISTINCT IdObj FROM ValList WHERE ");
			if(!strcmp(tbl->srchfmt, "_EVA_AGE") || !strncmp(tbl->srchfmt, add_sz_str("_EVA_DATE")))
			{
				if(qry_parse_matchmode(cntxt, &sql, "Val", ~0UL, tbl->srchwords.nbrows > 1 ? InBound : Begin, 0, &tbl->srchwords, NULL)) STACK_ERROR;
			}
			else
			{
				DYNTAB_ADD_CELL(&fields, 0, 0, &tbl->srchwords, i_word, 0);
				if(qry_parse_matchmode(cntxt, &sql, "Val", ~0UL, tbl->srchmode, 0, &fields, NULL)) STACK_ERROR;
			}
			if(sql && c->len)
			{
				if(qry_eval_sql_fieldexpr(cntxt, "ValList", c->txt, "IdList", NULL)) STACK_ERROR;
				if(sql_exec_query(cntxt, "ALTER TABLE ValList MODIFY Val TEXT") ||
					sql_exec_query(cntxt, sql->data)) STACK_ERROR;
				sql_drop_table(cntxt, "ValList");
			}
		}
		else
		{
			char *dotsep = c && c->txt ? strrchr(c->txt, '.') : NULL;
			char *relop = c && c->txt ? strrchr(c->txt, '-') : NULL;
			char *groupfn = c && c->txt ? strchr(c->txt, '(') : NULL;
			int b_reldotsep = dotsep && (!strcmp(".IdWhoCr", dotsep) ||
										 !strcmp(".IdWhoDel", dotsep) ||
										 !strcmp(".WhoCr", dotsep) ||
										 !strcmp(".WhoDel", dotsep) ||
										 !strcmp(".IdObj", dotsep) ||
										 !strcmp(".IdRelObj", dotsep) ||
										 !strcmp(".Obj", dotsep) ||
										 !strcmp(".RelObj", dotsep));
			M_FREE(expr);
			DYNBUF_ADD_CELLP(&expr, c, NO_CONV);
			if(!groupfn && (!dotsep || b_reldotsep || (relop && dotsep < relop)))
			{
				if(!strcmp("_EVA_DOCUMENT", dyntab_val(&tbl->format, i_col, 0)))
				{
					if(expr && expr->cnt >= 6 && !strcmp(expr->data + expr->cnt - 6, ".IdObj")) expr->cnt -= 6;
					DYNBUF_ADD_STR(&expr, "->_EVA_FILE_CLIENT_PATH.Value");
				}
				else if(!strncmp(dyntab_val(&tbl->format, i_col, 0), add_sz_str("_EVA_RELATION")))
				{
					DYNBUF_ADD_STR(&expr, "->.Value");
				}
				else if(!b_reldotsep) 
				{
					DYNBUF_ADD_STR(&expr, ".Value");
				}
			}
			qry_build_free(&flt);
			if(qry_parse_fieldexpr_nodes(cntxt, DYNBUF_VAL_SZ(expr), &flt)) CLEAR_ERROR_CONT;
			if(!flt.b_oper)
			{
				/* If trivial field : add node fields to trivial fields list & continue */
				unsigned long j;
				for(j = 0; j < flt.node->field.nbrows; j++)
					DYNTAB_ADD_CELL(&fields, fields.nbrows, 0, &flt.node->field, j, 0);
				continue;
			}

			/* Build & exec query for non trivial field - continue on error */
			if(	table_search_set_node(cntxt, i_ctrl, &flt, i_word) ||
				table_add_flt_idlist(cntxt, i_ctrl, &flt, 2))
					CLEAR_ERROR;
		}
	}

	/* Build query for trivial field expressions if applicable */
	if(fields.nbrows || tbl->srchcol & (1 << tbl->field.nbrows))
	{
		qry_build_free(&flt);
		if(	qry_add_filter(cntxt, &flt, RelNone, &fields, NoMatch, NULL) ||
			table_search_set_node(cntxt, i_ctrl, &flt, i_word) ||
			table_add_flt_idlist(cntxt, i_ctrl, &flt, 2))
			STACK_ERROR;
	}

	/* Replace IdList with IdListMatch */
	if(sql_exec_query(cntxt, "DROP TABLE IdList") ||
		sql_exec_query(cntxt, "CREATE TEMPORARY TABLE IdList TYPE=HEAP SELECT DISTINCT IdObj FROM IdListMatch") ||
		sql_exec_query(cntxt, "ALTER TABLE IdList ADD INDEX (IdObj)")) STACK_ERROR;
	tbl->totlines = cntxt->sql_nbrows;
	sql_drop_table(cntxt, "IdListMatch");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : table_word_search
** Description : return a list of objects matching search condition
*********************************************************************/
#define ERR_FUNCTION "table_word_search"
#define ERR_CLEANUP	qry_build_free(&flt); \
					DYNTAB_FREE(data); \
					cntxt->sql_trace = sql_trace
int table_word_search(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	ObjTableFormat *tbl = ctrl->objtbl;
	QryBuild flt = { 0 };
	size_t sz = tbl->input ? strlen(tbl->input) : 0;
	int b_outputall = !(tbl->status & TblCtrl_opensearch) || !sz;
	int sql_trace = cntxt->sql_trace;
	DynTable data = {0};
	unsigned long i, j, k;
	char *filter_type = CTRL_ATTR_VAL(FILTER_TYPE);
	char *val;
	if(tbl->b_ctrl) tbl->attr = &ctrl->attr;
	if(!tbl->input) tbl->input = "";

	/* Handle filter debug */
	if(*DYNTAB_FIELD_VAL(tbl->attr, DEBUG_SQL_FILTER) == '1') cntxt->sql_trace = DEBUG_SQL_RES;

	/* Build selected objects table from base filters if applicable */
	if(!tbl->from_idlist)
	{
		/* Create temporary results list table */
		if(sql_exec_query(cntxt, 
			"CREATE TEMPORARY TABLE IdList (IdObj INT NOT NULL, PRIMARY KEY (IdObj)) TYPE=HEAP"))
			STACK_ERROR;

		/* Search mode with insufficient input : return with empty list */
		if(tbl->status & TblCtrl_opensearch && tbl->minsearchlen && sz < tbl->minsearchlen) RETURN_OK;

		/* Select initial filters */
		if(!tbl->selfilter.nbrows && tbl->altfilter.nbrows)
		{
			char *sel = DYNTAB_FIELD_VAL(tbl->attr, DEFAUT_FILTERS);
			int isel = atoi(sel);
			if(isel > 0) isel--;
			if(*sel) DYNTAB_ADD_CELL(&tbl->selfilter, 0, 0, &tbl->altfilter, isel, 0);
		}

		/* Process extra filters list */
		for(i = 0; i < tbl->altfilter.nbrows; i++)
		{
			char *typ = dyntab_val(&tbl->typfilter, i, 0);
			if(!strcmp(typ, "_EVA_ALTFILTER_FILTER"))
			{
				/* Filter selected on condition : read & eval condition */
				DynTableCell *c = dyntab_cell(&tbl->condfilter, i, 0);
				if(!c || !c->txt || !c->len) continue;
				if(ctrl_eval_fieldexpr(cntxt, ctrl, &data, c->txt)) CLEAR_ERROR_CONT;

				/* Matched condition : set col attribute of tbl->typfilter */
				val = dyntab_val(&data, 0, 0);
				if(*val && strcmp("0", val) && stricmp("NULL", val)) 
					dyntab_cell(&tbl->typfilter, i, 0)->col = 1;
			}
			else if(*typ)
			{
				/* Filter when selected user group : check */
				int b_found = 0;
				for(k = 0; !b_found && k < tbl->groupfilter.nbcols; k++)
				{
					DynTableCell *c = dyntab_cell(&tbl->groupfilter, i, k);
					if(c && c->len)
						for(j = 0; !b_found && j < cntxt->user_groups.nbrows; j++)
							b_found = !dyntab_txt_cmp(c->txt, c->len, &cntxt->user_groups, j, 0);
				}

				/* Matched condition : set col attribute of tbl->typfilter */
				if(b_found ^ !strcmp(typ, "_EVA_NOTGROUP")) 
					dyntab_cell(&tbl->typfilter, i, 0)->col = 1;
			}
		}

		/* Add table main filter */
		flt.srcjoin = 0;
		DYNTAB_FIELD(&data, tbl->attr, BASE_FORMS);
		if(data.nbrows && qry_add_filter_forms(cntxt, &flt, &data, NULL)) STACK_ERROR;
		if((!data.nbrows || *filter_type && strcmp(filter_type, "_EVA_FILTER_FORM")) &&
			qry_parse_filter_nodes(cntxt, &flt, tbl->attr)) STACK_ERROR;
	}
	
	/* Add user selected filters */
	for(i = 0; i < tbl->selfilter.nbrows; i++) 
	{
		flt.srcjoin = 0;
		if(qry_obj_idfield(cntxt, &data, DYNTAB_TOULRC(&tbl->selfilter, i, 0), 0) ||
			data.nbrows && qry_parse_filter_nodes(cntxt, &flt, &data)) STACK_ERROR;
	}

	/* Add matched condition filters */
	if(!tbl->from_idlist) for(i = 0; i < tbl->typfilter.nbrows; i++) 
	{
		DynTableCell *c = dyntab_cell(&tbl->typfilter, i, 0);
		if(!c->col) continue;
		flt.srcjoin = 0;
		if(qry_obj_idfield(cntxt, &data, DYNTAB_TOULRC(&tbl->altfilter, i, 0), 0) ||
			data.nbrows && qry_parse_filter_nodes(cntxt, &flt, &data)) STACK_ERROR;
	}

	/* Fill IdList with initial filter results */
	if(qry_build_clauses(cntxt, &flt, 1) ||
		table_add_flt_idlist(cntxt, i_ctrl, &flt, tbl->from_idlist ? 3 : 0)) STACK_ERROR;
	tbl->totlines0 = tbl->totlines; 

	/* Search inactive or no input : return with full list */
	if(b_outputall) RETURN_OK;
	
	/* Process search string depending on column format */
	DYNTAB_FREE(data);
	DYNTAB_FREE(tbl->srchwords);
	if(sz && !strcmp(tbl->srchfmt, "_EVA_AGE"))
	{
		/* Search in age column : use date comparison */
		char date[16] = {0};
		char *agesep = strchr(tbl->input, '-');
		int agemin = atoi(tbl->input), agemax = agesep ? atoi(agesep + 1) : 0;
		agemin = (!agesep || agesep > tbl->input) ? atoi(tbl->input) : 0;
		agemax = agesep ? atoi(agesep + 1) : 0;
		if(agemin && !delay_to_datetxt(date, cntxt->timestamp, -agemin, 'Y'))
			DYNTAB_ADD(&tbl->srchwords, 1, 0, date, 0, NO_CONV);
		agemax = agemax ? agemax + 1 : agesep ? 0 : agemin + 1;
		if(agemax)
		{
			delay_to_datetxt(date, cntxt->timestamp, -agemax, 'Y');
			DYNTAB_ADD(&tbl->srchwords, 0, 0, date, 0, NO_CONV);
		}
		if(table_search_filter(cntxt, i_ctrl, 0)) STACK_ERROR;
	}
	else if(sz && !strncmp(tbl->srchfmt, add_sz_str("_EVA_DATE")))
	{
		/* Search in date column */
		char date[16], *datetxt = tbl->input;
		char *datesep = strchr(tbl->input, '-');
		size_t s;
		if(datesep) *datesep = 0;
		if(!input_to_datetxt(tbl->input, date)) datetxt = date;
		s = strlen(datetxt);
		DYNTAB_ADD(&tbl->srchwords, 0, 0, datetxt, 0, NO_CONV);
		if(datesep)
		{
			input_to_datetxt(datesep + 1, date);
			*datesep = '-';
		}
		else
		{
			delay_to_datetxt(date, date, 1, (char)(s > 9 ? 'h' : s > 7 ? 'D' : s > 5 ? 'M' : 'Y'));
			date[s] = 0;
		}
		DYNTAB_ADD(&tbl->srchwords, 1, 0, date, 0, NO_CONV);
		if((dyntab_sz(&tbl->srchwords, 0, 0) || dyntab_sz(&tbl->srchwords, 1, 0)) &&
			table_search_filter(cntxt, i_ctrl, 0)) STACK_ERROR;
	}
	else if(sz > 2 && tbl->input[0] == '"' && tbl->input[sz - 1] == '"')
	{
		/* Double quoted input : use single string */
		DYNTAB_ADD(&tbl->srchwords, 0, 0, tbl->input + 1, sz - 2, NO_CONV);
		if(table_search_filter(cntxt, i_ctrl, 0)) STACK_ERROR;
	}
	else
	{
		/* Split words in search string */
		unsigned long i;
		if(dyntab_from_list(&tbl->srchwords, tbl->input, strlen(tbl->input), " ", 0, 0)) 
			RETURN_ERR_MEMORY; 

		/* Build match list for each word */
		for(i = 0; i < tbl->srchwords.nbrows; i++)
			if(table_search_filter(cntxt, i_ctrl, i)) CLEAR_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
