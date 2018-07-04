/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : html_console.c
** Description : bascic server administration console (backup, restart, status)
**      Author : Alain BOUTE
**     Created : Mar 27 2003
*********************************************************************/
#define STRICT

#include "eva.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <WINUSER.H>

/*********************************************************************
** Typedef : Calendar definitions
** Description : 
*********************************************************************/
char *month_lab[] = {"Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet",
					 "Août", "Septembre", "Octobre", "Novembre", "Décembre"};

#define HOLLYDAYS (i == 6 || (MM == 1 && day_to_output == 1)  || (MM == 5 && day_to_output == 1) \
					|| (MM == 5 && day_to_output == 8) || (MM == 7 && day_to_output == 14) \
				    || (MM == 8 && day_to_output == 15) || (MM == 11 && (day_to_output == 1 \
					 || day_to_output == 11)) || (MM == 12 && day_to_output == 25))

/*********************************************************************
** Typedef : ConsoleData
** Description : context data for console application
*********************************************************************/
typedef struct _ConsoleData
{
	EVA_context c;					/* EVA execution context */
	char *user;						/* name of identified user */
	int b_allow_db_select;			/* database select list is output if not 0 */
	int b_restart_after_bkup;		/* MySQL restart after backup checkbox status */
	int b_cgi_proc_only;			/* List CGI processe only checkbox status */
	int b_seldb_list;				/* select database list was output if not 0 */
	char *seldb;					/* selected database (may be NULL) */
	DynTable dbnames;				/* list of databases on server */
	unsigned long dbnumber;			/* row of seldb in dbnames */
	unsigned long row;				/* current row number in table (for color alternance) */
}
	ConsoleData;


#define PROCESSE_HANDLE_COL 0
#define PROCESSE_ROW_COL	1
#define PROCESSE_NAME_COL	2
#define PROCESSE_SIZE_COL	3
#define PROCESSE_LAUNCH_COL 4

/*********************************************************************
** Macro : Miscellaneous definitions for texts and titles
** Description : 
*********************************************************************/
#define CONSOLE_MAIN_TITLE "Console d'administration"
#define BACKUP_TEXT "Cochez pour relancer le serveur après sauvegarde des données"
#define PROCESS_TEXT "Cochez pour ne lister que les process de type 'cgi'"
#define SERVER_TITLE "Site"
#define DATE_TITLE "Lancement"
#define USER_TITLE "Utilisateurs"
#define CLICK_TITLE "Clicks"
#define AVER_TIME_TITLE "Durée (s)"
#define CRASH_TITLE "Crashs"
#define CONSOLE_LOGS_TITLE "Visualisation des logs du jour"
#define CONSOLE_GRAPH_TITLE "Visualisation du graphe des logs du jour"
#define CONSOLE_KILL_PROC_TITLE "Fermeture d'un processus"
#define CONSOLE_PROCESS_TITLE "Liste des processus"
#define CONSOLE_BACKUP_TITLE "Sauvegarde des données du serveur SQL"
#define CONSOLE_STOPSQL_TITLE "Arrêt du serveur SQL"
#define CONSOLE_STARTSQL_TITLE "Démarrage du serveur SQL"
#define CONSOLE_RESTARTFORCE_TITLE "Redémarrage du serveur en mode forcé"
#define CONSOLE_RESTARTE_TITLE "Redémarrage du serveur"
#define CONSOLE_OPTIMIZE_TITLE "Optimisation des tables"
#define CONSOLE_RESTART_COMMENT "Redémarrage de l'ordinateur hébergeant le service" \
								"<br><font color=red size=-1><b>" \
								"Attention : cette opération interromp le service pendant quelques minutes</b></font>"
#define CONSOLE_RESTART_TOOLTIP "Redémarrer le serveur"
#define CONSOLE_LOGS_COMMENT "Lecture du fichier des logs"
#define CONSOLE_GRAPH_COMMENT "Graphe des logs de la journée, par utilisateur et par palier d'une demi_heure"
#define CONSOLE_LOGS_TOOLTIP "Lecture des logs"
#define CONSOLE_GRAPH_TOOLTIP "Graphe des logs par demi-heure"
#define CONSOLE_CGI_ONLY_LABEL "Process cgi seulement"
#define CONSOLE_PROCESS_TOOLTIP "Liste des process"
#define CONSOLE_PROCESS_COMMENT "Liste des processus en cours d'exécution sur l'ordinateur hébergeant le service"
#define CONSOLE_STOPSQL_COMMENT "Interromp le service - interdit tout accès aux données" \
								"<br><font color=red size=-1><b>" \
								"Attention : les utilisateurs ne pourront plus travailler</b></font>"
#define CONSOLE_STOPSQL_TOOLTIP "Arrêter le serveur de données"
#define CONSOLE_STARTSQL_COMMENT "Le serveur SQL est arrêté ! Cliquez pour le démarrer" \
								"<br><font color=red size=-1><b>" \
								"Le service est interrompu - les utilisateurs ne peuvent plus accéder aux donnée</b></font>"
#define CONSOLE_STARTSQL_TOOLTIP "Démarre le serveur SQL"
#define CONSOLE_BACKUP_COMMENT "Effectue une sauvegarde des données" \
								"<br><font color=red size=-1><b>" \
								"Attention : cette opération interromp le service pendant quelques secondes</b></font>"
#define CONSOLE_RESTARTSERVER_LABEL "Relancer le serveur"
#define CONSOLE_OPTIMIZE_COMMENT "Effectue une optimisation de la taille des tables"
#define CONSOLE_BACKUP_TOOLTIP "Sauvegarder les données"
#define CONSOLE_OPTIMIZE_TOOLTIP "Compacter les tables"
#define LAST_ACCESS_TEXT "Dernier accès"
#define CONSOLE_CHANGE_DB_COMMENT "Sélectionnez le site que vous voulez administrer"
#define REFRESH_LOGS_TOOLTIP "Rafraichir les logs"
#define REFRESH_GRAPH_TOOLTIP "Rafraichir le graphe des logs"
#define REFRESH_PROCESS_TOOLTIP "Rafraichir les process"
#define CONSOLE_ERROR "erreur"
#define CONSOLE_DONE "terminé"
#define BACKUP_ZIP_DATA "Compression des données du serveur SQL"
#define BACKUP_ZIP_DOC "Compression des documents"
#define BACKUP_DONE "Sauvegarde terminée avec succès"
#define BAKUP_RESTART "Démarrage du serveur SQL"
#define BACKUP_FAIL "Erreurs ou avertissements durant la sauvegarde"
#define RESTART_SERVER_SHUTDOWN "Arrêt du serveur ..."
#define RESTART_SERVER_LAUNCH "Redémarrer le serveur en mode forcé"
#define OPTIMIZE_TABLE_TITLE "Optimisation de la table"
#define PROCESS_SYS_ERROR "Impossible de lire la liste des process !"
#define PROCESS_SIZE_TITLE "Taille"
#define PROCESS_TITLE "Processus"
#define PROCESS_LAUNCH_TITLE "Lancement"
#define PROCESS_CPU_TITLE "CPU (%)"
#define PROCESS_CLOSE_TITLE "Clore le process"
#define CLOSE_PROCESS_TOOLTIP "Clore le process"
#define USER_NAME_FAIL "Recherche du nom utilisateur infructueuse."
#define USER_NAME_ERROR "Recherche du label '_EVA_USERNAME' infructueuse."
#define LOGS_FILES_ERROR "Echec lors de la lecture du fichier des logs."
#define LOGS_ID_NOTFOUND "Non identifié !"
#define CONSOLE_CLOSE_PROCESS "Clore un process"
#define CONSOLE_CLOSE_PID "PID du processus à tuer"
#define TOTAL_TEXT "Total"
#define LAST_SAVE_TEXT_CHANGE "Taille avant optimisation : "
#define PROCESS_CLOSEERROR "Echec lors de la fermeture du processus !"
#define GRAPH_LOGS_ERROR "Echec lors de la lecture des logs !"
#define PROCESS_CLOSED "Processus clos !"
#define CONSOLE "console"

/*********************************************************************
** Macro : PUT_PROCESSES_VOID
** Description : miscellaneous macros for table output
*********************************************************************/
#define PUT_PROCESSES_VOID printf("<td align=center bgcolor=#%s>&nbsp;</td>", bgcolor)
#define PUT_CONSOLE_TABLE_COMM(comment)	 printf("<td bgcolor=#%s>" comment "</td></tr>", ALT_ROW_COLOR)
#define PUT_CONSOLE_TABLE_VOID puts("<td bgcolor=#%s>&nbsp;</td>", ALT_ROW_COLOR)
#define STRSTRNULL(a, b) strstr((a) ? (a) : "", (b) ? (b) : "")
#define STRICMPNUL(a,b) stricmp((a) ? (a) : "",(b) ? (b) : "")
#define PUT_CONSOLE_HEAD(title) \
	puts("</font> - <font size=+1>" title "</font></b>" \
		"</td></tr></table>")

/*********************************************************************
 Macro : ALT_ROW_COLOR
** Description : handle table row color alternance
*********************************************************************/
#define ALT_ROW_COLOR ((csl->row & 1) ? "EEEEFF" : "F8F8F8")

/*********************************************************************
 Macro : miscellaneous outputs for log table. 
** Description : Output cells table
*********************************************************************/
#define OUTPUT_HTML_CELL(data)	puts("<td align=center><b>" data "</td>")
					  
#define PUT_CONSOLE_BUTTON(btnname, btnvalue) "<input type=submit name='B#" btnname "' value='" btnvalue "'>"
#define PUT_MENU_BUTTON puts("<br>" PUT_CONSOLE_BUTTON("MENU", "Retour au menu") "<br><br>\n")

/*********************************************************************
 Macro : PUT_LOGS_BUTTON
** Description : Output a menu button & a refresh logs list one.
*********************************************************************/
#define PUT_LOGS_BUTTON(name, txt) \
puts("<table width=100% border=0 cellpadding=5><tr><td align=center>" \
	PUT_CONSOLE_BUTTON("MENU", "Retour au menu") \
	"</td><td align=center>" \
	PUT_CONSOLE_BUTTON(name, txt) \
	"</td></tr></table>");

/*********************************************************************
 Macro : PUT_PROCESS_BUTTON
** Description : Output a menu button & a refresh processes list one.
*********************************************************************/
#define PUT_PROCESS_BUTTON \
puts("<table width=100% border=0 cellpadding=5><tr><td align=center>" \
	PUT_CONSOLE_BUTTON("MENU", "Retour au menu") \
	"</td><td align=center>" \
	PUT_CONSOLE_BUTTON("LISTPROCESS", REFRESH_PROCESS_TOOLTIP) \
	"</td></tr></table>");

/*********************************************************************
 Macro : PUT_CONSOLE_ROW_HEADER
** Description : Allow html color alternace between row of a table
*********************************************************************/
#define PUT_CONSOLE_ROW_HEADER \
{ csl->row++; printf("<tr height=40 bgcolor=#%s>", ALT_ROW_COLOR); }

/*********************************************************************
 Macro : PUT_CONSOLE_TABLE_BUTTON
** Description : Allow html color alternace between row of a table
*********************************************************************/
#define PUT_CONSOLE_TABLE_BUTTON(btnname, btntitle, btnsrc)	\
 { PUT_CONSOLE_ROW_HEADER;  PUT_BUTTON(btnname, btntitle, btnsrc); }

/**********************************************************************
** Macro : PUT_RAW_BUTTON
** Description : output with a button
*********************************************************************/
#define PUT_RAW_BUTTON(btnname, btntitle, btnsrc) \
	printf("<a href='' title=\"" btntitle "\">" \
			"<input type=image  name='B#" btnname "' src='../img/_eva_" btnsrc ".gif'" \
			"OnMouseOver=\"this.src='../img/_eva_" btnsrc "_s.gif';\" " \
			"OnMouseOut=\"this.src='../img/_eva_" btnsrc ".gif';\">" \
			"</a></font>")

/**********************************************************************
** Macro : PUT_BUTTON
** Description : output a cell with a button
*********************************************************************/
#define PUT_BUTTON(btnname, btntitle, btnsrc) \
{ printf("<td align=center>"); PUT_RAW_BUTTON(btnname, btntitle, btnsrc); printf("</td>"); }
 
/*********************************************************************
** Macro : PUT_CONSOLE_CHKBOX
** Description : output checkbox
*********************************************************************/
#define PUT_CONSOLE_CHKBOX(optname, opttitle, optsrc, optlabel) \
	printf("<table noborder><tr><td>" \
			"<a href='' title=\"%s\">" \
			"<input type=image name='B#" optname "' border=0 src='../img/_eva_%s.gif'" \
			"OnMouseOver=\"this.src='../img/_eva_%s_s.gif';\"" \
			"OnMouseOut=\"this.src='../img/_eva_%s.gif';\">" \
			"</a></td><td><font size='-2'>%s</font></td></tr></table>", opttitle, optsrc, optsrc, optsrc, optlabel)
					
/*********************************************************************
 Macro : SAME_PID_AND_SERVER
** Description : Compare a end log line with a begin one
**				 Return 1 (TRUE) if equals. 
*********************************************************************/
#define SAME_PID_AND_SERVER \
(!STRCMPNUL(server, dyntab_val(logsdata, i, *server_col)) && \
!STRCMPNUL(pid, dyntab_val(logsdata, i, *pid_col)))

/*********************************************************************
 Macro : END_OF_SESSION
** Description : Return 1 (TRUE) if the read line is a end-session line
*********************************************************************/	
#define END_OF_SESSION(that_row) \
(STRCMPNUL(dyntab_val(logsdata, that_row, *clickbtn_col), "") \
&& STRCMPNUL(dyntab_val(logsdata, that_row, *clickform_col), ""))

#define HOUR_STEP 49

/*********************************************************************
 Macro : PROCESSES_NOKILL
** Description : List of processus whitout authorization to kill
*********************************************************************/
#define PROCESSES_NOKILL \
"/System/Apache.exe/csrss.exe/gemback.exe/gemserv.exe/lsass.exe/ \
netdde.exe/services.exe/lserv.exe/smss.exe/spoolsv.exe/svchost.exe/ \
vsmon.exe/winlogon.exe/WinMgnt.exe/SERVNT.exe/nvsvc.exe/regsvc.exe/ \
MSTask.exe/stisvc.exe/LOOKOUT.exe/"

EVA_context *Cntxt;

/*********************************************************************
** Function : exec_command
** Description : execute a command & print result in HTML format
*********************************************************************/
int exec_command(char *command, int mode)
{
	char cmd[2048];
	int res, i, err = 0;
	sprintf(cmd, "%s >msg.txt 2>err.txt", command);
	res = system(cmd);
	for(i = 0; i < 2; i++)
	{
		FILE *f = fopen(i ? "msg.txt" : "err.txt", "r");
		memset(cmd, 0, sizeof(cmd));
		fread(cmd, sizeof(cmd) - 1, 1, f);
		fclose(f);
		if(!i && *cmd) err = 1;
		if(!*cmd || !(mode && ( 1 << i))) continue;
		printf("%s :<br>\n<pre>%s\n%s</pre>", (i || (mode & 4)) ? "==== message" : "**** erreur", cmd, cmd[sizeof(cmd) - 1] ? "...\n" : "");
	}
	remove("err.txt");
	remove("msg.txt");
	fflush(stdout);

	return err;
}

/*********************************************************************
** Function : shutdown_sql
** Description : shutdown SQL server
*********************************************************************/
void shutdown_sql(ConsoleData *csl)
{
	EVA_context *cntxt = &csl->c;
	if(!cntxt->dbname || !*cntxt->dbname) cntxt->dbname = "mysql";
	sql_open_session(cntxt);
	if(!cntxt->sql_session) return;
	printf("<br>=> " CONSOLE_STOPSQL_TITLE " ...\n");
	fflush(stdout);
	cntxt->err.text = sql_control(cntxt, 1);
	if(cntxt->err.text)
		printf(" " CONSOLE_ERROR "<br>%s<br>\n", cntxt->err.text);
	else
	{
		if(!exec_command("net stop MySql", 1)) printf(" " CONSOLE_DONE "<br>\n");
	}
	fflush(stdout);
}

/*********************************************************************
** Function : start_sql
** Description : start SQL server
*********************************************************************/
void start_sql(ConsoleData *csl)
{
	EVA_context *cntxt = &csl->c;
	printf("<br>=> " BAKUP_RESTART "\n");
	exec_command("net start MySql", 3);
	sql_open_session(cntxt);
	fflush(stdout);
}

/*********************************************************************
** Function : backup_server
** Description : shutdown SQL server & execute backup command
*********************************************************************/
void backup_server(			/* Return 0 on success, other in error */
	ConsoleData *csl		/* in: execution context data */
){
	EVA_context *cntxt = &csl->c;
	char cmd[1024];
	int b_err;

	/* Shutdown SQL server */
	printf("<br><b><u>Sauvegarde des données du serveur</b></u><br>\n");
	shutdown_sql(csl);

	/* Prepare ZIP operation */
	chdir(cntxt->rootdir);
	putenv("TZ=MET-1MEST");
	mkdir("backup");

	/* Zip MySql files & restarts server if applicable */
	printf("<br>=> " BACKUP_ZIP_DATA " ...\n");
	fflush(stdout);
	sprintf(cmd, "cgi\\zip -u -j -9 backup/%s-data.zip mysql/data/%s/*.*", csl->seldb, csl->seldb);
	b_err |= exec_command(cmd, 1);
	puts(CONSOLE_DONE);
	if(csl->b_restart_after_bkup) start_sql(csl);

	/* Zip documents */
	puts("<br>=> " BACKUP_ZIP_DOC " ...\n");
	fflush(stdout);
	sprintf(cmd, "cgi\\zip -u -p -r backup/%s-docs.zip docs/%s", csl->seldb, csl->seldb);
	b_err |= exec_command(cmd, 1);
	puts(CONSOLE_DONE);
	if(!b_err) puts("<br>==== " BACKUP_DONE "<br>\n");
	else printf("<br>**** " BACKUP_FAIL "<br>\n");

}
/*********************************************************************
** Function : print_dyntab
** Description : output a dyntable
*********************************************************************/
void print_dyntab(DynTable *tab)
{
	unsigned long i, j;
	printf("<table border cellspacing=0 cellpadding=1>");
	for(i = 0; i < tab->nbrows; i++) 
	{
		printf("<tr>");
		for(j = 0; j < tab->nbcols; j++) printf("<td>%s</td>\n", dyntab_val(tab, i, j));
		printf("</tr>");
	}
	printf("</table>");
}

/*********************************************************************
** Function : restart_server
** Description : handle 2-steps server restart
*********************************************************************/
void restart_server(		/* Return 0 on success, other in error */
	ConsoleData *csl,		/* in : execution context data */
	int mode				/* in : five step to restart server */
){
	EVA_context *cntxt = &csl->c;
	shutdown_sql(csl);
	if(cntxt->sql_session) return;
	printf("<br>=> " RESTART_SERVER_SHUTDOWN "\n");
	fflush(stdout);
	cntxt->err.text = sql_control(cntxt, mode);
	printf(" " CONSOLE_ERROR "<br>%s<br>\n"
			"<br>" PUT_CONSOLE_BUTTON("RESTARTFORCE", RESTART_SERVER_LAUNCH) "<br>",
			cntxt->err.text);
	PUT_MENU_BUTTON;
}

/*********************************************************************
** Function : put_db_files_info
** Description : output html code for DB files info
*********************************************************************/
void put_db_files_info(
	ConsoleData *csl,				/* in: execution context data */
	char *tblname					/* in : only output .myd && .myi file if not 0 */
){
	unsigned long i = 0;
	char path[128];
	char file[100];
	char date[32];
	char datefmt[64];
	struct stat fs;
	_off_t totsize = 0;

	/* Output enclosing table for files info */
	puts("<td colspan=3><table cellspacing=0 cellpadding=2 border=1 width=100%><tr>"
		"<td bgcolor=#DDDDDD>Fichier</td>"
		"<td bgcolor=#DDDDDD>Description</td>"
		"<td align=center bgcolor=#DDDDDD>Taille</td>"
		"<td align=right bgcolor=#DDDDDD>Dernière modification</td></tr>");

	/* Output backup & MySQL files info */
	for(i = 0; i < 8; i++)
	{
		char *name = i < 2 ? csl->seldb : (i-2)&1 ? "TVal" : "TLink";
		char *cname = i < 2 ? "Dernière sauvegarde" : (i-2)&1 ? "Fichier de valeurs" : "Fichier de relations";
		char *ext = i == 0 ? "-data" : i == 1 ? "-docs" : i < 4 ? "myd" : i < 6 ? "myi" : "frm";
		char *cext = i == 0 ? "données" : i == 1 ? "documents" : i < 4 ? "données" : i < 6 ? "index" : "structure";
		if(tblname && (strcmp(tblname, name) || !strcmp("frm", ext))) continue;
		if(i < 2)
		{
			sprintf(file, "%s%s.zip", name, ext);
			sprintf(path, "../backup/%s", file);
		}
		else
		{
			sprintf(file, "%s.%s", name, ext);
			sprintf(path, "../mysql/data/%s/%s", csl->seldb, file);
		}
		if(!stat(path, &fs))
		{
			time_to_datetxt(fs.st_mtime, date);
			datetxt_to_format(datefmt, date, NULL);
			printf("<tr><td>");
			if(i < 2)
				printf("<a href='%s' titlr='Télécharger le document'>%s</a>", path, file);
			else 
				printf("%s", file);
			printf("</td><td><font size=-1>%s - %s</font></td>"
				"<td align=center>%s</td><td align=right>%s</td></tr>", 
				cname, cext, human_filesize(fs.st_size), datefmt);
		}
	}
	puts("</table></td></tr>");
}

/*********************************************************************
** Function : optimize_table
** Description : optimize the sql 'tblname'.MYD	files
*********************************************************************/
#define ERR_FUNCTION "optimize_table"
#define ERR_CLEANUP	M_FREE(query); \
					DYNTAB_FREE(sqlres);
int optimize_table(			/* Return 0 on success, other in error */
	ConsoleData *csl,		/* in: execution context data */
	char *tblname			/* in: table name to optimize */
){
	EVA_context *cntxt = &csl->c;
	DynBuffer *query = NULL;
	DynTable sqlres = {0};

	DYNBUF_ADD3(&query, "OPTIMIZE TABLE ", tblname, 0, NO_CONV, "");
	printf("<u>Taille avant compactage</u><br>");
	put_db_files_info(csl, tblname);
	printf("<br>=> " OPTIMIZE_TABLE_TITLE " %s ...\n", tblname);
	fflush(stdout); cntxt->err.text = 0;
	printf(" %s<br>\n",	(sql_exec_query(cntxt, query->data) || 
			sql_get_table(cntxt, &sqlres, 3)) ? CONSOLE_ERROR : CONSOLE_DONE);

	if(cntxt->err.text) printf("%s<br>\n", cntxt->err.text);
	else print_dyntab(&sqlres);
	puts("");
	printf("<br><u>Taille après compactage</u>");
	put_db_files_info(csl, tblname);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_snapshot
** Description : Output a html table with all the processes running.
*********************************************************************/
#define ERR_FUNCTION "output_snapshot"
#define ERR_CLEANUP	M_FREE(proc_name);
int output_snapshot(			/* Return 0 on success, other in error */
	ConsoleData *csl			/* in	: execution context data */
){
	EVA_context *cntxt = &csl->c;
	HANDLE			hProcessSnap = NULL; 
	int				bRet      = 0; 
	PROCESSENTRY32	pe32      = {0};
	DynBuffer		*proc_name = NULL;
	MEMORYSTATUS	Buffer;
	DynTable  processes = {0};
	unsigned long row, i;
	unsigned long *cpu_time = NULL;

	M_ALLOC(cpu_time, 500 * sizeof(unsigned long));


	puts("<table cellspacing=0 cellpadding=2 border=1 width=100% rules=none><tr>"
		"<td align=center bgcolor=#DDDDDD width=200><b>" PROCESS_TITLE "</td>"
		"<td bgcolor=#DDDDDD align=center width=100><b>" PROCESS_SIZE_TITLE "</td>"
		"<td bgcolor=#DDDDDD align=center width=200><b>" PROCESS_LAUNCH_TITLE "</td>"
		"<td bgcolor=#DDDDDD align=center width=100><b>" PROCESS_CPU_TITLE "</td>"
		"<td bgcolor=#DDDDDD width=150>&nbsp;</td>"
		"</tr>");

	/* Retrieves the total system memory */
	GlobalMemoryStatus(&Buffer);

	/*  Take a snapshot of all processes in the system. */
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	/*  Fill in the structure size before using it. */
	pe32.dwSize = sizeof(PROCESSENTRY32);

		
	/*  Walk the snapshot of the processes, and display information. */ 
	for(i = 1; i < 3; i++)
	{
		if (hProcessSnap == INVALID_HANDLE_VALUE || !Process32First(hProcessSnap, &pe32))
			RETURN_ERROR(PROCESS_SYS_ERROR, NULL)

		row = 0;
		/* For each process in the snapshot */
		do 
		{ 
			FILETIME CrTime, ExitTime, KernelTime, UserTime;
			SYSTEMTIME creation;
			FILETIME LocalTime;
			PROCESS_MEMORY_COUNTERS pmc;						 

			/* Retrieve the 'process' handle in 'query' mode */
			HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION,
													0, pe32.th32ProcessID);


			if(process_handle && !(csl->b_cgi_proc_only && !(STRSTRNULL(pe32.szExeFile, ".cgi"))))
			{
				char launch_time[24] = {0};

				if(i == 1)
				{
					DYNTAB_ADD_INT(&processes, row, PROCESSE_HANDLE_COL, (int) &process_handle)
					DYNTAB_ADD_INT(&processes, row, PROCESSE_ROW_COL, (int) row)

					DYNTAB_ADD(&processes, row, PROCESSE_NAME_COL, pe32.szExeFile, 0, NO_CONV)

					/* Retrieve the process size RAM memory  */
					if(GetProcessMemoryInfo(process_handle, &pmc, sizeof(pmc)))
						DYNTAB_ADD(&processes, row, PROCESSE_SIZE_COL, human_filesize((size_t)pmc.WorkingSetSize), 0, NO_CONV)

					/* Retrieve the process launch time and cpu time */
					if(GetProcessTimes(process_handle, &CrTime, &ExitTime, &KernelTime, &UserTime)
						&& FileTimeToLocalFileTime(&CrTime, &LocalTime)
						&& FileTimeToSystemTime(&LocalTime, &creation)
						&& (&CrTime.dwLowDateTime || &CrTime.dwHighDateTime))
							 sprintf(launch_time, "%02u/%02u/%02u %02u:%02u:%02u"
									, creation.wDay, creation.wMonth, creation.wYear, creation.wHour
									, creation.wMinute, creation.wSecond);


					DYNTAB_ADD(&processes, row, PROCESSE_LAUNCH_COL, launch_time, 0, NO_CONV)

					cpu_time[row] = KernelTime.dwLowDateTime + UserTime.dwLowDateTime;

				}
				else
				{
					if(GetProcessTimes(process_handle, &CrTime, &ExitTime, &KernelTime, &UserTime))
						cpu_time[row] = KernelTime.dwLowDateTime + UserTime.dwLowDateTime - cpu_time[row];
				}

				row++;
			}
			CloseHandle(process_handle); 
		} 
		while (Process32Next(hProcessSnap, &pe32) & row < 500);

		/* Wait 1 sec for pick-up again cpu_time */
		Sleep(1000);
	}

	CloseHandle(hProcessSnap);

	dyntab_sort(&processes, qsort_col2);

	/* Output the html_processeslist in the user order */
	for(row = 0; row < processes.nbrows; row++)
	{
		char *data = NULL;
		char *bgcolor = ALT_ROW_COLOR;

		csl->row++;
		/* Output the process's name. */
		data = dyntab_val(&processes, row, PROCESSE_NAME_COL);
		printf("<tr><td bgcolor=#%s>%s</td>", bgcolor, data ? data : "&nbsp;");

		data = dyntab_val(&processes, row, PROCESSE_SIZE_COL);
		printf("<td align=center bgcolor=#%s>%s</td>", bgcolor, data ? data : "&nbsp;");
		
		data = dyntab_val(&processes, row, PROCESSE_LAUNCH_COL);
		printf("<td align=center bgcolor=#%s>%s</td>", bgcolor, data ? data : "&nbsp;");

		/* Output process cpu time in ms */
		printf("<td align=center bgcolor=#%s>%.2f</td>", bgcolor, (cpu_time[row])/ 100000.);

		/* Output the 'kill_process' button if not in restricted list */
		DYNBUF_ADD3(&proc_name, "/", dyntab_val(&processes, row, PROCESSE_NAME_COL), 0, NO_CONV, "/")
		if(!STRSTRNULL(PROCESSES_NOKILL, proc_name->data))
		{
			printf("<td align=center bgcolor=#%s><a href='' title=\"" CLOSE_PROCESS_TOOLTIP "\">"
					"<input type=image name='B#KILLPROC%lu'"
					"src='../img/kill_proc.gif'"
					"OnMouseOver=\"this.src='../img/kill_proc_s.gif';\""
					"OnMouseOut=\"this.src='../img/kill_proc.gif';\">"
					"</a></font></td>"
					, bgcolor, dyntab_val(&processes, row, PROCESSE_HANDLE_COL));
		}
		else PUT_PROCESSES_VOID;

		printf("</tr>");

		M_FREE(proc_name)
	}
	csl->row = 0;

	/* Finalize the 'processes''s table menu */
	puts("</table></td></tr></table></font>"); 

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : retrieve_user_name
** Description : Retrieve the user name from a given session number
*********************************************************************/
#define ERR_FUNCTION "retrieve_user_name"
#define ERR_CLEANUP	M_FREE(query); \
					DYNTAB_FREE(who_create); \
					DYNTAB_FREE(eva_username_idvalue);
int retrieve_user_name(				/* Return 0 on success, other in error */
	ConsoleData *csl,				/* in	: execution context data */ 
	char *session, 					/* in	: session Id used to retrieve username */
	char *dbname, 					/* in	: database name from which retrieve username */
	DynTable *sqlres				/* out	: username retrieve from database */
){
	EVA_context *cntxt = &csl->c;
	DynBuffer *query = NULL;
	DynTable eva_username_idvalue = {0};
	DynTable  who_create  = {0};

	/* Switch dbname connexion if applicable */
	if(STRCMPNUL(cntxt->dbname, dbname))
	{
		if(sql_control(cntxt, 0)) STACK_ERROR(1);
		cntxt->dbname = dbname;
	}


	/* Retrieve the tval 'idvalue' value for '_EVA_USERNAME' */
	form_eval_fieldexpr(cntxt, sqlres, 0, strtoul(session, NULL, 10), add_sz_str("_EVA_USER->_EVA_USERNAME"), NULL, 0);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : read_logsfile
** Description : Read the logs files.
*********************************************************************/
#define ERR_FUNCTION "read_logsfile"
#define ERR_CLEANUP M_FREE(logsfile);
int read_logsfile(					/* Return 0 on success, other in error */
	ConsoleData *csl,				/* in: execution context data */
	DynTable *logsdata,
	unsigned long *server_col,
	unsigned long *date_col,
	unsigned long *user_col,
	unsigned long *pid_col,
	unsigned long *clock_col,
	unsigned long *clickbtn_col,
	unsigned long *clickform_col
){
	EVA_context *cntxt = &csl->c;
	DynBuffer *logsfile = NULL;
	unsigned long t = ~0UL;
	unsigned long row = 1, col = 0;

	/* Build the logs name */
	DYNBUF_ADD_STR(&logsfile, "../logs/")
	DYNBUF_ADD(&logsfile, cntxt->logfile, 0, NO_CONV)

	/* Call the read logsfile function */
	if(file_read_tabrc(cntxt, logsdata, logsfile->data) && logsdata->nbrows > 1)
		RETURN_ERROR("Le programme n'a pas pu lire le fichier des logs!", NULL);

	/* Compute the first row to retrieve the searched cols head name */
	for(col = 0; col < logsdata->nbcols; col++)
	{
		char *headdata = dyntab_val(logsdata, 0, col);

		/* Search for date data col */
		if(headdata && !STRCMPNUL(headdata, "time"))
			*date_col = col;

		/* Search for cgi process name data col */
		if(headdata && !STRCMPNUL(headdata, "cgi")) 
			*server_col = col;
		
		/* Search for user name data col */

		if(headdata && !STRCMPNUL(headdata, "Session")) 
			*user_col = col;

		/* Search for pid process data col */
		if(headdata && !STRCMPNUL(headdata, "pid"))
			*pid_col = col;

		/* Search for clock data col */
		if(headdata && !STRCMPNUL(headdata, "clock"))
			*clock_col = col;

		/* Search for ClickBtn data col */
		if(headdata && !STRCMPNUL(headdata, "ClickBtn"))
			*clickbtn_col = col;

		/* Search for ClickForm data col */
		if(headdata && !STRCMPNUL(headdata, "ClickForm"))
			*clickform_col = col;
	}

	if(*date_col == t || *server_col == t || *user_col == t || *pid_col == t
		|| *clock_col == t || *clickbtn_col == t)
		{ERR_CLEANUP return 1;}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : retrieve_hour_step
** Description : Return the step positionfor an hour in a day
*********************************************************************/
unsigned long retrieve_hour_step(char *date_to_compute)
{
	char date[32];
	int Y0 = 0, Y1 = 0, M0 = 0, M1 = 0, D0 = 0, D1 = 0, h0 = 0, h1 = 0, m0 = 0, m1 = 0, s0 = 0, s1 = 0;
	unsigned long step;

	if(!date_to_compute	|| !*date_to_compute) return 0;

	if(time_to_datetxt(strtoul(date_to_compute, NULL, 10), date)) return 0;
	sscanf(date, "%4d%2d%2d%2d%2d%2d", &Y0, &M0, &D0, &h0, &m0, &s0);

	step = h0 * 2 + 1 + ((m0 < 31) ? 0 : 1);
	return step;
}

/*********************************************************************
** Function : compute_logs
** Description : Read data from the logs files.
*********************************************************************/
#define ERR_FUNCTION "compute_logs"
#define ERR_CLEANUP
int compute_logs(					/* Return 0 on success, other in error */
	ConsoleData *csl,				/* in	: execution context data */
	DynTable *logsdata,				/* in   : logs from logs file */
	DynTable *loglist,				/* out  : parsed logs, listed by server and user */
	unsigned long *server_col,		/* in   : server column in loglist */
	unsigned long *date_col,		/* in   : session launching date column in loglist */
	unsigned long *user_col,		/* in   : session column in loglist */
	unsigned long *pid_col,			/* in   : pid column in loglist */
	unsigned long *clock_col,		/* in   : session total exec time column in loglist */
	unsigned long *clickbtn_col,	/* in   : clicked button column in loglist */
	unsigned long *clickform_col,	/* in   : clicked form column in loglist */
	unsigned long *click_nb,		/* out  : user click number per server */
	unsigned long *crash_nb,		/* out  : crash number for each server */
	unsigned long *user_click_nb,	/* out  : user click number per 1/2 hour */
	double *reply_time				/* in   : average reply time column in loglist */
){
	EVA_context *cntxt = &csl->c;
	unsigned long row = 1, i = 0;
char *debug = NULL;

	csl->dbnumber = ~0UL;

	/* Search dbname on witch console is logged 	*/
	for(i = 0; i < csl->dbnames.nbrows; i++)
		if(!STRCMPNUL(csl->seldb, dyntab_val(&csl->dbnames, i, 0))) 
		{
			csl->dbnumber = i;
			break;
		}

	/* Search and compute session data */
	do
	{
		int begin_session = 0, yet_memorized = 0;
		unsigned long i, log_user_row = ~0UL, log_server_col = ~0UL, indice;
		char *server = NULL, *pid = NULL, *session = NULL, *user = NULL;
		int server_session = 0, computestat = ~0UL;
		DynTable username = {0};

		/* Read a suposed begin session line in loglist */
		server = dyntab_val(logsdata, row, *server_col);
		pid = dyntab_val(logsdata, row, *pid_col);

		if(!END_OF_SESSION(row)) begin_session = 1;

		/* Search if the server = console */
		if(!STRCMPNUL(server, "console")) server_session = 1;

		/* Search for the end of this session */
		if(begin_session) 
			for(i = row + 1; i < logsdata->nbrows; i++)
				if(SAME_PID_AND_SERVER && END_OF_SESSION(i))
				{
					session = dyntab_val(logsdata, i, *user_col);
					computestat = i;
					break;
				}	


		/* Search the server line number in csl->dbnames which began the server column in loglist */
		for(i = 0; i < csl->dbnames.nbrows; i++)
			if(!STRCMPNUL(server, dyntab_val(&csl->dbnames, i, 0))) 
			{
				log_server_col = i;
				break;
			}

		if(begin_session)
		{
			/* Retrieve user name in 'server' data base */
			if(!server_session)
			{
				if(computestat != ~0UL)
				{
					retrieve_user_name(csl, session, server, &username);
					user = dyntab_val(&username, 0, 0);
				}
			}
			else user = session;

			if(!user || !*user) user = "(Vide)";

			/* Search user name in loglist */
			for(i = 0; i < loglist->nbrows; i++)
				if(!STRICMPNUL(user, dyntab_val(loglist, i, 0)))
				{
					log_user_row = i;
					break;
				}

			/* if the user is not listed in loglist, add it */
			if(log_user_row == ~0UL)
			{
				DYNTAB_ADD(loglist, loglist->nbrows, 0, user, 0, NO_CONV)
				log_user_row = loglist->nbrows - 1;
			}

			if(computestat != ~0UL && log_server_col != ~0UL)	
			{
				char *date_to_compute = dyntab_val(logsdata, computestat, *date_col);
				unsigned long hour_step_col = retrieve_hour_step(date_to_compute);

				DYNTAB_ADD(loglist, log_user_row, log_server_col + 1, date_to_compute, 0, NO_CONV)
				indice = log_user_row * csl->dbnames.nbrows + log_server_col;
				click_nb[indice]  += 1;
				reply_time[indice] = reply_time[indice] + (strtod(dyntab_val(logsdata, computestat, *clock_col), NULL));

				/* To display Graph only on actual data base */
				if(log_server_col == csl->dbnumber)
					user_click_nb[(log_user_row * HOUR_STEP) + hour_step_col] += 1;
			}
			else if(log_server_col != ~0UL)	
				crash_nb[log_server_col] += 1;

			/* Memorize this server has user clicks */
			if(log_server_col != ~0UL) DYNTAB_ADD(&csl->dbnames, log_server_col, 1, "1", 0, NO_CONV)
		}	
				
		row++;

	} while(row < logsdata->nbrows - 1);


	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_html_list_logs
** Description : Output(puts + printf) a html table with the day logs.
*********************************************************************/
#define ERR_FUNCTION "output_html_list_logs"
#define ERR_CLEANUP	
int output_html_list_logs(	/* Return 0 on success, other in error */
	ConsoleData *csl,				/* in	: execution context data */
	DynTable *loglist,				/* in   : parsed logs, listed by user */
	unsigned long *click_nb,		/* in   : user click number per server */
	unsigned long *crash_nb,		/* in   : crash number for each server */
	double *reply_time				/* in   : average reply time column in loglist */
){
	EVA_context *cntxt = &csl->c;
	unsigned long row = 1, col = 0, i, indice;
	unsigned long *total_clicks, *reply_time_den;
	double *total_rt;
	unsigned long dbname_number;

	if(!loglist || !click_nb || !crash_nb || !reply_time) RETURN_ERR_PARAM(1)

	dbname_number = csl->dbnames.nbrows + 1;
	C_ALLOC(reply_time_den, dbname_number * sizeof(unsigned long), 1);
	C_ALLOC(total_clicks, dbname_number * sizeof(unsigned long), 1);
	C_ALLOC(total_rt, dbname_number * sizeof(double), 1);
	
	/* Output the head of html loglist table */
	puts("<table cellspacing=0 cellpadding=0 border=1 width='100%' rules=none style=\"font-size:-1\">"
		 "<tr><td><table cellspacing=0 cellpadding=0 border=1 width='100%' rules=cols>\n");

	
	/* Output the server names line and subtitle line */
	puts("<tr><td><table cellspacing=0 cellpadding=0 width='100%'><tr><td>&nbsp;</td></tr></table></td>");
	for(i = 0; i < csl->dbnames.nbrows; i++)
		if(!STRCMPNUL("1", dyntab_val(&csl->dbnames, i, 1)))
			printf("<td><table cellspacing=0 cellpadding=0 width='100%%'>"
				   "<tr><td align=center><b>%s</td></tr></table></td>", dyntab_val(&csl->dbnames, i, 0));

	puts("</tr><tr align=center bgcolor=#DDDDDD><td><table cellspacing=0 cellpadding=0 border=1 width='100%%'>"
		 "<tr><td align=center>&nbsp;Utilisateur</td></tr></table></td>");
	for(i = 0; i < csl->dbnames.nbrows; i++)
		if(!STRCMPNUL("1", dyntab_val(&csl->dbnames, i, 1)))
			puts("<td><table cellspacing=0 cellpadding=0 border=1 width='100%' rules=rows><tr>"
				 "<td align=left width='10%'><b>Cliks</td><td width='10%'>&nbsp;</td>"
				 "<td align=center width='60%'><b>Heure</td><td width='10%'>&nbsp;</td>"
				 "<td align=right width='10%'><b>Tps</td></tr></table></td>");
	puts("</tr>");

	/* Output HTML data from loglist */
	for(row = 0; row < loglist->nbrows; row++)
	{
		/* Output the user name */
		printf("<tr bgcolor=#%s><td align=left>%s</td>", ALT_ROW_COLOR, dyntab_val(loglist, row, 0));
		csl->row++;

		for(i = 0; i < csl->dbnames.nbrows; i++)
		{
			char valdate[32];
			char outputdate[32];
			double ave_reply_time;

			if(STRCMPNUL("1", dyntab_val(&csl->dbnames, i, 1))) continue;
			
			/* Output the user click number & average working time & last session formated date */ 
			indice = row * csl->dbnames.nbrows + i;
			if(click_nb[indice])
			{
				puts("<td><table cellspacing=0 cellpadding=0 border=0 width='100%' rules=none>"
					 "<tr><td align=left width='10%'><b>");
				printf("%lu</td>", click_nb[indice]);
				total_clicks[i] += click_nb[indice];

				if(!time_to_datetxt(strtoul(dyntab_val(loglist, row, i + 1), NULL, 10),
					valdate) && !datetxt_to_format(outputdate, valdate, "_"))
						printf("<td width='10%%'>&nbsp;</td><td align=center width='60%%'>%s</td><td width='10%%'>&nbsp;</td>", outputdate + 11);

				else puts("<td width='10%'>&nbsp;</td><td align=center width='60%'>&nbsp;</td><td width='10%'>&nbsp;</td>");

				ave_reply_time = reply_time[indice] / (1000 * click_nb[indice]);
				printf("<td align=right width='10%%'>%#.3f</td></tr></table></td>", ave_reply_time);
				total_rt[i] += ave_reply_time;
				reply_time_den[i] += click_nb[indice];
			}
			else puts("<td>&nbsp;</td>");
		}
		puts("</tr>");
	}

	/*  Output the crash number */
	puts("<tr><td align=left>&nbsp;*** Crash</td>");
	for(i = 0; i < csl->dbnames.nbrows; i++)
	{
		if(STRCMPNUL("1", dyntab_val(&csl->dbnames, i, 1))) continue;

		if(crash_nb[i]) printf("<td align=left><b>%lu</td>", crash_nb[i]);
		else puts("<td align=left>&nbsp;</td>");
		total_clicks[i] += crash_nb[i];
	}

	/* Output sum row */
	puts("</tr><tr bgcolor=#DDDDDD><td><table cellspacing=0 cellpadding=0 border=1 width='100%'>"
		 "<tr><td align=center><b>Total</td></tr></table></td>");
	for(i = 0; i < csl->dbnames.nbrows; i++)
		if(!STRCMPNUL("1", dyntab_val(&csl->dbnames, i, 1)))
			printf("<td><table cellspacing=0 cellpadding=0 border=1 width='100%%' rules=rows><tr>"
				 "<td align=left width='30%%'><b>%u</td><td align=center width='40%%'>&nbsp;</td>"
				 "<td align=right width='30%%'><b>%#.3f</td></tr></table></td>", total_clicks[i],
				 reply_time_den[i] ? (total_rt[i] / reply_time_den[i]) : total_rt[i]);

	puts("</tr></table></td></tr></table></td></tr></table>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_graph_logs
** Description : Handle the the day logs output .
*********************************************************************/
#define ERR_FUNCTION "output_graph_logs"
#define ERR_CLEANUP	 DYNTAB_FREE(g); \
					 M_FREE(html)
int output_graph_logs(
	ConsoleData *csl,				/* in	: execution context data */
	DynTable *loglist,				/* in   : parsed logs, listed by user */
	unsigned long *user_click_nb	/* in   : # user cliks by user	*/
){
	EVA_context *cntxt = &csl->c;
	unsigned long i, j, since = 1, until = 1, total_user = 0;
	DynTable g = { 0 }, *graphdata = &g;
	EVA_form f = {0}, *form = &f;
	DynBuffer *html = dynbuf_init(1024);

	if(!loglist)  RETURN_ERR_PARAM(1)

	DYNTAB_SET(graphdata, FACING_ROW, FACING_COL, "V")

	DYNTAB_SET(graphdata, LEGENDPOS_ROW, LEGENDPOS_COL, "2")

	DYNTAB_SET(graphdata, WIDTH_ROW, WIDTH_COL, "100%")

	DYNTAB_SET(graphdata, HEIGHT_ROW, HEIGHT_COL, "50%")

	DYNTAB_SET(graphdata, MINWIDTH_ROW, MINWIDTH_COL, "3")

	DYNTAB_ADD(graphdata, TITLE_ROW, TITLE_COL, CONSOLE_GRAPH_TITLE, 0, NO_CONV)

	DYNTAB_SET(graphdata, SUBTITLE_ROW, SUBTITLE_COL, "Horaires")

	/* Add user names if exist 'last date' click for the logged database */
	for(i = 0; i < loglist->nbrows; i++)
		if(*dyntab_val(loglist, i, csl->dbnumber + 1))
		{
			DYNTAB_SET_CELL(graphdata, SUBTITLE_ROW, i + SUBTITLE_COL + 1, loglist, i, 0)
			total_user += 1;
		}

	DYNTAB_SET(graphdata, SUBTITLEUNITY_ROW, SUBTITLEUNITY_COL, "clic(s)")

	DYNTAB_SET(graphdata, UNITYTYPE_ROW, UNITYTYP_COL, "1")
	
	DYNTAB_ADD_INT(graphdata, DIMENSION_ROW, DIMENSION_COL, total_user ? total_user : 1)

	DYNTAB_SET(graphdata, SINGLECOL_ROW, SINGLECOL_COL, "1")

	DYNTAB_SET(graphdata, MAXVALUE_ROW, MAXVALUE_COL, "")

	DYNTAB_SET(graphdata, MINVALUE_ROW, MINVALUE_COL, "0")

	/* Compute the start & end hours */
	for(i = 1; i < HOUR_STEP; i++)
	{
		for(j = 0; j < loglist->nbrows; j++)
		{
			if(user_click_nb[(j * HOUR_STEP) + i] && (since == 1)) since = i;
			if(user_click_nb[(j * HOUR_STEP) + i]) until = i;
		}
	}

	/* Display 8h graph width since 8h00 if apliable */
	if(since == 1) since = 17;
	until = (until < 16 + since) ? ((since + 16 < HOUR_STEP) ? since + 16 : HOUR_STEP - 1) : until;

	/* Add labels & values */
	for(i = since; i < until + 1; i++)
	{
		DynBuffer *buf = NULL; 
		div_t res;

		res = div(i - 1, 2);
		if(!res.rem)
		{
			if(res.quot < 10)	DYNBUF_ADD3_INT(&buf, "0", res.quot, "")
			else				DYNBUF_ADD3_INT(&buf, "", res.quot, "h")
									
			 DYNTAB_ADD_BUF(graphdata, FIRSTVALUE_ROW + i - since, VALUE_COL, buf)
		}

		for(j = 0; j < loglist->nbrows; j++)
			if(*dyntab_val(loglist, j, csl->dbnumber + 1))
				DYNTAB_ADD_INT(graphdata, FIRSTVALUE_ROW + i - since, j + VALUE_COL + 1, user_click_nb[(j * HOUR_STEP) + i])
	}

	cntxt->form = form;
	cntxt->form->html = &html;
	if(put_html_graphe(cntxt, graphdata)) RETURN_ERR_PARAM(1)
	else if(*cntxt->form->html) puts((*cntxt->form->html)->data);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_logs
** Description : Handle the the day logs output .
*********************************************************************/
#define ERR_FUNCTION "output_logs"
#define ERR_CLEANUP	DYNTAB_FREE(list); \
					DYNTAB_FREE(logsdata); \
					M_FREE(click_nb); \
					M_FREE(crash_nb);
int output_logs(	/* Return 0 on success, other in error */
	ConsoleData *csl,		/* in	: execution context data */
	int graph				/* in: pseudo boolean to hadle graph/list logs */
){
	EVA_context *cntxt = &csl->c;

	/* Memorized & parsed tabulated text file */
	DynTable logsdata = { 0 };
	DynTable list = { 0 }, *loglist = &list;
	int altern_color = 0;
	unsigned long *click_nb = NULL, *crash_nb = NULL, *user_click_nb;
	unsigned long row = 1, col = 0;
	unsigned long server_col = ~0UL, date_col = ~0UL, user_col = ~0UL, pid_col = ~0UL, clock_col = ~0UL;
	unsigned long clickbtn_col = 0, clickform_col = 0, dbname_number;
	double *reply_time = NULL;
	char *day = "toto";

	if(!cntxt->logfile) {ERR_CLEANUP return 1;}

	if(read_logsfile(csl, &logsdata, &server_col, &date_col, &user_col,
					&pid_col, &clock_col, & clickbtn_col, &clickform_col)) 
		RETURN_ERROR(LOGS_FILES_ERROR, NULL)

	dbname_number = csl->dbnames.nbrows + 1;
	C_ALLOC(click_nb, logsdata.nbrows * sizeof(unsigned long), 1);
	C_ALLOC(reply_time, logsdata.nbrows * sizeof(double), 1);
	C_ALLOC(crash_nb, sizeof(unsigned long) * dbname_number, 1);
	C_ALLOC(user_click_nb, sizeof(unsigned long) * logsdata.nbrows * HOUR_STEP, 1);



	/* Read & parse the logs data & output logs or graph is apliable */
	compute_logs(csl, &logsdata, loglist, &server_col, &date_col, &user_col,
					&pid_col, &clock_col, & clickbtn_col, &clickform_col,
					click_nb, crash_nb, user_click_nb, reply_time);

	if(!graph && output_html_list_logs(csl, loglist, click_nb, crash_nb, reply_time))
		RETURN_ERR_PARAM(1)

	if(csl->seldb && *csl->seldb && graph && output_graph_logs(csl, loglist, user_click_nb))
		RETURN_ERR_PARAM(1)

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : retrieve_cgi_database
** Description : return the cgi programs names with their correponding
**				 data base names
*********************************************************************/
#define ERR_FUNCTION "retrieve_cgi_database"
#define ERR_CLEANUP	DYNTAB_FREE(cginames);
int retrieve_cgi_databases(			/* Return 0 on success, other in error */
	ConsoleData *csl				/* in: execution context data */
){
	EVA_context *cntxt = &csl->c;
	DynTable cginames = {0};
	long handle = 0;
	struct _finddata_t filedata = {0};
	unsigned long row = 0;
	unsigned long i = 0;
	struct stat directory_info = {0};

	chdir(cntxt->path);

	/* Search for all cgi files in path */
	handle = _findfirst("*.cgi", &filedata);
	if(handle == -1L) RETURN_ERR_DIRECTORY(1)

	do
	{
		filedata.name[strlen(filedata.name)-4] = 0;

		DYNTAB_ADD(&cginames, row++, 0, filedata.name, 0, NO_CONV)

	} while (!_findnext(handle, &filedata));

	_findclose(handle);
	
	/* Search for each cgi files in path the corresponding database in mysql/data */

	if(chdir("..\\mysql\\data")) RETURN_ERR_DIRECTORY(1)
	row = 0;
	for(i = 0; i < cginames.nbrows; i++)
		if(!stat(dyntab_val(&cginames, i, 0), &directory_info)) /*&& (directory_info.st_mode & _S_IFDIR))*/
			DYNTAB_ADD_CELL(&csl->dbnames, row++, 0, &cginames, i, 0)
			
	if(!stat("mysql", &directory_info)) DYNTAB_ADD(&csl->dbnames, row, 0, CONSOLE, 0, NO_CONV)

	chdir(cntxt->path);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : kill_process
** Description : kill a given process.
*********************************************************************/
#define ERR_FUNCTION "kill_process"
#define ERR_CLEANUP
int kill_process(			/* Return 0 on success, other in error */
	ConsoleData *csl,		/* in: execution context data */
	char *procpidtokill		/* in: process id to kill */ 
){
	EVA_context *cntxt = &csl->c;
	HANDLE process_handle;
	DWORD proctokill = 0;
	UINT ExitCode = 0;
	unsigned long i = 0;
	int exit_code = 0;
	
	/* Output the result action title */
	puts("<br><b><u>" CONSOLE_CLOSE_PROCESS "</b></u><br>\n");

	/* Output process to kill ID */
	if(procpidtokill && *procpidtokill)
	{
		proctokill = strtoul(procpidtokill, NULL, 10);
		sprintf(CONSOLE_CLOSE_PID " %s<br>", procpidtokill);
	
		/* Retrieve process handle & kill */
		process_handle = OpenProcess(PROCESS_TERMINATE, 0, proctokill);

		/* TODO : check process is not in restricted list */

		if(!TerminateProcess(process_handle, ExitCode))
			exit_code = 1;

		CloseHandle(process_handle);
	}

	ERR_CLEANUP
	return exit_code;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_database_list
** Description : Output HTML select list of server databases.
*********************************************************************/
#define ERR_FUNCTION "output_database_list"
#define ERR_CLEANUP
int output_database_list(			/* Return 0 on success, other in error */
	ConsoleData *csl				/* in: execution context data */
){
	EVA_context *cntxt = &csl->c;
	unsigned long i = 0;

	/* Output HTML list header */
	PUT_CONSOLE_ROW_HEADER;
	printf(
		"<td align=center bgcolor=#%s>Site "
		"<select size=0 name='D#SELDB' onChange=\"mainform.submit();\">"
		"<option></option>",
		ALT_ROW_COLOR, csl->seldb ? "" : " selected");

	/* Output options */
	for(i = 0; i < csl->dbnames.nbrows; i++)
	{
		char *option_name = dyntab_val(&csl->dbnames, i, 0);
		printf("<option value=%s%s>%s</option>", 
				option_name,
				(!STRCMPNUL(csl->seldb, option_name)) ? " selected" : "",
				option_name);
	}
	
	/* Output HTML list footer */
	puts("</select></td>");	
	PUT_CONSOLE_TABLE_COMM(CONSOLE_CHANGE_DB_COMMENT);
	csl->b_seldb_list = 1;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_db_specific_controls
** Description : output html code for DB specific controls
*********************************************************************/
#define ERR_FUNCTION "put_db_specific_controls"
#define ERR_CLEANUP
int put_db_specific_controls(		/* Return 0 on success, other in error */
	ConsoleData *csl				/* in: execution context data */
){
	EVA_context *cntxt = &csl->c;

	/* Check params */
	if(!csl->seldb || strlen(csl->seldb) > 64) RETURN_ERROR("Nom de base de données invalide", NULL);

	/* Output backup & MySQL files info */
	put_db_files_info(csl, 0);

	/* Output Optimize & Backup buttons */
	PUT_CONSOLE_TABLE_BUTTON("OPTIMIZE", CONSOLE_OPTIMIZE_TOOLTIP, "compact_sqltables");
	PUT_CONSOLE_TABLE_COMM(CONSOLE_OPTIMIZE_COMMENT);
	PUT_CONSOLE_ROW_HEADER;
	printf("<td align=center>");
	PUT_RAW_BUTTON("BACKUP", CONSOLE_BACKUP_TOOLTIP, "save_sqltables"); 

	/* Output Restart MySQL after backup checkbox if applicable */
	if(csl->b_allow_db_select)
		PUT_CONSOLE_CHKBOX("RSTBK", BACKUP_TEXT , 
					csl->b_restart_after_bkup ? "checkbox_yes" : "checkbox_no",
					CONSOLE_RESTARTSERVER_LABEL);
	printf("</td>");
	PUT_CONSOLE_TABLE_COMM(CONSOLE_BACKUP_COMMENT);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_global_buttons
** Description : Output html code for sql, process and logs buttons.
*********************************************************************/
#define ERR_FUNCTION "put_global_buttons"
#define ERR_CLEANUP
int put_global_buttons(				/* return : 0 on success, other in error */
	ConsoleData *csl				/* in: execution context data */
){
	EVA_context *cntxt = &csl->c;

	/* Output logs button */
	PUT_CONSOLE_TABLE_BUTTON("LOGS", CONSOLE_LOGS_TOOLTIP, "logslist");
	PUT_CONSOLE_TABLE_COMM(CONSOLE_LOGS_COMMENT);

	/* Output 'DayLog' Graph button */
	if(csl->seldb && *csl->seldb)
	{
		PUT_CONSOLE_TABLE_BUTTON("GRAPH", CONSOLE_GRAPH_TOOLTIP, "daylog_graph");
		PUT_CONSOLE_TABLE_COMM(CONSOLE_GRAPH_COMMENT);
	}

/* Output CALENDAR 
PUT_CONSOLE_TABLE_BUTTON("CALENDAR", "Calendrier du jour", "daylog_graph");
PUT_CONSOLE_TABLE_COMM("Calendrier du jour");	*/
	
	/* Output 'process list' button & cgi only checkbox */
	PUT_CONSOLE_ROW_HEADER;
	printf("<td align=center>");
	PUT_RAW_BUTTON("LISTPROCESS", CONSOLE_PROCESS_TOOLTIP, "processlist");
	PUT_CONSOLE_CHKBOX("CGIPR", PROCESS_TEXT , 
						csl->b_cgi_proc_only ? "checkbox_yes" : "checkbox_no",
						CONSOLE_CGI_ONLY_LABEL);
	printf("</td>");
	PUT_CONSOLE_TABLE_COMM(CONSOLE_PROCESS_COMMENT);

	/* Output start /stop SQL server button depending on connexion status */
	if(!cntxt->sql_session)
	{
		PUT_CONSOLE_TABLE_BUTTON("STARTSQL", CONSOLE_STARTSQL_TOOLTIP, "start_sqlserver");
		PUT_CONSOLE_TABLE_COMM(CONSOLE_STARTSQL_COMMENT);
	}
	else
	{
		PUT_CONSOLE_TABLE_BUTTON("STOPSQL", CONSOLE_STOPSQL_TOOLTIP, "stop_sqlserver");
		PUT_CONSOLE_TABLE_COMM(CONSOLE_STOPSQL_COMMENT);
	}

	/* Output 'restart' button */
	PUT_CONSOLE_TABLE_BUTTON("RESTART", CONSOLE_RESTART_TOOLTIP, "restartserver");
	PUT_CONSOLE_TABLE_COMM(CONSOLE_RESTART_COMMENT);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : get_cgi_data
** Description : read appropriate CGI values in csl
*********************************************************************/
#define ERR_FUNCTION "get_cgi_data"
#define ERR_CLEANUP DYNTAB_FREE(ret);
int get_cgi_data(				/* Return 0 on success, other in error */
	ConsoleData *csl			/* in: execution context data */
){
	EVA_context *cntxt = &csl->c;
	DynTable ret = {0};
	
	/* Retrieve applicable CGI data */
	if(cgi_get_values(cntxt, &ret, "D#DBSEL", 0)) STACK_ERROR(1);
	csl->b_allow_db_select = ret.nbrows ? !strcmp(dyntab_val(&ret, 0, 0), "1") : cntxt->argc == 1;
	if(cgi_get_values(cntxt, &ret, "D#SELDB", 0)) STACK_ERROR(1);
	csl->seldb = dyntab_sz(&ret, 0, 0) ? dyntab_val(&ret, 0, 0) : cntxt->argc > 1 ? cntxt->argv[1] : NULL;
	if(csl->seldb && !*csl->seldb) csl->seldb = NULL;
	if(cgi_get_values(cntxt, &ret, "D#RSTBK", 0)) STACK_ERROR(1);
	csl->b_restart_after_bkup = ret.nbrows ? !strcmp(dyntab_val(&ret, 0, 0), "1") : 1;
	if(cgi_get_values(cntxt, &ret, "D#CGIPR", 0)) STACK_ERROR(1);
	csl->b_cgi_proc_only = ret.nbrows ? !strcmp(dyntab_val(&ret, 0, 0), "1") : 1;


	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_cgi_data
** Description : output console status as CGI hidden input
*********************************************************************/
void put_cgi_data(ConsoleData *csl)
{
	EVA_context *cntxt = &csl->c;
	if(!csl->b_seldb_list && csl->seldb)
		printf("\n<input type=hidden name='D#SELDB' value=%s>", csl->seldb);
	printf("\n<input type=hidden name='D#DBSEL' value=%d>", csl->b_allow_db_select);
	printf("\n<input type=hidden name='D#RSTBK' value=%d>", csl->b_restart_after_bkup);
	printf("\n<input type=hidden name='D#CGIPR' value=%d>", csl->b_cgi_proc_only);
}

/*********************************************************************
** Function : put_login_page
** Description : output identification page
*********************************************************************/
void put_login_page(ConsoleData *csl)
{
	csl = NULL;
	puts(
		"<table cellspacing=0 cellpadding=0 border=0 width=100% rules=none>"
		"<tr><td><a href='http://www.evasoft.com' target='EVASoft'><img  border=0 src='../img/line-logo-evasoft-left.jpg'></a></td>"
		"<td align=center width=100%  background='../img/line-logo-evasoft-mid.jpg'>"
		"<font face='Arial' size=+1 ><b><i>"
		"Accès à la console d'administration du serveur"
		"</font></i></b></td><td>"
		"<a href='http://www.abing.fr' target='ABI'>"
		"<img  border=0 src='../img/line-logo-evasoft-right.jpg'></a>"
		"</td></tr></table>"
		"<table cellspacing=0 cellpadding=5 border=3 width=100% rules=none "
		"background='../img/fondgris.jpg'><tr>"
		"<td align='center' valign='middle' colspan=2><font face='Arial'><i>"
		"<i>Merci de vous identifier pour accéder à la console</font>"
		"</font></i></td></tr><tr><td align='right'>"
		"<font face='Arial'>Mot de passe</font></td><td>"
		"<input type=password name='D#PWD' size=10>"
		"</td></tr><tr><td align='center' colspan=2>"
		"<input type=submit name='B#MENU' value=\"Valider l'identification\" border='0'>"
		"</td></tr></table>"
		"<script>document.mainform['D#PWD'].focus();</script>"
		);
}

/*********************************************************************
** Function : encode_session
** Description : compute a key form 2 strings (typically user name & timestamp) to secure identification
*********************************************************************/
unsigned long encode_session(char *c)
{
	unsigned long res = 0, i = 0;
	if(!c) return 0;
	while(*c)
	{
		unsigned long j, p = *c;
		for(j = 0; j < i % 8; j++) p *= *c;
		res += p;
		c++; i++;
	}
	return res;
}

/*********************************************************************
** Function : check_password
** Description : read & check identification
*********************************************************************/
#define ERR_FUNCTION "check_password"
#define ERR_CLEANUP M_FREE(k); \
					DYNTAB_FREE(cgipasswd)
int check_password(				/* return : 0 on success, other on error */
	ConsoleData *csl			/* in : execution context data */
){
	EVA_context *cntxt = &csl->c;
	DynTable cgipasswd = { 0 };
	unsigned long i = 0, key = 0;
	DynBuffer *k = NULL;

	/* Read session data from CGI */
	if(get_cgi_data(csl)) STACK_ERROR(1);
	retrieve_cgi_databases(csl);

	/* Check previous session identification */
	if(cgi_get_values(cntxt, &cgipasswd, "D#USER", 0)) STACK_ERROR(1);
	if(cgipasswd.nbrows)
	{
		csl->user = dyntab_val(&cgipasswd, 0, 0);
		if(cgi_get_values(cntxt, &cgipasswd, "D#TIME", 0)) STACK_ERROR(1);
		DYNBUF_ADD_CELL(&k, &cgipasswd, 0, 0, NO_CONV);
		DYNBUF_ADD(&k, csl->user, 0, NO_CONV);
		if(cgi_get_values(cntxt, &cgipasswd, "D#PID", 0)) STACK_ERROR(1);
		if(!k || !sscanf(dyntab_val(&cgipasswd, 0, 0), "%lx", &key) ||
			key != encode_session(k->data))
			csl->user = NULL;
	}

	if(!csl->user || !*csl->user)
	{
		/* Read password from CGI data & search in configuration file (holds 1 password per line) */
		if(cgi_get_values(cntxt, &cgipasswd, "D#PWD", 0)) STACK_ERROR(1);
		for(i = 0; i < cntxt->id_cnf.nbrows &&
			dyntab_cmp(&cntxt->id_cnf, i, 0, &cgipasswd, 0, 0); i ++);
		csl->user = dyntab_val(&cntxt->id_cnf, i, 1);
		if(!*csl->user) csl->user = NULL;
	}

	if(csl->user)
	{
		/* Identification Ok : output next key & return */
		printf("<input type=hidden name='D#USER' value=\"%s\">", csl->user);
		printf("<input type=hidden name='D#TIME' value=%s>", cntxt->timestamp);
		M_FREE(k);
		DYNBUF_ADD(&k, cntxt->timestamp, 0, NO_CONV);
		DYNBUF_ADD(&k, csl->user, 0, NO_CONV);
		printf("<input type=hidden name='D#PID' value=%lx>", encode_session(k->data));
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP


/*********************************************************************
** Function : html_console
** Description : handle administration console HTML dialog
*********************************************************************/
#define ERR_FUNCTION "html_console"
#define ERR_CLEANUP
int html_console(				/* Return 0 on success, other in error */
	ConsoleData *csl			/* in: execution context data */
){
	EVA_context *cntxt = &csl->c;
	char *cgibtn = cntxt->cgi ? cntxt->cgi[cntxt->cgibtn].name + 2 : "";
	unsigned long i = 0, j = 0, b_menu = 0;


	if(check_password(csl)) STACK_ERROR(1);
	if(!csl->user)
	{
		put_login_page(csl);
		RETURN_OK;
	}


	/* Output page header */
	printf("<font face='Arial'><table cellspacing=0 cellpadding=0 border=0 width=100%% rules=none>"
		 "<tr><td align=center height=50><b><font size=+2>%s", csl->seldb ? csl->seldb : "Console globale");

	/* Check SQL server status (attempt to connect to mysql database) */
	cntxt->dbname = "mysql";
	if(!cntxt->sql_session) sql_open_session(cntxt);

	/* Parse user input */
	if(!strcmp(cgibtn, "OPTIMIZE"))
	{

		/* Switch dbname connexion if applicable */
		if(STRCMPNUL(cntxt->dbname, csl->seldb))
		{
			if(sql_control(cntxt, 0)) STACK_ERROR(1);
			cntxt->dbname = csl->seldb;
		}
		PUT_CONSOLE_HEAD(CONSOLE_OPTIMIZE_TITLE);
		optimize_table(csl, "TLink");
		printf("<br>");
		optimize_table(csl, "TVal");

		PUT_MENU_BUTTON;
	}
	else if(!strcmp(cgibtn, "RESTART"))
	{
		PUT_CONSOLE_HEAD(CONSOLE_RESTARTE_TITLE);
		restart_server(csl, 2);
		PUT_MENU_BUTTON;
	}
	else if(!strcmp(cgibtn, "RESTARTFORCE"))
	{
		PUT_CONSOLE_HEAD(CONSOLE_RESTARTFORCE_TITLE);
		restart_server(csl, 3);
		PUT_MENU_BUTTON;
	}
	else if(!strcmp(cgibtn, "STARTSQL"))
	{
		PUT_CONSOLE_HEAD(CONSOLE_STARTSQL_TITLE);
		exec_command("net start MySql", 3);
		PUT_MENU_BUTTON;
	}
	else if(!strcmp(cgibtn, "STOPSQL"))
	{
		PUT_CONSOLE_HEAD(CONSOLE_STOPSQL_TITLE);
		shutdown_sql(csl);
		PUT_MENU_BUTTON;
	}
	else if(!strcmp(cgibtn, "LOGS"))
	{
		PUT_CONSOLE_HEAD(CONSOLE_LOGS_TITLE);
		PUT_LOGS_BUTTON("LOGS", REFRESH_LOGS_TOOLTIP);
		if(output_logs(csl, 0))	puts("<br>" GRAPH_LOGS_ERROR);
		PUT_LOGS_BUTTON("LOGS", REFRESH_LOGS_TOOLTIP);
	}
	else if(!strcmp(cgibtn, "GRAPH"))
	{
		PUT_CONSOLE_HEAD(CONSOLE_GRAPH_TITLE);
		PUT_LOGS_BUTTON("GRAPH", REFRESH_GRAPH_TOOLTIP);
		if(output_logs(csl, 1))	puts("<br>" GRAPH_LOGS_ERROR);
		PUT_LOGS_BUTTON("GRAPH", REFRESH_GRAPH_TOOLTIP);
	}
	else if(!strcmp(cgibtn, "BACKUP"))
	{
		PUT_CONSOLE_HEAD(CONSOLE_BACKUP_TITLE);
		backup_server(csl);
		PUT_MENU_BUTTON;
	}
	else if(!strcmp(cgibtn, "LISTPROCESS"))
	{
		PUT_CONSOLE_HEAD(CONSOLE_PROCESS_TITLE);
		PUT_PROCESS_BUTTON;
		output_snapshot(csl);
		PUT_PROCESS_BUTTON;
	}
	else if(!strcmp(cgibtn, "CGIPR"))
	{
		csl->b_cgi_proc_only = !csl->b_cgi_proc_only;
		b_menu = 1;
	}
	else if(!strcmp(cgibtn, "RSTBK"))
	{
		csl->b_restart_after_bkup = !csl->b_restart_after_bkup;
		b_menu = 1;
	}
	else if(!strncmp(cgibtn, add_sz_str("KILLPROC")))
	{
		PUT_CONSOLE_HEAD(CONSOLE_KILL_PROC_TITLE);

		if(!kill_process(csl, cgibtn + sizeof("KILLPROC") - 1))
			puts("<br>" PROCESS_CLOSED "<br><br><br>");
		else
			puts(PROCESS_CLOSEERROR " %s<br><br><br>");

		PUT_PROCESS_BUTTON;
	}
/*	else if(!strncmp(cgibtn, add_sz_str("CALENDAR")) || !strncmp(cgibtn, add_sz_str("ENABLED")))
	{
		PUT_CONSOLE_HEAD(CONSOLE_GRAPH_TITLE);
		PUT_LOGS_BUTTON("CALENDAR", REFRESH_GRAPH_TOOLTIP);
		if(handle_calendar(csl, cgibtn)) puts("<br>" GRAPH_LOGS_ERROR);
		PUT_LOGS_BUTTON("CALENDAR", REFRESH_GRAPH_TOOLTIP);
	}*/
	else
		/* Default action : display main menu page */
		b_menu = 1;
	
	/* Display main menu page */
	if(b_menu)
	{
		PUT_CONSOLE_HEAD(CONSOLE_MAIN_TITLE);
		puts("<table cellspacing=0 cellpadding=4 border=1 width=100% rules=none>");

		/* Output console menu controls & buttons */
		if(csl->b_allow_db_select && output_database_list(csl) ||
			csl->seldb && put_db_specific_controls(csl) ||
			put_global_buttons(csl))
			STACK_ERROR(1);
		puts("</table></font>");

	}

	/* Output console status as CGI hidden input */
	put_cgi_data(csl);

	/* Prepare logs output */
	cntxt->dbname = "console";
	DYNTAB_ADD(&cntxt->session, 0, 0, csl->user, 0, NO_CONV)

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : main
** Description : executable entry point
*********************************************************************/
int main(int argc, char **argv, char **envp)
{
	/* Initialize context */
	ConsoleData c = {0};
	EVA_context *cntxt = &c.c;
	Cntxt = cntxt;

	cntxt->argc = argc;
	cntxt->argv = argv;
	cntxt->envp = envp;

	/* Extract DB name from executable path (basename) */
	cntxt->dbname = basename(cntxt->argv[0], 0);
	cntxt->path = mem_strdup(cntxt->argv[0]);
	cntxt->path[cntxt->dbname - cntxt->argv[0]] = 0;
	cntxt->dbname = mem_strdup(cntxt->dbname);
	cntxt->dbname[strlen(cntxt->dbname)-4] = 0;

	/* Output page header */
	put_html_page_header(cntxt, NULL, "EVA - console d'administration", NULL, 3);

	/* Read CGI data & process call */
	if(!cgi_init_call(cntxt)) html_console(&c);

	if(cntxt->err.text) printf("<p align=center><font face=Arial color=red>Erreur : %s</font></p>", cntxt->err.text);

#ifdef _DEBUG
	cntxt->debug = DEBUG_CGI;
	debug_put_cgi(cntxt);
#endif

	put_html_page_trailer(cntxt, NULL);

	/* Output console logs */
	output_log_end(cntxt);

	return 0;
}

