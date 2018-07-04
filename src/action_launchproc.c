/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_launchproc.c
** Description : handling fonctions for lauch process action
**      Author : Alain BOUTE
**     Created : June 9 2003
*********************************************************************/

#include "eva.h"

int FactureAEMO(EVA_context *cntxt, unsigned long i_ctrl);
int EcheancierAEMO(EVA_context *cntxt, unsigned long i_ctrl);
int FactureTPS(EVA_context *cntxt, unsigned long i_ctrl);
int EcheancierTPS(EVA_context *cntxt, unsigned long i_ctrl);


/*********************************************************************
** Function : office_launchproc
** Description : lauch an external office program
*********************************************************************/
#define ERR_FUNCTION "office_launchproc"
#define ERR_CLEANUP	
int office_launchproc(			/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	char *proctyp,				/* in : office program to lanch (_EVA_EXCEL/_EVA_WORD/_EVA_ACCESS) */
	char *procname				/* in : office template to transfer to current directory & open */
){
	struct stat fs = {0};
	char exepath[1024] = {0};
	char filename[256] = {0};
	char cmd[2048] = {0};

	/* Prepare executable & file path */
	snprintf(add_sz_str(exepath),  "%s\\Office\\%s.EXE", dyntab_val(&cntxt->cnf_server, 0, 0), 
												!strcmp(proctyp, "_EVA_EXCEL") ? "EXCEL" : 
												!strcmp(proctyp, "_EVA_WORD") ? "WINWORD" : "MSACCESS");
	snprintf(add_sz_str(filename), "%stemplates\\%s", cntxt->rootdir, procname);
	if(stat(exepath, &fs)) RETURN_ERROR("Impossible de lancer le traitement", ERR_PUT_TXT("\nFichier programme non trouvé : ", exepath, 0));
	if(stat(filename, &fs)) RETURN_ERROR("Impossible de lancer le traitement", ERR_PUT_TXT("\nFichier modèle non trouvé : ", procname ? procname : "(null)", 0));

	/* Copy template & launch office program */
	snprintf(add_sz_str(cmd), "COPY %s . >exe.txt 2>exeerr.txt", filename);
	if(system(cmd) == -1 || stat(filename, &fs))
		RETURN_ERROR("Erreur la préparation du traitement (copie procédure)", NULL);
	snprintf(cmd, sizeof(cmd)-1, "call \"%s\" \"%s\" >exe.txt 2>exeerr.txt", exepath, procname);
	if(system(cmd) == -1 || stat("exe.txt", &fs) || stat("exeerr.txt", &fs) || fs.st_size > 0)
		RETURN_ERROR("Erreur lors de l'appel du traitement", { ERR_PUT_FILE("\nexe : ", "exe.txt"); ERR_PUT_FILE("\nexeerr : ", "exeerr.txt"); });
	remove("exe.txt");
	remove("exeerr.txt");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : proc_output_params
** Description : compute & output parameters for external processing
*********************************************************************/
#define ERR_FUNCTION "proc_output_params"
#define ERR_CLEANUP	DYNTAB_FREE(symbol); \
					DYNTAB_FREE(vartype); \
					DYNTAB_FREE(varval); \
					DYNTAB_FREE(objtype); \
					DYNTAB_FREE(listobj); \
					DYNTAB_FREE(fieldexpr); \
					DYNTAB_FREE(inputobj); \
					DYNTAB_FREE(calcobj); \
					DYNTAB_FREE(dataobj); \
					DYNTAB_FREE(tableobj); \
					DYNTAB_FREE(filterobj); \
					DYNTAB_FREE(valmult); \
					DYNTAB_FREE(field); \
					DYNTAB_FREE(format); \
					DYNTAB_FREE(obj); \
					DYNTAB_FREE(data); \
					M_FREE(buf); \
					qry_build_free(&flt)
int proc_output_params(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	FILE *f							/* in : output file stream */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable symbol = { 0 };
	DynTable vartype = { 0 };
	DynTable varval = { 0 };
	DynTable objtype = { 0 };
	DynTable listobj = { 0 };
	DynTable fieldexpr = { 0 };
	DynTable inputobj = { 0 };
	DynTable calcobj = { 0 };
	DynTable dataobj = { 0 };
	DynTable tableobj = { 0 };
	DynTable filterobj = { 0 };
	DynTable valmult = { 0 };
	DynTable field = { 0 };
	DynTable format = { 0 };
	DynTable obj = { 0 };
	DynTable data = { 0 };
	DynBuffer *buf = NULL;
	QryBuild flt = {0};
	unsigned long i, j;

	/* Read params definition */
	CTRL_ATTR_TAB(symbol, SYMBOL);
	CTRL_ATTR_TAB(vartype, VAR_TYPE);
	CTRL_ATTR_TAB(varval, VAR_VALUE);
	CTRL_ATTR_TAB(objtype, VAR_OBJTYPE);
	CTRL_ATTR_TAB(listobj, VALOBJ);
	CTRL_ATTR_TAB(fieldexpr, VAR_FIELDEXPR);
	CTRL_ATTR_TAB(inputobj, INPUTOBJ);
	CTRL_ATTR_TAB(calcobj, CALCOBJ);
	CTRL_ATTR_TAB(dataobj, DATAOBJ);
	CTRL_ATTR_TAB(filterobj, FILTEROBJ);
	CTRL_ATTR_TAB(tableobj, TABLEOBJ);
	CTRL_ATTR_TAB(valmult, USE_MULTIPLE);

	/* Output parameters in a separate table */
	fprintf(f, "\n");
	for(i = 0; i < vartype.nbrows; i++) 
	{
		/* Handle parameter type */
		char *typ = dyntab_val(&vartype, i, 0);
		fprintf(f, "%s\t%s\t", dyntab_val(&symbol, i, 0), typ + 5);

		/* Constant value : output value */
		if(!strcmp(typ, "_EVA_VALUE"))
		{
			if(buf) buf->cnt = 0;
			DYNBUF_ADD_CELL(&buf, &varval, i, 0, EXPORT_TABRC);
			if(buf) fprintf(f, "%s", buf->data);
		}

		/* Field value or SQL expression : read & handle multiple values */
		else if(!strcmp(typ, "_EVA_FIELDVAL") || !strcmp(typ, "_EVA_SQLEXPR"))
		{
			int b_date = 0, b_num = 0;
			char *vmult = dyntab_val(&valmult, i, 0);
			char *objtyp = dyntab_val(&objtype, i, 0);
			if(!*objtyp) objtyp = "_EVA_CURRENTOBJ";

			/* Read source object, storage field & field values */
			DYNTAB_SET_CELL(&obj, 0, 0, &listobj, i, 0);
			DYNTAB_FREE(field);
			DYNTAB_FREE(format);
			if(!strcmp(typ, "_EVA_FIELDVAL"))
			{
				unsigned long idobj = DYNTAB_TOULRC(&inputobj, i, 0);
				if(qry_obj_idfield(cntxt, &field, idobj, cntxt->val_FIELD)) STACK_ERROR;
				if(qry_obj_field(cntxt, &format, idobj, "_EVA_FORMAT")) STACK_ERROR;
				if(qry_obj_idfield(cntxt, &data, idobj, cntxt->val_TYPE)) STACK_ERROR;
				if(!dyntab_txt_cmp(add_sz_str("_EVA_DATE"), &data, 0, 0)) b_date = 1;
				else if(!dyntab_txt_cmp(add_sz_str("_EVA_NUMBER"), &data, 0, 0)) b_num = 1;
			}
			else
				DYNTAB_SET_CELL(&field, 0, 0, &fieldexpr, i, 0);
			if(ctrl_eval_valtyp(cntxt, ctrl, &data, objtyp, &obj, &field)) STACK_ERROR;

			/* Handle multiple values */
			if(*vmult) vmult += 5;
			if(strcmp(vmult, "LINES") && strcmp(vmult, "COMMA") && strcmp(vmult, "SPACE"))
				dyntab_group(&data, vmult);
			for(j = 0; j < data.nbrows; j++)
			{
				DynTableCell *c = dyntab_cell(&data, j, 0);
				if(buf) buf->cnt = 0;
				if(b_date)
				{
					char date[64];
					datetxt_to_format(cntxt, date, c->txt, dyntab_val(&format, 0, 0));
					DYNBUF_ADD(&buf, date, 0, NO_CONV);
				}
				else if(b_num)
				{
					DYNBUF_ADD(&buf, number_to_format(c->txt, dyntab_val(&format, 0, 0)), 0, NO_CONV);
				}
				else
					DYNBUF_ADD_CELLP(&buf, c, EXPORT_TABRC);
				if(!strcmp(vmult, "LINES"))
					fprintf(f, "%s%s", j ? "<RC>" : "", buf->data);
				else if(!strcmp(vmult, "COMMA"))
					fprintf(f, "%s'%s'", j ? ", " : "", buf->data);
				else if(!strcmp(vmult, "SPACE"))
					fprintf(f, "%s'%s'", j ? " " : "", buf->data);
				else
					fprintf(f, "%s%s\t%lu\t%lu", j ? "\n\t\t" : "", buf->data, c->Num, c->Line);
			}
		}

		/* Calc-ed value : read control, evaluate & output values */
		else if(!strcmp(typ, "_EVA_CALC"))
		{
			EVA_ctrl *ctl = form->ctrl + form->nb_ctrl;
			if(qry_cache_idobj(&ctl->attr, DYNTAB_TOULRC(&calcobj, i, 0)) ||
				ctrl_read_baseobj(cntxt, &ctl[0].attr) ||
				ctrl_set_common_attr(cntxt, ctl)) STACK_ERROR;
			if(ctrl_output_exprval(cntxt, ctl, 2)) STACK_ERROR;
			for(j = 0; j < ctl->val.nbrows; j++)
			{
				DynTableCell *c = dyntab_cell(&ctl->val, j, 0);
				fprintf(f, "%s%s\t%lu\t%lu", j ? "\n\t\t" : "", (c + 1)->txt, c->Num, c->Line);
			}
			form_free_ctrl(cntxt, form->nb_ctrl);
		}

		/* List of values : exec filters */
		else if(!strcmp(typ, "_EVA_LISTOBJ"))
		{
			DYNTAB_FREE(field);
			qry_build_free(&flt);
			for(j = 0; j < filterobj.nbcols; j++) DYNTAB_SET_CELL(&field, j, 0, &filterobj, i, j);
			if(qry_add_filter_forms(cntxt, &flt, NULL, &field) ||
				qry_filter_objects(cntxt, &data, &flt)) STACK_ERROR;

			for(j = 0; j < data.nbrows; j++)
			{
				DynTableCell *c = dyntab_cell(&data, j, 0);
				if(!c->txt) continue;
				fprintf(f, "%s%s\t%lu\t%lu", j ? "\n\t\t" : "", c->txt, c->Num, c->Line);
			}
		}

		/* Table data : output data in file */
		else if(!strcmp(typ, "_EVA_DATATABLE"))
		{
			EVA_ctrl *lstctrl = NULL;
			ObjTableFormat *tbl = NULL;
			unsigned long ictrl = 0, idctrl = DYNTAB_TOULRC(&dataobj, i, 0);
			int b_selobj = 0;
			char fname[64];

			/* Search for table in current form */
			for(j = 1; j < form->nb_ctrl && !ictrl; j++)
				if(DYNTAB_TOUL(&form->ctrl[j].id) == idctrl) ictrl = j;

			/* If table is in form : use loaded control */
			if(ictrl)
				lstctrl = form->ctrl + ictrl;
			else
			{
				/* If table is not in form : load in last control */
				ictrl = form->nb_ctrl;
				lstctrl = form->ctrl + ictrl;
				if(qry_obj_field(cntxt, &lstctrl->attr, idctrl, NULL) ||
					ctrl_read_baseobj(cntxt, &lstctrl->attr) ||
					ctrl_set_common_attr(cntxt, lstctrl) ||
					table_read_controls(cntxt, ictrl, NULL)) STACK_ERROR;
			}
			tbl = lstctrl->objtbl;

			/* Output text file with table data */
			snprintf(add_sz_str(fname), "%lu.txt", idctrl);
			if(table_init_obj_list(cntxt, ictrl, &b_selobj)) STACK_ERROR;
			tbl->line = 0;
			tbl->lines = tbl->totlines;
			if(table_sort_obj_list(cntxt, ictrl, b_selobj)) STACK_ERROR;
			if(table_read_obj_list(cntxt, ictrl, b_selobj)) STACK_ERROR;
			if(table_prepare_rows(cntxt, ictrl, "")) STACK_ERROR;
			if(file_write_tabrc(cntxt, &tbl->cellval, fname)) STACK_ERROR;
			fprintf(f, "%s", fname);
			if(ictrl == form->nb_ctrl) form_free_ctrl(cntxt, ictrl);
		}
		fprintf(f, "\n");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : result_data
** Description : handle object data result for launchproc button
*********************************************************************/
#define ERR_FUNCTION "result_data"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
		M_FREE(obj);
int result_data(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	DynTable *res				/* in : result of launched process */
){
	EVA_form *form = cntxt->form;
	DynTable data = { 0 };
	unsigned long i;
	unsigned long idobj = DYNTAB_TOUL(&form->id_obj);
	unsigned long idform = DYNTAB_TOUL(&form->id_form);
	unsigned long *obj = NULL, sz_obj = 0;

	/* For each result line */
	for(i = 1; i < res->nbrows; i++)
	{
		/* Handle operation mode */
		char *mode = dyntab_val(res, i, 0);
		if(!strcmp(mode, "IdObj"))
			/* Set updated object */
			idobj = DYNTAB_TOULRC(res, i, 1);
		else if(!strcmp(mode, "IdForm"))
			/* Set updated CGI form */
			idform = DYNTAB_TOULRC(res, i, 1);
		else if(!strcmp(mode, "NewObj"))
		{
			/* Create new object */
			unsigned long numobj = DYNTAB_TOULRC(res, i, 1);
			idform = DYNTAB_TOULRC(res, i, 2);
			if(!sz_obj)
			{
				sz_obj = (numobj > 100 ? numobj : 100);
				M_ALLOC(obj, sz_obj * sizeof(*obj));
			}
			else if(sz_obj <= numobj)
			{
				unsigned long incr = numobj - sz_obj, *newobj;
				if(incr < 100) incr = 100;
				newobj = mem_realloc(obj, sz_obj * sizeof(*obj), incr * sizeof(*obj));
				if(!newobj) RETURN_ERR_MEMORY(NULL);
				obj = newobj;
				sz_obj += incr;
			}
			if(qry_add_new_idobj(cntxt, &idobj, idform)) STACK_ERROR;
			obj[numobj] = idobj;
		}
		else
		{
			DynTableCell *v = dyntab_cell(res, i, 2), c = { 0 };
			c.txt = v->txt;
			c.len = v->len;
			c.field = dyntab_val(res, i, 1);
			c.Num = DYNTAB_TOULRC(res, i, 3);
			c.Line = DYNTAB_TOULRC(res, i, 4);
			c.IdObj = idobj;
			c.b_relation = (char) !strncmp(mode + 6, add_sz_str("Rel"));
			/* Handle reference to new object */
			if(c.b_relation && c.txt[0] == '#')
			{
				char *end = NULL;
				unsigned long newobj = 0, numobj = strtoul(c.txt + 1, &end, 10);
				if(!end || *end || !c.txt[1]) continue;
				if(numobj < sz_obj) newobj = obj[numobj];
				if(!newobj) continue;
				c.IdValue = newobj;
				c.len = 0;
				c.txt = NULL;
			}
			if(!strncmp(mode + 3, add_sz_str("Obj")))
			{
				if(qry_add_val(cntxt, &c, strncmp(mode, add_sz_str("Set")) ? 2 : 3, NULL)) STACK_ERROR;
			}
			else
			{
				DYNTAB_SET_CELLP(&data, 0, 0, &c);
				if(cgi_set_field_values(cntxt, idform, idobj,
					DYNTAB_VAL_SZ(res, i, 1), &data,
					!strncmp(mode, add_sz_str("Set")) ? "_EVA_REPLACE" : "", 0)) STACK_ERROR;
			}
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : result_file
** Description : handle file result for launchproc button
*********************************************************************/
#define ERR_FUNCTION "result_file"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(id_form)
int result_file(				/* return : 0 on success, other on error */
	EVA_context *cntxt,			/* in/out : execution context data */
	unsigned long i_ctrl,		/* in : control index in cntxt->form->ctrl */
	DynTable *res				/* in : result of launched process */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable id_obj = { 0 };
	DynTable id_form = { 0 };
	DynTable data = { 0 };
	char filename[256] = {0};
	char fpath[2048] = {0};
	size_t sz = 0;
	unsigned long i, j;

	/* If result field given */
	if(dyntab_sz(res, 1,0))
	{
		/* Build unique file name based on original file name*/
		DynTableCell *fileproc = DYNTAB_FIELD_CELL(&cntxt->cnf_data, FORM_FILEPROC);
		char *bname = basename(DYNTAB_VAL_SZ(res, 1, 2));
		char *ext = strrchr(bname, '.');
		sz = strlen(bname) - (ext ? strlen(ext) : 0);
		snprintf(add_sz_str(filename), "%.*s-%lX%X%s", sz, bname, clock(), rand(), ext ? ext : "");
		sz = file_compatible_name(filename);

		/* Create object data for result */
		DYNTAB_SET(&data, 0, 0, "_EVA_FILE_SERVER_NAME");
		DYNTAB_ADD(&data, 0, 1, filename, sz, NO_CONV);
		DYNTAB_SET(&data, 0, 2, "V");
		DYNTAB_SET(&data, 0, 3, "1");
		DYNTAB_SET(&data, data.nbrows, 0, "_EVA_FILE_CLIENT_PATH");
		DYNTAB_ADD_CELL(&data, data.nbrows-1, 1, res, 1, 2);
		DYNTAB_SET(&data, data.nbrows-1, 2, "V");
		DYNTAB_SET(&data, data.nbrows-1, 3, "1");
		DYNTAB_SET(&data, data.nbrows, 0, "_EVA_FILE_SIZE");
		{
			struct stat fs;
			char size[64];
			stat(dyntab_val(res, 1, 1), &fs);
			sprintf(size, "%lu", (unsigned long)fs.st_size);
			DYNTAB_ADD(&data, data.nbrows-1, 1, size, 0, NO_CONV);
		}
		DYNTAB_SET(&data, data.nbrows-1, 2, "V");
		DYNTAB_SET(&data, data.nbrows-1, 3, "1");
		DYNTAB_SET(&data, data.nbrows, 0, "_EVA_PROC_DURATION");
		{
			clock_t t = clock();
			char dur[64];
			sprintf(dur, "%1.2f", (float)t/CLOCKS_PER_SEC);
			DYNTAB_ADD(&data, data.nbrows-1, 1, dur, 0, NO_CONV);
		}
		DYNTAB_SET(&data, data.nbrows-1, 2, "V");
		DYNTAB_SET(&data, data.nbrows-1, 3, "1");
		DYNTAB_SET(&data, data.nbrows, 0, "_EVA_OBJ");
		DYNTAB_ADD_CELL(&data, data.nbrows-1, 1, &form->id_obj, 0, 0);
		DYNTAB_SET(&data, data.nbrows-1, 2, "R");
		DYNTAB_SET(&data, data.nbrows-1, 3, "1");
		DYNTAB_SET(&data, data.nbrows, 0, "_EVA_CTRL");
		DYNTAB_ADD_CELL(&data, data.nbrows-1, 1, &ctrl->id, 0, 0);
		DYNTAB_SET(&data, data.nbrows-1, 2, "R");
		DYNTAB_SET(&data, data.nbrows-1, 3, "1");
		DYNTAB_SET(&data, data.nbrows, 0, "_EVA_FORM");
		DYNTAB_ADD_CELL(&data, data.nbrows-1, 1, &form->ctrl->id, 0, 0);
		DYNTAB_SET(&data, data.nbrows-1, 2, "R");
		DYNTAB_SET(&data, data.nbrows-1, 3, "1");

		/* Create new object with file description */
		DYNTAB_SET_CELLP(&id_form, 0, 0, fileproc);
		if(qry_add_obj_data(cntxt, &id_obj, &data, &id_form, 0))
			STACK_ERROR;

		/* If object was saved : add add reference to file object in DB */ 
		if(*CTRL_ATTR_VAL(SAVE))
		{
			dyntab_free(&data);
			DYNTAB_ADD_CELL(&data, 0, 0, res, 1, 0);
			DYNTAB_ADD_CELL(&data, 0, 1, &id_obj, 0, 0);
			DYNTAB_SET(&data, 0, 2, "R");
			DYNTAB_SET(&data, 0, 3, "1");
			if(qry_add_obj_data(cntxt, &form->id_obj, &data, &id_form, 1))
				STACK_ERROR;
		}
		
		/* Add relation to file object in given field */
		if(cgi_set_field_values(cntxt, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj),
								DYNTAB_VAL_SZ(res, 1, 0), &id_obj, "_EVA_REPLACE", 0)) STACK_ERROR;

		/* Set result file in docs directory */
		snprintf(fpath, sizeof(fpath)-1, "%s" DIRECTORY_DOCS "\\%s\\%s",
				cntxt->rootdir, cntxt->dbname, filename);
	}
	else if(dyntab_sz(res, 1, 1))
	{
		/* No result field : set result file to user directory */
		snprintf(add_sz_str(filename), "%s", dyntab_val(res, 1, 2));
		sz = file_compatible_name(filename);
		snprintf(add_sz_str(fpath), "%s" DIRECTORY_DOCS "\\%s\\%s\\%s",
				cntxt->rootdir, cntxt->dbname, dyntab_val(&cntxt->id_user, 0, 0), filename);
		remove(fpath);
	}

	/* Move result file to destination directory */
	if(dyntab_sz(res, 1, 1) &&
		rename(dyntab_val(res, 1, 1), fpath)) 
		RETURN_ERR_DIRECTORY({ERR_PUT_CELL("Error during rename\nsrc:", res, 1, 1); ERR_PUT_TXT("\ndest:", fpath, 0)});

	/* Following lines are field values : process each field */
	for(i = 2; i < res->nbrows; i++)
	{
		DYNTAB_FREE(data);
		for(j = 1; j < res->nbcols; j++) if(dyntab_sz(res, i, j)) DYNTAB_SET_CELL(&data, data.nbrows, 0, res, i, j);
		if(!data.nbrows) DYNTAB_ADD_CELLP(&data, 0, 0, NULL);
		if(cgi_set_field_values(cntxt, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj),
								DYNTAB_VAL_SZ(res, i, 0), &data, "_EVA_REPLACE", 0)) STACK_ERROR;

		/* If object was saved : write in DB */ 
		if(*CTRL_ATTR_VAL(SAVE))
		{
			if(qry_update_idobj_idfield(cntxt, DYNTAB_TOUL(&form->id_obj), 
					get_id_value(cntxt, DYNTAB_VAL_SZ(res, i, 0)), &data, 3)) STACK_ERROR;
		}
	}

	/* Display link to file if message or no output field given */
	if(dyntab_sz(res, 1, 1) && (!dyntab_sz(res, 1, 0) || dyntab_sz(res, 0, 1) || dyntab_sz(res, 0, 2)))
	{
		DYNBUF_ADD3(&cntxt->html, 
				"<font face=Arial><center><font size=+1>"
				"<hr><b>", form->ctrl->LABEL, 0, TO_HTML, " - ");
		DYNBUF_ADD(&cntxt->html, ctrl->LABEL, 0, TO_HTML);
		DYNBUF_ADD_STR(&cntxt->html, "</font><br><br>Traitement terminé avec succès</b></center><hr>");
		if(*dyntab_val(res, 0, 1) == '<')
			DYNBUF_ADD3_CELL(&cntxt->html, "", res, 0, 1, NO_CONV, "<hr>")
		else if(dyntab_sz(res, 0, 1))
			DYNBUF_ADD3_CELL(&cntxt->html, "", res, 0, 1, TO_HTML, "<hr>");
		cntxt->b_terminate = 15;
		if(file_output_link(cntxt, &cntxt->html,
								add_sz_str("Fichier résultat : "),
								DYNTAB_VAL_SZ(res, 1, 2), filename, sz, dyntab_sz(res, 1, 0) ? "#docs" : "#user",
								NULL, 0, DYNTAB_VAL_SZ(res, 0, 2), 
								6  | ((dyntab_sz(res, 0, 1) || dyntab_sz(res, 0, 2)) ? 0 : 1)
									| (*CTRL_ATTR_VAL(SAVE) ? 8 : 0))) STACK_ERROR;
		DYNBUF_ADD_STR(&cntxt->html, "<br><br>");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_launchproc
** Description : button click handler for OPENFORM
*********************************************************************/
#define ERR_FUNCTION "action_launchproc"
#define ERR_CLEANUP	DYNTAB_FREE(res); \
					DYNTAB_FREE(data); \
					M_FREE(msg); \
					chdir(cntxt->rootdir); chdir("cgi"); \
					if(f) fclose(f); f = NULL
int action_launchproc(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	char cmd[2048] = {0};
	char procid[32] = {0};
	char filename[256] = {0};
	DynTable res = { 0 };
	DynTable data = { 0 };
	DynBuffer *msg = NULL;
	FILE *f = NULL;
	unsigned long i, j;
	char *proctyp = CTRL_ATTR_VAL(PROCTYPE);
	char *msoexe = !strcmp(proctyp, "_EVA_EXCEL") ? "EXCEL" : 
					!strcmp(proctyp, "_EVA_WORD") ? "WINWORD" : 
					!strcmp(proctyp, "_EVA_ACCESS") ? "MSACCESS" : NULL;
	char *procname = CTRL_ATTR_VAL(PROCNAME);
	char exepath[1024] = {0};
	struct stat fs = {0};
	char *err;

	/* Handle save before */
	int b_done = 1;
	if((form->b_modified || form->prevstep == HtmlSaveDlg || form->b_newobj) && 
		form_btn_do_save(cntxt, i_ctrl, &b_done)) STACK_ERROR;
	if(!b_done) RETURN_OK;
	form = cntxt->form;
	ctrl = form->ctrl + i_ctrl;

	/* Handle integrated DLL functions */
	if(!strcmp(proctyp, "_EVA_DLL"))
	{
		char *lib = CTRL_ATTR_VAL(DLLFILE);
		if(!strcmp(procname, "FactureAEMO"))
		{
			if(FactureAEMO(cntxt, i_ctrl)) STACK_ERROR;
		}
		else if(!strcmp(procname, "EcheancierAEMO"))
		{
			if(EcheancierAEMO(cntxt, i_ctrl)) STACK_ERROR;
		}
		else if(!strcmp(procname, "FactureTPS"))
		{
			if(FactureTPS(cntxt, i_ctrl)) STACK_ERROR;
		}
		else if(!strcmp(procname, "EcheancierTPS"))
		{
			if(EcheancierTPS(cntxt, i_ctrl)) STACK_ERROR;
		}
		else
		{
			if(dll_call(cntxt, i_ctrl, lib, procname)) STACK_ERROR;
		}
		RETURN_OK;
	}														 

	/* Prepare proc directory */
	snprintf(procid, sizeof(procid)-1, "%X-%X",	time(NULL), getpid());
	if(chdir(cntxt->path)) RETURN_ERR_DIRECTORY(1);
	mkdir("proc");
	if(chdir("proc")) RETURN_ERR_DIRECTORY(1);
	mkdir(procid);
	if(chdir(procid)) RETURN_ERR_DIRECTORY(1);

	/* Create params file call.txt  */
	f = fopen("call.txt", "wc");
	if(!f) RETURN_ERR_DIRECTORY(1);

	/* Output general information */
	fprintf(f, "127.0.0.1\n");
	fprintf(f, "%s\n", cntxt->dbname);
	fprintf(f, "%s\n", dyntab_val(&cntxt->id_user, 0, 0));
	fprintf(f, "%s\n", dyntab_val(&ctrl->id, 0, 0));
	fprintf(f, "%s\n", dyntab_val(&form->id_obj, 0, 0));
	fprintf(f, "%s\n", dyntab_val(&form->id_form, 0, 0));

	/* Output data for current object */
	fprintf(f, "\n");
	for(i = 1; i < form->nb_ctrl; i++) 
	{
		EVA_ctrl *c = form->ctrl + i;
		if(!*c->FIELD || strcmp(c->CONTROL, "_EVA_INPUT")) continue;

		/* Prevent output of the same control twice */
		for(j = 0; j < i && (strcmp(c->FIELD, form->ctrl[j].FIELD) || strcmp(form->ctrl[j].CONTROL, "_EVA_INPUT")); j++);
		if(j < i) continue;

		for(j = 0; j < c->val.nbrows; j++)
		{
			DynTableCell *v = dyntab_cell(&c->val, j, 0);
			if(msg) msg->cnt = 0;
			DYNBUF_ADD(&msg, v->txt, v->len, EXPORT_TABRC);
			fprintf(f, "%s\t%s\t%lu\t%lu\n", c->FIELD, msg->data, v->Num, v->Line);
		}
	}
	M_FREE(msg);

	/* Output parameters */
	if(proc_output_params(cntxt, i_ctrl, f)) STACK_ERROR;
	fclose(f);

	/* Prepare executable path */
	if(msoexe)
	{
		/* Handle MS Office documents */
		snprintf(add_sz_str(exepath),  "%s\\Office\\%s.EXE", dyntab_val(&cntxt->cnf_server, 0, 0), msoexe);
		if(stat(exepath, &fs)) RETURN_ERROR("Impossible de lancer le traitement",
											ERR_PUT_TXT("\nFichier programme non trouvé : ", exepath, 0));
	}
	else
		*exepath = 0;

	/* Prepare file path */
	snprintf(add_sz_str(filename), "..\\..\\..\\templates\\%s\\%s", cntxt->dbname, procname);
	if(stat(filename, &fs)) snprintf(add_sz_str(filename), "..\\..\\..\\templates\\%s", procname);
	if(stat(filename, &fs)) RETURN_ERROR("Impossible de lancer le traitement",
										ERR_PUT_TXT("\nFichier modèle non trouvé : ", procname, 0));

	/* Copy template & launch program */
	snprintf(add_sz_str(cmd), "COPY %s . >exe.txt 2>exeerr.txt", filename);
	if(system(cmd) == -1 || stat(filename, &fs))
		RETURN_ERROR("Erreur la préparation du traitement (copie procédure)", NULL);
	if(*exepath)
		snprintf(cmd, sizeof(cmd)-1, "call \"%s\" \"%s\" >exe.txt 2>exeerr.txt", exepath, procname);
	else
		snprintf(cmd, sizeof(cmd)-1, "call \"%s\" >exe.txt 2>exeerr.txt", procname);
	if(system(cmd) == -1 || stat("exeerr.txt", &fs) || fs.st_size > 0)
		RETURN_ERROR("Erreur lors de l'appel du traitement", ERR_PUT_FILE("\nexeerr : ", "exeerr.txt"));

	/* Read result in res.txt & process errors */
	if(chdir_user_doc(cntxt)) STACK_ERROR;
	if(chdir(cntxt->path) || chdir("proc") || chdir(procid)) RETURN_ERR_DIRECTORY(1);

	/* Handle no result file */
	if(file_read_tabrc(cntxt, &res, "res.txt") || !res.nbrows)
	{
		/* Check exe.txt presence for EXE procedures */
		if(!strcmp(proctyp, "_EVA_EXE") && !stat("exe.txt", &fs) && fs.st_size > 0)
		{
			/* Read exe.txt & output as procedure result */
			CLEAR_ERROR_NOWARN;
			if(file_to_dynbuf(cntxt, &msg, "exe.txt")) STACK_ERROR;
		}
		else
			RETURN_ERROR("Le traitement n'a pas donné de résultat", NULL);
	}

	/* Display error message if first word is not Ok */
	err = dyntab_val(&res, 0, 0);
	if(strcmp(err, "Ok"))
	{
		/* Translate error messages to HTML if applicable */
		int b_err = msg != NULL;
		if(*err == '<')
		{
			if(dyntab_to_dynbuf(&res, &msg, "\n", 1, "\n", 1, NO_CONV)) RETURN_ERR_MEMORY;
		}
		else
		{
			if(dyntab_to_dynbuf(&res, &msg, " | ", 1, "<br>", 4, TO_HTML)) RETURN_ERR_MEMORY;
		}

		/* Output error message */
		put_html_page_header(cntxt, NULL, NULL, NULL, 3);
		printf("<font face=Arial><hr><b>%s l'exécution du traitement</b><hr>", b_err ? "Erreur durant" : "Résultat de");
		if(b_err)
			printf("<<u>Description :</u> <font size=+1 face=Courier>%s</font>", msg ? msg->data : "Pas de message d'erreur");
		else
			printf("<pre>%s</pre>", msg ? msg->data : "Pas de message d'erreur");
		printf("<br><hr><center><input type=image name='NOP' src='../img/_eva_btn_gobackobj_fr.gif'><hr></center></font>");
		cntxt->b_terminate = 15;

		/* Keep temporary directory if applicable */
		if(!strncmp(err, add_sz_str("Err")) && res.nbcols == 1 && res.nbrows == 1) RETURN_OK;
	}
	else
	{
		/* Handle result type */
		if(!strcmp(dyntab_val(&res, 0, 1), "OBJDATA"))
		{
			if(result_data(cntxt, &res)) STACK_ERROR;
		}
		else
		{
			if(result_file(cntxt, i_ctrl, &res)) STACK_ERROR;
		}
	}

	/* Clean temp directory */
	if(cntxt->user_ip && strcmp(cntxt->user_ip, "127.0.0.1") && !(chdir(cntxt->path) || chdir("proc") || chdir(procid)))
	{
		system("del /q *.*");
		chdir("..");
		rmdir(procid);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
