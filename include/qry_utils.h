/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : qry_utils.h
** Description : utility functions for building SQL queries
**      Author : Alain BOUTE
**     Created : Feb 26 2002
**********************************************************************/

/*********************************************************************
** Function : qry_recursive_relation
** Description : get multiple levels related objects
*********************************************************************/
int qry_recursive_relation(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *res,					/* out : result : list of IdObj */
	DynTable *srcidobj,				/* in : list of base objects */
	DynTable *relfields				/* in : list of relation fields (all if NULL) */
);

/*********************************************************************
** Function : qry_add_table_col
** Description : evaluate a field expression based on table 1st col & place result in 1st available col
*********************************************************************/
int qry_add_table_col(							/* return : 0 on success, other on error */
	EVA_context *cntxt,							/* in/out : execution context data */
	DynTable *res,								/* in/out : table to add values to : col 0 contains base IdObj */
	char *expr, size_t expr_sz					/* in : field expression to evaluate */
);

/*********************************************************************
** Function : qry_complete_data
** Description : complete object data with values from given object
*********************************************************************/
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
											or with new Num index else*/
	char *tables			/* in : comma/newline separated list of fields
									to handle as tables (TODO) */
);

/*********************************************************************
** Function : qry_values_list
** Description : build a comma separated list of values suitable for IN() SQL statement
*********************************************************************/
int qry_values_list(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynTable *tab,						/* in : table of ids or values */
	int mode,							/* in : handling of tab elements 
											0 : list of Ids
											1 : list of values 
											2 : list of values - find Ids 
											3 : list of values - find Ids in col 4 of tab */
	DynBuffer **ret						/* buffer to add the list to */
);

/*********************************************************************
** Function : qry_field_value
** Description : exec a query with field/value criteria
*********************************************************************/
int qry_field_value(		/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	DynTable *res,			/* out : list of matching objects Ids */
	DynBuffer *srchfld,		/* in : search fields name */
	MatchMode matchmode,	/* in : how is searched value ? */
	DynTable *srchval,		/* in : values searched for fields listed in srchfld */
	DynTable *srchform		/* in : select only objects with form stamp in the list */
);

/*********************************************************************
** Function : qry_make_idlist
** Description : transfer a list of ids to IdList table
*********************************************************************/
int qry_make_idlist(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_list,				/* in : list of objects ids to put in table */
	char *table						/* in : name of table to create */
);

/*********************************************************************
** Function : qry_get_obj_data
** Description : return a table of object data with values description
*********************************************************************/
int qry_get_obj_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	char *where						/* in : where clause for select statement */
);

/*********************************************************************
** Function : qry_obj_idfield
** Description : return single field data for single object
*********************************************************************/
int qry_obj_idfield(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	unsigned long id_obj,			/* in : object id to read */
	unsigned long id_field			/* in : field to read 
											if NULL : all data is read */
);

/*********************************************************************
** Function : qry_obj_field
** Description : return single field data for a list of objects
*********************************************************************/
int qry_obj_field(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	unsigned long id_obj,			/* in : object id to read */
	char *field						/* in : field to read 
											if NULL : all data is read */
);

/*********************************************************************
** Function : qry_listobj_field
** Description : return single field data for mutliple objects
*********************************************************************/
int qry_listobj_field(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	DynTable *obj,					/* in : list of objects id to read */
	char *field						/* in : field to read 
											if NULL : all data is read */
);

/*********************************************************************
** Function : qry_obj_data
** Description : return fields data for a list objects
*********************************************************************/
int qry_obj_data(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing object values */
	DynTable *id_obj,				/* in : list of objects to read (in column 0) */
	DynTable *fields				/* in : list of fields to read (in column 0)
											if NULL : all data is read */
);

/*********************************************************************
** Function : qry_add_new_idobj
** Description : add a new object in db
*********************************************************************/
int qry_add_new_idobj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long *id_obj,			/* out : new object id */
	unsigned long formstamp			/* in : form stamp for object in cell(0,0) */
);

/*********************************************************************
** Function : qry_add_new_obj
** Description : add a new object in db
*********************************************************************/
int qry_add_new_obj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_obj,				/* out : new object id */
	unsigned long formstamp			/* in : form stamp for object in cell(0,0) */
);

/*********************************************************************
** Function : qry_update_idobj_idfield
** Description : update field values for an object in DB
*********************************************************************/
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
										4 = add new values - check Line, not Num */
);

/*********************************************************************
** Function : qry_check_idobj
** Description : check IdObj exists with formstamp in DB
*********************************************************************/
int qry_check_idobj(				/* return : 1 if object exists, 0 else */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idobj				/* in : object id to check */
);

/*********************************************************************
** Function : qry_add_val
** Description : add an object value in db
*********************************************************************/
int qry_add_val(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTableCell *val,				/* in : value to add to DB 
										note : IdObj, field or IdField, IdValue or txt & sz or IdRelObj,
												Num & Line attributes must be set properly */
	int check,						/* in : check if value already exists before adding
											0 = no check
											1 = check value & field
											2 = check value, field, Num & Line */
	unsigned long *_pkey				/* in : record to set value to
									   out : Pkey of set record / 0 if not set */
);

/*********************************************************************
** Function : qry_update_idobj_idfield_1
** Description : update an object single value in db
*********************************************************************/
int qry_update_idobj_idfield_1(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long idobj,			/* in : object id */
	unsigned long idfield,			/* in : field id */
	char *txt, size_t len			/* in : value text */
);

/*********************************************************************
** Function : qry_add_idobj_field_rel
** Description : add an object relation in db
*********************************************************************/
int qry_add_idobj_field_rel(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long id_obj,			/* in : object id */
	char *field,					/* in : field name */
	unsigned long idrelobj,			/* in : related object id */
	unsigned long num,				/* in : value index */
	unsigned long line,				/* in : line index */
	int check						/* in : check if value exist - see qry_add_val for details */
);

/*********************************************************************
** Function : qry_add_idobj_field_val
** Description : add an object value in db
*********************************************************************/
int qry_add_idobj_field_val(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long id_obj,			/* in : object id */
	char *field,					/* in : field name */
	char *txt, size_t sz,			/* in : value text */
	unsigned long num,				/* in : value index */
	unsigned long line,				/* in : line index */
	int check						/* in : check if value exist - see qry_add_val for details */
);

/*********************************************************************
** Function : qry_add_obj_field_val
** Description : add an object value in db
*********************************************************************/
int qry_add_obj_field_val(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_obj,				/* in : object id */
	char *field,					/* in : field name */
	char *txt, size_t sz,			/* in : value text */
	char b_relation,				/* in : store value as relation if not 0 - txt must be a valid number then */
	unsigned long numval			/* in : value index */
);

/*********************************************************************
** Function : qry_add_obj_data
** Description : add object values in db
*********************************************************************/
int qry_add_obj_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_obj,				/* in/out : object id */
	DynTable *data,					/* in : object data : Field, Value, Storage (V/M/R), Num */
	DynTable *formstamp,			/* in : form id for object formstamp */
	int b_clearprev					/* in : clear previous values if not 0 */
);

/*********************************************************************
** Function : qry_list_data
** Description : return data for objects with Ids in table IdList
*********************************************************************/
int qry_list_data(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	ObjTableFormat *tbl				/* in/out : list parameters */
);

/*********************************************************************
** Function : sql_get_table
** Description : return values of a SQL query as a table for each column
*********************************************************************/
int sql_get_table				/* return : 0 on success, other on error */
(
	EVA_context *cntxt,			/* in : execution data */
	DynTable *tab, 				/* in/out : table to add rows to */
	int b_flags					/* in : table output format flags :
									bit 0 : set first line with field names
									bit 1 : clear tab before processing */
);

/*********************************************************************
** Function : qry_build_flt_select
** Description : build SELECT clause corresponding to a filter
*********************************************************************/
int qry_build_flt_select			/* return : 0 on success, other on error */
(
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **sql,				/* in/out : SQL query is added to the buffer */
	DynTable *idlist,				/* in : list of objects Ids - if NULL, read from table IdList */
	QryBuild *flt,					/* in : query build data */
	unsigned long row,				/* in : 1st row for LIMIT clause */
	unsigned long nbrows			/* in : # of rows for LIMIT clause */
);

/*********************************************************************
** Function : sql_get_buf
** Description : return values of a SQL query as a list of values
*********************************************************************/
int sql_get_buf					/* return : 0 on success, other on error */
(
	EVA_context *cntxt,			/* in : execution data */
	DynBuffer **res,			/* in/out : values list */
	int options					/* in : table output format flags :
									bit 0 : quote values
									bit 1 : clear res before processing */
);

/*********************************************************************
** Function : sql_get_listcol
** Description : execute a SQL request & return values
*********************************************************************/
int sql_get_listcol					/* return : 0 on success, other on error */
(
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : table containing result */
	DynTable *idlist,				/* in : list of objects Ids - if NULL, read from table IdList */
	QryBuild *flt,					/* in : query build data */
	unsigned long col				/* in : column to store in cell->col */
);

/*********************************************************************
** Function : sql_drop_table
** Description : drop a temporary SQL table
*********************************************************************/
void sql_drop_table(
	EVA_context *cntxt,						/* in/out : execution context data */
	char *table								/* in : table name */
);
