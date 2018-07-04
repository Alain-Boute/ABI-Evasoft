/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : qry_obj_label.c
** Description : handling functions for object label & symbol
**      Author : Alain BOUTE
**     Created : Feb 26 2002
**********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : ctrl_get_label_img
** Description : return images to symbolize a control
*********************************************************************/
#define ERR_FUNCTION "ctrl_get_label_img"
#define ERR_CLEANUP
int ctrl_get_label_img(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **img,				/* out : symbol normal image */
	DynBuffer **imgsel,				/* out : symbol selected image */
	DynBuffer **label,				/* out : symbol label */
	DynBuffer **notes,				/* out : object notes */
	DynTable *data,					/* in : object data (must contain _EVA_CONTROL & _EVA_TYPE attributes) */
	unsigned long beg 				/* in : first processed lines in data */
){
	char *t_control = dyntab_field_val(data, "_EVA_CONTROL", ~0UL, beg);
	char *t_type = dyntab_field_val(data, "_EVA_TYPE", ~0UL, beg);
	DynTableCell *t_notes = dyntab_field_cell(data, "_EVA_NOTES", ~0UL, beg);
	DynTableCell *t_tnotes = dyntab_field_cell(data, "_EVA_TECHNOTES", ~0UL, beg);
	char *mult = dyntab_field_val(data, "_EVA_MULTIPLE", ~0UL, beg);
	int b_mult = strcmp("No", mult) && *mult;
	unsigned long lines = atoi(dyntab_field_val(data, "_EVA_LINES", ~0UL, beg));

	/* Check params */
	if(!data || !data->nbrows) RETURN_OK;

	/* Store notes */
	if(t_notes) DYNBUF_ADD_CELLP(notes, t_notes, NO_CONV);
	if(t_notes && t_tnotes) DYNBUF_ADD_STR(notes, "\n");
	if(t_tnotes) DYNBUF_ADD_CELLP(notes, t_tnotes, NO_CONV);

#define LABEL_IMAGE(l, i) { DYNBUF_ADD_STR(label, l) DYNBUF_ADD_STR(img, i) }
	/* Switch on type of object */
	if(!strcmp("_EVA_INPUT", t_control))
	{
		if(!strcmp("_EVA_LIST", t_type))
		{
			if(b_mult || lines > 1) LABEL_IMAGE("Choix dans une liste", "_eva_ctrl_list")
			else LABEL_IMAGE("Liste déroulante", "_eva_ctrl_combo")
		}
		else if(!strcmp("_EVA_TEXT", t_type)) LABEL_IMAGE("Saisie texte libre", "_eva_ctrl_input_txt")
		else if(!strcmp("_EVA_EMAIL", t_type)) LABEL_IMAGE("Saisie email", "_eva_ctrl_input_txt")
		else if(!strcmp("_EVA_HTTPLINK", t_type)) LABEL_IMAGE("Saisie adresse internet", "_eva_ctrl_input_txt")
		else if(!strcmp("_EVA_COLOR", t_type)) LABEL_IMAGE("Saisie couleur", "_eva_ctrl_input_color")
		else if(!strcmp("_EVA_FILEUPLOAD", t_type)) LABEL_IMAGE("Fichier téléchargé", "_eva_ctrl_input_fileupload")
		else if(!strcmp("_EVA_DATE", t_type)) LABEL_IMAGE("Saisie date", "_eva_ctrl_input_date")
		else if(!strcmp("_EVA_HOUR", t_type)) LABEL_IMAGE("Saisie heure", "_eva_ctrl_input_nb")
		else if(!strcmp("_EVA_NUMBER", t_type)) LABEL_IMAGE("Saisie nombre", "_eva_ctrl_input_nb")
		else if(!strcmp("_EVA_TELEPHONE", t_type)) LABEL_IMAGE("Saisie téléphone", "_eva_ctrl_input_nb")
		else if(!strcmp("_EVA_FIELD", t_type)) LABEL_IMAGE("Saisie champ de stockage", "_eva_ctrl_field")
		else if(!strcmp("_EVA_CHECKBOX", t_type)) LABEL_IMAGE("Case à cocher", "_eva_ctrl_checkbox")
		else if(!strcmp("_EVA_IMAGE", t_type)) LABEL_IMAGE("Saisie image", "_eva_ctrl_input_image")
		else if(!strcmp("_EVA_RELATION", t_type)) LABEL_IMAGE("Saisie relation", "_eva_ctrl_rel")
		else LABEL_IMAGE("Champ de saisie", "_eva_ctrl_unknown")
	}
	else if(!strcmp("_EVA_CONTAINER", t_control))
	{
		if(!strcmp("_EVA_GROUP", t_type)) LABEL_IMAGE("Cadre de groupe", "_eva_ctrl_group")
		else if(!strcmp("_EVA_TAB", t_type)) LABEL_IMAGE("Onglet", "_eva_ctrl_tab")
		else if(!strcmp("_EVA_TABLE", t_type)) LABEL_IMAGE("Tableau", "_eva_ctrl_table")
		else if(!strcmp("_EVA_PACKAGE", t_type)) LABEL_IMAGE("Groupe à réutiliser", "_eva_ctrl_packg")
		else if(!strcmp("_EVA_TITLE", t_type)) LABEL_IMAGE("Titre", "_eva_ctrl_title")
		else if(!strcmp("_EVA_MENUBAR", t_type)) LABEL_IMAGE("Menu", "_eva_ctrl_menu")
		else LABEL_IMAGE("Conteneur", "_eva_ctrl_unknown")
	}
	else if(!strcmp("_EVA_BUTTON", t_control)) LABEL_IMAGE("Bouton", "_eva_ctrl_butn")
	else if(!strcmp("_EVA_OPTION", t_control)) LABEL_IMAGE("Option", "_eva_ctrl_list_option")
	else if(!strcmp("_EVA_OUTPUT", t_control))
	{
		if(!strcmp("_EVA_LIST_SIMPLE", t_type)) LABEL_IMAGE("Liste de fiches", "_eva_ctrl_srchlist")
		else if(!strcmp("_EVA_STATLIST", t_type)) LABEL_IMAGE("Tableau de statistiques", "_eva_ctrl_pvtbl")
		else if(!strcmp("_EVA_RELTREE", t_type)) LABEL_IMAGE("Arbre de relations", "_eva_ctrl_tree")
		else if(!strcmp("_EVA_CATALOG", t_type)) LABEL_IMAGE("Catalogue", "_eva_ctrl_tree")
		else if(!strcmp("_EVA_CALENDAR", t_type)) LABEL_IMAGE("Calendrier", "_eva_ctrl_calendar")
		else if(!strcmp("_EVA_PLANNING", t_type)) LABEL_IMAGE("Planning", "_eva_ctrl_calendar")
		else LABEL_IMAGE("Liste/tableau", "_eva_ctrl_unknown")
	}
	else if(!strcmp("_EVA_STATICTEXT", t_control)) LABEL_IMAGE("Texte fixe", "_eva_ctrl_txt")
	else if(!strcmp("_EVA_CALC", t_control)) LABEL_IMAGE("Valeur calculée", "_eva_ctrl_calc")
	else if(!strcmp("_EVA_IMAGE", t_control)) LABEL_IMAGE("Image", "_eva_ctrl_image")
	else if(!strcmp("_EVA_ANCHOR", t_control)) LABEL_IMAGE("Adresse internet", "_eva_ctrl_anchor")
	else if(!strcmp("_EVA_FORM", t_control)) LABEL_IMAGE("Formulaire", "_eva_ctrl_form");
#undef LABEL_IMAGE
	
	if(label && !*label)
	{
		DYNBUF_ADD_STR(label, "Contrôle inconnu")
		DYNBUF_ADD_STR(img, "_eva_ctrl_unknown")
	}

	if(img) DYNBUF_ADD3_BUF(imgsel, "", *img, NO_CONV, "_s.gif")
	DYNBUF_ADD_STR(img, ".gif")

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_build_obj_label
** Description : return descriptive labels for an object
*********************************************************************/
#define ERR_FUNCTION "qry_build_obj_label"
#define ERR_CLEANUP DYNTAB_FREE(res); \
					M_FREE(expr)
int qry_build_obj_label(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer **label,					/* out : object label */
	DynTable *values,					/* in/out : table of values for each field (for optimization purposes) */
	FormsCache *fc,						/* in : form data */
	DynTable *dispselect,				/* in : display fields select :
												* process field if row value not empty
												* process non computed values & labels if NULL */
	unsigned long id_obj,				/* in : object id */
	DynTable *data,						/* in : object data */
	unsigned long beg,					/* in : first line for object in data */
	unsigned long strip					/* in : max # of chars for field values */
){
	DynTable res = {0};		    
	DynBuffer *expr = NULL;
	size_t cnt0 = (label && *label) ? (*label)->cnt : 0;
	unsigned long i, j;
	int b_select = dispselect != NULL;

	/* Check params */
	if(!label || !values) RETURN_OK;
	if(b_select)
	{
		for(i = 0; i < dispselect->nbrows && !dyntab_sz(dispselect, i, 0); i++);
		b_select =  i < dispselect->nbrows;
	}

	/* Get fields data for object label */
	for(i = 0; i < fc->fields.nbrows && (!*label || (*label)->cnt < cnt0 + (strip * 10)); i++)	if(!b_select || dyntab_sz(dispselect, i, 0))
	{
		/* If values not already computed for this field */
		DynTableCell *c = dyntab_cell(values, i, 0);
		int b_strip = 0;
		size_t cnt = 0;
		int b_compute = !c || !c->col;
		if(b_compute)
		{
			/* Evaluate field expression - continue on error */
			M_FREE(expr);
			if(qry_eval_sqlexpr_var(cntxt, &expr, dyntab_val(&fc->fields, i, 0), &fc->data, NULL) ||
				form_eval_fieldexpr(cntxt, &res, fc->idform, id_obj, DYNBUF_VAL_SZ(expr), data, beg))
				CLEAR_ERROR_CONT;

			/* Store values in corresponding row */
			for(j = 0; j < (res.nbrows ? res.nbrows : 1); j++) DYNTAB_ADD_CELL(values, i, j, &res, j, 0);
			c = dyntab_cell(values, i, 0);
			c->col = 1;
		}
		else if(!dispselect) continue;
		
		/* Skip if no value */
		if(!dyntab_sz(values, i, 0)) continue;

		/* Add fields separator */
		if(dispselect && *label && (*label)->cnt > cnt0)
		{
			if(i && dyntab_sz(&fc->separ, i - 1, 0)) DYNBUF_ADD_STR(label, " ")
			else DYNBUF_ADD_STR(label, ", ");
		}

		/* Add prefix with label if applicable */
		if((!dispselect || dyntab_sz(&fc->pfx, i, 0)) && dyntab_sz(&fc->labels, i, 0))
		{
			if(!dispselect && label && *label) DYNBUF_ADD_STR(label, "\n");
			DYNBUF_ADD_CELL(label, &fc->labels, i, 0, NO_CONV);
			DYNBUF_ADD_STR(label, " ")
		}

		/* Add formated values to object label */
		for(j = 0; j < values->nbcols && dyntab_sz(values, i, j) && !b_strip; j++)
		{
			if(j) DYNBUF_ADD_STR(label, "+");
			DYNBUF_HANDLE_STRIP(label, strip, cnt, b_strip,
				if(put_value_fmt(cntxt, label, DYNTAB_VAL_SZ(values, i, j), dyntab_val(&fc->format, i, 0))) CLEAR_ERROR);
		}

		/* Add display notes if applicable */
		if(dyntab_sz(&fc->dnotes, i, 0))
		{
			DYNBUF_ADD3_CELL(label, " ", &fc->dnotes, i, 0, NO_CONV, "");
		}
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : qry_obj_label
** Description : return descriptive labels for an object
*********************************************************************/
#define ERR_FUNCTION "qry_obj_label"
#define ERR_CLEANUP DYNTAB_FREE(fields); \
					DYNTAB_FREE(format); \
					DYNTAB_FREE(labels); \
					DYNTAB_FREE(intitle); \
					DYNTAB_FREE(inlabel); \
					DYNTAB_FREE(pfx); \
					DYNTAB_FREE(separ); \
					DYNTAB_FREE(values); \
					DYNTAB_FREE(_data); \
					M_FREE(sql)
int qry_obj_label(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer **label,					/* out : form label */
	DynBuffer **fnotes,					/* out : form notes */
	DynBuffer **objname,				/* out : object label */
	DynBuffer **objtitle,				/* out : object title */
	DynBuffer **notes,					/* out : object titles not in title */
	DynBuffer **img,					/* out : symbol for normal image */
	DynBuffer **imgsel,					/* out : symbol for selected image */
	unsigned long *i_formcache,			/* out : index of form in cntxt->fc */
	unsigned long id_form,				/* in : form id */
	DynTable *data,						/* in : object data */
	unsigned long beg					/* in : first line to process in data */
){
	DynTable fields = {0};
	DynTable format = {0};
	DynTable labels = {0};
	DynTable intitle = {0};
	DynTable inlabel = {0};
	DynTable pfx = {0};
	DynTable separ = {0};
	DynTable values = {0};
	DynTable _data = {0};
	DynTable *formdata = NULL;
	DynBuffer *sql = NULL;
	unsigned long i, j, k;
	DynTableCell *objdata = dyntab_cell(data, beg, 0);
	unsigned long id_obj = objdata ? objdata->IdObj : cntxt->form ? DYNTAB_TOUL(&cntxt->form->id_obj) : 0;
	if(i_formcache) *i_formcache = 0;

	/* Read object data if not given */
	if(!data && beg)
	{
		id_obj = beg;
		beg = 0;
		data = &_data;
		if(qry_obj_idfield(cntxt, data, id_obj, 0)) STACK_ERROR;
		objdata = dyntab_cell(data, 0, 0);
	}

	/* Initialize output buffers */
	if(label) M_FREE(*label);
	if(objname) M_FREE(*objname);
	if(objtitle) M_FREE(*objtitle);
	if(fnotes) M_FREE(*fnotes);
	if(notes) M_FREE(*notes);
	if(img) M_FREE(*img);
	if(imgsel) M_FREE(*imgsel);

	/* Use current form & object if no data given */
	if(!objdata && cntxt->form) id_form = DYNTAB_TOUL(&cntxt->form->id_form);

	/* Check for form in data if none given - return if not found */
	else if(!id_form) id_form = strtoul(dyntab_field_val(data, "_EVA_FORMSTAMP", ~0UL, beg), NULL, 10);
	if(!id_form) RETURN_OK;

	/* Search for form in cache */
	for(i = 0; i < cntxt->nbfc && cntxt->fc[i].idform && cntxt->fc[i].idform != id_form; i++);
	if(i == cntxt->nbfc && i < FORM_CACHE_SIZE)
	{
		/* Form not found in cache : read form data & set cache values */
		formdata = &cntxt->fc[i].data;
		if(qry_cache_idobj(formdata, id_form) || ctrl_read_baseobj(cntxt, formdata)) STACK_ERROR;

		/* Store form data in cache */
		cntxt->fc[i].idform = id_form;
		DYNBUF_ADD_CELLP(&cntxt->fc[i].label, DYNTAB_FIELD_CELL(formdata, LABEL), NO_CONV);
		DYNBUF_ADD_CELLP(&cntxt->fc[i].notes, DYNTAB_FIELD_CELL(formdata, NOTES), NO_CONV);
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].img, formdata, IMAGE);
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].imgsel, formdata,IMAGESEL);
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].cond, formdata, IMAGE_FILTER);
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].condlbl, formdata, IMAGE_LABEL);

		/* Search for title control in CTRLTREE */
		DYNTAB_FREE(fields);
		DYNTAB_FIELD(&fields, formdata, CTRLTREE);
		if(fields.nbrows)
		{
			DYNBUF_ADD_STR(&sql, 
				"-- Forms label cache : Search for title control in CTRLTREE \n"
				"SELECT TLink.IdObj FROM TLink \n"
				"INNER JOIN TVal AS TField ON TField.IdValue=TLink.IdField \n"
				"INNER JOIN TVal AS TVal ON TVal.IdValue=TLink.IdValue \n"
				"WHERE TLink.IdObj IN(");
			if(qry_values_list(cntxt, &fields, 0, &sql)) STACK_ERROR
			DYNBUF_ADD_STR(&sql, ") \n"
					"AND TLink.DateDel IS NULL \n"
					"AND TField.TxtValue='_EVA_TYPE' \n"
					"AND TVal.TxtValue='_EVA_TITLE' \n"
				"GROUP BY TLink.IdObj \n"
				"ORDER BY TLink.Line, TLink.Num");

			/* Exec query & read title data */
			if(sql_exec_query(cntxt, sql->data) ||
				sql_get_table(cntxt, &fields, 2) ||
				qry_obj_data(cntxt, &values, &fields, NULL)) STACK_ERROR;
			M_FREE(sql);
		}
		/* Set list of displayed fields from title */
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].fields, &values, DISPLAYFIELDS);
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].format, &values, DISPLAYFORMAT);
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].labels, &values, DISPLAYLABELS);
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].pfx, &values, DISPLAYLBLPFX);
		for(j = 0; j < cntxt->fc[i].fields.nbrows; j++) DYNTAB_SET(&cntxt->fc[i].intitle, j, 0, "1");

		/* Read displayed fields from form data */
		DYNTAB_FIELD_TAB(&fields, formdata, DISPLAYFIELDS);
		DYNTAB_FIELD_TAB(&labels, formdata, DISPLAYLABELS);
		DYNTAB_FIELD_TAB(&format, formdata, DISPLAYFORMAT);
		DYNTAB_FIELD_TAB(&intitle, formdata, DISPLAYINTITLE);
		DYNTAB_FIELD_TAB(&inlabel, formdata, DISPLAYINLABEL);
		DYNTAB_FIELD_TAB(&pfx, formdata, DISPLAYLBLPFX);
		DYNTAB_FIELD_TAB(&separ, formdata, SEPARATOR);
		DYNTAB_FIELD_TAB1(&cntxt->fc[i].dnotes, formdata, DISPLAYNOTES);

		/* Merge form & title display fields */
		for(j = 0; j < fields.nbrows; j++)
		{
			for(k = 0; k < cntxt->fc[i].fields.nbrows && dyntab_cmp(&fields, j, 0, &cntxt->fc[i].fields, k , 0); k++);
			if(k == cntxt->fc[i].fields.nbrows)
			{
				DYNTAB_ADD_CELL(&cntxt->fc[i].fields, k, 0, &fields, j, 0);
				DYNTAB_ADD_CELL(&cntxt->fc[i].format, k, 0, &format, j, 0);
				if(dyntab_sz(&intitle, j, 0)) DYNTAB_ADD_CELL(&cntxt->fc[i].intitle, k, 0, &intitle, j, 0);
			}
			if(dyntab_sz(&inlabel, j, 0)) DYNTAB_ADD_CELL(&cntxt->fc[i].inlabel, k, 0, &inlabel, j, 0);
			if(dyntab_sz(&labels, j, 0) && !dyntab_sz(&cntxt->fc[i].labels, k, 0)) DYNTAB_ADD_CELL(&cntxt->fc[i].labels, k, 0, &labels, j, 0);
			if(dyntab_sz(&pfx, j, 0)) DYNTAB_ADD_CELL(&cntxt->fc[i].pfx, k, 0, &pfx, j, 0);
			if(dyntab_sz(&separ, j, 0)) DYNTAB_ADD_CELL(&cntxt->fc[i].separ, k, 0, &separ, j, 0);
		}
		for(j = 0; j < cntxt->fc[i].fields.nbrows; j++)
			if(dyntab_sz(&cntxt->fc[i].intitle, j, 0))
				DYNTAB_SET(&cntxt->fc[i].inlabel, j, 0, "1");
		cntxt->nbfc++;
	}
	if(i_formcache) *i_formcache = i;

	/* If object is a control : use special naming & imaging function */
	if(id_form == OBJ_FORM_CONTROL)
	{
		if(!objdata) { data = &cntxt->form->objdata; beg = 0; }
		if(ctrl_get_label_img(cntxt, img, imgsel, label, notes, data, beg)) STACK_ERROR;
		if(objname) DYNBUF_ADD_CELLP(objname, dyntab_field_cell(data, "_EVA_LABEL", ~0UL, beg), NO_CONV);
		if(objtitle) DYNBUF_ADD_CELLP(objtitle, dyntab_field_cell(data, "_EVA_LABEL", ~0UL, beg), NO_CONV);
		RETURN_OK;
	}
	
	/* Return if no form found */
	if(i == cntxt->nbfc) RETURN_OK;
		
	/* Set images : look for first matched condition */
	j = cntxt->fc[i].img.nbrows;
	if(img) for(j = 0; j < cntxt->fc[i].img.nbrows; j++)
	{
		DynTableCell *c = dyntab_cell(&cntxt->fc[i].cond, j, 0);
		int b_match = !(c && c->txt && c->len);
		if(!b_match)
		{
			if(form_eval_fieldexpr(cntxt, &fields, id_form, id_obj, c->txt, c->len, data, beg))
				CLEAR_ERROR;
			for(k = 0; k < fields.nbrows && !b_match; k++)
				b_match = dyntab_sz(&fields, k, 0) && strcmp("0", dyntab_val(&fields, k, 0));
		}
		if(b_match)
		{
			/* Matched condition : set images */
			DYNBUF_ADD_CELL(img, &cntxt->fc[i].img, j, 0, NO_CONV);
			DYNBUF_ADD_CELL(imgsel, &cntxt->fc[i].imgsel, j, 0, NO_CONV);
			break;
		}
	}

	/* Set form label */
	if(j < cntxt->fc[i].img.nbrows && dyntab_sz(&cntxt->fc[i].condlbl, j, 0))
		DYNBUF_ADD_CELL(label, &cntxt->fc[i].condlbl, j, 0, NO_CONV)
	else if(cntxt->fc[i].label)
		DYNBUF_ADD_BUF(label, cntxt->fc[i].label, NO_CONV)
	else DYNBUF_ADD_STR(label, "Fiche");

	/* Set object label & notes */
	if(objtitle && *objtitle) DYNBUF_ADD_STR(objtitle, " : ");
	if(objtitle && qry_build_obj_label(cntxt, objtitle, &values, cntxt->fc + i, &cntxt->fc[i].intitle, id_obj, data, beg, 100))
		STACK_ERROR;
	if(objname && *objname) DYNBUF_ADD_STR(objname, " - ");
	if(objname && qry_build_obj_label(cntxt, objname, &values, cntxt->fc + i, &cntxt->fc[i].inlabel, id_obj, data, beg, 100))
		STACK_ERROR;
	if(notes && *notes) DYNBUF_ADD_STR(notes, "\n\n");
	if(notes && qry_build_obj_label(cntxt, notes, &values, cntxt->fc + i, NULL, id_obj, data, beg, 10000))
		STACK_ERROR;

	/* Set form notes */
	if(fnotes && *fnotes) DYNBUF_ADD_STR(fnotes, "\n\n");
	DYNBUF_ADD_BUF(fnotes, cntxt->fc[i].notes, NO_CONV);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
