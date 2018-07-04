/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : EVA_dll.c
** Description : Windows interface for EVA data access
**        Note : library files must be compiled with the _EVA_DLL macro defined (compiler definition)
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#define STRICT

#include <windows.h>
#include "eva.h"

EVA_context _cntxt;
EVA_context *cntxt = &_cntxt;
DynTable ObjCache = {0};
DynTable ValuesCache = {0};
PivotTable PvCache = {0};


/*********************************************************************
** Function : DllMain
** Description : Windows DLL entry point
*/
BOOL WINAPI DllMain( HINSTANCE hinstDLL, 
                     DWORD fdwReason,
                     LPVOID lpReserved )
{
	/* Nothing special to do - just use params for no compiler warnings */
	int b = hinstDLL || fdwReason || lpReserved;

	/* Initialize context */
	dyntab_from_list(&cntxt->daylong, add_sz_str("dimanche,lundi,mardi,mercredi,jeudi,vendredi,samedi"), ",", 0, 0);
	dyntab_from_list(&cntxt->dayshort, add_sz_str("dim,lun,mar,mer,jeu,ven,sam"), ",", 0, 0);
	dyntab_from_list(&cntxt->monthlong, add_sz_str("Janvier,Février,Mars,Avril,Mai,Juin,Juillet,Août,Septembre,Octobre,Novembre,Décembre"), ",", 0, 0);
	dyntab_from_list(&cntxt->monthshort, add_sz_str("Jan,Fev,Mar,Avr,Mai,Jun,Jul,Aou,Sep,Oct,Nov,Dec"), ",", 0, 0);

	return( b | 1 );

}


/*********************************************************************
** Function : DateUnix_to_asc
** Description : convert a unix time to local date format
*/
long __stdcall DateUnix_to_asc(				/* return : 0 if Ok, 1 on error */
	char *res,								/* out : converted date */
	long t,									/* in : unix time to convert */
	long b_french							/* in : format to use
													0 = MM/DD/YYYY HH:MM:SS,
												other = DD/MM/YYYY HH:MM:SS */
){
	struct tm *lt = localtime(&((time_t)t));
	if(!lt || !res) return 1;
	sprintf(res, "%02d/%02d/%04d %02d:%02d:%02d",
		b_french ? lt->tm_mday : lt->tm_mon + 1,
		b_french ? lt->tm_mon + 1 : lt->tm_mday,
		lt->tm_year + 1900,
		lt->tm_hour,
		lt->tm_min,
		lt->tm_sec);
	return 0;
}

/*********************************************************************
** Function : DB_connect
** Description : select database for next DLL calls
*/
long __stdcall DB_connect(			/* return : 0 if Ok, 1 on error */
	char *server_adr,						/* in : MySql server address */
	char *db_name							/* in : MySql database to use */
){
	/* Check & initialize params */
	if(!db_name) return 1;
	if(server_adr && !*server_adr) server_adr = NULL;

	/* Return if same as current connection */
	cntxt->sql_trace = DEBUG_SQL_RES;
	if(!STRCMPNUL(db_name, cntxt->dbname) && !STRCMPNUL(server_adr, cntxt->srvaddr) && cntxt->sql_session) return 0;

	/* Close prev session, set cx params & open new session */
	sql_control(cntxt, 0);
	M_FREE(cntxt->dbname);
	cntxt->dbname = mem_strdup(db_name);
	M_FREE(cntxt->srvaddr);
	if(server_adr) cntxt->srvaddr = mem_strdup(server_adr);
	sql_open_session(cntxt);

	return cntxt->err.text || !cntxt->sql_session;
}

/*********************************************************************
** Function : return_dyntab_value
** Description : return a value from DynTable
*/
long return_dyntab_value(					/* return : 0 if Ok, 1 on error */
	char *res,								/* out : object label */
	size_t sz_max,							/* in : max # of chars to output to res */
	DynTable *tab,							/* in : table to read values from */
	unsigned long row,						/* in : desired value row */
	unsigned long col,						/* in : desired value column */
	unsigned long *num,						/* out : value Num index */
	unsigned long *line,					/* out : value Line index */
	unsigned long *nbval					/* out : total # of values for field */
){
	DynTableCell *c = NULL;

	/* Check & initialize params */
	if(!res || !sz_max) return 1;
	res[0] = 0;
	if(num) *num = 0;
	if(line) *line = 0;
	if(nbval) *nbval = tab->nbrows;

	/* Output value to result if found */
	c = dyntab_cell(tab, row, col);
	if(c)
	{
		if(c->txt) strncpy(res, c->txt, sz_max - 1);
		res[sz_max - 1] = 0;
		if(num) *num = c->Num;
		if(line) *line = c->Line;
	}

	return 0;
}

/*********************************************************************
** Function : Get_result_value
** Description : return a value from results cache
*/
long __stdcall Get_result_value(			/* return : 0 if Ok, 1 on error */
	char *res,								/* out : required value */
	size_t sz_max,							/* in : max # of chars to output to res */
	unsigned long row,						/* in : desired value row */
	unsigned long col,						/* in : desired value column */
	unsigned long *num,						/* out : value Num index */
	unsigned long *line,					/* out : value Line index */
	unsigned long *nbval					/* out : total # of values for field */
){
	return return_dyntab_value(res, sz_max, &ValuesCache, row, col, num, line, nbval);
}

/*********************************************************************
** Function : Get_debug_msg
** Description : return debug messages
*/
long __stdcall Get_debug_msg(				/* return : 0 if Ok, 1 on error */
	char *res,								/* out : object label */
	size_t sz_max							/* in : max # of chars to output to res */
){
	if(cntxt->debug_msg) strncpy(res, cntxt->debug_msg->data, sz_max);
	M_FREE(cntxt->debug_msg);
	return 0;
}

/*********************************************************************
** Function : Get_debug_html
** Description : return debug messages
*/
long __stdcall Get_debug_html(				/* return : 0 if Ok, 1 on error */
	char *res,								/* out : object label */
	size_t sz_max							/* in : max # of chars to output to res */
){
	err_save_context(cntxt);
	if(cntxt->debug_html) strncpy(res, cntxt->debug_html->data, sz_max);
	err_clear(cntxt, NULL, 0, 0);
	cntxt->sql_cnt = 0;
	M_FREE(cntxt->debug_html);
	return 0;
}

/*********************************************************************
** Function : Get_field_value
** Description : return a value of an object field
*/
long __stdcall Get_field_value(				/* return : 0 if Ok, 1 on error */
	char *res,								/* out : object label */
	size_t sz_max,							/* in : max # of chars to output to res */
	unsigned long idobj,					/* in : object id to read */
	char *field,							/* in : desired field */
	unsigned long *nbval					/* out : total # of values for field */
){
	DynTable id_obj = {0};
	char buf[32];
	int b_err = !res || !sz_max || !field || !*field;
	cntxt->sql_cnt = 0;

	/* Read object data if not in cache */
	b_err = b_err ||
			(!ObjCache.cell || ObjCache.cell->IdObj != idobj) &&
			dyntab_add(&id_obj, 0, 0, itoa(idobj, buf, 10), 0, NO_CONV) ||
			qry_obj_data(cntxt, &ObjCache, &id_obj, NULL);

	/* Read field values from object data */
	dyntab_free(&ValuesCache);
	b_err = b_err || dyntab_filter_field(&ValuesCache, 0, &ObjCache, field, ~0UL, NULL);

	/* Output value to result if found */
	b_err = return_dyntab_value(res, sz_max, &ValuesCache, 0, 0, NULL, NULL, nbval) || b_err;
	DYNTAB_FREE(id_obj);
	return b_err;
}

/*********************************************************************
** Function : Calc_pivot_table
** Description : calc pivot table for given parameters & set results cache
*/
long __stdcall Calc_pivot_table(			/* return : 0 if Ok, 1 on error */
	char *res,								/* out : pivot table data */
	size_t sz_max,							/* in : max # of chars to output to res */
	char *forms,							/* in : allowed formstamps (comma separated IdObj) */
	char *condexpr,							/* in : selection condition (SQL/relational expression) */
	char *rowexpr,							/* in : data for rows (SQL/relational expression) */
	char *colexpr,							/* in : data for columns (SQL/relational expression) */
	char *dataexpr,							/* in : data for cells (SQL/relational expression) */
	char *function,							/* in : group function for cells */
	unsigned long *nbrows,					/* out : total # of rows (including column labels) */
	unsigned long *nbcols					/* out : total # of columns (including row labels) */
){
	int b_err = !res || !sz_max;
	cntxt->sql_cnt = 0;

	/* Initialize pivot table */
	dyntab_add(&PvCache.cols[0].expr, 0, 0, colexpr, 0, NO_CONV);
	PvCache.nbcol = 1;
	dyntab_add(&PvCache.rows[0].expr, 0, 0, rowexpr, 0, NO_CONV);
	PvCache.nbrow = 1;
	dyntab_add(&PvCache.datas[0].expr, 0, 0, dataexpr, 0, NO_CONV);
	PvCache.nbdata = 1;
	dyntab_add(&PvCache.datas[0].function, 0, 0, function, 0, NO_CONV);
	PvCache.condexpr = condexpr;
	dyntab_from_list(&PvCache.forms, forms, strlen(forms), ",", 0, 1);
	/* Compute pivot table */
	b_err = b_err ||
			pivottable_calc_single(cntxt, &PvCache);

	/* Copy result in TAB/RC format */
	if(!b_err)
	{
		unsigned long i, j;
		size_t sz = 0;
		for(i = 0; i < PvCache.res->nbrows && sz < sz_max; i++)
		{
			if(i) sz += sprintf(res + sz, "\r");
			for(j = 0; j < PvCache.res->nbcols; j++)
			{
				size_t sz1 = dyntab_sz(PvCache.res, i, j);
				char *txt = dyntab_val(PvCache.res, i, j);
				if(j) sz += sprintf(res + sz, "\t");
				if(sz + sz1 + 40 >= sz_max)
				{
					sprintf(res + sz, "#ErrTooLong");
					sz = sz_max;
					break;
				}
				if(!sz1 && !(j && !i && PvCache.res->nbcols > 2 || i && !j && PvCache.res->nbrows > 2)) continue;
				if(!*txt) txt = "(non renseigné)";
				sz += sprintf(res + sz, "%s", txt);
			}
		}
	}
	else
		snprintf(res, sz_max, "#%s#", cntxt->err.text);


	if(nbrows) *nbrows = PvCache.res->nbrows;
	if(nbcols) *nbcols = PvCache.res->nbcols;
	return b_err;
}

/*********************************************************************
** Function : Get_filter_idobj
** Description : return	list of object matching given form / filter & condition
*/
long __stdcall Get_filter_idobj(			/* return : 0 if Ok, 1 on error */
	char *res,								/* out : list of matching IdObj (RC separated) */
	size_t sz_max,							/* in : max # of chars to output to res */
	unsigned long *nbres,					/* out : # of IdObj in res */
	char *forms,							/* in : allowed formstamps (comma separated IdObj) */
	char *filters,							/* in : list of filter to apply (comma separated IdObj - AND combined) */
	char *condexpr							/* in : selection condition (SQL/relational expression) */
){
	QryBuild flt = {0};
	DynTable idforms = {0};
	DynTable idfilters = {0};
	DynTable idobj = {0};
	DynTable empty = {0};
	DynBuffer *sql = NULL;
	int b_err = !res || !sz_max;
	cntxt->sql_cnt = 0;

	/* Build & evaluate forms & filters condition */
	b_err = b_err ||
			dyntab_from_list(&idforms, forms, strlen(forms), ",", 0, 1) ||
			dyntab_from_list(&idfilters, filters, strlen(filters), ",", 0, 1) ||
			qry_add_filter_forms(cntxt, &flt, &idforms, &idfilters) ||
			qry_build_clauses(cntxt, &flt, 1) ||
			dynbuf_add(&sql, add_sz_str(
				"-- Get_filter_idobj : evaluate forms & filters condition\n"
				"CREATE TEMPORARY TABLE IdList TYPE=HEAP\n"), NO_CONV) ||
			qry_build_flt_select(cntxt, &sql, &empty, &flt, 0, 0) ||
			sql_exec_query(cntxt, sql->data);
	b_err = b_err ||
			qry_filter_sql_fieldexpr(cntxt, &idobj, condexpr, "IdList", NULL);

	/* Copy result in TAB/RC format */
	if(nbres) *nbres = idobj.nbrows;
	if(!b_err)
	{
		unsigned long i;
		size_t sz = 0;
		sz_max -= 15;
		for(i = 0; i < idobj.nbrows && sz < sz_max; i++)
		{
			if(i) sz += sprintf(res + sz, "\r");
			sz += sprintf(res + sz, "%s", dyntab_val(&idobj, i, 0));
		}
	}
	else
		snprintf(res, sz_max, "#%s#", cntxt->err.text);

	/* Free allocated ressources */
	qry_build_free(&flt);
	dyntab_free(&idforms);
	dyntab_free(&idfilters);
	dyntab_free(&idobj);
	M_FREE(sql);
	sql_drop_table(cntxt, "IdList");
	return b_err;
}

/*********************************************************************
** Function : Eval_expression
** Description : return the result of a mixed SQL / fields expression
*/
long __stdcall Eval_expression(				/* return : 0 if Ok, 1 on error */
	char *res,								/* out : object label */
	size_t sz_max,							/* in : max # of chars to output to res */
	unsigned long idobj,					/* in : object id to use as current object for expression */
	char *expr,								/* in : expression to evaluate */
	unsigned long *nbval					/* out : total # of values for field */
){
	int b_err = !res || !sz_max || !expr || !*expr;

	/* Evaluate expression & return 1st value */
	b_err = b_err || form_eval_fieldexpr(cntxt, &ValuesCache, 0, idobj, expr, strlen(expr), NULL, 0);
	b_err = return_dyntab_value(res, sz_max, &ValuesCache, 0, 0, NULL, NULL, nbval) || b_err;
	return b_err;
}

/*********************************************************************
** Function : Get_table_data
** Description : read table data in values buffer
*/
long __stdcall Get_table_data(				/* return : 0 if Ok, 1 on error */
	unsigned long idobj,					/* in : object id for table calculation */
	unsigned long idctrl,					/* in : object id of table control */
	char *extracols,						/* in : list of fields (comma separated) to add to table columns */
	char *extrafmt,							/* in : list of added columns formats (comma separated) */
	char *selobj,							/* in : list of object Ids to display in table	(comma separated)
												"" = use table search filter
												"*" = use relation value (table control must be a relation) */
	unsigned long *nbrows,					/* out : # of rows in table */
	unsigned long *nbcols					/* out : # of columns in table */
){
	/* Read control data */
	EVA_form *form = calloc(1, sizeof(*form));
	EVA_ctrl *ctrl = calloc(1, sizeof(*ctrl));
	int b_err;
	char buf[32];
	if(!form || !ctrl) return 1;
	cntxt->form = form;
	form->ctrl = ctrl;
	form->nb_ctrl = 1;

	/* Read table control */
	if(idobj) dyntab_add(&form->id_obj, 0, 0, buf, snprintf(add_sz_str(buf), "%lu", idobj), NO_CONV);
	b_err = qry_obj_field(cntxt, &ctrl->attr, idctrl, "") || !ctrl->attr.nbrows;
	b_err = b_err || ctrl_set_common_attr(cntxt, ctrl) || !ctrl->TYPE[0];
	b_err = b_err || table_read_controls(cntxt, 0, NULL) || !ctrl->objtbl->field.nbrows;

	/* Read table contents */
	if(!b_err && extracols && *extracols)
	{
		if(ctrl->objtbl->field.nbrows > ctrl->objtbl->format.nbrows) dyntab_add(&ctrl->objtbl->format, ctrl->objtbl->field.nbrows - 1, 0, NULL, 0, NO_CONV);
		if(ctrl->objtbl->field.nbrows > ctrl->objtbl->label.nbrows) dyntab_add(&ctrl->objtbl->label, ctrl->objtbl->field.nbrows - 1, 0, NULL, 0, NO_CONV);
		b_err = b_err || dyntab_from_list(&ctrl->objtbl->field, extracols, strlen(extracols), ",", 0, 0) ||
						dyntab_from_list(&ctrl->objtbl->label, extracols, strlen(extracols), ",", 0, 0) ||
						dyntab_from_list(&ctrl->objtbl->format, extrafmt, strlen(extrafmt), ",", 0, 4);
	}
	if(!selobj) selobj = "";
	else if(*selobj == '*')
		b_err = b_err || qry_obj_field(cntxt, &ctrl->val, idobj, DYNTAB_FIELD_VAL(&ctrl->attr, FIELD));
	else if(!b_err && *selobj)
		b_err = b_err || dyntab_from_list(&ctrl->val, selobj, strlen(selobj), ",", 0, 0);
	b_err = b_err || table_word_search(cntxt, 0);
	if(!b_err) ctrl->objtbl->lines = ctrl->objtbl->totlines;
	b_err = b_err || table_read_obj_list(cntxt, 0, *selobj != 0);
	b_err = b_err || table_prepare_rows(cntxt, 0, "");

	/* Transfer table data to values cache on success */
	if(!b_err)
	{
		unsigned long i, row = ctrl->objtbl->cellval.nbrows;
		dyntab_free(&ValuesCache);
		memcpy(&ValuesCache, &ctrl->objtbl->cellval, sizeof(ValuesCache));
		memset(&ctrl->objtbl->cellval, 0, sizeof(ctrl->objtbl->cellval));
		for(i = 0; i < ctrl->objtbl->format.nbrows; i++)
			dyntab_add(&ValuesCache, row, i + 1, DYNTAB_VAL_SZ(&ctrl->objtbl->format, i, 0), NO_CONV);
		if(nbcols) *nbcols = ValuesCache.nbcols;
		if(nbrows) *nbrows = ValuesCache.nbrows;
	}

	/* Free allocated ressources */
	form_free(cntxt);
	return b_err;
}

/*********************************************************************
** Function : Date_add
** Description : add a period to a date
*/
long __stdcall Date_add(			/* return : 0 if Ok, 1 on error */
	char *res,						/* out : result date - must be 32 bytes long minimum */
	char *date_ref,					/* in : date reference from wich to compute date; if null use current time */
	char *delay						/* in : delay to add followed by unit among h/m/s/D/M/Y */
){
	int b_err = !res;
	char fmt = delay[strlen(delay)-1];
	int dl = atoi(delay);
	char d0[32], d1[32];
	b_err |= input_to_datetxt(date_ref, d0) ||
				delay_to_datetxt(d1, d0, dl, fmt) ||
				datetxt_to_format(cntxt, res, d1, NULL);
	return b_err;
}

/*********************************************************************
** Function : Table_add_column
** Description : add a column to the cache table
*/
long __stdcall Table_add_column(			/* return : 0 if Ok, 1 on error */
	char *expr								/* in : expression to evaluate */
){
	return qry_add_table_col(cntxt, &ValuesCache, expr, expr ? strlen(expr) : 0);
}

/*********************************************************************
** Function : Recurse_relations
** Description : build list of objects recursively related to the given objects
*/
long __stdcall Recurse_relations(				/* return : 0 if Ok, 1 on error */
	char *res,								/* out : comma separated idobj */
	char *relobj,							/* out : comma/slash separated idrelobj for each idobj */
	long sz_max,							/* in : max # of chars to output to res */
	char *idobj,							/* in : base objects id (comma separated) */
	char *relfields							/* in : relation fields (comma separated) */
){
	DynTable fields = {0};
	DynTable id_obj = {0};
	DynTable restbl = {0};
	DynBuffer *txtres = NULL;
	int b_err;
	size_t sz;
	unsigned long i;
	if(!res || sz_max <= 0) return 1;
	if(!idobj) return 0;

	/* Build & copy list of objects */
	b_err = dyntab_from_list(&fields, relfields, strlen(relfields), ",", 0, 0);
	b_err |= dyntab_from_list(&id_obj, idobj, strlen(idobj), ",", 0, 0);
	b_err |= qry_recursive_relation(cntxt, &restbl, &id_obj, &fields);
	b_err |= qry_values_list(cntxt, &restbl, 0, &txtres);
	if(!b_err && txtres) strncpy(res, txtres->data, (size_t)sz_max);
	sz = snprintf(relobj, sz_max, "%s", dyntab_val(&restbl, 0, 1));
	for(i = 1; i < restbl.nbrows && sz < (size_t)sz_max; i++)
		sz += snprintf(relobj + sz, sz_max - sz, "/%s", dyntab_val(&restbl, i, 1));

	DYNTAB_FREE(fields);
	DYNTAB_FREE(id_obj);
	DYNTAB_FREE(restbl);
	M_FREE(txtres);

	return b_err;
}

/*********************************************************************
** Function : Get_object_label
** Description : return label of an object
*/
long __stdcall Get_object_label(			/* return : 0 if Ok, 1 on error */
	char *res,								/* out : object label */
	long _sz_max,							/* in : max # of chars to output to res */
	long idobj								/* in : object id to read */
){
	DynTable id_obj = {0};
	DynTable data = {0};
	DynBuffer *label = NULL;
	DynBuffer *formlabel = NULL;
	char buf[32];
	size_t sz_max = (size_t)(_sz_max > 0 ? _sz_max : -_sz_max);
	M_FREE(cntxt->debug_msg);
	cntxt->sql_cnt = 0;

	/* Check & initialize params */
	if(!res || !sz_max) return 1;
	res[0] = 0;

	/* Read object label - set error message on failure */
	dyntab_add(&id_obj, 0, 0, itoa(idobj, buf, 10), 0, NO_CONV);
	if(qry_obj_data(cntxt, &data, &id_obj, NULL) ||
		qry_obj_label(cntxt, &formlabel, NULL, &label, NULL, NULL, NULL, NULL, NULL, 0, &data, 0))
	{
		snprintf(res, sz_max, "#Error#%s", cntxt->err.text);
		err_clear(cntxt, NULL, 0, 0);
	}
	else if(label)
	{
		/* Output label to result on success */
		if(_sz_max > 0)
			snprintf(res, sz_max, "%s", label->data);
		else
			snprintf(res, sz_max, "%s [%s]", formlabel ? formlabel->data : "???", label->data);
	}

	/* Free temp data & return error status */
	M_FREE(label);
	M_FREE(formlabel);
	DYNTAB_FREE(id_obj);
	DYNTAB_FREE(data);
	return cntxt->err.text != NULL;
}

/*********************************************************************
** Function : qry_search_form_txt
** Description : search a text in objects created with given form label
*********************************************************************/
#define ERR_FUNCTION "qry_search_form_txt"
#define ERR_CLEANUP M_FREE(sql); \
					M_FREE(sql1)
int qry_search_form_txt(					/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in : execution context data */
	DynTable *id_list,						/* out : results list (content depends on b_obj) */
	char *form,								/* in : form label */
	char *fields,							/* in : comma separated fields to search (all if empty) */
	char *srchtxt,							/* in : text to search for */
	char *srchmode,							/* in : search mode = Exact|Content|Begin */
	int b_obj								/* in : return IdObj if not 0, Pkey otherwise */
){
	DynBuffer *sql = NULL;
	DynBuffer *sql1 = NULL;

	/* Check params */
	if(!srchtxt || !*srchtxt || !id_list) return 0;

	/* Prepare SELECT clause */
	DYNBUF_ADD3(&sql,
		"SELECT DISTINCT TLink.", b_obj ? "IdObj" : "Pkey", 0, NO_CONV, " FROM TLink \n"
		"INNER JOIN TVal ON TVal.IdValue=TLink.IdValue \n");

	/* Add joins for form label condition */
	if(form && *form) DYNBUF_ADD_STR(&sql,
		"INNER JOIN TLink AS TLink0 ON TLink0.IdObj=TLink.IdObj \n");

	/* Prepare WHERE clause for given text */
	DYNBUF_ADD_STR(&sql,
		"WHERE TLink.DateDel IS NULL AND TVal.TxtValue");
	if(!stricmp(srchmode, "Content")) DYNBUF_ADD3(&sql, " LIKE '%", srchtxt, 0, SQL_NO_QUOTE, "%' \n")
	else if(!stricmp(srchmode, "Begin")) DYNBUF_ADD3(&sql, " LIKE '", srchtxt, 0, SQL_NO_QUOTE, "%' \n")
	else DYNBUF_ADD3(&sql, "='", srchtxt, 0, SQL_NO_QUOTE, "' \n");

	/* Process fields list */
	if(fields && *fields)
	{
		char *p0 = fields;
		int nb = 0;

		/* Add condition on search fields */
		DYNBUF_ADD_STR(&sql, "AND TLink.IdField IN (");
		while(p0)
		{
			char *p1 = strchr(p0, ',');
			size_t sz = p1 ? p1 - p0 : strlen(p0);
			char *field = malloc(sz + 1);
			unsigned long id;
			if(!field) RETURN_ERR_MEMORY;
			sprintf(field, "%.*s", sz, p0);
			if(sql_id_value(cntxt, field, sz, &id)) STACK_ERROR;
			if(id)
			{
				if(nb++) DYNBUF_ADD_STR(&sql, ",");
				DYNBUF_ADD_INT(&sql, id)
			}
			free(field);
			p0 = p1;
		}

		/* Do not exec query if no fields found */
		if(!nb) M_FREE(sql)
		else DYNBUF_ADD_STR(&sql, ") \n");
	}

	/* Add condition on form label */
	if(sql && form && *form)
	{
		/* Search for forms with given label */
		DYNBUF_ADD3_INT(&sql1,
			"SELECT DISTINCT TLink.IdObj FROM TLink \n"
			"INNER JOIN TVal ON TVal.IdValue=TLink.IdValue \n"
			"INNER JOIN TLink AS TLink0 ON TLink0.IdObj=TLink.IdObj \n"
			"INNER JOIN TLink AS TLink1 ON TLink1.IdObj=TLink.IdObj \n"
			"WHERE TLink0.DateDel IS NULL \n"
			"AND TLink0.IdField=", IDVALUE(FORMSTAMP), " \n");
		DYNBUF_ADD3_INT(&sql1,
			"AND TLink0.IdRelObj=", OBJ_FORM_CONTROL, " \n");
		DYNBUF_ADD3(&sql1,
			"AND TLink.DateDel IS NULL \n"
			"AND TVal.TxtValue='", form, 0, SQL_NO_QUOTE, "' \n");
		DYNBUF_ADD3_INT(&sql1,
			"AND TLink.IdField=", IDVALUE(LABEL), " \n");
		DYNBUF_ADD3_INT(&sql1,
			"AND TLink1.DateDel IS NULL \n"
			"AND TLink1.IdValue=", IDVALUE(FORM), " \n");
		DYNBUF_ADD3_INT(&sql1,
			"AND TLink1.IdField=", IDVALUE(CONTROL), " \n");
		if(sql_exec_query(cntxt, sql1->data) || sql_get_table(cntxt, id_list, 2)) STACK_ERROR;

		/* If form was found - add condition on form */
		if(id_list->nbrows)
		{
			DYNBUF_ADD3_INT(&sql,
				"AND TLink0.DateDel IS NULL \n"
				"AND TLink0.IdField=", IDVALUE(FORMSTAMP), " \n");
			DYNBUF_ADD3_CELL(&sql,
				"AND TLink0.IdRelObj=", id_list, 0, 0, NO_CONV, NULL);
		}
		else
			/* Do not exec query if no form found */
			M_FREE(sql);
	}

	/* Exec query if applicable */
	dyntab_free(id_list);
	if(sql && sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, id_list, 2)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : Form_txt_search
** Description : search a text in objects created with given form label
*/
int __stdcall Form_txt_search(				/* return : matching data */
	char *res,								/* out : found data */
	long _sz_max,							/* in : max # of chars to output to res */
	char *form,								/* in : form label */
	char *fields,							/* in : comma separated fields to search (all if empty) */
	char *srchtxt,							/* in : text to search for */
	char *srchmode,							/* in : search mode = Exact|Content|Begin */
	char *retfield,							/* in : returned field expression */
	char *separator							/* in : returned field expression */
){
	DynTable id_obj = {0};
	DynTable data = {0};
	QryBuild flt = {0};
	int b_err = 0;
	size_t sz_max = (size_t) _sz_max - 1;
	size_t sep_len;
	M_FREE(cntxt->debug_msg);
	cntxt->sql_cnt = 0;

	if(!separator || !*separator) separator = ", ";
	sep_len = strlen(separator);
	memset(res, 0, sz_max);

	b_err = 
		/* Search for objects matching conditions */
		qry_search_form_txt(cntxt, &id_obj, form, fields, srchtxt, srchmode, 1) || id_obj.nbrows && (

		/* Read matching objects data */
		qry_parse_fieldexpr(cntxt, retfield, strlen(retfield), &flt) ||
		sql_get_listcol(cntxt, &data, &id_obj, &flt, 0));

	/* Set result string */
	if(b_err)
		snprintf(res, sz_max, "#Error#");
	else
	{
		size_t cnt = 0;
		unsigned long i;
		for(i = 0; i < data.nbrows && cnt + dyntab_sz(&data, i, 0) + sep_len < sz_max; i++)
		{
			if(cnt) cnt += sprintf(res + cnt, "%s", separator);
			cnt += sprintf(res + cnt, "%s", dyntab_val(&data, i, 0));
		}
	}

	/* Free temp data & return result */
	qry_build_free(&flt);
	dyntab_free(&id_obj);
	dyntab_free(&data);
	return b_err;
}
