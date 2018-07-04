/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : sql_values.h
** Description : values handling functions for EVA DB schema
**      Author : Alain BOUTE
**     Created : Aug 15 2001
*********************************************************************/


/*********************************************************************
** Function : sql_id_values
** Description : return value identifiers matching val in table TVal
*********************************************************************/
int sql_id_values(			/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	DynTable *res,			/* out : matching IdValues */
	char *val,				/* in : value to search for */
	size_t sz_val,			/* in : val string length */
	MatchMode matchmode,	/* in : how is searched value ? */
	unsigned long limit		/* in : max # of returned values */
);

/*********************************************************************
** Function : sql_id_value
** Description : return a value identifier in table TVal
*********************************************************************/
int sql_id_value(			/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	char *val,				/* in : value to search for */
	size_t sz_val,			/* val string length */
	unsigned long *id		/* out : value Id or NULL on error  */
);

/*********************************************************************
** Function : get_id_value
** Description : return a value identifier in table TVal
*********************************************************************/
unsigned long get_id_value(	/* return : value Id or 0 on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	char *val,				/* in : value to search for */
	size_t sz_val			/* val string length */
);

/*********************************************************************
** Function : sql_add_value
** Description : return a value identifier / create if necessary
*********************************************************************/
int sql_add_value(			/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in/out : execution context data */
	char *val,				/* in : value to search for & create */
	size_t sz_val,			/* in : val string length */
	unsigned long *id		/* out : value Id - 0 on error or empty value */
);

/*********************************************************************
** Macro : SQL_STAMP_RECORD
** Description : add DateCr / IdWhoCr info to an insert or update query
*********************************************************************/
#define SQL_STAMP_RECORD(sql) { \
	DYNBUF_ADD3(sql, ",DateCr='", cntxt->timestamp, 14, NO_CONV, "',IdWhoCr="); \
	if(dyntab_sz(&cntxt->id_user, 0, 0)) DYNBUF_ADD_CELL(sql, &cntxt->id_user, 0, 0, NO_CONV) \
	else DYNBUF_ADD_STR(sql, "Null")}
