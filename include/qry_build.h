/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : qry_build.c
** Description : functions for building SQL queries from filters and field expressions
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/


/*********************************************************************
** Function : qry_cache_idobj_field
** Description : read a single object data
*********************************************************************/
#define qry_cache_idobj(res, idobj) qry_cache_idobj_field(cntxt, res, idobj, NULL, 3)
int qry_cache_idobj_field(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result : object data */
	unsigned long idobj,			/* in : object Id */
	char *field,					/* in : field values to read - read all fields if NULL */
	int mode						/* in : bit mask
										bit 0 : copy data if not 0
										bit 1 : free res if not 0 */
);

/*********************************************************************
** Function : qry_uncache_idobj
** Description : remove object data from cache
*********************************************************************/
void qry_uncache_idobj(
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idobj				/* in : object Id to remove from cache */
);

/*********************************************************************
** Function : qry_cache_objlist_field
** Description : read multiple objects data
*********************************************************************/
#define qry_cache_objlist(res, idobj) qry_cache_objlist_field(cntxt, res, idobj, NULL, 3)
int qry_cache_objlist_field(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result : object data */
	DynTable *idobj,				/* in : objects Id list */
	char *field,					/* in : field values to read - read all fields if NULL */
	int mode						/* in : bit mask
										bit 0 : copy data if not 0
										bit 1 : free res if not 0 */
);

/*********************************************************************
** Function : qry_eval_sqlexpr_var
** Description : replace variables by their values in a mixed SQL / fields expression
*********************************************************************/
int qry_eval_sqlexpr_var(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer **sqlexpr,				/* out : SQL expression with variables evaluated */
	char *expr,							/* in : SQL expression to process */
	DynTable *vardata,					/* in : variables definition data */
	char *srctbl						/* in : source table name (for #SRCTBL symbol) */
);


/*********************************************************************
** Function : qry_eval_var_expr_table
** Description : replace variables with their values in a table of expressions
*********************************************************************/
int qry_eval_var_expr_table(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *varexpr,				/* in/out : table of expressions */
	DynTable *var_data				/* in : object data with variable definitions */
);

/*********************************************************************
** Function : qry_eval_fieldexpr
** Description : read result of qry_eval_sql_fieldexpr in a DynTable
*********************************************************************/
int qry_eval_fieldexpr(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result values in col 0 */
	char *expr,						/* in : SQL expression to evaluate */
	char *srctbl,					/* in : source table name - IdObj column contains objects Ids */
	DynTable *vardata				/* in : variables definitions */
);

/*********************************************************************
** Function : qry_eval_sql_fieldexpr
** Description : return results of a mixed SQL / fields expression
*********************************************************************/
int qry_eval_sql_fieldexpr(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *dsttbl,						/* in : destination table (IdObj,Val,Line) */
	char *expr,							/* in : SQL expression to evaluate */
	char *srctbl,						/* in : source table name - IdObj column contains objects Ids
												or list of comma separated IdObj (TODO) */
	DynTable *vardata					/* in : variables definitions */
);

/*********************************************************************
** Function : qry_filter_sql_fieldexpr
** Description : select objects matching a mixed SQL / fields expression
*********************************************************************/
int qry_filter_sql_fieldexpr(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynTable *res,						/* out : matched objects list */
	char *expr,							/* in : SQL expression to evaluate */
	char *srcobj,						/* in : source table name - IdObj column contains objects Ids */
	DynTable *vardata					/* in : variables definitions */
);

/*********************************************************************
** Function : qry_build_free_nodes
** Description : clean up filter nodes
*********************************************************************/
void qry_build_free_nodes(QryBuild *flt);

/*********************************************************************
** Function : qry_add_filter
** Description : add a field / value condition to a filter
*********************************************************************/
void qry_build_free(QryBuild *flt);

/*********************************************************************
** Function : qry_parse_field_modifier
** Description : return modifier id for a field modifier
*********************************************************************/
FieldModifier qry_parse_field_modifier(	/* return : field modifier id */
	char *fm							/* in : field modifier */
);

/*********************************************************************
** Function : qry_copy_filter
** Description : copy a filter to another
*********************************************************************/
int qry_copy_filter(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	QryBuild *dest,					/* out : destination filter */
	QryBuild *src					/* in/out : source filter */
);

/*********************************************************************
** Function : qry_add_filter_simple_txt
** Description : build filter for simple text condition
*********************************************************************/
int qry_add_filter_simple_txt(				/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	QryBuild *flt,							/* in/out : filter data */
	RelationMode rel,						/* in : how is build the join */
	char *field,							/* in : search fields list (comma separated) */
	MatchMode matchmode,					/* in : how is searched value ? */
	char *value								/* in : search value */
);

/*********************************************************************
** Function : qry_add_filter_simple_int
** Description : build filter for simple int condition
*********************************************************************/
int qry_add_filter_simple_int(				/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	QryBuild *flt,							/* in/out : filter data */
	RelationMode rel,						/* in : how is build the join */
	char *field,							/* in : search fields list (comma separated) */
	MatchMode matchmode,					/* in : how is searched value ? */
	unsigned long value						/* in : search value */
);

/*********************************************************************
** Function : qry_add_filter
** Description : add a field / value condition to a filter
*********************************************************************/
int qry_add_filter(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	QryBuild *flt,						/* in/out : filter build data */
	RelationMode joinmode,				/* in : how is build the join */
	DynTable *fltfld,					/* in : search fields list */
	MatchMode matchmode,				/* in : how is searched value ? */
	DynTable *fltval					/* in : search values list */
);

/*********************************************************************
** Function : qry_add_filter_forms
** Description : build filter for objects matching form & filter condition
*********************************************************************/
int qry_add_filter_forms(					/* return : 0 on success, other on error */
	EVA_context *cntxt,						/* in/out : execution context data */
	QryBuild *flt,							/* in/out : filter data */
	DynTable *forms,						/* in : forms condition */
	DynTable *filters						/* in : filter conditions */
);

/*********************************************************************
** Function : qry_parse_matchmode
** Description : translate matchmode to SQL expression
*********************************************************************/
int qry_parse_matchmode(		/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in : execution context data */
	DynBuffer **sql,			/* in/out : buffer to output SQL expression */
	char *member,				/* in : table member to compare */
	unsigned long join,			/* in : join number */
	MatchMode match,			/* in : match operator to parse */
	MatchType mtyp,				/* in : match value type */
	DynTable *values,			/* in : values to compare */
	int *b_valjoin				/* in : do not optimize if NULL
								  out : (if not NULL) 0 if no join used */
);

/*********************************************************************
** Function : qry_build_clauses
** Description : build SQL query clauses from filter nodes
*********************************************************************/
int qry_build_clauses(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data  */
	QryBuild *flt,					/* in/out : query build data
										in : flt->node
										out : select, join, where, group_fn, b_userelobj */
	int mode						/* in : build mode
										0 = values of last node
									other = IdObj of first node */
);

/*********************************************************************
** Function : qry_filterlist_to_sql
** Description : build SQL expression for a combination of filters
*********************************************************************/
int qry_filterlist_to_sql(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **sqlexpr,			/* in/out : SQL expression is added to the buffer */
	DynTable *idflt,				/* in : list of filters to translate to SQL */
	char *oper						/* in : separator between filters translation (typically AND | OR) */
);

/*********************************************************************
** Function : qry_parse_relval_fields
** Description : split values & relations fields of filter controls
*********************************************************************/
int qry_parse_relval_fields(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *fldval,				/* out : value fields */
	DynTable *fldrel,				/* out : relation fields */
	DynTable *flt_data				/* in : object data of filter */
);

/*********************************************************************
** Function : qry_parse_filter_nodes
** Description : build query nodes for a filter from filter data
*********************************************************************/
int qry_parse_filter_nodes(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	QryBuild *flt,					/* in/out : filter calculation data */
	DynTable *flt_data				/* in : object data of filter */
);

/*********************************************************************
** Function : qry_parse_filter
** Description : build SQL query clauses for a filter from filter data
*********************************************************************/
int qry_parse_filter(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data  */
	QryBuild *flt,					/* in/out : filter calculation data */
	DynTable *flt_data				/* in : object data of filter */
);

/*********************************************************************
** Function : qry_filter_objects
** Description : execute query from filter nodes
*********************************************************************/
int qry_filter_objects(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result - objects selected by filter */
	QryBuild *flt					/* in : filter with nodes set */
);

/*********************************************************************
** Function : qry_filter_listobj
** Description : execute query from filter nodes - output to dynbuf
*********************************************************************/
int qry_filter_listobj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **res,				/* out : result - comma separated list of selected objects */
	QryBuild *flt					/* in : filter with nodes set */
);
/*********************************************************************
** Function : qry_to_listobj
** Description : execute query from filter list - output to dynbuf
*********************************************************************/
int qry_to_listobj(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **res,				/* out : result - comma separated list of selected objects */
	DynTable *forms,				/* in : list of accepted formstamps combined with OR */
	DynTable *filters				/* in : list of filters (IdObj) combined with AND */   
);

/*********************************************************************
** Function : qry_filter_table
** Description : execute query from filter nodes - output to SQL table
*********************************************************************/
int qry_filter_table(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *res,						/* out : result - SQL table name */
	QryBuild *flt,					/* in : filter with nodes set */
	int options						/* in : bit mask
										bit 0 : INSERT if set, CREATE else */
);

/*********************************************************************
** Function : qry_parse_fieldexpr
** Description : build SQL query clauses for a field expression
*********************************************************************/
int qry_parse_fieldexpr(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *field, size_t sz_field,	/* in : field / relation expression - see documentation for details */
	QryBuild *flt					/* in/out : filter calculation data */
);

/*********************************************************************
** Function : qry_parse_fieldexpr_nodes
** Description : build select & join statements for a field expression
*********************************************************************/
int qry_parse_fieldexpr_nodes(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data  */
	char *field, size_t sz_field,	/* in : field / relation expression - see documentation for details */
	QryBuild *flt					/* in/out : filter calculation data */
);

/*********************************************************************
** Macro : FOR_EACH_BRACKET{...}END_EACH_BRACKET
** Description : execute code for each string enclose in brackets
*********************************************************************/
#define FOR_EACH_BRACKET { next = expr; while(*next) { \
		char *fieldbeg = strchr(next, '[');	\
		char *fieldend = fieldbeg ? strchr(fieldbeg, ']') : NULL; \
		if(!fieldbeg || !fieldend) break; 
#define END_EACH_BRACKET next = fieldend + 1; }}

#define DECLARE_FIELDEXPR_OP \
		char *dotsep; \
		char *reldir; \
		char *relrev; \
		char *relop; \
		char *paropn; \
		char *oper

#define SEARCH_FIELDEXPR_OP(field) \
		dotsep = strchr(field, '.'); \
		reldir = strstr(field, "->"); \
		relrev = strstr(field, "<-"); \
		relop = (reldir && (!relrev || reldir < relrev)) ? reldir : relrev; \
		paropn = strchr(field, '('); \
		oper = (dotsep && (!relop || dotsep < relop)) ? dotsep : relop

