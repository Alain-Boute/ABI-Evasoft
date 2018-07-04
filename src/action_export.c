/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_export.c
** Description : handling functions for export button
**      Author : Alain BOUTE
**     Created : Sept 8 2005
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : action_export_triplets
** Description : export data in triplets format for objects in table IdList
*********************************************************************/
#define ERR_FUNCTION "action_export_triplets"
#define ERR_CLEANUP	DYNTAB_FREE(sqlres); \
					M_FREE(forms); \
					M_FREE(qry)
int action_export_triplets(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	FILE *f							/* in/out : output file */
){
	DynTable sqlres = {0};
	DynBuffer *forms = NULL;
	DynBuffer *qry = NULL;
	unsigned long level = 0, nb_cur = 1, nb_prev = 0;
	char printbuf[2048];

	/* Build list of forms */
	if(sql_exec_query(cntxt, "SELECT DISTINCT IdObj FROM IdList") || sql_get_table(cntxt, &sqlres, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(&qry, 
		"-- action_export : build forms list\n"
		"SELECT TLink.IdObj FROM TLink \n"
		"INNER JOIN TLink AS TLink0 ON TLink.IdObj=TLink0.IdObj \n"
		"WHERE TLink.IdObj NOT IN (");
	if(qry_values_list(cntxt, &sqlres, 0, &qry)) STACK_ERROR;
	dynbuf_print4(&qry, ") AND TLink.DateDel IS NULL AND TLink.IdField=%lu AND TLink.IdRelObj=%lu \n"
		"AND TLink0.DateDel IS NULL AND TLink0.IdField=%lu AND TLink0.IdValue=%lu \n"
		"GROUP BY IdObj", 
		IDVALUE(FORMSTAMP), OBJ_FORM_CONTROL, IDVALUE(CONTROL), IDVALUE(FORM));
	if(sql_exec_query(cntxt, qry->data) || sql_get_table(cntxt, &sqlres, 2) ||
		qry_values_list(cntxt, &sqlres, 0, &forms)) STACK_ERROR;


	/* Read objects children */
	while(nb_cur > nb_prev)
	{
		level++;
		M_FREE(qry);
		DYNBUF_ADD3_INT(&qry, 
			"-- action_export : get child ids - level=", level, "\n");
		DYNBUF_ADD3_INT(&qry, 
			"CREATE TEMPORARY TABLE IdListTmp TYPE=HEAP \n" 
			"SELECT TLink.IdRelObj, ", level, " AS Lvl \n" 
			"FROM TLink \n"
			"INNER JOIN IdList ON TLink.IdObj=IdList.IdObj \n"); 
		DYNBUF_ADD3_INT(&qry, 
			"WHERE IdList.Lvl=", level - 1, " AND TLink.DateDel IS NULL AND TLink.IdRelObj IS NOT NULL \n"); 
		DYNBUF_ADD3_BUF(&qry, 
			"AND TLink.IdRelObj NOT IN (", forms, NO_CONV,") \n" 
			"GROUP BY TLink.IdRelObj");
		if(sql_exec_query(cntxt, qry->data) ||
			sql_exec_query(cntxt,
				"INSERT IGNORE IdList SELECT * FROM IdListTmp")) STACK_ERROR;
		if(sql_exec_query(cntxt, "DROP TABLE IdListTmp") || 
			sql_exec_query(cntxt, "SELECT COUNT(*) FROM IdList") || 
			sql_get_table(cntxt, &sqlres, 2)) STACK_ERROR;
		nb_prev = nb_cur;
		nb_cur = DYNTAB_TOUL(&sqlres);
	}

	/* Output used forms ids & labels */
	M_FREE(qry);
	DYNBUF_ADD3_BUF(&qry, 
		"SELECT TLink0.IdObj AS IdObj,TVal.TxtValue AS Label \n" 
		"FROM TLink \n"
		"INNER JOIN IdList ON TLink.IdObj=IdList.IdObj \n" 
		"INNER JOIN TLink AS TLink0 ON TLink.IdRelObj=TLink0.IdObj \n"
		"INNER JOIN TVal as TField ON TLink0.IdField=TField.IdValue \n"
		"INNER JOIN TVal ON TLink0.IdValue=TVal.IdValue \n"
		"WHERE TField.TxtValue='_EVA_LABEL' AND TLink.DateDel IS NULL AND TLink0.DateDel IS NULL \n"
		"AND TLink.IdRelObj IN (", forms, NO_CONV,") \n" 
		"GROUP BY TLink0.IdObj,TVal.TxtValue \n" 
		"ORDER BY IdObj");
	if(sql_exec_query(cntxt, qry->data) ||
		sql_get_table(cntxt, &sqlres, 3) ||
		FILE_write_tabrc(cntxt, &sqlres, f)) STACK_ERROR;
	fprintf(f, "\n");

	/* Read objects data & write to output file */
	if(sql_exec_query(cntxt,
		"-- action_export : read objects data\n"
		"SELECT DISTINCT TLink.IdObj AS IdObj,"
			"TField.TxtValue AS Field,"
			"IF(TVal.TxtValue IS NULL,TLink.IdRelObj,TVal.TxtValue) AS Value," 
			"IF(TLink.IdRelObj IS NULL,'V','R') AS Type,"
			"TLink.Num,TLink.Line \n" 
		"FROM TLink INNER JOIN IdList ON TLink.IdObj=IdList.IdObj \n" 
		"INNER JOIN TVal as TField ON TLink.IdField=TField.IdValue \n"
		"LEFT JOIN TVal ON TLink.IdValue=TVal.IdValue \n"
		"WHERE TLink.DateDel IS NULL \n"
		"ORDER BY IdList.Lvl,TLink.IdObj,TLink.IdField,TLink.Line,TLink.Num") ||
		sql_get_table(cntxt, &sqlres, 3) ||
		FILE_write_tabrc(cntxt, &sqlres, f) ||
		sql_exec_query(cntxt, "DROP TABLE IdList")) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_export
** Description : button click handler for EXPORT
*********************************************************************/
#define ERR_FUNCTION "action_export"
#define ERR_CLEANUP	DYNTAB_FREE(forms); \
					DYNTAB_FREE(filter); \
					M_FREE(formname); \
					M_FREE(objname); \
					M_FREE(name); \
					M_FREE(fname); \
					qry_build_free(&flt); \
					if(f) fclose(f)
int action_export(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable forms = {0};
	DynTable filter = {0};
	DynBuffer *formname = NULL;
	DynBuffer *objname = NULL;
	DynBuffer *name = NULL;
	DynBuffer *fname = NULL;
	DynBuffer **html;
	QryBuild flt = {0};
	FILE *f = NULL;

	/* Create a temporary heap table to hold selected objects */
	if(sql_exec_query(cntxt, "-- action_export : create objects Ids table\n"
		"CREATE TEMPORARY TABLE IdList (IdObj INT NOT NULL, Lvl INT, PRIMARY KEY (IdObj)) TYPE=HEAP")) STACK_ERROR;

	/* Handle objects selection */
	CTRL_ATTR(filter, EXPORT_FILTER);
	if(filter.nbrows)
	{
		/* Create table IdList from filters */
		DYNBUF_ADD_STR(&flt.optselect, "0 AS Lvl");
		if(qry_add_filter_forms(cntxt, &flt, NULL, &filter) ||
			qry_filter_table(cntxt, "IdList", &flt, 1)) CLEAR_ERROR_RETURN;
	}
	else if(dyntab_sz(&form->id_obj, 0, 0))
	{
		/* Insert current object in table */
		char sql[128];
		sprintf(sql, 
			"INSERT IGNORE INTO IdList (IdObj,Lvl) VALUES (%lu, 0)", DYNTAB_TOUL(&form->id_obj));
		if(sql_exec_query(cntxt, sql)) CLEAR_ERROR;

	}

	/* Handle output file name : DBname + form & object label */
	DYNBUF_ADD(&fname, cntxt->dbname, 0, NO_CONV);
	if(qry_obj_label(cntxt, &formname, NULL, &objname, NULL, NULL, NULL, NULL, NULL, 0, &form->objdata, 0)) CLEAR_ERROR;
	DYNBUF_ADD3_BUF(&fname, "-", formname, NO_CONV, " ");
	DYNBUF_ADD_BUF(&fname, objname, NO_CONV);

	/* Handle file name & location */
	DYNBUF_ADD_BUF(&name, fname, NO_CONV);
	fname->cnt = file_compatible_name(fname->data);
	DYNBUF_ADD_STR(&fname, ".txt");
	if(chdir(cntxt->rootdir)) RETURN_ERR_DIRECTORY(1);
	mkdir("objlib");
	if(chdir("objlib")) RETURN_ERR_DIRECTORY(1);

	/* Open output file */
	f = fopen(fname->data, "w");
	if(!f) RETURN_ERROR("Impossible d'écrire dans le fichier", NULL);

	/* Export selected objects in triplets format */
	if(action_export_triplets(cntxt, f)) STACK_ERROR;
	fclose(f); f = NULL;

	/* Output link to result file */
	put_html_page_header(cntxt, NULL, "Exporter des données", NULL, 3);
	cntxt->form->html = &cntxt->html;
	html = cntxt->form->html;
	cntxt->b_terminate = 15;
	DYNBUF_ADD3_BUF(html, "<font face=Arial><center><font size=+1><b>", formname, TO_HTML, " - ");
	DYNBUF_ADD_BUF(html, objname, TO_HTML);
	DYNBUF_ADD_STR(html, "</font><br><br>"
			"Export de données terminé avec succès</b><hr><br><br>");
	if(file_output_link(cntxt, html, 
		add_sz_str("Fichier exporté : "), DYNBUF_VAL_SZ(name), DYNBUF_VAL_SZ(fname), "objlib",
		add_sz_str("Fichier texte au format export EVA4"), NULL, 0, 7 | (form->b_modified ? 0 : 8)))
		STACK_ERROR;
	DYNBUF_ADD_STR(html, "<br><br>");
	M_FREE(name);
	DYNBUF_ADD3_BUF(&name, "B$#.DELOBJLIB=", fname, NO_CONV, "$1");
	if(put_html_button(cntxt, name->data, "Supprimer le fichier", "_eva_btn_delfile_fr.gif", "_eva_btn_delfile_fr_s.gif",
							"Attendez que le téléchargement soit terminé pour supprimer le fichier", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "<br><br>"
						"Si vous n'utilisez pas le bouton 'Supprimer le fichier',<br>"
						"il sera visible par les utilisateurs pouvant importer des données<br><br>");
	DYNBUF_ADD_STR(html, "</font><br><br>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
