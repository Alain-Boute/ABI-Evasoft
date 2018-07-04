/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_import.c
** Description : handling fonctions for import actions
**      Author : Alain BOUTE
**     Created : April 5 2003
*********************************************************************/

#include "eva.h"


/*********************************************************************
** TypeDef : ImportRelSpec
** Description : import relation specification
*********************************************************************/
typedef struct _ImportRelSpec 
{
	unsigned long id;
	int b_key;
	unsigned long i_rel;
	unsigned long id_relfld;
	DynBuffer *l_relfld;
	FieldModifier modif;
} ImportRelSpec;

/*********************************************************************
** TypeDef : ImportSpec
** Description : import parameters & results
*********************************************************************/
typedef struct _ImportSpec 
{
	DynTable data;					/* table holding file data : fields title line + 1 row per record */
	DynTable formstamp;				/* form stamp for new objects */
	DynTable multvalsep;			/* multiple values separator */
	DynTable keyfields;				/* primary key fields for import */
	DynTable reltype;				/* relation type : direct / reverse */
	DynTable relfldsrc;				/* relation fields in data */
	DynTable relflddest;			/* relation fields in DB */
	DynTable relformdest;			/* form stamp for relation fields in DB */
	ImportRelSpec *rel;				/* relations to other objects */
} ImportSpec;

/*********************************************************************
** Fonction : import_tabrc_file
** Description : import a tab/rc text file (1 line per object)
*********************************************************************/
#define ERR_FUNCTION "import_tabrc_file"
#define ERR_CLEANUP DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(val); \
					DYNTAB_FREE(newval); \
					DYNTAB_FREE(srchval); \
					DYNTAB_FREE(relobj); \
					DYNTAB_FREE(relform); \
					qry_build_free(&flt)
int import_tabrc_file(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *data,					/* in : table holding file data : fields title line + 1 row per record */
	DynTable *formstamp,			/* in : form stamp for new objects */
	DynTable *keyfields,			/* in : primary key fields for import */
	DynTable *multvalsep,			/* in : multiple values separator */
	DynTable *reltype,				/* in : relation type : direct / reverse */
	DynTable *relfldsrc,			/* in : relation fields in data */
	DynTable *relflddest,			/* in : relation fields in DB */
	DynTable *relformdest			/* in : form stamp for relation fields in DB */
){
	DynTable id_obj = { 0 };
	DynTable val = { 0 };
	DynTable newval = { 0 };
	DynTable srchval = { 0 };
	DynTable relobj = { 0 };
	DynTable relform = { 0 };
	ImportRelSpec *fields = NULL;
	QryBuild flt = {0};
	unsigned long i, j, k, idformstamp = DYNTAB_TOUL(formstamp);
	int b_update = !strcmp(dyntab_val(keyfields, 0, 0), ".IdObj") &&
					!strcmp(dyntab_val(data, 0, 0), "IdObj");

	if(!data || !formstamp) RETURN_OK;

	/* Process fields names in 1st line */
	C_ALLOC(fields, sizeof(fields[0]), data->nbcols);
	for(j = b_update ? 1 : 0; j < data->nbcols; j++)
	{
		/* Check if field has a modifier (dot extension) */
		char *field = dyntab_val(data, 0, j);
		char *modif = strchr(field, '.');
		if(modif)
		{
			/* If field has a modifier (only IdRelObj is currently supported) */
			dyntab_cell(data, 0, j)->len = modif - field;
			*modif = 0;
			fields[j].modif = qry_parse_field_modifier(modif + 1);
		}

		/* Check if field is a relation */
		else if(relfldsrc) for(i = 0; i < relfldsrc->nbrows; i++) 
			if(!dyntab_cmp(data, 0, j, relfldsrc, i, 0))
			{
				/* If field is a relation : retrieve related field id */
				if(sql_id_value(cntxt, DYNTAB_VAL_SZ(relflddest, i, 0), &fields[j].id_relfld)) STACK_ERROR;
				fields[j].i_rel = fields[j].id_relfld ? i : -1;
				if(fields[j].id_relfld) DYNBUF_ADD_INT(&fields[j].l_relfld, fields[j].id_relfld);
				break;
			}

		/* Check if field is in primary key */
		if(keyfields && !b_update) for(i = 0; i < keyfields->nbrows; i++) 
			if(!dyntab_cmp(data, 0, j, keyfields, i, 0))
			{
				/* If field is in primary keys : store key data column in cell->col */
				dyntab_cell(keyfields, i, 0)->col = j;
				dyntab_cell(keyfields, i, 0)->b_relation = 0;
				fields[j].b_key = 1;
				break;
			}

		/* Search / Create field IdValue */
		if(sql_add_value(cntxt,DYNTAB_VAL_SZ(data, 0, j), &fields[j].id)) STACK_ERROR;
	}

	/* Loop on data lines */
	for(i = 1; i < data->nbrows; i++) 
	{
		/* If update mode : check target object formstamp */
		int b_new = 1;
		int idobj = DYNTAB_TOULRC(data, i, 0);
		if(b_update)
		{
			if(qry_obj_idfield(cntxt, &val, idobj, cntxt->val_FORMSTAMP)) STACK_ERROR;
			for(j = 0; j < val.nbrows && dyntab_cmp(formstamp, 0, 0, &val, j, 0); j++);
			if(j == val.nbrows)
			{
				printf("**** %lu/%lu : %s, %s, %s, %s<br>\n", i, data->nbrows - 1, dyntab_val(data, i, 0), dyntab_val(data, i, 1), dyntab_val(data, i, 2), dyntab_val(data, i, 3));
				fflush(stdout);
				continue;
			}
			DYNTAB_SET_CELL(&id_obj, 0, 0, data, i, 0);
			b_new = 0;
		}
		/* Else : check primary keys - search for existing object */
		else if(keyfields->nbrows)
		{
			/* Initialize filter with AND condition & destination form */
			qry_build_free(&flt);
			dyntab_free(&srchval);
			DYNTAB_ADD_TXTID(&srchval, 0, 0, "_EVA_FORMSTAMP", cntxt->val_FORMSTAMP);
			if(qry_add_filter(cntxt, &flt, RelNone, &srchval, RelList, formstamp)) STACK_ERROR;

			/* Build query for each key field */
			for(j = 0; j < keyfields->nbrows; j++)
			{
				unsigned long col = dyntab_cell(keyfields, j, 0)->col;
				MatchMode match = dyntab_cell(keyfields, j, 0)->b_relation ? RelList : Like;
				dyntab_from_list(&val, DYNTAB_VAL_SZ(data, i, col), dyntab_val(multvalsep, 0, 0), 0, 2);
				dyntab_from_list(&srchval, DYNTAB_VAL_SZ(keyfields, j, 0), ",", 0, 2);
				if(dyntab_sz(&val, 0, 0) && qry_add_filter(cntxt, &flt, RelNone, &srchval, match, &val)) STACK_ERROR;
			}

			/* Query for objects matching key filter */
			if(qry_filter_objects(cntxt, &id_obj, &flt)) STACK_ERROR;
			b_new = !id_obj.nbrows;
			if(!b_new)
			{
				idobj = DYNTAB_TOUL(&id_obj);
				id_obj.nbrows = 1;
			}
		}

		/* Create new object if applicable */
		if(b_new)
		{
			printf("++++");
			if(qry_add_new_obj(cntxt, &id_obj, idformstamp)) STACK_ERROR;
			idobj = DYNTAB_TOUL(&id_obj);
		}
		else printf("====");
		printf(" %lu/%lu : %s, %s, %s, %s<br>\n", i, data->nbrows - 1, dyntab_val(data, i, 0), dyntab_val(data, i, 1), dyntab_val(data, i, 2), dyntab_val(data, i, 3));
		fflush(stdout);

		/* Loop on data columns : update object data - all fields if new - non key fields else */
		for(j = b_update ? 1 : 0; j < data->nbcols; j++) if(b_new || !fields[j].b_key)
		{
			/* Get field values */
			dyntab_from_list(&val, DYNTAB_VAL_SZ(data, i, j), dyntab_val(multvalsep, 0, 0), 0, 2);

			/* Handle update mode */
			if(b_update)
			{
				if(fields[j].modif == F_RelObj)
					for(k = 0; k < val.nbrows; k++) dyntab_cell(&val, k, 0)->b_relation = 1;
				if(qry_update_idobj_idfield(cntxt, idobj, fields[j].id, &val, 2)) STACK_ERROR;
				continue;
			}

			/* Loop on multiple values : add each value */
			DYNTAB_FREE(newval);
			for(k = 0; k < val.nbrows; k++)
			{
				/* Initialize value */
				DynTableCell cell = {0};
				cell.txt = dyntab_val(&val, k, 0);
				cell.len = dyntab_sz(&val, k, 0);
				cell.Num = k + 1;
				STRIP_SPACES_AROUND(cell.txt, cell.len);

				/* Ignore empty values */
				if(!cell.len) continue;

				/* If field is not relation */
				if(!fields[j].id_relfld && !fields[j].i_rel)
				{
					cell.IdField = fields[j].id;
					cell.field = dyntab_val(data, 0, j);
					cell.b_relation = (char)(fields[j].modif == F_RelObj);
					DYNTAB_ADD_CELLP(&newval, newval.nbrows, 0, &cell);
				}
				else if(fields[j].i_rel >= 0)
				{
					/* Search for related object */
					DYNTAB_SET_CELLP(&srchval, 0, 0, &cell);
					DYNTAB_SET_CELL(&relform, 0, 0, relformdest, fields[j].i_rel, 0);
					DYNTAB_FREE(relobj);
					if(qry_field_value(cntxt, &relobj, fields[j].l_relfld, Exact, &srchval, &relform) ||
						(!relobj.nbrows &&
							qry_field_value(cntxt, &relobj, fields[j].l_relfld, Like, &srchval, &relform)))
						STACK_ERROR;

					/* If related object not found */
					if(!relobj.nbrows)
					{
						/* Create new related object & add field value */
						if(qry_add_new_obj(cntxt, &relobj, DYNTAB_TOUL(&relform))) STACK_ERROR;
						cell.IdObj = DYNTAB_TOUL(&relobj);
						cell.IdField = fields[j].id_relfld;
						cell.Num = 1;
						if(qry_add_val(cntxt, &cell, 0, NULL)) STACK_ERROR;
					}

					/* Add relation */
					cell.IdField = fields[j].id;
					cell.field = dyntab_val(data, 0, j);
					cell.b_relation = 1;
					cell.IdValue = 0;
					cell.IdField = fields[j].id;
					if(!strcmp("_EVA_RELREVERSE", dyntab_val(reltype, fields[j].i_rel, 0)))
					{
						/* Add relation in original object if reverse relation */
						char sql[1024];
						cell.IdObj = DYNTAB_TOUL(&relobj);
						cell.txt = dyntab_val(&id_obj, 0, 0);
						cell.len = dyntab_sz(&id_obj, 0, 0);
								
						/* Read maximum value number */
						sprintf(sql, "SELECT MAX(Num) FROM TLink WHERE IdObj=%lu AND IdField=%lu AND DateDel IS NULL",
									cell.IdObj, cell.IdField);
						if(sql_exec_query(cntxt, sql) || sql_get_table(cntxt, &srchval, 2)) STACK_ERROR;
						cell.Num = DYNTAB_TOUL(&srchval) + 1;
						if(qry_add_val(cntxt, &cell, b_new ? 0 : 1, NULL)) STACK_ERROR;
					}
					else
					{
						/* Add relation in imported object if direct relation */
						cell.txt = dyntab_val(&relobj, 0, 0);
						cell.len = dyntab_sz(&relobj, 0, 0);
						cell.Num = k + 1;
						DYNTAB_ADD_CELLP(&newval, newval.nbrows, 0, &cell);
					}
				}
			}

			/* Update field values in object */
			if((!fields[j].id_relfld && !fields[j].i_rel || 
				strcmp("_EVA_RELREVERSE", dyntab_val(reltype, fields[j].i_rel, 0))) &&
				qry_update_idobj_idfield(cntxt, idobj, fields[j].id, &newval, b_new ? 0 : 2)) STACK_ERROR;
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : import_handle_idlist
** Description : handle corespondance list between old & new objects ids during import
*********************************************************************/
#define ERR_FUNCTION "import_handle_idlist"
#define ERR_CLEANUP	DYNTAB_FREE(id_obj)
int import_handle_idlist(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTableCell *id_old,			/* in : old object id */
	DynTable *id_list,				/* in/out : list of imported objects ids */
	unsigned long *i_list			/* out : row of given object in id_list (create if needed) */
){
	DynTable id_obj = {0};
	unsigned long i;

	/* Search object id in objects table */
	for(i = id_list->nbrows; i && strcmp(dyntab_val(id_list, i, 0), id_old->txt); i--);

	/* If id not found */
	if(strcmp(dyntab_val(id_list, i, 0), id_old->txt))
	{
		/* Create a new object & add old / new ids with pkey to id_list table */
		i = id_list->nbrows;
		if(qry_add_new_obj(cntxt, &id_obj, 0)) STACK_ERROR;
		DYNTAB_ADD_CELLP(id_list, i, 0, id_old);
		DYNTAB_ADD_CELL(id_list, i, 1, &id_obj, 0, 0);
	}

	*i_list = i;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : import_objdata_check
** Description : search for existing objects & build correspondance table
*********************************************************************/
#define ERR_FUNCTION "import_objdata_check"
#define ERR_CLEANUP	DYNTAB_FREE(sqlres); \
					DYNTAB_FREE(curobjdata); \
					DYNTAB_FREE(dbobjdata); \
					DYNTAB_FREE(value); \
					qry_build_free(&flt); \
					M_FREE(sql)
int import_objdata_check(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *objdata,				/* in : object data to import */
	DynTable *id_list				/* in/out : IdObj correspondance table */
){
	DynTable sqlres = {0};
	DynTable curobjdata = {0};
	DynTable dbobjdata = {0};
	DynTable value = {0};
	QryBuild flt = {0};
	DynBuffer *sql = NULL;
	unsigned long i, j, k, cnt = 0;
	int b_next = 0;
	DynTableCell *val, *c;

	/* Allow big selects */
	if(sql_exec_query(cntxt, "SET SESSION max_join_size=DEFAULT")) STACK_ERROR;

	/* Search for existing objects in db (except first obj) */
	if(objdata->nbrows <= 2) RETURN_OK;
	for(i = objdata->nbrows - 1, k = i; dyntab_cmp(objdata, 1, 0, objdata, i + 1, 0); i--)
	{
		MatchMode mode = Like;

		/* Display progress bar */
		while(cnt <= 60 * (objdata->nbrows - i) / objdata->nbrows) { printf("*"); fflush(stdout); cnt++; }

		/* If obj different from previous line */
		if((b_next || curobjdata.nbrows) && dyntab_cmp(objdata, i, 0, objdata, i + 1, 0))
		{
			/* If search is relevant */
			val = dyntab_cell(objdata, k, 0);
			if(val && !b_next)
			{
				/* Search for object with same values */
				int exactsamecnt = 0;
				M_FREE(sql);
				DYNTAB_FREE(sqlres);
				flt.b_straightjoin = 1;
				if(qry_build_clauses(cntxt, &flt, 0) ||
					qry_build_flt_select(cntxt, &sql, &sqlres, &flt, 0, 0) ||
					sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &sqlres, 2))
					STACK_ERROR;

				/* Check for values count */
				M_FREE(sql);
				DYNBUF_ADD_STR(&sql,
					"SELECT IdObj,COUNT(*) AS NbVal FROM TLink\n"
					"WHERE DateDel IS NULL AND IdObj IN( ");
				if(qry_values_list(cntxt, &sqlres, 0, &sql)) STACK_ERROR;
				DYNBUF_ADD3_INT(&sql, ")\n"
					"GROUP BY IdObj HAVING NbVal=", k - i, " ORDER BY IdObj DESC");
				if(sql_exec_query(cntxt, sql->data) || sql_get_table(cntxt, &sqlres, 2))
					STACK_ERROR;

				/* Look for exactly same object */
				dyntab_sort(&curobjdata, qsort_compare_values);
				for(j = 0; j < sqlres.nbrows; j++)
				{
					unsigned long r;
					if(qry_obj_field(cntxt, &dbobjdata, DYNTAB_TOULRC(&sqlres, j, 0), NULL)) STACK_ERROR;
					dyntab_sort(&dbobjdata, qsort_compare_values);
					for(r = 0; r < dbobjdata.nbrows; r++)
					{
						DynTableCell *c1 = dyntab_cell(&curobjdata, r, 0);
						DynTableCell *c2 = dyntab_cell(&dbobjdata, r, 0);
						if(c1->Num != c2->Num || c1->Line != c2->Line || 
							STRCMPNUL(c1->txt, c2->txt) || STRCMPNUL(c1->field, c2->field))
							break;
					}
					if(r == dbobjdata.nbrows)
					{
						/* Same object : set as used obj if none & increment counter */
						if(j && dyntab_sz(&sqlres, 0, 0)) DYNTAB_SET_CELL(&sqlres, 0, 0, &sqlres, j, 0);
						exactsamecnt++;
					}
					else
						DYNTAB_SET_CELLP(&sqlres, j, 0, NULL);
				}

				/* If object(s) found : add most recent to id_list */
				if(exactsamecnt)
				{
					unsigned long row = id_list->nbrows;
					DYNTAB_SET_CELL(id_list, row, 0, objdata, i + 1, 0);
					DYNTAB_ADD_CELL(id_list, row, 1, &sqlres, 0, 0);
					val->col = exactsamecnt > 1 ? 4 : 3;
					dyntab_cell(id_list, row, 1)->col = k;
				}
				else
					val->col = 2;
			}
			else if(val && !val->col) val->col = 1;

			/* Initialize for next object - skip if already processed */
			k = i;
			qry_build_free(&flt);
			val = dyntab_cell(objdata, i, 0);
			b_next = val->col > 1;
			dyntab_free(&curobjdata);
		}

		/* Do not process until next object if required */
		if(b_next) continue;

		/* Store object value */
		DYNTAB_SET_CELL(&curobjdata, curobjdata.nbrows, 0, objdata, i, 2);
		c = dyntab_cell(&curobjdata, curobjdata.nbrows - 1, 0);
		c->field = dyntab_val(objdata, i, 1);

		/* If value is a relation */
		if(!strcmp("R", dyntab_val(objdata, i, 3)))
		{
			/* Search related object id in objects table */
			for(j = 0; j < id_list->nbrows && dyntab_cmp(id_list, j, 0, objdata, i, 2); j++);
			if(j >= id_list->nbrows)
			{
				/* Related object not found : object is different - skip search */
				b_next = 1;
				continue;
			}
			mode = RelList;
			DYNTAB_SET_CELL(&value, 0, 0, id_list, j, 1);
			c->b_relation = 1;
		}
		else
			DYNTAB_SET_CELL(&value, 0, 0, objdata, i, 2);

		/* Store object value */
		c->txt = value.cell->txt;
		c->len = value.cell->len;
		c->Num = DYNTAB_TOULRC(objdata, i, 4);
		c->Line = DYNTAB_TOULRC(objdata, i, 5);

		/* Add up to 29 filter conditions (32 tables max in MySQL) */
		if(flt.nbnode < 29)
		{
			QryBuilNode *node = flt.node + flt.nbnode;
			unsigned long join = flt.nbnode;
			char printbuf[128];
			DYNTAB_SET_CELL(&node->field, node->field.nbrows, 0, objdata, i, 1);
			DYNTAB_SET_CELL(&node->value, node->value.nbrows, 0, &value, 0, 0);
			dynbuf_print4(&flt.where, "%sTLink%ld.Num=%ld AND TLink%ld.Line", flt.where ? " AND " : "",
									join, c->Num, join);
			if(c->Line)
				dynbuf_print(&flt.where, "=%ld", c->Line);
			else
				DYNBUF_ADD_STR(&flt.where, " IS NULL");
			node->rel = RelNone;
			node->match = mode;
			node->srcjoin = 0;
			flt.nbnode++;
		}
	}
	while(cnt <= 60) { printf("*"); fflush(stdout); cnt++; }

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : cgi_select_files
** Description : select / download files with a CGI dialog
*********************************************************************/
#define ERR_FUNCTION "cgi_select_files"
#define ERR_CLEANUP	M_FREE(buf); \
					M_FREE(name); \
					DYNTAB_FREE(libfiles); \
					DYNTAB_FREE(cgival)
int cgi_select_files(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	DynTable *files					/* in :list of files to import */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *buf = NULL;
	DynBuffer *name = NULL;
	DynTable libfiles = {0};
	DynTable cgival = {0};
	DynBuffer **html;
	char *btn = CGI_CLICK_BTN_SUBFIELD;
	unsigned long i;
	size_t szmax = 0;

	/* Read file selection status */
	dyntab_free(files);
	CTRL_GET_CGI_SUBFIELD("SAVETO=objlib");
	if(!cgival.nbrows || !cgival.cell || !cgival.cell->len) CTRL_GET_CGI_SUBFIELD("SELFILES");
	if(find_files(cntxt, &libfiles, "*.txt", IncludeInfos)) CLEAR_ERROR;
	for(i = 0; i < libfiles.nbrows; i++) if(dyntab_sz(&libfiles, i, 1) > szmax) szmax = dyntab_sz(&libfiles, i, 1);

	/* Handle Import button click - transfer selected file to result & return */
	if(!strcmp(btn, "IMPORT"))
	{
		DYNTAB_ADD_CELL(files, 0, 0, &cgival, 0, 0);
		RETURN_OK;
	}

	/* Output page header */
	cntxt->cgiencodemultipart = 1;
	put_html_page_header(cntxt, NULL, "Importer des données", NULL, 3);
	cntxt->form->html = &buf;
	html = &buf;
	cntxt->b_terminate = 15;
	fflush(stdout);
	DYNBUF_ADD_STR(html, "<table noborder>");

	/* Output file upload input & button */
	DYNBUF_ADD_STR(html, "<tr><td align=right>Télécharger un fichier</td><td>");
	CTRL_CGINAMESUBFIELD(&name, NULL, "SAVETO=objlib");
	DYNBUF_ADD3_BUF(form->html, "<input type=file name='", name, NO_CONV, "'");
	DYNBUF_ADD3_INT(form->html, " size=", szmax > 40 ? szmax - 20 : 20, "></td><td>");
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("UPLOAD"));
	if(put_html_button(cntxt, name->data, "Télécharger", "_eva_download.gif", "_eva_download_s.gif",
		"Sélectionnez un fichier avec le bouton 'Parcourir ...' et cliquez sur ce bouton pour le transférer vers le serveur", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td></tr>");

	/* Output files list */
	if(libfiles.nbrows)
	{
		DYNBUF_ADD_STR(html, "<tr><td align=right>Importer depuis la bibliothèque</td><td>");
		DYNTAB_ADD_CELLP(&libfiles, libfiles.nbrows, 0, NULL);
		dyntab_sort(&libfiles, qsort_col0);
		CTRL_CGINAMESUBFIELD(&name, NULL, "SELFILES");
		if(put_html_list(cntxt, name, &libfiles, &cgival, 1, 0, 0, 0, 0, NULL)) STACK_ERROR;
	}

	/* Output import & goback buttons */
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("IMPORT"));
	DYNBUF_ADD_STR(html, "</td><td>");
	if(put_html_button(cntxt, name->data, "Importer", "_eva_import.gif", "_eva_import_s.gif", "Lance l'import du fichier sélectionné", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td></tr></table><br><hr><br>");
	if(put_html_button(cntxt, "NOP", "Revenir", "_eva_btn_gobackobj_fr.gif", "_eva_btn_gobackobj_fr_s.gif", "Retourne à la page précédente", 0, 0)) STACK_ERROR;

	/* Output page footer */
	printf("<font face=Arial><center><b>Importer des données</b><hr><br>%s<br><br></font>", buf->data);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : import_objdata_file
** Description : import data from an object records file 
*********************************************************************/
#define ERR_FUNCTION "import_objdata_file"
#define ERR_CLEANUP	M_FREE(name); \
					M_FREE(buf); \
					DYNTAB_FREE(objdata); \
					DYNTAB_FREE(forms); \
					DYNTAB_FREE(sqlres); \
					DYNTAB_FREE(id_list); \
					DYNTAB_FREE(objFORM); \
					DYNTAB_FREE(newform)
int import_objdata_file(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	DynTable *files					/* in :list of files to import */
){
	EVA_form *form = cntxt->form;
	DynBuffer *buf = NULL;
	DynBuffer *name = NULL;
	DynBuffer **html;
	DynTable objdata = {0};
	DynTable forms = {0};
	DynTable sqlres = {0};
	DynTable id_list = {0};
	DynTable objFORM = {0};
	DynTable newform = {0};
 	char qry[1024];
	unsigned long i = i_ctrl, j, k, k0, idobj = 0, newobj = 0;
	unsigned long val_LABEL, val_CONTROL, val_FORM, label;

	/* Check params */
	if(!files) return 0;

	/* Read selected files if absent - return if none selected */
	if(chdir(cntxt->rootdir) || chdir("objlib")) RETURN_ERR_DIRECTORY(1);
	if(!files->nbrows && cgi_select_files(cntxt, i_ctrl, files)) STACK_ERROR;
	if (!files->nbrows) RETURN_OK;

	/* Read object data & value Ids */
	if(file_read_import_data(cntxt, &forms, &objdata, dyntab_val(files, 0, 0)) ||
		sql_id_value(cntxt, add_sz_str("_EVA_LABEL"), &val_LABEL) ||
		sql_id_value(cntxt, add_sz_str("_EVA_CONTROL"), &val_CONTROL) ||
		sql_id_value(cntxt, add_sz_str("_EVA_FORM"), &val_FORM)) STACK_ERROR;
	DYNTAB_ADD_INT(&objFORM, 0, 0, OBJ_FORM_CONTROL);

	/* Output page header */
	put_html_page_header(cntxt, NULL, "Importer des données", NULL, 3);
	cntxt->form->html = &buf;
	html = &buf;
	cntxt->b_terminate = 15;
	printf("<font face=Arial><center><b>Importer des données : %s</b></center><hr>", dyntab_val(files, 0, 0));
	fflush(stdout);

	/* Search for referenced forms in db */
	for(i = 1; i < forms.nbrows; i++)
	{
		unsigned long row = id_list.nbrows;

		/* Build query for form with given label */
		if(sql_id_value(cntxt, DYNTAB_VAL_SZ(&forms, i, 1), &label)) STACK_ERROR;
		sprintf(qry,
			"SELECT DISTINCT TLink0.IdObj \n"
			"FROM TLink AS TLink0,TLink AS TLink1,TLink AS TLink2 \n"
			"WHERE TLink0.IdObj=TLink1.IdObj AND TLink0.IdObj=TLink2.IdObj \n"
				"AND TLink0.DateDel IS NULL AND TLink0.IdField=%lu AND TLink0.IdRelObj=%lu \n"
				"AND TLink1.DateDel IS NULL AND TLink1.IdField=%lu AND TLink1.IdValue=%lu \n"
				"AND TLink2.DateDel IS NULL AND TLink2.IdField=%lu AND TLink2.IdValue=%lu",
				cntxt->val_FORMSTAMP, OBJ_FORM_CONTROL, val_CONTROL, val_FORM, val_LABEL, label);
		if(sql_exec_query(cntxt, qry) || sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;
		if(sqlres.nbrows == 1)
		{
			/* Store form Id if form found */
			DYNTAB_SET_CELL(&id_list, row, 0, &forms, i, 0);
			DYNTAB_ADD_CELL(&id_list, row, 1, &sqlres, 0, 0);
		}
		else if(sqlres.nbrows == 0)
		{
			/* Create new form with given label if not found */
			DYNTAB_SET(&sqlres, 0, 0, "_EVA_CONTROL");
			DYNTAB_SET(&sqlres, 0, 1, "_EVA_FORM");
			DYNTAB_SET(&sqlres, 1, 0, "_EVA_LABEL");
			DYNTAB_ADD_CELL(&sqlres, 1, 1, &forms, i, 1);
			DYNTAB_FREE(newform);
			if(qry_add_obj_data(cntxt, &newform, &sqlres, &objFORM, 0)) STACK_ERROR;

			/* Store new form Id */
			DYNTAB_SET_CELL(&id_list, row, 0, &forms, i, 0);
			DYNTAB_ADD_CELL(&id_list, row, 1, &newform, 0, 0);
		}
		else
		{
			RETURN_ERROR("Référence à un formulaire non unique", 
						ERR_PUT_TXT("Formulaire : ", dyntab_val(&forms, i, 1), dyntab_sz(&forms, i, 1)));
		}
	}

	/* Set first object id as current id if defined */
	if(dyntab_sz(&form->id_obj, 0, 0))
	{
		unsigned long row = id_list.nbrows;
		DYNTAB_SET_CELL(&id_list, row, 0, &objdata, 1, 0);
		DYNTAB_SET_CELL(&id_list, row, 1, &form->id_obj, 0, 0);
	}

	/* Search for existing objects in db */
	printf( "<font face=Courier><b><pre>"
			"Recherche des fiches existantes\n%s", WAIT_TXTSCALE);
	for(i = 0, k = objdata.nbrows, k0 = 0; i < 50 && k && k != k0; i++)
	{
		if(import_objdata_check(cntxt, &objdata, &id_list)) STACK_ERROR;
		for(j = 1, k0 = k, k = 0; j < objdata.nbrows; j++)
			if(dyntab_cell(&objdata, j, 0)->col == 1) k++; 
		printf(" (%lu:%lu)\n", i, k);
	}
	printf("</pre></font>");

	/* Output file contents summary */
	DYNBUF_ADD_STR(html, "<table border><tr><td colspan=6 align=center><b>Contenu du fichier importé</td></tr>"
						"<tr><td><b>Formulaire</td><td><b>Fiches</td><td>Indet</td><td>Nouv</td><td>Exist</td><td>Dbl</td></tr>");
	for(i = 1; i < forms.nbrows; i++)
	{
		unsigned long cnt[5] = {0}, status = dyntab_sz(&form->id_obj, 0, 0) ? 3 : 2;
		DYNBUF_ADD3_CELL(html, "<tr><td>", &forms, i, 1, NO_CONV, "</td>");
		for(j = 1; j < objdata.nbrows; j++) 
		{
			DynTableCell *val = dyntab_cell(&objdata, j, 0);
			/* val->col values mean :
				1: unable to perform search - reference to objects no yet searched for
				2: object does not exist
				3: object exist and is unique
				4: object exist with multiple instance */
			if(val->col) status = val->col;
			if(!strcmp("_EVA_FORMSTAMP", dyntab_val(&objdata,j, 1)) && 
				!dyntab_cmp(&forms, i, 0, &objdata,j, 2))
			{
				cnt[0]++;
				cnt[status ? status : 2]++;
			}
		}
		for(j = 0; j < 5; j++) 
			DYNBUF_ADD3_INT(html, "<td align=center>", cnt[j], "</td>");
		DYNBUF_ADD_STR(html, "</tr>");
	}
	DYNBUF_ADD_STR(html, "<tr><td align=right><b>Total</td>");
	{
		unsigned long cnt[5] = {0}, status = dyntab_sz(&form->id_obj, 0, 0) ? 3 : 2;
		for(j = 1; j < objdata.nbrows; j++) 
		{
			DynTableCell *val = dyntab_cell(&objdata, j, 0);
			if(val->col) status = val->col;
			if(j && dyntab_cmp(&objdata, j - 1, 0, &objdata,j, 0))
			{
				cnt[0]++;
				cnt[status ? status : 2]++;
			}
		}
		for(j = 0; j < 5; j++) 
			DYNBUF_ADD3_INT(html, "<td align=center><b>", cnt[j], "</td>");
	}
	DYNBUF_ADD_STR(html, "</table>");
	puts(buf->data);
	buf->cnt = 0;
	fflush(stdout);

	/* Clear current object if defined */
	k0 = 0;
	if(dyntab_sz(&form->id_obj, 0, 0))
	{
		sprintf(qry, "UPDATE TLink SET DateDel='%s', IdWhoDel=%s WHERE IdObj=%s AND DateDel IS NULL",
				cntxt->timestamp, dyntab_val(&cntxt->id_user, 0, 0), dyntab_val(&form->id_obj, 0, 0));
		if(sql_exec_query(cntxt, qry)) STACK_ERROR;
	}
	else k0 = 1;

	/* Loop on objects data */
	printf("<font face=Courier><b><pre>Import des données\n%s", WAIT_TXTSCALE);
	fflush(stdout);
	for(i = 1, k = 0; i < objdata.nbrows; i++)
	{
		unsigned long pkey = 0, id = DYNTAB_TOULRC(&objdata, i, 0);
		DynTableCell cell = {0};

		/* Handle progress bar */
		while(k <= 60 * i / objdata.nbrows) { printf("*"); fflush(stdout); k++; }

		/* If new object */
		if(id != idobj)
		{
			/* Search object id in id_list table */
			DynTableCell *val;
			if(import_handle_idlist(cntxt, dyntab_cell(&objdata, i, 0), &id_list, &j)) STACK_ERROR;
			val = dyntab_cell(&id_list, j, 1);
			idobj = id;

			/* Skip object if found in DB */
			if(val->col)
			{
				i = val->col;
				continue;
			}
			/* Use empty record created for object if applicable */
			else if(val->Pkey)
			{
				/* Set pkey to object record & clear pkey in table */
				pkey = val->Pkey;
				val->Pkey = 0;
			}
			newobj = strtoul(val->txt, NULL, 10);
			if(i > 1) k0++;
		}

		/* Prepare new value for object & add to db */
		cell.IdObj = newobj;
		cell.field = dyntab_val(&objdata, i, 1);
		cell.Num = atoi(dyntab_val(&objdata, i, 4));
		cell.Line = atoi(dyntab_val(&objdata, i, 5));

		/* If value is a relation */
		if(*dyntab_val(&objdata, i, 3) == 'R')
		{
			/* Relation to an imported object : set value to new id in id_list table */
			if(import_handle_idlist(cntxt, dyntab_cell(&objdata, i, 2), &id_list, &j)) STACK_ERROR;
			cell.txt = dyntab_val(&id_list, j, 1);
			cell.len = dyntab_sz(&id_list, j, 1);
			cell.b_relation = 1;
		}
		else
		{
			/* Plain value : set text */
			cell.txt = dyntab_val(&objdata, i, 2);
			cell.len = dyntab_sz(&objdata, i, 2);
		}

		/* Add value to db */
		if(qry_add_val(cntxt, &cell, 2, &pkey)) STACK_ERROR;
	}

	/* Output footer */
	while(k <= 60) { printf("*"); fflush(stdout); k++; }
	printf("</pre></font><br><b><u>Nombre de fiches créées : %lu</u></b><br><br>", k0);
	DYNBUF_ADD_STR(html, "<table noboder width=100%><tr><td align=center>");
	if(put_html_button(cntxt, "NOP", "Revenir", "_eva_btn_gobackobj_fr.gif", "_eva_btn_gobackobj_fr_s.gif",
							"Retourne à la page précédente", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td><td align=center>");
	DYNBUF_ADD3_CELL(&name, "B$#.DELOBJLIB=", files, 0, 0, HTML_NO_QUOTE, "$1");
	if(put_html_button(cntxt, name->data, "Supprimer le fichier", "_eva_btn_delfile_fr.gif", "_eva_btn_delfile_fr_s.gif",
							"Supprimer le fichier qui vient d'être importé", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</td></tr></table><br>");
	puts(buf->data);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_import
** Description : button click handler for IMPORT
*********************************************************************/
#define ERR_FUNCTION "action_import"
#define ERR_CLEANUP	DYNTAB_FREE(file); \
					DYNTAB_FREE(formdest); \
					DYNTAB_FREE(keyfields); \
					DYNTAB_FREE(multvalsep); \
					DYNTAB_FREE(reltype); \
					DYNTAB_FREE(relfldsrc); \
					DYNTAB_FREE(relflddest); \
					DYNTAB_FREE(relformdest); \
					DYNTAB_FREE(data); \
					M_FREE(name); \
					M_FREE(sql)
int action_import(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	DynTable file = { 0 };
	DynTable formdest = { 0 };
	DynTable keyfields = { 0 };
	DynTable multvalsep = { 0 };
	DynTable reltype = { 0 };
	DynTable relfldsrc = { 0 };
	DynTable relflddest = { 0 };
	DynTable relformdest = { 0 };
	DynTable data = { 0 };
	DynBuffer *name = NULL;
	DynBuffer *sql = NULL;
	int b_clear = DYNTAB_FIELD_CELL(&form->objdata, CLEAR_FORM) != NULL;
	char *btn = CGI_CLICK_BTN_SUBFIELD;

	/* Read import basic parameters */
	if(form_get_field_values(cntxt, &file, "_EVA_FILE", 0, 0)) STACK_ERROR;
	DYNTAB_FIELD(&formdest, &form->objdata, FORM);

	/* If flat file import */
	if(formdest.nbrows && file.nbrows)
	{
		char *filename = dyntab_val(&file, 0, 0);
		put_html_page_header(cntxt, NULL, "Import de données", NULL, 3);
		printf("<font face=Arial><b>Import du fichier %s</b><br>", filename);
		fflush(stdout);
		if(b_clear)
		{
			/* Search existing objects with destination formstamp */
			char qry[1024];
			printf("Préparation à l'effacement des fiches ...");
			fflush(stdout);
			sprintf(qry,
				"SELECT DISTINCT IdObj FROM TLINK WHERE IdField=%lu AND IdRelObj=%s", 
						cntxt->val_FORMSTAMP, dyntab_val(&formdest, 0, 0));
			if(sql_exec_query(cntxt, qry)) STACK_ERROR;

			/* Ask for user confirmation if applicable */
			if(cntxt->sql_nbrows && strcmp(btn, "DELCONFIRM"))
			{
			}
			printf(" %lu fiches trouvées<br>", cntxt->sql_nbrows);
			fflush(stdout);

			/* Clear existing objects with destination formstamp */
			if(cntxt->sql_nbrows)
			{
				printf(" Effacement ...");
				fflush(stdout);
				DYNBUF_ADD_STR(&sql,
					"DELETE FROM TLink WHERE TLink.IdObj IN (");
				if(sql_get_table(cntxt, &data, 2) || qry_values_list(cntxt, &data, 0, &sql)) STACK_ERROR;
				DYNBUF_ADD_STR(&sql, ") OR TLink.IdRelObj IN (");
				if(qry_values_list(cntxt, &data, 0, &sql)) STACK_ERROR;
				DYNBUF_ADD_STR(&sql, ")");
				if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
				printf(" terminé<br>");
				fflush(stdout);
			}
		}

		/* Read import parameters */
		DYNTAB_FIELD(&multvalsep, &form->objdata, MULTIVAL_SEP);
		DYNTAB_FIELD(&keyfields, &form->objdata, KEY_FIELDS);
		DYNTAB_FIELD_TAB(&reltype, &form->objdata, RELTYPE);
		DYNTAB_FIELD_TAB(&relfldsrc, &form->objdata, RELFIELD_SRC);
		DYNTAB_FIELD_TAB(&relflddest, &form->objdata, RELFIELD_DEST);
		DYNTAB_FIELD_TAB(&relformdest, &form->objdata, RELFORM);

		/* Read & import data */
		if(file_read_tabrc(cntxt, &data, filename) ||
			import_tabrc_file(cntxt, &data, &formdest, &keyfields, &multvalsep, &reltype, &relfldsrc, &relflddest, &relformdest))
			STACK_ERROR;

		cntxt->b_terminate = 15;
	}
	else 
	{
		/* Import EVA controls format file */
		if(import_objdata_file(cntxt, i_ctrl, &file)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
