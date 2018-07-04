/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : qry_build_to_sql.c
** Description : utility functions for converting to filters to relational expressions
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : qry_filter_rel_to_sql
** Description : build query nodes for a filter from filter data
*********************************************************************/
#define ERR_FUNCTION "qry_filter_rel_to_sql"
#define ERR_CLEANUP DYNTAB_FREE(fltobj); \
					DYNTAB_FREE(fltval); \
					DYNTAB_FREE(resdata)
int qry_filter_rel_to_sql(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **sqlexpr,			/* in/out : SQL expression is added to the buffer */
	DynTable *fltfield,				/* in : filter fields */
	DynTable *flt_data				/* in : filter definition data */
){
	DynTable fltval = { 0 };
	DynTable fltobj = { 0 };
	DynTable resdata = { 0 };
	char *reltype = DYNTAB_FIELD_VAL(flt_data, FILTER_RELTYPE);
	char *fltop = DYNTAB_FIELD_VAL(flt_data, FILTER_RELWITH);
	int b_not = DYNTAB_FIELD_VAL(flt_data, FILTER_NOT)[0] != 0;
	DYNTAB_FIELD(&fltobj, flt_data, LISTOBJ);
	DYNTAB_FIELD(&fltval, flt_data, FILTER_LINKFIELD);

	/* Add relation type, field, & NOT operator if applicable */
	DYNBUF_ADD(sqlexpr, !strcmp("_EVA_RELREVERSE", reltype) ? "[FIRST(<-" : "[FIRST(", 0, NO_CONV);
	if(!fltfield->nbrows)
	{
		DYNBUF_ADD_STR(sqlexpr, "->");
	}
	else
		if(dyntab_to_dynbuf(fltfield, sqlexpr, ",", 1, ",", 1, NO_CONV)) RETURN_ERR_MEMORY;
	DYNBUF_ADD_STR(sqlexpr, ")]");

	if(*fltop)
	{
		/* Relation to field/expression of given object : evaluate field expression */
		if(form_eval_valtype(cntxt, &resdata, fltop, &fltobj, &fltval)) STACK_ERROR;
		if(b_not) DYNBUF_ADD_STR(sqlexpr, " NOT");
		DYNBUF_ADD_STR(sqlexpr, " IN(");
		if(qry_values_list(cntxt, &resdata, 0, sqlexpr)) STACK_ERROR;
		DYNBUF_ADD_STR(sqlexpr, ")");
	}
	else 
	{
		/* Relation to objects matching other filter */
		DYNTAB_FIELD(&fltval, flt_data, FILTERS);
		if(fltval.nbrows)
		{
			RETURN_ERROR("Relation to objects matching other filter is not implemented yet", NULL);
		}
		else
			DYNBUF_ADD_STR(sqlexpr, "<>''");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_filter_val_to_sql
** Description : build query nodes for a filter from filter data
*********************************************************************/
#define ERR_FUNCTION "qry_filter_val_to_sql"
#define ERR_CLEANUP DYNTAB_FREE(fltobj); \
					DYNTAB_FREE(fltval); \
					DYNTAB_FREE(resdata)
int qry_filter_val_to_sql(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **sqlexpr,			/* in/out : SQL expression is added to the buffer */
	DynTable *fltfield,				/* in : filter fields */
	DynTable *flt_data				/* in : filter definition data */
){
	DynTable fltval = { 0 };
	DynTable fltobj = { 0 };
	DynTable resdata = { 0 };
	unsigned long i;
	char *fltop = DYNTAB_FIELD_VAL(flt_data, FILTER_OP);
	char *valtype = DYNTAB_FIELD_VAL(flt_data, FILTER_VAL_TYPE);
	int b_std = 1, b_num = 0;
	char *bef = " IN (", *aft = ")", *oper = NULL;
	DYNTAB_FIELD(&fltobj, flt_data, LISTOBJ);
	DYNTAB_FIELD(&fltval, flt_data, FILTER_VALUE);

	/* Set match mode & values depending on comparison type */
#define USE_STD_VAL(tag, b, a, o) if(!strcmp("_EVA_"#tag, fltop)) { bef = b; aft = a; oper = o; }
	USE_STD_VAL(DIFFERENT, " NOT IN (", ")", NULL) else
	USE_STD_VAL(SMALLER, "<GREATEST('',", ")", NULL) else
	USE_STD_VAL(SMALLEREQUAL, "<=GREATEST('',", ")", 0) else
	USE_STD_VAL(GREATER, ">LEAST('',", ")", NULL) else
	USE_STD_VAL(GREATEREQUAL, ">=LEAST('',", ")", NULL) else
	USE_STD_VAL(BEGINWITH, " LIKE '", "%'", " OR ") else
	USE_STD_VAL(NOT_BEGINWITH, " NOT LIKE '", "%'", " OR ") else
	USE_STD_VAL(CONTAIN, " LIKE '%", "%'", " AND ") else
	USE_STD_VAL(NOT_CONTAIN, " NOT LIKE '%", "%'", " AND ") else
	USE_STD_VAL(LIKE, " LIKE '", "'", " OR ") else
	USE_STD_VAL(NOT_LIKE, " NOT LIKE '", "'", " AND ")	else
	USE_STD_VAL(ISEMPTY, "=''", "", "")	else
	USE_STD_VAL(ISNOTEMPTY, "<>''", "", "")	
#undef USE_STD_VAL
	else if(!strcmp("_EVA_INTERVAL", fltop) || !strcmp("_EVA_NOT_INTERVAL", fltop))
	{
		b_std = 0;
		if(strcmp("_EVA_INTERVAL", fltop)) DYNBUF_ADD_STR(sqlexpr, "NOT ");

		/* Evaluate min comparison values */
		DYNTAB_FIELD(&fltval, flt_data, FILTER_VAL_MIN);
		if(form_eval_valtype(cntxt, &resdata, valtype, &fltobj, &fltval)) STACK_ERROR;
		DYNBUF_ADD_STR(sqlexpr, "([FIRST(");
		if(dyntab_to_dynbuf(fltfield, sqlexpr, ",", 1, ",", 1, NO_CONV)) RETURN_ERR_MEMORY;
		DYNBUF_ADD_STR(sqlexpr, ")]>LEAST(");
		if(qry_values_list(cntxt, &resdata, b_num ? 0 : 5, sqlexpr)) STACK_ERROR;
		DYNBUF_ADD_STR(sqlexpr, ")");

		/* Evaluate max comparison values */
		DYNTAB_FIELD(&fltval, flt_data, FILTER_VAL_MAX);
		if(form_eval_valtype(cntxt, &resdata, valtype, &fltobj, &fltval)) STACK_ERROR;
		DYNBUF_ADD_STR(sqlexpr, " AND [FIRST(");
		if(dyntab_to_dynbuf(fltfield, sqlexpr, ",", 1, ",", 1, NO_CONV)) RETURN_ERR_MEMORY;
		DYNBUF_ADD_STR(sqlexpr, ")]<=GREATEST(");
		if(qry_values_list(cntxt, &resdata, b_num ? 0 : 5, sqlexpr)) STACK_ERROR;
		DYNBUF_ADD_STR(sqlexpr, "))");
	}

	/* If comparison value not processed */
	if(b_std)
	{
		/* Evaluate comparison value if applicable */
		if((!oper || *oper) && form_eval_valtype(cntxt, &resdata, valtype, &fltobj, &fltval)) STACK_ERROR;
		
		/* Combine values if applicable */
		if(oper && *oper) for(i = 0; !i || i < resdata.nbrows; i++)
		{
			DYNBUF_ADD_STR(sqlexpr, "[FIRST(");
			if(dyntab_to_dynbuf(fltfield, sqlexpr, ",", 1, ",", 1, NO_CONV)) RETURN_ERR_MEMORY;
			DYNBUF_ADD_STR(sqlexpr, ")]");
			if(i) DYNBUF_ADD3(sqlexpr, " ", oper, 0, NO_CONV, " ");
			DYNBUF_ADD(sqlexpr, bef, 0, NO_CONV);
			DYNBUF_ADD_CELL(sqlexpr, &resdata, i, 0, NO_CONV);
			DYNBUF_ADD(sqlexpr, aft, 0, NO_CONV);
		}
		else
		{
			/* Add list of values */
			DYNBUF_ADD_STR(sqlexpr, "[FIRST(");
			if(dyntab_to_dynbuf(fltfield, sqlexpr, ",", 1, ",", 1, NO_CONV)) RETURN_ERR_MEMORY;
			DYNBUF_ADD_STR(sqlexpr, ")]");
			DYNBUF_ADD(sqlexpr, bef, 0, NO_CONV);
			if(!oper && qry_values_list(cntxt, &resdata, b_num ? 0 : 5, sqlexpr)) STACK_ERROR;
			DYNBUF_ADD(sqlexpr, aft, 0, NO_CONV);
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_filter_to_sql
** Description : build query nodes for a filter from filter data
*********************************************************************/
#define ERR_FUNCTION "qry_filter_to_sql"
#define ERR_CLEANUP DYNTAB_FREE(fltfield); \
					DYNTAB_FREE(fltobj); \
					DYNTAB_FREE(fltval)
int qry_filter_to_sql(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **sqlexpr,			/* in/out : SQL expression is added to the buffer */
	DynTable *flt_data				/* in : filter definition data */
){
	DynTable fltfield = { 0 };
	DynTable fltval = { 0 };
	DynTable fltobj = { 0 };
	char *flttype = DYNTAB_FIELD_VAL(flt_data, FILTER_TYPE);

	/* Process depending on filter type */
	if(!strcmp("_EVA_FILTER_LISTOBJ", flttype) ||
		!strcmp("_EVA_LISTOBJ", flttype))
	{
		/* List of predefined objects */
		DYNTAB_FIELD(&fltobj, flt_data, LISTOBJ);
		DYNBUF_ADD_STR(sqlexpr, "[MIN(_EVA_FORMSTAMP.IdObj)] IN(");
		if(dyntab_to_dynbuf(&fltobj, sqlexpr, ",", 1, ",", 1, NO_CONV)) RETURN_ERR_MEMORY;
		DYNBUF_ADD_STR(sqlexpr, ")");
	}
	else if(!strcmp("_EVA_FILTER_FORM", flttype))
	{
		/* Objects created/edited with one or more forms */
		unsigned long i;
		DYNTAB_FIELD(&fltobj, flt_data, FILTER_FORM);
		if(fltobj.nbrows > 1) DYNBUF_ADD_STR(sqlexpr, "(");
		for(i = 0; i < fltobj.nbrows; i++)
		{
			if(i) DYNBUF_ADD_STR(sqlexpr, " OR ");
			DYNBUF_ADD_CELL(sqlexpr, &fltobj, i, 0, NO_CONV);
			DYNBUF_ADD_STR(sqlexpr, " IN ([_EVA_FORMSTAMP])");
		}
		if(fltobj.nbrows > 1) DYNBUF_ADD_STR(sqlexpr, ")");
	}
	else if(!strcmp("_EVA_FILTER_CTRLVAL", flttype))
	{
		if(qry_parse_relval_fields(cntxt, &fltval, &fltobj, flt_data)) STACK_ERROR;
		if(fltval.nbrows && qry_filter_val_to_sql(cntxt, sqlexpr, &fltval, flt_data)) STACK_ERROR;
		if(fltobj.nbrows && qry_filter_rel_to_sql(cntxt, sqlexpr, &fltobj, flt_data)) STACK_ERROR;
	}
	else if(!strcmp("_EVA_FILTER_FIELD", flttype))
	{
		/* Objects with given fields & values */
		DYNTAB_FIELD(&fltfield, flt_data, FILTER_FIELD);
		if(qry_filter_val_to_sql(cntxt, sqlexpr, &fltfield, flt_data)) STACK_ERROR;
	}
	else if(!strcmp("_EVA_FILTER_REL", flttype))
	{
		/* Relation with other objects */
		DYNTAB_FIELD(&fltfield, flt_data, FILTER_REL);
		if(qry_filter_rel_to_sql(cntxt, sqlexpr, &fltfield, flt_data)) STACK_ERROR;
	}
	else if(!strcmp("_EVA_FILTER", flttype) ||
			!strcmp("_EVA_FILTER_AND", flttype) ||
			!strcmp("_EVA_FILTER_ANDJOIN", flttype) ||
			!strcmp("_EVA_FILTER_OR", flttype))
	{
		/* AND / OR combination of filters on same object  - process filter list */
		DYNTAB_FIELD(&fltval, flt_data, FILTERS);
		if(qry_filterlist_to_sql(cntxt, sqlexpr, &fltval, strcmp("_EVA_FILTER_OR", flttype) ? "AND" : "OR")) STACK_ERROR;
	}
	else if(!strcmp("_EVA_FILTER_SQLCOND", flttype))
	{
		/* Evaluate variables in expression && add to result expresion */
		if(qry_eval_sqlexpr_var(cntxt, sqlexpr, DYNTAB_FIELD_VAL(flt_data, FILTER_COND), flt_data, NULL)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_filterlist_to_sql
** Description : build SQL expression for a combination of filters
*********************************************************************/
#define ERR_FUNCTION "qry_filterlist_to_sql"
#define ERR_CLEANUP DYNTAB_FREE(fltdata)
int qry_filterlist_to_sql(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **sqlexpr,			/* in/out : SQL expression is added to the buffer */
	DynTable *idflt,				/* in : list of filters to translate to SQL */
	char *oper						/* in : separator between filters translation (typically AND | OR) */
){
	DynTable fltdata = { 0 };
	unsigned long i;

	/* Parse each filter */
	for(i = 0; i < idflt->nbrows; i++)
	{
		/* Add operator to expression */
		if(i) DYNBUF_ADD3(sqlexpr, " ", oper, 0, NO_CONV, " ");

		/* Read filter data */
		if(qry_cache_idobj(&fltdata, DYNTAB_TOULRC(idflt, i, 0))) STACK_ERROR;

		/* Add SQL to expression */
		if(qry_filter_to_sql(cntxt, sqlexpr, &fltdata)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
