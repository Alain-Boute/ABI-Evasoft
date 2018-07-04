/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : cgi_utils.c
** Description : handling of cgi input format
**      Author : Alain BOUTE
**     Created : Feb 15 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : cgi_free_files
** Description : free allocated memory for uploaded files
*********************************************************************/
void cgi_free_files(EVA_context *cntxt)
{
	if(cntxt && cntxt->cgi_file) 
	{
		unsigned long i;
		for(i = 0; i < cntxt->cgi_file_nb; i++)
			dyntab_free(&cntxt->cgi_file[i].descr);
		M_FREE(cntxt->cgi_file);
	}
}

/*********************************************************************
** Function : cgi_init_call
** Description : read args & configuration for cgi calls
*********************************************************************/
#define ERR_FUNCTION "cgi_init_call"
#define ERR_CLEANUP	DYNTAB_FREE(urlswitch); \
					DYNTAB_FREE(pgconfig)
int cgi_init_call(				/* return : 0 on success, other on error */
	EVA_context *cntxt
){
	char tmp[2048];
	struct stat st;
	FILE *f = NULL;
	DynTable urlswitch = { 0 };
	DynTable pgconfig = { 0 };
	unsigned long i = 0;

	if(cntxt->user_ip && !strcmp(cntxt->user_ip, "127.0.0.1")) cntxt->debug |= DEBUG_CGI_RAW;

	/* Initialize memory trace file */
	MEM_TRACE(NULL, NULL);

	/* Output log start */
	if(!cntxt->b_task)
	{
		time_to_datetxt(cntxt->tm0, tmp);
		datetxt_to_format(cntxt, cntxt->logfile, tmp, "_EVA_DATE_SORT");
		sprintf(cntxt->logfile + strlen(cntxt->logfile), ".txt");
		chdir(cntxt->rootdir);
		if(chdir("logs")) { mkdir("logs"); chdir("logs"); }
		if(stat(cntxt->logfile, &st))
		{
			/* Create new log file every day */
			f = fopen(cntxt->logfile, "w");
			fprintf(f, "time\tpid\tclock\tcgi\tClickBtn\tClickForm\tClickSubField\tSession\tMainForm\tMainObj\tAltForm\tAltObj\tSrchTxt\n");

			/* Send mail report */
			snprintf(tmp, sizeof(tmp), "%s" DD "BLAT.EXE - -body \"http://%s%s\" -s \"Notification EVA CGI %s\" -noh2 -to eva@abing.fr",
							cntxt->path, cntxt->srvname, cntxt->requri, cntxt->srvname);
			system(tmp);
		}
		else
			f = fopen(cntxt->logfile, "a");
		fprintf(f, "%lu\t%u\t%lu\t%s\n", time(NULL), getpid(), clock(), cntxt->dbname);
		fclose(f);
	}

	/* Read DB config */
	snprintf(add_sz_str(tmp), "SELECT DISTINCT IdObj FROM TLink WHERE IdField=%lu AND DateDel IS NULL", IDVAL("_EVA_ADMINISTRATOR"));
	if(sql_exec_query(cntxt, tmp) || sql_get_table(cntxt, &cntxt->id_cnf, 0)) STACK_ERROR;
	if(qry_obj_data(cntxt, &cntxt->cnf_data, &cntxt->id_cnf, NULL)) STACK_ERROR;
	if(cntxt->id_cnf.nbrows != 1 || !cntxt->cnf_data.nbrows) RETURN_ERROR("Configuration du serveur incorrecte", NULL);

	/* Read server config file */
	if(file_read_config(cntxt)) STACK_ERROR;

	/* Read date labels */
	dyntab_from_list(&cntxt->daylong, add_sz_str("dimanche,lundi,mardi,mercredi,jeudi,vendredi,samedi"), ",", 0, 0);
	dyntab_from_list(&cntxt->dayshort, add_sz_str("dim,lun,mar,mer,jeu,ven,sam"), ",", 0, 0);
	dyntab_from_list(&cntxt->monthlong, add_sz_str("Janvier,Février,Mars,Avril,Mai,Juin,Juillet,Août,Septembre,Octobre,Novembre,Décembre"), ",", 0, 0);
	dyntab_from_list(&cntxt->monthshort, add_sz_str("Jan,Fev,Mar,Avr,Mai,Jun,Jul,Aou,Sep,Oct,Nov,Dec"), ",", 0, 0);

	/* Read public user account */
	cntxt->id_public = strtoul(DYNTAB_FIELD_VAL(&cntxt->cnf_data, UNKNOWNUSER), NULL, 10);

	/* Read value constants */
	if(sql_id_value(cntxt, add_sz_str("_EVA_FORMSTAMP"), &cntxt->val_FORMSTAMP)) STACK_ERROR;
	if(sql_id_value(cntxt, add_sz_str("_EVA_TYPE"), &cntxt->val_TYPE)) STACK_ERROR;
	if(sql_id_value(cntxt, add_sz_str("_EVA_FIELD"), &cntxt->val_FIELD)) STACK_ERROR;

	/* Handle URL dependent settings : menubar, homepage, page title */
	DYNTAB_FIELD_TAB(&urlswitch, &cntxt->cnf_data, URL_SWITCH);
	if(cntxt->srvname)
	{
		DynTableCell *val = NULL;
		for(i = 0; i < urlswitch.nbrows; i++)
		{
			val = dyntab_cell(&urlswitch, i, 0);
			if(!(val && val->txt && *val->txt) || strstr(cntxt->srvname, val->txt))
			{
				DYNTAB_FIELDLINE(&pgconfig, &cntxt->cnf_data, URL_PAGE_CONFIG, i);
				break;
			}
		}
	}
	if(urlswitch.nbrows && cntxt->srvname)
	{
		i++;
		DYNTAB_FIELDLINE(&cntxt->menubar, &cntxt->cnf_data, MENUBAR, i);
		DYNTAB_FIELDLINE(&cntxt->homepage, &cntxt->cnf_data, HOMEPAGE, i);
		DYNTAB_FIELDLINE(&cntxt->pagetitle, &cntxt->cnf_data, PAGE_TITLE, i);
		DYNTAB_FIELDLINE(&cntxt->pagebottom, &cntxt->cnf_data, PAGE_BOTTOM, i);
		DYNTAB_FIELDLINE(&cntxt->pagebtctrl, &cntxt->cnf_data, PAGE_FOOTER, i);
	}
	else
	{
		DYNTAB_FIELD(&cntxt->menubar, &cntxt->cnf_data, MENUBAR);
		DYNTAB_FIELD(&cntxt->homepage, &cntxt->cnf_data, HOMEPAGE);
		DYNTAB_FIELD(&cntxt->pagetitle, &cntxt->cnf_data, PAGE_TITLE);
		DYNTAB_FIELD(&cntxt->pagebottom, &cntxt->cnf_data, PAGE_BOTTOM);
		DYNTAB_FIELD(&cntxt->pagebtctrl, &cntxt->cnf_data, PAGE_FOOTER);
	}

	/* Read	CGI input data */
	if(cgi_read_data(cntxt)) STACK_ERROR;

	/* Disable / enable javascript if applicable */
	{
		char *msie = cntxt->user_agent ? strstr(cntxt->user_agent, " MSIE ") : NULL;
		int vers = msie ? atoi(msie + 6) : -1;
		cntxt->b_pda = cntxt->user_agent && strstr(cntxt->user_agent, " IEMobile ") != 0;
		if(vers > 0 && vers < 5 || cntxt->b_pda)
		{
			cntxt->jsenabled = 0;
			cntxt->imgwait = 0;
		}
		else if(!cntxt->cgi) cntxt->jsenabled = 1;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : output_log_end
** Description : output termination line in cgi log file
*********************************************************************/
void output_log_end(EVA_context *cntxt)
{
	FILE *f;

	chdir(cntxt->rootdir);
	chdir("logs");
	f = fopen(cntxt->logfile, "a");
	fprintf(f, "%lu\t%u\t%lu\t%s\t%lu\t%lu\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
					time(NULL), getpid(), clock(), cntxt->dbname,
					cntxt->log_clkbtn,										/* clicked button id */
					cntxt->log_clkform,										/* clicked button form id */
					(cntxt->cgi && cntxt->cgi[cntxt->cgibtn].subfield) ? 
						cntxt->cgi[cntxt->cgibtn].subfield : ""				/* clicked button subfield */
					,dyntab_val(&cntxt->session, 0, 0)						/* session id */
					,dyntab_val(&cntxt->id_form, 0, 0)						/* current form id */
					,dyntab_val(&cntxt->id_obj, 0, 0)						/* current object id */
					,dyntab_val(&cntxt->alt_form, 0, 0)						/* alternate form id */
					,dyntab_val(&cntxt->alt_obj, 0, 0)						/* current object id in alternate form */
					,cntxt->log_srchtxt ? cntxt->log_srchtxt->data : ""		/* search string */
					);
	fclose(f);
}

/*********************************************************************
** Fonction : basename
** Description : return the base name of a path
*********************************************************************/
char *basename(char *path, size_t path_len)
{
#define DIR_DELIM "\\/:"
	char *name;
	if(!path || !*path) return "";
	if(!path_len) path_len = strlen(path);
	name = path + path_len - 1;
	while(!strchr(DIR_DELIM, *name) && name > path) name--;
	if(strchr(DIR_DELIM, *name)) name++;
	return name;
}
#undef DIR_DELIM

/*********************************************************************
** Fonction : cgi_free
** Description : free CGI data
*********************************************************************/
void cgi_free(EVA_context *cntxt)
{
	unsigned long i;
	for(i = 0; i < cntxt->nb_cgi; i++)
	{
		CGIData *cgi = cntxt->cgi + i;
		M_FREE(cgi->name);
		M_FREE(cgi->value);
		M_FREE(cgi->field);
		M_FREE(cgi->subfield);
	}
	M_FREE(cntxt->cgi);
	cntxt->nb_cgi = 0;
}

/*********************************************************************
** Function : cgi_extract
** Description : extract tokens and put theim in a table
*********************************************************************/
#define ERR_FUNCTION "cgi_extract"
#define ERR_CLEANUP	
int cgi_extract(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	char *val,						/* in : text containing tokens */
	char sep,						/* in : tokens separator */
	DynTable *res, ...				/* out : list of DynTable* pointers to receive values in cell 0,0 
											MUST BE NULL TERMINATED if # of tokens may exceed # of tables */
){
	DynTable *t = res;
	char *nextval = val; 
	size_t sz;
	va_list	p;

	/* Args loop */
	t = res;
	va_start(p, res);
	while(t && nextval)
	{
		nextval = strchr(val, sep);
		sz = nextval ? nextval - val : strlen(val);
		dyntab_free(t);
		if(sz) DYNTAB_ADD(t, 0, 0, val, sz, NO_CONV); 
		val = nextval+1;
		t = va_arg(p, DynTable*);
	}
	va_end(p);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_build_name

	CGI names have the following format :
			
			  <type><ctrl>/<form>$<idobj>#<field>.<subfield>$<valnum> 

	<type>     : the first char of name describes the type of input
				D = user input
				O = original values
				M = user modified values
				T = tab button
				S = session data
				F = form status data
				B = button
	<ctrl>	   : control object id that generated the input
	<form>	   : form object id that generated the input
	<idobj>    : object id edited in form
	<field>    : input field name - DB storage field name
	<subfield> : for additional control inputs (such as table search string or age for date input)
	<valnum>   : value index number (for tables & multiple values or langages) in format Num.Line.Lang

** Description : build a CGI input name
*********************************************************************/
#define ERR_FUNCTION "cgi_build_name"
#define ERR_CLEANUP
int cgi_build_name(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **name,				/* out : built cgi name */
	char type,						/* in : input type is first name char - see file header */
	unsigned long id_ctrl,			/* in : control id */
	unsigned long id_form,			/* in : form id */
	unsigned long id_obj,			/* in : object id */
	char *field,					/* in : field name */
	size_t field_sz,				/* in : field name string size */
	char *subfield,					/* in : subfield name */
	size_t subfield_sz,				/* in : subfield name string size */
	unsigned long num,				/* in : value index to handle multiple values */
	unsigned long row,				/* in : value index to handle table rows */
	unsigned long lvl				/* in : value index to handle list options level */
){
	/* Check params */
	if(!name || !id_form) RETURN_ERROR("Erreur du programme", NULL);
	if(*name) { (*name)->cnt = 0; memset((*name)->data, 0, (*name)->sz); }

	/* Add input type */
	DYNBUF_ADD(name, &type, 1, NO_CONV);

	/* Add control id / form id */
	if(id_ctrl) DYNBUF_ADD3_INT(name, "", id_ctrl, "/");
	DYNBUF_ADD_INT(name, id_form);

	/* Add "$" & id_obj */
	DYNBUF_ADD_STR(name, "$");
	if(id_obj) DYNBUF_ADD_INT(name, id_obj);

	/* Add "#" & field */
	DYNBUF_ADD_STR(name, "#");
	DYNBUF_ADD(name, field, field_sz, NO_CONV);

	/* Build final name */
	if(cgi_build_finalname(cntxt, name, 0, subfield, subfield_sz, num, row, lvl)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_build_basename
** Description : build the CGI input name of a control with value indexes
*********************************************************************/
#define ERR_FUNCTION "cgi_build_basename"
#define ERR_CLEANUP
int cgi_build_basename(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **name,				/* out : built cgi name */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char type						/* in : input type is first name char - see file header */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	
	/* Check params */
	if(!name) RETURN_ERROR("Erreur du programme", NULL);
	if(*name) { (*name)->cnt = 0; memset((*name)->data, 0, (*name)->sz); }

	/* Add input type */
	DYNBUF_ADD(name, &type, 1, NO_CONV);

	/* Add control id / form id */
	if(i_ctrl && form->ctrl) DYNBUF_ADD3_CELL(name, "", &ctrl->id, 0, 0, NO_CONV, "/");
	DYNBUF_ADD_CELL(name, &form->id_form, 0, 0, NO_CONV);

	/* Add "$" & id_obj */
	DYNBUF_ADD3_CELL(name, "$", &form->id_obj, 0, 0, NO_CONV, "");

	/* Add "#" & field */
	DYNBUF_ADD_STR(name, "#");
	DYNBUF_ADD(name, ctrl->FIELD, strlen(ctrl->FIELD), NO_CONV)

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_build_finalname
** Description : build the complete CGI input name of a control
*********************************************************************/
#define ERR_FUNCTION "cgi_build_finalname"
#define ERR_CLEANUP
int cgi_build_finalname(			/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **name,				/* in : CGI base name
									  out : final CGI name with value indexes */
	char type,						/* in : input type - control type is usedif NULL
											see cgi_build_name for details */
	char *subfield,					/* in : subfield name */
	size_t subfield_sz,				/* in : subfield name string size */
	unsigned long num,				/* in : value index to handle multiple values */
	unsigned long row,				/* in : value index to handle table rows */
	unsigned long lang				/* in : value index to handle langage */
){
	if(!name || !*name) RETURN_OK;

	/* Copy control base name & set type */
	if(type) (*name)->data[0] = type;

	/* Add .subfield if needed */
	if(subfield && *subfield)
		DYNBUF_ADD3(name, ".", subfield, subfield_sz, NO_CONV, "");
	
	/* Add "$" and value indexes */
	DYNBUF_ADD_STR(name, "$");
	if(num || row || lang) DYNBUF_ADD_INT(name, num ? num : 1);
	if(row || lang) DYNBUF_ADD3_INT(name, ".", row, "");
	if(lang) DYNBUF_ADD3_INT(name, ".", lang, "");
	(*name)->data[(*name)->cnt] = 0;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_parse_name
** Description : parse name of a cgi input in cntxt->cgi
*********************************************************************/
#define ERR_FUNCTION "cgi_parse_name"
#define ERR_CLEANUP	M_FREE(name); M_FREE(name1)
int cgi_parse_name(			/* return : 0 on success, other on error */
	EVA_context *cntxt,		/* in : context data */
	CGIData *cgi			/* in/out : CGI input to process */
){
	char *name = NULL, *name1 = NULL;
	char *idobj, *field, *index, *t;
	size_t len;

	if(!cgi->name || !cgi->name_sz) RETURN_OK;

	/* Remove .x & .y extensions from image clicks */
	if(strchr("IBT", cgi->name[0]) && cgi->name_sz > 2 &&
		(!strcmp(cgi->name + cgi->name_sz - 2, ".x") ||
		 !strcmp(cgi->name + cgi->name_sz - 2, ".y")))
	{
		cgi->name_sz -= 2;
		cgi->name[cgi->name_sz] = 0;
	}

	/* Split name parts */
	M_STRDUP(name, cgi->name, cgi->name_sz);
	idobj = name + 1; while(*idobj && *idobj != '$') idobj++;
	if(*idobj)
	{
		*idobj = 0;
		field = ++idobj;
		index = field;
		while(*index && *index != '$') index++;
		if(*index)
		{
			*index = 0;
			index++;

			/* Store indexes */
			cgi->Num = strtoul(index, NULL, 10);
			t = strchr(index, '.');
			if(t) 
			{
				char *t1 = strchr(t + 1, '.');
				cgi->Line = strtoul(t + 1, NULL, 10);
				if(t1) cgi->Lang = strtoul(t1 + 1, NULL, 10);
			}
		}
		while(*field && *field != '#') field++;
		if(*field)
		{
			*field = 0;
			++field;

			/* Store field & subfield */
			t = strchr(field, '.');
			if(t)
			{
				M_STRDUP(cgi->subfield, t + 1, strlen(t));
				M_STRDUP(cgi->field, field, t - field);
			}
			else
				M_STRDUP(cgi->field, field, strlen(field));
		}
		/* Store object id */
		if(*idobj) cgi->IdObj = strtoul(idobj, NULL, 10);
	}

	/* Store form id  & control id & path */
	len = strlen(name);
	t = name + len;
	while(t > name && isdigit(*(t-1))) t--;
	if(t > name) cgi->IdForm = strtoul(t, NULL, 10);
	cgi->IdCtrl = strtoul(name + 1, NULL, 10);

	/* Parse CGI input belonging to cntxt->cgi
		Parsing retrieves various data necessary in first steps of processing, 
		such as session information, encoding type for next form, clicked button ... */
	if(cgi >= cntxt->cgi && cgi <= cntxt->cgi + cntxt->nb_cgi - 1)
	{
		unsigned long i = cgi - cntxt->cgi, j;

		/* Handle encoding type : multipart if previous form sent an input with TRANSMIT_FILE subfield */
		if(!STRCMPNUL(cgi->subfield, "TRANSMIT_FILE"))
			cntxt->cgiencodemultipart = 1;

		/* Handle JavaScript : detect if enabled */
		if(!strcmp(cgi->name, "JSINPUT"))
		{
			cntxt->jsenabled = 1;
			cntxt->imgwait = 1;

			/* If input holds a button click : set name as button name */
			if(cgi->value && strchr("IBT", cgi->value[0]))
			{
				M_FREE(cgi->name);
				cgi->name = cgi->value;
				cgi->name_sz = cgi->value_sz;
				cgi->value = NULL;
				cgi->value_sz = 0;
				if(cgi_parse_name(cntxt, cgi)) STACK_ERROR;
			}
			RETURN_OK;
		}

		switch(cgi->name[0])
		{
		case 'T':	/* TAB button click */
		case 'B':	/* BUTTON control click */
		case 'I':	/* control info button click */
			cntxt->cgibtn = i;
			break;
		
		case 'S':	/* Session data */
			if(cgi_extract(cntxt, cgi->value, '$',
							&cntxt->session, 
							&cntxt->id_form, &cntxt->id_obj, 
							&cntxt->alt_form, &cntxt->alt_obj, NULL)) STACK_ERROR; 
			break;
		case 'O':	/* Original input (to check for user changes) */
			/* Look for corresponding data input & add empty input if not found */
			j = i > 1 ? i - 1 : 0;
#define ISNOTCURVAL (cntxt->cgi[j].name[0] != 'D' || strcmp(cgi->name + 1, cntxt->cgi[j].name + 1))
			if(ISNOTCURVAL) for(j = 0; j < cntxt->nb_cgi && (j == i || ISNOTCURVAL); j++);
#undef ISNOTCURVAL
			if(j == cntxt->nb_cgi)
			{
				/* Add empty input in cntxt->cgi */
				M_STRDUP(name1, cgi->name, cgi->name_sz);
				name1[0] = 'D';
				if(cgi_add_input(cntxt, &j, name1, cgi->name_sz, NULL, 0)) STACK_ERROR;
				cgi = cntxt->cgi + i;
			}
			cntxt->cgi[j].i_old = i;

			/* If value changed  */
			if(STRCMPNUL(cgi->value, cntxt->cgi[j].value))
				/* Mark old value for no reoutput */
				cntxt->cgi[i].b_dontkeep = 1;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_to_cell
** Description : return a static cell with given cgi data
*********************************************************************/
DynTableCell *cgi_to_cell(		/* return : cell with given cgi data */
	CGIData *cgi,				/* in : cgi data to transfer */
	unsigned long i				/* in : index of cgi in cntxt->cgi */
){
	static DynTableCell cell;
	memset(&cell, 0, sizeof(cell));
	cell.txt = cgi->value;
	cell.len = cgi->value_sz;
	cell.b_dontfreetxt = 1;
	cell.field = cgi->field;
	cell.b_dontfreefield = 1;
	cell.IdObj = cgi->IdObj;
	cell.Num = cgi->Num;
	cell.Line = cgi->Line;
	cell.Lang = cgi->Lang;
	cell.i_cgi = i;
	return &cell;
}

/*********************************************************************
** Function : cgi_get_values
** Description : return	cgi inputs matching the given name
*********************************************************************/
#define ERR_FUNCTION "cgi_get_values"
#define ERR_CLEANUP 
int cgi_get_values(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *ret,					/* out : CGI values table */
	char *name,						/* in : name to search for in cgi data */
	int mode						/* in : search mode :
											0 = exact search
											1 = type+form+obj+field match
											2 = type+obj+field match
											3 = type+field match
											4 = type+ctrl+form+obj+field+subfield match */
)
{
	CGIData cgi = {0};

	/* Initialize search */
	if(!cntxt->nb_cgi || !ret || !name || !*name) RETURN_OK;

	/* If exact match mode : loop on cgi data */
	if(mode == 0)
	{
		unsigned long i;
		dyntab_free(ret);
		for(i = 0; i < cntxt->nb_cgi; i++)
			if(!strcmp(cntxt->cgi[i].name, name) &&
				dyntab_set(ret, ret->nbrows, 0, cgi_to_cell(cntxt->cgi + i, i))) 
				RETURN_ERR_MEMORY;
	}

	/* Parse given CGI name & search depending on mode */
	cgi.name = name;
	cgi.name_sz = strlen(name);
	if(cgi_parse_name(cntxt, &cgi) ||
		mode == 4 ? cgi_filter_values(cntxt, ret, cgi.name[0], cgi.IdCtrl, cgi.IdForm, cgi.IdObj, cgi.field, cgi.subfield, 0, 0) :
		mode == 1 ? cgi_filter_values(cntxt, ret, cgi.name[0], ~0UL, cgi.IdForm, cgi.IdObj, cgi.field, NULL, 0, 0) :
		mode == 2 ? cgi_filter_values(cntxt, ret, cgi.name[0], ~0UL, ~0UL, cgi.IdObj, cgi.field, NULL, 0, 0) :
		mode == 3 ? cgi_filter_values(cntxt, ret, cgi.name[0], ~0UL, ~0UL, ~0UL, cgi.field, NULL, 0, 0) : 0
		) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_filter_values
** Description : return	cgi inputs matching the given name
*********************************************************************/
#define ERR_FUNCTION "cgi_filter_values"
#define ERR_CLEANUP 
int cgi_filter_values(				/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *ret,					/* out : matching CGI inputs table */
	char type,						/* in : 1st letter of name to search for - do not use if 0 */
	unsigned long IdCtrl,			/* in : control to search for - do not use if ~0UL */
	unsigned long IdForm,			/* in : form to search for - do not use if ~0UL */
	unsigned long IdObj,			/* in : object to search for - do not use if ~0UL */
	char *field,					/* in : field to search for - do not use if NULL */
	char *subfield,					/* in : subfield to search for - do not use if NULL */
	unsigned long Num,				/* in : number index to search for - do not use if 0 */
	unsigned long Line				/* in : line index to search for - do not use if 0 */
){
	unsigned long i;

	/* Initialize search */
	if(!ret) RETURN_OK;
	dyntab_free(ret);

	/* Search in cgi values */
	for(i = 0; i < cntxt->nb_cgi; i++)
	{
		CGIData *cgi = cntxt->cgi + i;
		if(	cgi->name[0] &&
			(!type || type == cgi->name[0]) && 
			(IdCtrl == ~0UL || IdCtrl == cgi->IdCtrl) &&
			(IdForm == ~0UL || IdForm == cgi->IdForm) &&
			(IdObj == ~0UL || IdObj == cgi->IdObj) &&
			(!field || !STRCMPNUL(field, cgi->field)) &&
			(!subfield || !STRCMPNUL(subfield, cgi->subfield)) &&
			(!Num || Num == cgi->Num) &&
			(!Line || Line == cgi->Line)
			)
		{
			/* Store value, value number, object id, form id, CGI index */
			if(dyntab_set(ret, ret->nbrows, 0, cgi_to_cell(cgi, i))) RETURN_ERR_MEMORY;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_check_form_change
** Description : check for modified values in given form/object pair CGI data
*********************************************************************/
int cgi_check_form_change(			/* return : 0 = unchanged 
												1 = modified 
												2 = menubar or  home page */		
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long idform,			/* in : form id to search for */
	unsigned long idobj				/* in : object id to search for */
){
	unsigned long i;

	if(idform == DYNTAB_TOUL(&cntxt->menubar) || idform == DYNTAB_TOUL(&cntxt->homepage)) 
		return 2;

	/* Search form & return its modified status */
	for(i = 0; i < cntxt->nb_cgi; i++) 
	{
		CGIData *cgi = cntxt->cgi + i;
		if(cgi->name[0] == 'F' && idobj == cgi->IdObj && idform == cgi->IdForm)
			return cgi->value[cgi->value_sz - 1] == '1';
	}
	
	return 0;
}

/*********************************************************************
** Function : cgi_clear_form_inputs
** Description : clear lines of cntxt->cgi for given form & obj
*********************************************************************/
void cgi_clear_form_inputs(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long idform,			/* in : form id to search for */
	unsigned long idobj,			/* in : object id to search for */
	int b_keepstatus				/* in : keep status info if not 0 */
)
{
	unsigned long i;

	/* Search in cgi values */
	for(i = 0; i < cntxt->nb_cgi; i++)
	{
		CGIData *cgi = cntxt->cgi + i;
		if(idobj == cgi->IdObj && idform  == cgi->IdForm)
		{
			if(!b_keepstatus || strchr("DO", cgi->name[0]) && !cgi->subfield && strcmp(cgi->field, "_EVA_SELTAB"))
			{
				cgi->name[0] = 0;
				cgi->name_sz = 0;
			}
			cgi->b_dontkeep = 1;
		}
	}
}

/*********************************************************************
** Function : cginame_setkeep
** Description : mark lines of cntxt->cgi associated with given name
*********************************************************************/
void cginame_setkeep(
	EVA_context *cntxt,				/* in : execution context data */
	char *name,						/* in : input name to search for in cntxt->cgi */
	int b_dontkeep					/* in : how to mark
										0 = keep value
										1 = dont keep value
										2 = clear name */
){
	unsigned long i;
	if(!name || !*name) return;
	for(i = 0; i < cntxt->nb_cgi; i++)
		if(!strcmp(cntxt->cgi[i].name, name))
			cgi_setkeep(cntxt, i, b_dontkeep);
}

/*********************************************************************
** Function : cgi_setkeep
** Description : mark lines of cntxt->cgi associated with given input
*********************************************************************/
void cgi_setkeep(
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long i,				/* in : input index in cntxt->cgi */
	int b_dontkeep					/* in : how to mark
										0 = keep value
										1 = dont keep value
										2 = clear name */
){
	CGIData *cgi = cntxt->cgi + i;
	if(b_dontkeep < 2) cgi->b_dontkeep = b_dontkeep;
	else
	{
		cgi->name[0] = 0;
		cgi->name_sz = 0;

	}
	if(b_dontkeep && cgi->i_old) 
	{
		if(b_dontkeep < 2) cntxt->cgi[cgi->i_old].b_dontkeep = b_dontkeep;
		else 
		{
			cntxt->cgi[cgi->i_old].name[0] = 0;
			cntxt->cgi[cgi->i_old].name_sz = 0;
		}
	}
}

/*********************************************************************
** Function : cgi_value_setkeep
** Description : mark lines of cntxt->cgi associated with given value
*********************************************************************/
void cgi_value_setkeep(
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *cgival,				/* in : values from cgi_filter_values() */
	unsigned long i,				/* in : value index to process in cgival */
	int b_dontkeep					/* in : how to mark
										0 = keep value
										1 = dont keep value
										2 = clear name */
){
	unsigned long j;
	for(j = 0; j < cgival->nbcols; j++)
	{
		DynTableCell *val = dyntab_cell(cgival, i, j); 
		if(val && val->i_cgi && val->i_cgi < cntxt->nb_cgi) 
			cgi_setkeep(cntxt, val->i_cgi, b_dontkeep);
	}
}

/*********************************************************************
** Function : cgi_put_hiddenold
** Description : ouput hidden old input for a control
*********************************************************************/
#define ERR_FUNCTION "cgi_put_hiddenold"
#define ERR_CLEANUP 
int cgi_put_hiddenold(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer *name,				/* in : CGI name for data input */
	char *txt, size_t len			/* in original value */
){
	DYNBUF_ADD3(cntxt->form->html, "<input type=hidden name='O", name->data + 1, name->cnt - 1, NO_CONV, "' ");
	DYNBUF_ADD3(cntxt->form->html, "value='", txt, len, HTML_NO_QUOTE, "'>");
	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(cntxt->form->html, "\n");
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_put_hidden
** Description : ouput hidden input for CGI values not marked
*********************************************************************/
#define ERR_FUNCTION "cgi_put_hidden"
#define ERR_CLEANUP 
int cgi_put_hidden(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt				/* in : execution context data */
){
	unsigned long i;

	/* For each CGI input */
	for(i = 0; i < cntxt->nb_cgi; i++) 
	{
		CGIData *cgi = cntxt->cgi + i;

		/* Reouput CGI input if applicable */
		if(!cgi->b_dontkeep && cgi->name[0] && strchr("DFO", cgi->name[0]))
		{
			DYNBUF_ADD3_CGI(&cntxt->htmlhidden, "<input type=hidden name='", i, name, NO_CONV, "'");
			DYNBUF_ADD3_CGI(&cntxt->htmlhidden, " value='", i, value, HTML_NO_QUOTE, "'>");
			if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(&cntxt->htmlhidden, "\n");
		}
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_add_inputold
** Description : add an input / value pair to cntxt->cgi with corresponding old value
*********************************************************************/
#define ERR_FUNCTION "cgi_add_inputold"
#define ERR_CLEANUP	
int cgi_add_inputold(				/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long *i_cgi,			/* out : index of added value in cntxt->cgi */
	char *name, size_t name_sz,		/* in : name data */
	char *value, size_t value_sz	/* in : value data */
){
	if(cgi_add_input(cntxt, i_cgi, name, name_sz, value, value_sz)) STACK_ERROR;
	if(*name == 'D')
	{
		char c = *name;
		unsigned long i;
		*name = 'O';
		if(cgi_add_input(cntxt, &i, name, name_sz, value, value_sz)) STACK_ERROR;
		*name = c;
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_add_input
** Description : add an input / value pair to cntxt->cgi
*********************************************************************/
#define ERR_FUNCTION "cgi_add_input"
#define ERR_CLEANUP	M_FREE(buf)
int cgi_add_input(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	unsigned long *i_cgi,			/* out : index of added value in cntxt->cgi */
	char *name, size_t name_sz,		/* in : name data */
	char *value, size_t value_sz	/* in : value data */
){
	unsigned long i;
	CGIData *cgi;
	DynBuffer *buf = NULL;

	/* Search same name & value in CGI input list */
	for(i = 0; i < cntxt->nb_cgi; i++)
	{
		CGIData *cgi = cntxt->cgi + i;

		if(	name_sz == cgi->name_sz && 
			value_sz == cgi->value_sz && 
			!strncmp(name, cgi->name, name_sz) && 
			(!value_sz || !strncmp(value ? value : "", cgi->value ? cgi->value : "", value_sz)))
		{
			/* If same name & value found : do not add */
			if(i_cgi) *i_cgi = i;
			cgi->b_dontkeep = 0;
			RETURN_OK;
		}
	}
	
	/* Add cgi input in cntxt->cgi */
	M_REALLOC(CGIData, cntxt->cgi, cntxt->nb_cgi);
	cgi = cntxt->cgi + (cntxt->nb_cgi - 1);

	/* Copy name */
	M_STRDUP(cgi->name, name, name_sz);
	cgi->name_sz = name_sz;

	/* Copy value */
	if(value_sz && value && *value)
	{
		DYNBUF_ADD(&buf, value, value_sz, CRLF_TO_LF);
		M_STRDUP(cgi->value, buf->data, buf->cnt);
		cgi->value_sz = buf->cnt;
	}

	/* Parse name */
	if(cgi_parse_name(cntxt, cgi)) STACK_ERROR;
	if(i_cgi) *i_cgi = i;
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_read_urlencoded
** Description : read CGI data in www-form-urlencoded format
*********************************************************************/
#define ERR_FUNCTION "cgi_read_urlencoded"
#define ERR_CLEANUP M_FREE(parsed)
int cgi_read_urlencoded(	/* return : 0 on success, other on error */
	EVA_context *cntxt		/* in : 
								cntxt->input : CGI input to process
							   out : 
								cntxt->cgi : table of read & parsed CGI inputs */
){
	char c[2], prevdelim = '&';
	char *name = "";
	int bname;
	size_t i, len, sz, name_sz = 0;
	DynBuffer *parsed = NULL;

	/* Loop to copy & delimit (null terminate) names & values changing %XX entities */
	c[1] = 0;
	if(cntxt->input) for(i = 0; i <= cntxt->input->cnt; i++)
	{
		*c = cntxt->input->data[i];
		switch(*c) {

		/* + stands for space in CGI */
		case '+':
			*c = ' ';
			break;

		/* CGI character codes (%XX) conversion */
		case '%':
			if(memcmp("%0D%0A", cntxt->input->data + i, 6))
			{
				/* Convert %XX to ascii value */
				*c = (char)(TO_HEX(cntxt->input->data[i+1])*16 + TO_HEX(cntxt->input->data[i+2]));
				i += 2;
			}
			else
			{
				/* Change \r\n to \n */
				*c = '\n';
				i += 5;
			}
			break;

		/* CGI delimiters : check for correct alternance */
		case '=':
		case '&':
			if(!((prevdelim == '&' && *c == '=') || (prevdelim == '=' && *c == '&')))
				RETURN_ERROR("Syntax error in CGI data - server is probably misconfigured", NULL);
			prevdelim = *c;
			*c = 0;
			break;
		}
		DYNBUF_ADD(&parsed, c, 1, NO_CONV);
	}

	/* Add tags value and name to cntxt->cgi table */
	if(parsed) for(i = 0, bname = 1; i < parsed->cnt; i += len + 1, bname = !bname)
	{
		/* Calc value length - return error on null name */
		len = strlen(parsed->data + i);
		if(bname) 
		{
			if(!len) RETURN_ERROR("Null name in CGI data", NULL);
			name = parsed->data + i;
			name_sz = len;
		}
		else 
		{
			/* Strip spaces around the value */
			char *val = parsed->data + i;
			sz = len;
			STRIP_SPACES_AROUND(val, sz);

			/* Add input in cntxt->cgi */
			if(cgi_add_input(cntxt, NULL, name, name_sz, val, sz)) STACK_ERROR;
		}
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_read_file
** Description : handle files in multipart CGI message : write it to disk & create object in DB
*********************************************************************/
#define ERR_FUNCTION "cgi_read_file"
#define ERR_CLEANUP
int cgi_read_file(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in : execution context data */
	char **input,					/* in : filename
									  out : first char after file data */
	unsigned long i_cgi,			/* in : index of corresponding CGI input in cntxt->cgi */
	char *separator,				/* in : values separator */
	size_t sz_sep					/* in : separator length */
){
	int k = 0;
	EVA_cgi_file *cgi_file;

	/* Read file name */
	char *v0 = *input;
	char *v1 = strchr(v0, '"');
	char *v2, *v3;
	if(!v1) RETURN_ERROR("Bad filename format in CGI data", NULL);

	/* Find next value separator */
	v2 = *input;
	v3 = cntxt->input->data + cntxt->input->cnt;
	while(strncmp(v2, separator, sz_sep) && v2 < v3) v2++;
	*input = v2;
	if(v2 == v3) RETURN_ERROR("Bad value format in CGI data", NULL);

	/* Empty file name : skip value */
	if(v0 == v1) RETURN_OK;

	/* Add file data to cntxt->cgi_file */
	M_REALLOC(EVA_cgi_file, cntxt->cgi_file, cntxt->cgi_file_nb);
	cgi_file = cntxt->cgi_file + (cntxt->cgi_file_nb - 1);
	cgi_file->i_cgi = i_cgi;

	/* Add path to object data */
	DYNTAB_SET(&cgi_file->descr, k, 3, "1");
	DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_FILE_CLIENT_PATH");
	DYNTAB_ADD(&cgi_file->descr, k, 1, v0, v1 - v0, NO_CONV);
	DYNTAB_SET(&cgi_file->descr, k, 2, "V");
	k++;

	/* Find file MIME cgi_file->descr & add to object cgi_file->descr */
	v2 = strstr(v1, "\r\n\r\n");
	if(v2 > v1 + 8)
	{
		DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_FILE_MIME_INFO");
		DYNTAB_ADD(&cgi_file->descr, k, 1, v1+3, v2 - v1 - 3, CRLF_TO_LF);
		DYNTAB_SET(&cgi_file->descr, k, 2, "V");
		DYNTAB_SET(&cgi_file->descr, k, 3, "1");
		k++;
	}

	/* Store file data location */
	if((*input - v2) > 6) cgi_file->len = *input - v2 - 6;
	cgi_file->data = v2 + 4;

	/* Get HTTP_USER_AGENT env var & add to object cgi_file->descr */
	v3 = cntxt->user_agent;
	if(v3 && *v3)
	{
		DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_FILE_USER_AGENT");
		DYNTAB_ADD(&cgi_file->descr, k, 1, v3, strlen(v3), NO_CONV);
		DYNTAB_SET(&cgi_file->descr, k, 2, "V");
		DYNTAB_SET(&cgi_file->descr, k, 3, "1");
		k++;
	}

	/* Get REMOTE_ADDR en var & add to object cgi_file->descr */
	v3 = cntxt->user_ip;
	if(v3 && *v3)
	{
		DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_FILE_REMOTE_ADDR");
		DYNTAB_ADD(&cgi_file->descr, k, 1, v3, strlen(v3), NO_CONV);
		DYNTAB_SET(&cgi_file->descr, k, 2, "V");
		DYNTAB_SET(&cgi_file->descr, k, 3, "1");
		k++;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : file_find_duplicate
** Description : find object id of an existing file
*********************************************************************/
#define ERR_FUNCTION "file_find_duplicate"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					M_FREE(buf)
int file_find_duplicate(			/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *idobj,				/* out : object id of existing file - empty if not found */
	char *txt, size_t sz			/* in : file data to search */
){
	DynTable data = { 0 };
	char sql[1024];
	char sztxt[16];
	unsigned long i;
	FILE *f = NULL;
	char *buf = NULL;

	/* Get IdValue of file size - return if not found */
	unsigned long idval = get_id_value(cntxt, sztxt, snprintf(add_sz_str(sztxt), "%lu", sz));
	dyntab_free(idobj);
	if(!idval) RETURN_OK;
	M_ALLOC(buf, sz);

	/* Build & exec query for files with this size */
	snprintf(add_sz_str(sql),
		"-- file_find_duplicate\n"
		"SELECT DISTINCT IdObj FROM TLink WHERE "
		"IdValue=%lu AND IdField=%lu AND DateDel IS NULL",
		idval, IDVAL("_EVA_FILE_SIZE"));
	if(sql_exec_query(cntxt, sql) || sql_get_table(cntxt, idobj, 0)) STACK_ERROR;
	if(idobj->nbrows && 
		qry_listobj_field(cntxt, &data, idobj, "_EVA_FILE_SERVER_NAME")) STACK_ERROR;
	dyntab_free(idobj);

	/* For each found file */
	for(i = 0; i < data.nbrows && !idobj->nbrows; i++)
	{
		/* Check actual size - ignore if different or not found */
		DynTableCell *c = dyntab_cell(&data, i, 0);
		struct stat fs;
		size_t sz1 = 0;
		if(stat(c->txt, &fs) || (size_t)fs.st_size != sz) continue;

		/* Same size : read file contents & close - ignore file on error */
		f = fopen(c->txt, "rb");
		if(!f) continue;
		*buf = (char)(*txt + 1);
		sz1 = fread(buf, sz, 1, f);
		fclose(f);

		/* Ignore if files differ */
		if(sz1 != 1 || memcmp(buf, txt, sz)) continue;
		
		/* Duplicate found : set result */
		DYNTAB_ADD_INT(idobj, idobj->nbrows, 0, c->IdObj);
		ERR_PUT_TXT("Server file : ", c->txt, c->len);
		RETURN_ERROR("Le fichier est déjà sur le serveur", 0)
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_save_files
** Description : write uploaded files to disk & create objects in DB
*********************************************************************/
#define ERR_FUNCTION "cgi_save_files"
#define ERR_CLEANUP DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(id_form); \
					DYNTAB_FREE(data); \
					if(f) fclose(f)
int cgi_save_files(					/* return : 0 if Ok, other on error */		
	EVA_context *cntxt				/* in : execution context data */
){
	DynTable id_obj = { 0 };
	DynTable id_form = { 0 };
	DynTable data = { 0 };
	char name[1024];
	FILE *f = NULL;
	unsigned long i, k;

	/* Get uploaded files form id form conf data - return if none */
	DYNTAB_FIELD(&id_form, &cntxt->cnf_data, FORM_FILEUPLOAD)
	if(!cntxt->cgi_file || !cntxt->cgi_file_nb || !id_form.nbrows) RETURN_OK;

	/* For each uploaded file */
	for(i = 0; i < cntxt->cgi_file_nb; i++)
	{
		EVA_cgi_file *cgi_file = cntxt->cgi_file + i;
		CGIData *cgi = cntxt->cgi ? cntxt->cgi + cgi_file->i_cgi : NULL;
		char *bname = basename(DYNTAB_VAL_SZ(&cgi_file->descr, 0, 1));
		char *ext = strrchr(bname, '.');
		size_t sz_ext = ext ? strlen(ext) : 0;
		size_t sz0 = strlen(bname) - sz_ext;
		char serial[16] = {0};
		size_t sz_max = sizeof(name) - sz_ext - sizeof(serial);
		if(sz0 > sz_max) sz0 = sz_max;

		/* Continue if empty file */
		if(!cgi_file->len) continue;

		/* If location specified in subfield - save to location */
		if(cgi && cgi->subfield && !strncmp(cgi->subfield, add_sz_str("SAVETO=")))
		{
			if(chdir(cntxt->rootdir) || chdir(cgi->subfield + 7)) continue;
			snprintf(add_sz_str(name), "%s", bname);
			file_compatible_name(name);
			f = fopen(name, "wb");
			if(!f) RETURN_ERROR("Cannot create file in subdirectory", NULL);
			if(fwrite(cgi_file->data, cgi_file->len, 1, f) != 1) RETURN_ERROR("Cannot write downloaded file", NULL);
			fclose(f);
			f = NULL;

			/* Add file name as value in corresponding CGI input */
			cgi->value_sz = strlen(name);
			M_STRDUP(cntxt->cgi[cgi_file->i_cgi].value, name, cgi->value_sz);
			continue;
		}

		/* Switch to docs directory */
		if(chdir_db_doc(cntxt)) STACK_ERROR;

		/* Search for duplicate file */
		if(file_find_duplicate(cntxt, &id_obj, cgi_file->data, cgi_file->len)) CLEAR_ERROR;
		if(!id_obj.nbrows)
		{
			/* Duplicate not found : save file & create object */
			unsigned long iserial = 0;
			struct stat fs;

			/* Build unique file name based on original file name */
			do
			{
				snprintf(add_sz_str(name), "%.*s%s%s", sz0, bname, serial, ext ? ext : "");
				file_compatible_name(name);
				snprintf(add_sz_str(serial), "#%lu", ++iserial);
			}
			while(!stat(name, &fs));
			
			/* Create new file & write file data */
			f = fopen(name, "wb");
			if(!f) RETURN_ERROR("Cannot create file in docs directory", NULL);
			if(fwrite(cgi_file->data, cgi_file->len, 1, f) != 1) RETURN_ERROR("Cannot write downloaded file", NULL);
			fclose(f);
			f = NULL;

			/* Add file length to object data */
			k = cgi_file->descr.nbrows;
			DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_FILE_SIZE");
			DYNTAB_ADD_INT(&cgi_file->descr, k, 1, cgi_file->len);
			DYNTAB_SET(&cgi_file->descr, k, 2, "V");
			DYNTAB_SET(&cgi_file->descr, k, 3, "1");
			k++;

			/* Add file name to object data */
			DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_FILE_SERVER_NAME");
			DYNTAB_ADD(&cgi_file->descr, k, 1, name, 0, NO_CONV);
			DYNTAB_SET(&cgi_file->descr, k, 2, "V");
			DYNTAB_SET(&cgi_file->descr, k, 3, "1");
			k++;

			/* Add form/obj/ctrl info */
			if(cgi)
			{
				if(cgi->IdObj)
				{
					DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_OBJ");
					DYNTAB_ADD_INT(&cgi_file->descr, k, 1, cgi->IdObj);
					DYNTAB_SET(&cgi_file->descr, k, 2, "R");
					DYNTAB_SET(&cgi_file->descr, k, 3, "1");
					k++;
				}
				if(cgi->IdCtrl)
				{
					DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_CTRL");
					DYNTAB_ADD_INT(&cgi_file->descr, k, 1, cgi->IdCtrl);
					DYNTAB_SET(&cgi_file->descr, k, 2, "R");
					DYNTAB_SET(&cgi_file->descr, k, 3, "1");
					k++;
				}
				if(cgi->IdForm)
				{
					DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_FORM");
					DYNTAB_ADD_INT(&cgi_file->descr, k, 1, cgi->IdForm);
					DYNTAB_SET(&cgi_file->descr, k, 2, "R");
					DYNTAB_SET(&cgi_file->descr, k, 3, "1");
					k++;
				}
			}

			/* Get file description from CGI user input */
			if(cgi && cgi_filter_values(cntxt, &data, 'D', cgi->IdCtrl, cgi->IdForm, cgi->IdObj ? cgi->IdObj : ~0UL, cgi->field, "FILE_DESCRIPTION", 0, 0))
				STACK_ERROR;
			if(dyntab_sz(&data, 0, 0))
			{
				DYNTAB_SET(&cgi_file->descr, k, 0, "_EVA_NOTES");
				DYNTAB_ADD_CELL(&cgi_file->descr, k, 1, &data, 0, 0);
				DYNTAB_SET(&cgi_file->descr, k, 2, "V");
				DYNTAB_SET(&cgi_file->descr, k, 3, "1");
				k++;
			}

			/* Create new object with file description */
			if(qry_add_obj_data(cntxt, &id_obj, &cgi_file->descr, &id_form, 0))
				STACK_ERROR;
		}
		DYNTAB_FREE(cgi_file->descr);

		/* Add object id as value in corresponding CGI input */
		cgi->value_sz = dyntab_sz(&id_obj, 0, 0);
		M_STRDUP(cntxt->cgi[cgi_file->i_cgi].value, dyntab_val(&id_obj, 0, 0),
													dyntab_sz(&id_obj, 0, 0));
		DYNTAB_FREE(id_obj);
	}
	cgi_free_files(cntxt);
	cntxt->cgi_file = NULL;
	cntxt->cgi_file_nb = 0;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_read_multipart
** Description : read CGI data in multipart/form-data format
*********************************************************************/
#define ERR_FUNCTION "cgi_read_multipart"
#define ERR_CLEANUP	M_FREE(separator)
int cgi_read_multipart(		/* return : 0 on success, other on error */
	EVA_context *cntxt		/* in : 
								cntxt->input : CGI input to process
							   out : 
								cntxt->cgi : table of read & parsed CGI inputs */
){
	char *txt = cntxt->input->data;
	char *end = txt + cntxt->input->cnt;
	size_t len, sz_sep;
	char *separator;
#define CGI_DELIM_DESCR "Content-Disposition: "
#define CGI_DELIM_NAME "name=\""
#define CGI_DELIM_FILE "; filename=\""

	/* Find record separator */
	separator = strstr(txt, CGI_DELIM_DESCR);
	if(!separator) RETURN_OK;
	sz_sep = separator - txt - 2;
	M_ALLOC(separator, sz_sep+4);
	memcpy(separator, txt, sz_sep);
	separator[sz_sep] = 0;

	while(txt && txt < end)
	{
		char *name = strstr(txt, CGI_DELIM_NAME);
		char *val, *endval;
		size_t name_sz;
		if(!name) break;
		name += strlen(CGI_DELIM_NAME);
		val = strchr(name, '"');
		if(!val) RETURN_ERROR("Bad name format in CGI data", NULL);
		name_sz = val - name;

		/* Check for file name */
		if(!strncmp(val+1, add_sz_str(CGI_DELIM_FILE)))
		{
			/* File input : add input in cntxt->cgi */
			unsigned long i_cgi;
			if(cgi_add_input(cntxt, &i_cgi, name, name_sz, NULL, 0)) STACK_ERROR;

			/* Read file data */
			name = val + 1 + strlen(CGI_DELIM_FILE);
			if(cgi_read_file(cntxt, &name, i_cgi, separator, sz_sep)) STACK_ERROR;
			endval = name;
		}
		else
		{
			/* ASCII value input : find value start */
			val = strstr(val, "\r\n\r\n");
			if(!val) RETURN_ERROR("Bad value format in CGI data", NULL);
			val += 4;

			/* Find value end */
			endval = strstr(val, separator);
			if(!endval) RETURN_ERROR("Bad value format in CGI data", NULL);
			len = endval - val;

			/* Strip spaces around the value */
			STRIP_SPACES_AROUND(val, len);

			/* Add input in cntxt->cgi */
			if(cgi_add_input(cntxt, NULL, name, name_sz, val, len)) STACK_ERROR;
		}

		/* Switch to next part */
		txt = endval;
	}
	
#undef CGI_DELIM_DESCR
#undef CGI_DELIM_NAME
#undef CGI_DELIM_FILE

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_trace_input
** Description : output CGI data in a trace file
*********************************************************************/
void cgi_trace_input(EVA_context *cntxt)
{ 
	FILE *f = NULL;
	chdir(cntxt->path);
	f = fopen("cgi.txt", "wb");
	if(f)
	{
		fwrite(cntxt->input->data, cntxt->input->cnt, 1, f);
		fclose(f);
	}
}

/*********************************************************************
** Function : cgi_read_data
** Description : read CGI data into a DynTable
*********************************************************************/
#define ERR_FUNCTION "cgi_read_data"
#define ERR_CLEANUP	if(f) fclose(f)
int cgi_read_data(			/* return : 0 on success, other on error */
	EVA_context *cntxt		/* in : context data
							   out : 
								cntxt->cgi : table of read & parsed CGI inputs
									col 0 : name
									col 1 : value
									col 2 : number
									col 3 : object id
									col 4 : form id
								cntxt->session : session id
								cntxt->id_form : current form id
								cntxt->cgibtn : index of the first clicked button
							*/
){
	char *enctype = getenv("CONTENT_TYPE");
	char *length = getenv("CONTENT_LENGTH");
	FILE *f = NULL;

	/* If trace mode */
	if(cntxt->argc > 1 && !strcmp(cntxt->argv[cntxt->argc - 1], "CGITrace"))
	{
		/* Read input from text file */
		chdir(cntxt->path);
		f = fopen("cgi.txt", "rb");
		if(f)
		{
			size_t len = filelength(fileno(f));
			cntxt->input = dynbuf_init(len+4);
			if(!cntxt->input) RETURN_ERR_MEMORY;
			if(fread(cntxt->input->data, len, 1, f) != 1) RETURN_ERROR("CGI read error from file", NULL);
			cntxt->input->cnt = len;
			fclose(f);
			f = NULL;
			cntxt->urlencoded = *cntxt->input->data != '-';
		}
		cntxt->argc = 1;
	}

	/* Else : read from stdin */
	else if(length)
	{
		size_t len = atoi(length);
		clock_t t0 = clock();

		cntxt->urlencoded = !enctype || !strcmp(enctype, "application/x-www-form-urlencoded");
		cntxt->rxsize = len;
		cntxt->input = dynbuf_init(len+4);
		if(!cntxt->input) RETURN_ERR_MEMORY;
		setmode(fileno(stdin), _O_BINARY);

		/* If not urlencoded and size over 50k */
		if(!cntxt->urlencoded && len > 500000000L)
		{
			/* TODO - deactivated - doesnt work : displayed after file was transmitted */
			/* Display progress bar & use multiple blocks read */
			size_t sz = 0, blk = 1024;
			unsigned long cnt = 0;
			cntxt->imgwait = 1;
#ifndef _EVA_DLL
			put_html_page_header(cntxt, NULL, "Téléchargement de fichier", NULL, 2);
#endif
			printf("Téléchargement : ");
			while(sz < len)
			{
				if(blk > len - sz) blk = len - sz;
				if(fread(cntxt->input->data + sz, blk, 1, stdin) != 1) 
					RETURN_ERROR("CGI read error from stdin", ERR_PUT_INT("\n\texpected size = ", len));
				sz += blk;
				while(cnt < 50 * sz / len) { printf("*"); fflush(stdout); cnt++; }
			}
		}
		else
			/* Else use single block read */
			if(fread(cntxt->input->data, len, 1, stdin) != 1) RETURN_ERROR("CGI read error from stdin", ERR_PUT_INT("\n\texpected size = ", len));
		cntxt->input->cnt = len;
		cntxt->rxtime = clock() - t0;

		/* Trace CGI data if applicable */
		if(cntxt->debug & DEBUG_CGI_RAW) cgi_trace_input(cntxt);
		if(len > cntxt->input->cnt) RETURN_ERROR("CGI read incomplete :", ERR_PUT_INT("\n\tinput size = ", cntxt->input->cnt))
	}

	/* Call CGI read function depending on encoding type */
	cntxt->urlencoded = enctype ? !strcmp(enctype, "application/x-www-form-urlencoded") : 
									cntxt->input && *cntxt->input->data != '-';
	if(cntxt->input && (cntxt->urlencoded ? cgi_read_urlencoded : cgi_read_multipart)(cntxt))
		STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_get_subfield
** Description : return	cgi inputs matching the given name
*********************************************************************/
#define ERR_FUNCTION "cgi_get_subfield"
#define ERR_CLEANUP
int cgi_get_subfield(				/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *cgival,				/* out : CGI values table */
	EVA_ctrl *ctrl,					/* in : related control */
	char *subfield,					/* in : subfield name */
	int b_keep,						/* in : mark for no reoutput if 0 */
	unsigned long num,				/* in : value index */
	unsigned long line				/* in : value index */
)
{
	dyntab_free(cgival); 
	if(cgi_filter_values(cntxt, cgival, 'D', DYNTAB_TOUL(&ctrl->id), 
										DYNTAB_TOUL(&cntxt->form->id_form),
										DYNTAB_TOUL(&cntxt->form->id_obj),
										ctrl->FIELD, 
										subfield, num, line))
		STACK_ERROR; 
	if(!b_keep) CGI_VALUES_DONTKEEP(cgival); 

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_put_subfield
** Description : output hidden CGI input for control options
*********************************************************************/
#define ERR_FUNCTION "cgi_put_subfield"
#define ERR_CLEANUP	M_FREE(name)
int cgi_put_subfield(				/* return : 0 if Ok, other on error */		
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_ctrl *ctrl,					/* in : related control */
	char *subfield,					/* in : subfield name */
	char *val, size_t val_sz		/* in : value to use */
)
{
	DynBuffer *name = NULL;	
	if(cgi_build_name(cntxt, &name, 'D',
		DYNTAB_TOUL(&ctrl->id), 
		DYNTAB_TOUL(&cntxt->form->id_form), 
		DYNTAB_TOUL(&cntxt->form->id_obj), 
		ctrl->FIELD, strlen(ctrl->FIELD), subfield, strlen(subfield), 0, 0, 0))
		STACK_ERROR; 
	DYNBUF_ADD3_BUF(cntxt->form->html, "<input type=hidden name='", name, NO_CONV, "'"); 
	DYNBUF_ADD3(cntxt->form->html, " value='", val, val_sz, HTML_NO_QUOTE, "'>");	
	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(cntxt->form->html, "\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
