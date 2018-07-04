/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : sql_dbif.c
** Description : SQL interface functions - MySQL version
**      Author : Alain BOUTE
**     Created : Aug 15 2001
*********************************************************************/

#include "eva.h"

/* MySQL includes */
/* Disable warning message C4115 in rpcasync.h : '_RPC_ASYNC_STATE' : named type definition in parentheses */
#pragma warning( disable : 4115 )
#include <windows.h>
#include <mysql.h>
#include <errmsg.h>

/*********************************************************************
** Function : dll_call
** Description : load a dynamic library & call given function
*********************************************************************/
#define ERR_FUNCTION "dll_call"
#define ERR_CLEANUP	if(h) FreeLibrary(h)
int dll_call(						/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *lib,						/* in : library file name */
	char *fnct						/* in : function name */
){
	char dllpath[1024];
	HINSTANCE h;
	FARPROC f;

	/* Load DLL - return on error */
	snprintf(add_sz_str(dllpath), "%s" DD "%s", cntxt->path, lib);
	h = LoadLibrary(dllpath);
	if(!h) RETURN_ERROR("Cannot load DLL", NULL);

	/* Get function adress - return on error */
	f = GetProcAddress(h, fnct);
	if(!f) RETURN_ERROR("DLL function not found", NULL);

	/* Call function - return on error */
	if(f(cntxt, i_ctrl)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : sql_use_db
** Description : set given database as default for queries
*********************************************************************/
#define ERR_FUNCTION "sql_use_db"
#define ERR_CLEANUP
int sql_use_db(						/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *dbname					/* in : database name */
){
	/* Connect to MySql server - return on error */
	if(!cntxt->dbpwd && file_read_config(cntxt)) STACK_ERROR;
	if(!mysql_real_connect(cntxt->sql_session, cntxt->srvaddr, "root", cntxt->dbpwd, dbname, MYSQL_PORT, NULL, 0))
	{
		RETURN_ERROR("Pas de connexion au serveur SQL", NULL);
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : sql_open_session
** Description : open the SQL database connexion
*********************************************************************/
#define ERR_FUNCTION "sql_open_session"
#define ERR_CLEANUP
int sql_open_session(				/* return : 0 on success, other on error */
	EVA_context *cntxt				/* in : execution context data
									   out : sql_session = opened session */
){
	/* Initialize MySql - return on error */
	clock_t t0 = clock();

	if(cntxt->sql_session) RETURN_OK;
	cntxt->sql_session = mysql_init(NULL);
	if(!cntxt->sql_session)
	{
		RETURN_ERROR("Problème d'initialisation du serveur SQL", NULL);
	}

	/* Connect to MySql server - return on error */
	if(!mysql_real_connect(cntxt->sql_session, cntxt->srvaddr, "root", cntxt->dbpwd, cntxt->dbname, MYSQL_PORT, NULL, 0))
	{
		sql_control(cntxt, 0);
		RETURN_ERROR("Pas de connexion au serveur SQL", NULL);
	}
	cntxt->sqltime += clock() - t0;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : find_files
** Description : call a function for each file in the given path
*********************************************************************/
#define ERR_FUNCTION "find_files"
#define ERR_CLEANUP _findclose(handle)
int find_files(						/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *files,				/* out : found files
										col0 : file name (no path)
										col1 : file summary (name, date, size)
										col2 : file attributes (D for subdir)
										col3 : file size
										col4 : contaning subdir path
										col5 : last modification date */
	char *path,						/* in : file path (wildcards accepted) */
	int mode						/* in : list mode */
){
	long handle = 0;
	struct _finddata_t filedata = {0};
	char cwd[_MAX_PATH];
	char date[16] = {0};

	if(!files) return 0;
	if(mode & FreeFiles) dyntab_free(files);

	/* Find first file in path */
	handle = _findfirst(path, &filedata);
	if(handle == -1L) RETURN_OK;
	getcwd(add_sz_str(cwd));

	/* Process each file entry */
	do
	{
		unsigned long row = files->nbrows;

		/* If entry must be listed */
		if(!(filedata.attrib & _A_SUBDIR) || mode & ListSubdirs)
		{
			/* Add file name & infos */
			DYNTAB_ADD(files, row, 0, filedata.name, 0, NO_CONV);
			if(mode & IncludeInfos)
			{
				char buf[_MAX_PATH*2] = {0};
				char date1[32] = {0};
				time_to_datetxt(filedata.time_write, date);
				datetxt_to_format(cntxt, date1, date, NULL);
				DYNTAB_ADD(files, row, 5, date, 0, NO_CONV);
				snprintf(add_sz_str(buf), "%s (%s - %s)", filedata.name, date1, human_filesize(filedata.size));
				DYNTAB_ADD(files, row, 1, buf, 0, NO_CONV);
				DYNTAB_ADD(files, row, 2, filedata.attrib & _A_SUBDIR ? "D" : NULL, 0, NO_CONV);
				DYNTAB_ADD_INT(files, row, 3, filedata.size);
				DYNTAB_ADD(files, row, 4, cwd, 0, NO_CONV);
			}
		}

		/* Recurse subdir if applicable */
		if(filedata.attrib & _A_SUBDIR && mode & RecurseSubdirs && !chdir(filedata.name))
		{
			if(find_files(cntxt, files, path, mode)) CLEAR_ERROR;
			chdir("..");
		}

	} while (!_findnext(handle, &filedata));

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : sql_control
** Description : shutdown the SQL database server
*********************************************************************/
char *sql_control(EVA_context *cntxt, int mode)
{
	clock_t t0 = clock();
	switch(mode)
	{
	case 0:	/* Close session */
		if(!cntxt->sql_session) return NULL;
		mysql_close(cntxt->sql_session);
		cntxt->sql_session = NULL;
		return NULL;

	case 1: /* MySQL shutdown */
		if(!cntxt->sql_session) return NULL;
		switch(mysql_shutdown(cntxt->sql_session))
		{
		case 0:
			cntxt->sql_session = NULL;
			cntxt->sqltime += clock() - t0;
			return NULL;
		case CR_COMMANDS_OUT_OF_SYNC: return "CR_COMMANDS_OUT_OF_SYNC";
		case CR_SERVER_GONE_ERROR: return "CR_SERVER_GONE_ERROR";
		case CR_SERVER_LOST: return "CR_SERVER_LOST";
		case CR_UNKNOWN_ERROR: return "CR_UNKNOWN_ERROR";
		}
		cntxt->sqltime += clock() - t0;
		return "Impossible d'arréter le serveur SQL";

	case 2: /* Server soft reboot */
	case 3: /* Server hard reboot */
		{
			HANDLE hToken; 
			TOKEN_PRIVILEGES tkp; 
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) ; 
			LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
			tkp.PrivilegeCount = 1;
			tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
			AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
		}
		if(!ExitWindowsEx(EWX_REBOOT | (mode == 2 ? EWX_SHUTDOWN : EWX_FORCE), 0))
			return "Redémarrage refusé par Windows";
		Sleep(20000);
		return "Le serveur n'a toujours pas redémarré";

	case 4:	/* Debug break */
		DebugBreak();
		break;

	case 5:	/* Server initialisation */
		if(mysql_server_init(0, NULL, NULL))
			return "SQL server initialisation failure";
		break;

	case 6:	/* Server usage end */
		mysql_server_end();
		break;

	case 10:	/* Server random wait (for deadlock resolution during new obj Id creation) */
		Sleep(50 * (DWORD)(rand() % 100));
		break;

	default:	/* Server charge balancing for lengthy processes */
		if(mode < 0)
		{
			int i = mode;
			while(i)
			{
				Sleep(0);	/* Give control to other processes */
				i++;
			}
		}
	}
	return NULL;
}

/*********************************************************************
** Function : sql_insert_id
** Description : return the last generated Pkey
*********************************************************************/
unsigned long sql_insert_id(EVA_context *cntxt)
{
	return (unsigned long)mysql_insert_id(cntxt->sql_session);
}

/*********************************************************************
** Function : sql_free_result
** Description : free the SQL last query result
*********************************************************************/
void sql_free_result(EVA_context *cntxt)
{
	if(!cntxt->sql_result) return;
	mysql_free_result(cntxt->sql_result);
	cntxt->sql_result = NULL;
}

/*********************************************************************
** Function : sql_free_row
** Description : free a sql row data
*********************************************************************/
void sql_free_row(EVA_sql_row *row)
{
	if(!row) return;
	M_FREE(row->name);
	memset(row, 0, sizeof(row[0]));
}

/*********************************************************************
** Function : sql_get_unsigned_status_var
** Description : return the value of a system status variable as unsigned
*********************************************************************/
unsigned long sql_get_unsigned_status_var(	/* return : variable value or ~0UL on error */
	EVA_context *cntxt,						/* in : execution context data */
	char *varname							/* in : variable name */
){
	char qry[512];
	MYSQL_RES *res;
	MYSQL_ROW row;

	snprintf(add_sz_str(qry), "SHOW STATUS LIKE '%s'", varname);
	if(mysql_query(cntxt->sql_session, qry)) return ~0UL;
	res = mysql_store_result(cntxt->sql_session);
	if(!res) return ~0UL;
	row = mysql_fetch_row(res);
	if(!row || !row[1]) return ~0UL;
	return strtoul(row[1], NULL, 10);
}

/*********************************************************************
** Function : sql_exec_query
** Description : execute a SQL request
*********************************************************************/
#define ERR_FUNCTION "sql_exec_query"
#define ERR_CLEANUP if(cntxt->sql_result && cntxt->err.text) { \
						mysql_free_result(cntxt->sql_result);	\
						cntxt->sql_result = NULL; } \
					cntxt->sql_trace = sql_trace;
int sql_exec_query				/* return : 0 on success, other on error */
(
	EVA_context *cntxt,			/* in : cntxt->sql_session
								  out : cntxt->sql_result */
	char *query					/* in : SQL query */
){
	clock_t t1, t2;
	int sql_trace = cntxt->sql_trace;
	size_t query_sz = query ? strlen(query) : 0;

	cntxt->sql_cnt++;
	cntxt->sql_nbrows = 0;

	if(!sql_trace)
	{
		cntxt->sql_trace = cntxt->debug & (DEBUG_SQL | DEBUG_SQL_RES);
		if(cntxt->sql_trace & DEBUG_SQL_RES && !(cntxt->sql_trace & DEBUG_FILTER))
		{
			unsigned long i = strtoul(DYNTAB_FIELD_VAL(&cntxt->user_data, SQL_INDEX), NULL, 10);
			if(i != cntxt->sql_cnt) cntxt->sql_trace ^= DEBUG_SQL_RES;
		}
	}

	/* Open MySql session if needed */
	if(!cntxt->sql_session && sql_open_session(cntxt)) STACK_ERROR;

	/* Store query for further reference */
	if(cntxt->sql_qry) cntxt->sql_qry->cnt = 0;
	DYNBUF_ADD(&cntxt->sql_qry, query, query_sz, NO_CONV);

	/* Exec query on MySql server - return on error */
	t1 = clock();
	sql_free_result(cntxt);
	if(mysql_query(cntxt->sql_session, query))
	{
		cntxt->sqltime += clock() - t1;
		ERR_PUT_TXT("*** Error : ", (char*)mysql_error(cntxt->sql_session), 0);
		ERR_PUT_TXT("\n*** Query : ", query, query_sz);
		if(cntxt->sql_trace) 
			DYNBUF_ADD3(&cntxt->debug_msg, "\n*** Error : ", (char*)mysql_error(cntxt->sql_session), 0, NO_CONV, "\n");
		RETURN_ERROR("Erreur dans une requête SQL", NULL);
	}

	/* Ask for results - return on error */
    cntxt->sql_result = mysql_store_result(cntxt->sql_session) ;
	if(!cntxt->sql_result && mysql_field_count(cntxt->sql_session)) 
		RETURN_ERROR("Impossible de lire le résultat d'une requête à la base de données", NULL);
	cntxt->sql_nbrows = (unsigned long)mysql_affected_rows(cntxt->sql_session);
 	t2 = clock();
	cntxt->sqltime += t2 - t1;
	cntxt->sql_restime = (double)(t2-t1)/CLOCKS_PER_SEC;

	/* Output debug info */
	if(cntxt->sql_trace)
	{
		DYNBUF_PRINTF(&cntxt->debug_msg, 64, "\n%4ld : ", cntxt->sql_cnt, NO_CONV);
		DYNBUF_PRINTF(&cntxt->debug_msg, 64, "%1.3f s ", (double)(t2-t1)/CLOCKS_PER_SEC, NO_CONV);
	}
	if(cntxt->sql_trace & DEBUG_SQL_RES)
	{
		if(cntxt->form && cntxt->form->ctrl && cntxt->form->i_ctrl < cntxt->form->nb_ctrl)
			DYNBUF_ADD3(&cntxt->debug_msg, "(", cntxt->form->ctrl[cntxt->form->i_ctrl].LABEL, 0, NO_CONV, ")");
		DYNBUF_ADD3(&cntxt->debug_msg, "\n", query, query_sz, NO_CONV, "\n");
	}
	else if(cntxt->sql_trace & DEBUG_SQL)
	{
		size_t sz = strlen(query);
		DYNBUF_ADD(&cntxt->debug_msg, query, sz, NO_TABCR); 
		if(cntxt->form && cntxt->form->ctrl && cntxt->form->i_ctrl < cntxt->form->nb_ctrl)
			DYNBUF_ADD3(&cntxt->debug_msg, "(", cntxt->form->ctrl[cntxt->form->i_ctrl].LABEL, 0, NO_CONV, ")");
	}
	if(cntxt->sql_trace & (DEBUG_SQL | DEBUG_SQL_RES))
	{
		if(cntxt->sql_result) DYNBUF_ADD3_INT(&cntxt->debug_msg, " *** Result : ", cntxt->sql_nbrows, " rows")
		else DYNBUF_ADD3_INT(&cntxt->debug_msg, " *** Affected : ", cntxt->sql_nbrows, " rows");
		if(cntxt->sql_trace & DEBUG_SQL_RES) DYNBUF_ADD_STR(&cntxt->debug_msg, "\n");
	}
	else if(cntxt->debug & DEBUG_SQL_SLOW && cntxt->sql_restime > DEBUG_SQL_SLOW_TH)
	{
		DYNBUF_ADD3_INT(&cntxt->debug_msg, "\n=========> Slow query #", cntxt->sql_cnt, "");
		DYNBUF_PRINTF(&cntxt->debug_msg, 128, " - time = %1.3f s", (double)(t2-t1)/CLOCKS_PER_SEC, NO_CONV);
		DYNBUF_ADD3_INT(&cntxt->debug_msg, " - ", cntxt->sql_nbrows, " rows\n");
		if(cntxt->form && cntxt->form->ctrl)
		{
			DYNBUF_ADD3(&cntxt->debug_msg, "Form : ", cntxt->form->ctrl->LABEL, 0, NO_CONV, "\n")
			if(cntxt->form->ctrl && cntxt->form->i_ctrl < cntxt->form->nb_ctrl)
				DYNBUF_ADD3(&cntxt->debug_msg, "Ctrl : ", cntxt->form->ctrl[cntxt->form->i_ctrl].LABEL, 0, NO_CONV, "\n")
		}
		DYNBUF_ADD3(&cntxt->debug_msg, "", query, query_sz, NO_CONV, "\n")
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : sql_result_next_row
** Description : get next row of a sql result
*********************************************************************/
#define ERR_FUNCTION "sql_result_next_row"
#define ERR_CLEANUP
int sql_result_next_row(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	EVA_sql_row *row					/* out : row data - row->value is NULL on end of rows*/

){
	MYSQL_FIELD *fields;
	unsigned long i;

	if(!row->nbcols)
	{
		row->nbcols = mysql_num_fields(cntxt->sql_result);
		M_FREE(row->name);
		row->name = mem_alloc(sizeof(row->name[0]) * row->nbcols);
		if(!row->name) RETURN_ERR_MEMORY;
		fields = mysql_fetch_fields(cntxt->sql_result);
		for(i = 0; i < row->nbcols; i++) row->name[i] = fields[i].name;
	}

	row->value = mysql_fetch_row(cntxt->sql_result);
	row->sz = (size_t *)(mysql_fetch_lengths(cntxt->sql_result));

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
