/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : sql_dbif.c
** Description : EVA SQL interface functions - MySQL version
**      Author : Alain BOUTE
**     Created : Aug 15 2001
*********************************************************************/


/*********************************************************************
** Function : dll_call
** Description : load a dynamic library & call given function
*********************************************************************/
int dll_call(						/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *lib,						/* in : library file name */
	char *fnct						/* in : function name */
);

/*********************************************************************
** Function : sql_shutdown
** Description : shutdown the SQL database server
*********************************************************************/
char *sql_control(EVA_context *cntxt, int mode);

/*********************************************************************
** Function : sql_use_db
** Description : set given database as default for queries
*********************************************************************/
int sql_use_db(						/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *dbname					/* in : database name */
);

/*********************************************************************
** Function : sql_open_session
** Description : open the SQL database connexion
*********************************************************************/
int sql_open_session				/* return : 0 on success, other on error */
(
	EVA_context *cntxt
);

/*********************************************************************
** Function : find_files
** Description : call a function for each file in the given path
*********************************************************************/
int find_files(						/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *files,				/* out : found files - handled by cb() */
	char *path,						/* in : file path (wildcards accepted) */
	int mode						/* in : list mode */
);
#define ListSubdirs		1	/* list subdir entries */
#define RecurseSubdirs	2	/* recurse subdirs */
#define FreeFiles		4	/* free files before processing */
#define IncludeInfos	8	/* include file infos */

/*********************************************************************
** Function : sql_insert_id
** Description : return the last generated Pkey
*********************************************************************/
unsigned long sql_insert_id(EVA_context *cntxt);

/*********************************************************************
** Function : sql_free_result
** Description : free the SQL last query result
*********************************************************************/
void sql_free_result(EVA_context *cntxt);

/*********************************************************************
** Macro : sql_free_row
** Description : free a sql row data
*********************************************************************/
void sql_free_row(EVA_sql_row *row)	;

/*********************************************************************
** Function : sql_exec_query
** Description : execute a SQL request
*********************************************************************/
int sql_exec_query				/* return : 0 on success, other on error */
(
	EVA_context *cntxt,			/* in : cntxt->sql_session
								  out : cntxt->sql_result */
	char *query					/* in : SQL query */
);

/*********************************************************************
** Function : qry_exec_filter
** Description : execute query for a filter
*********************************************************************/
int qry_exec_filter(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	QryBuild *flt,					/* in : filter data */
	char *qry						/* in : SQL query to exec */
);

/*********************************************************************
** Function : sql_result_next_row
** Description : get next row of a sql result
*********************************************************************/
int sql_result_next_row(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	EVA_sql_row *row					/* ou : row data - row->value is NULL on end of rows*/

);
