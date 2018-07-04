/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
** File : form_save.c
** Description : forms save & close functions
** Author : Alain BOUTE
** Created : Jan 11 2003
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : form_btn_do_save
** Description : handles form data save before button clicks
*********************************************************************/
#define ERR_FUNCTION "form_btn_do_save"
#define ERR_CLEANUP	
int form_btn_do_save(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : button index in cntxt->form->ctrl */
	int *b_ok						/* out : 0 if not confirmed */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	char *save = CTRL_ATTR_VAL(SAVE);
	char *confirm = CTRL_ATTR_VAL(CONFIRM);

	/* Save form data if applicable */
	if(*save && form->prevstep != HtmlView)
	{
		int b_done = BUTN_SAVE_NEXT;
		if(form_save_dialog(cntxt, i_ctrl, confirm, BUTN_SAVE_NEXT, &b_done)) STACK_ERROR;
		if(b_ok) *b_ok = b_done == BUTN_SAVE_NEXT;
	}
	else if(b_ok) *b_ok = 1;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_check_keys
** Description : check key duplicates for the current form
*********************************************************************/
#define ERR_FUNCTION "form_check_keys"
#define ERR_CLEANUP	DYNTAB_FREE(keyfields); \
					DYNTAB_FREE(keytype); \
					DYNTAB_FREE(keymsg); \
					DYNTAB_FREE(field); \
					DYNTAB_FREE(value); \
					DYNTAB_FREE(duplicates); \
					DYNTAB_FREE(data); \
					M_FREE(msg); \
					qry_build_free(&flt)
					
int form_check_keys(					/* return : 0 if Ok, other on error */
	EVA_context *cntxt					/* in : execution context data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl;
	DynTable keyfields = {0};
	DynTable keytype = {0};
	DynTable keymsg = {0};
	DynTable keycond = {0};
	DynTable field = {0};
	DynTable value = {0};
	DynTable duplicates = {0};
	DynTable data = {0};
	DynBuffer *msg = {0};
	QryBuild flt = {0};
	unsigned long i, j, k, id;
	DynTableCell *c;
	DynBuffer **html = form->html;
	form->html = &form->html_err;

	/* Read form key fields */
	CTRL_ATTR_TAB(keyfields, KEY_FIELDS);
	CTRL_ATTR_TAB(keytype, KEY_TYPE);
	CTRL_ATTR_TAB(keymsg, KEY_MSG);
	CTRL_ATTR_TAB(keycond, KEY_COND);

	/* For each key (group of fields on same line) */
	for(i = 0; i < keyfields.nbrows; i++)
	{
		/* Check key condition if any */
		int b_error;
		char *ktype = dyntab_val(&keytype, i, 0);
		DynTableCell *c = dyntab_cell(&keycond, i, 0);
		if(c && c->txt && c->len)
		{
			if(ctrl_eval_fieldexpr(cntxt, ctrl, &data, c->txt)) CLEAR_ERROR;
			for(j = 0; j < data.nbrows; j++)
			{
				char *v = dyntab_val(&data, j, 0);
				if(*v && strcmp("0", v)) break;
			}

			/* Skip key if condition not matched */
			if(j >= data.nbrows) continue;
		}

		/* Initialize filter with FORMSTAMP condition */
		qry_build_free(&flt);
		dyntab_free(&field);
		M_FREE(msg);
		DYNTAB_ADD_TXTID(&field, 0, 0, "_EVA_FORMSTAMP", cntxt->val_FORMSTAMP);
		dyntab_free(&value);
		if(form->id_formsave)
			DYNTAB_ADD_INT(&value, 0, 0, form->id_formsave)
		if(qry_add_filter(cntxt, &flt, RelNone, &field, RelList, &value)) STACK_ERROR;

		/* Build query for each key field */
		for(j = 0; j < keyfields.nbcols; j++)
			if(dyntab_sz(&keyfields, i, j))
		{
			char *fld;
			MatchMode match = Like;

			/* Check field type on 1st field - multiple fields of same type are allowed */
			dyntab_from_list(&field, DYNTAB_VAL_SZ(&keyfields, i, j), ",", 0, 2);
			fld = dyntab_val(&field, 0, 0);

			/* Check field type - search input control in form */
			for(k = 0; k < form->nb_ctrl && strcmp(fld, form->ctrl[k].FIELD); k++);

			/* Handle message for key */
			if(msg) DYNBUF_ADD_STR(&msg, ", ");
			DYNBUF_ADD(&msg, k < form->nb_ctrl ? ctrl[k].LABEL : dyntab_val(&keyfields, i, j), 0, NO_CONV);

			/* If control is a relation : use appropriate match mode */
			if(k < form->nb_ctrl && !strcmp(form->ctrl[k].TYPE, "_EVA_RELATION")) match = RelList;

			/* Read values for fields */
			if(form_get_field_values(cntxt, &value, dyntab_val(&keyfields, i, j), DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj))) STACK_ERROR;

			/* Handle empty value */
			if(!value.nbrows)
			{
				/* Ignore value if primary key */
				if(!*ktype || !strcmp(ktype, "_EVA_KEY")) continue;
				else
				{
					/* Ignore key else */
					flt.nbnode = 0;
					break;
				}
			}

			/* Add filter node for values */
			if(qry_add_filter(cntxt, &flt, RelNone, &field, match, &value)) STACK_ERROR;
		}

		/* Ignore key if no field */
		if(flt.nbnode < 2) continue;
		
		/* Exclude current object if not new */
		if(dyntab_sz(&form->id_obj, 0, 0))
			DYNBUF_ADD3_CELL(&flt.where, "TLink0.IdObj<>", &form->id_obj, 0, 0, NO_CONV, "");

		/* Handle message for keys */
		DYNTAB_ADD_BUF(&keymsg, i, 1, msg);

		/* Query for objects matching key filter & set error depending on result & error setting for key */
		if(qry_filter_objects(cntxt, &value, &flt)) STACK_ERROR;
		b_error = value.nbrows ? !strcmp(ktype, "_EVA_KEY_WARN") ? 1 : 6 : 0;
		if(!b_error) continue;
		form->error |= b_error;
		dyntab_cell(&keyfields, i, 0)->col = b_error;

		/* Add found objects to duplicates list */
		for(j = 0; j < value.nbrows; j++) 
		{
			/* Search object in duplicates */
			id = DYNTAB_TOULRC(&value, j, 0);
			for(k = 0; k < duplicates.nbrows && dyntab_cmp(&duplicates, k, 0, &value, j, 0); k++);

			/* If object not found : add to duplicates */
			if(k == duplicates.nbrows) DYNTAB_ADD_CELL(&duplicates, k, 0, &value, j, 0);

			/* Associate key with duplicate */
			if(i) dyntab_add(&duplicates, k, i, NULL, 0, NO_CONV);
			dyntab_cell(&duplicates, k, i)->IdObj = id;
		}
	}

	/* Handle duplicates if applicable */
	if(duplicates.nbrows)
	{
		int b_show_dup = !CTRL_ATTR_CELL(NO_DUPLICATE_DISPLAY);
		char printbuf[1024];
		char datecr[32] = {0}, *errtyp;

		/* Output table header for duplicates message */
		dynbuf_print3(form->html,
			"<table border=1 rules=rows cellspacing=0 cellpadding=5 width=100%% bgcolor=#FFFFFF>"
			"<tr><td colspan=2 align=center><font size=+1><font color=#%s><b>Doublons %s</b></font><br>"
			"<u>La fiche que vous voulez enregistrer %s</u></font></td></tr>",
			form->error & 4 ? "FF0000" : "FF8844",
			form->error & 4 ? "interdits" : "déconseillés",
			form->error & 4 ? "existe déjà" : "ressemble à d'autres fiches"
			);

		/* Output keys & messages */
		for(i = 0; i < keyfields.nbrows; i++)
		{
			j = dyntab_cell(&keyfields, i, 0)->col;
			if(j)
			{
				errtyp = j & 4 ? "interdits" : "déconseillés";
				dynbuf_print(form->html, "<tr><td valign=top>Doublons %s sur le(s) champs <b><u>", errtyp);
				DYNBUF_ADD_CELL(form->html, &keymsg, i, 1, TO_HTML)
				DYNBUF_ADD_STR(form->html, "</u></b></td><td valign=top><i>");
				DYNBUF_ADD_CELL(form->html, &keymsg, i, 0, TO_HTML)
				DYNBUF_ADD_STR(form->html, "</i></td></tr>");
			}
		}
		dynbuf_print3(form->html, "</table><p align=center>Il y a %lu fiche%s qui ressemble%s à celle que vous voulez enregistrer</p>",
			duplicates.nbrows, duplicates.nbrows > 1 ? "s" : "", duplicates.nbrows > 1 ? "nt" : "");

		/* If output duplicates objects  */
		if(b_show_dup)
		{
			DYNBUF_ADD_STR(form->html, "<table border=1 rules=rows cellpadding=0 width=100%><tr>"
										"<th>Ouvrir</th><th>Fiche</th><th>Création</th><th>Doublon sur le(s) champs</th></tr>");
			for(i = 0; i < duplicates.nbrows; i++)
			{
				c = dyntab_cell(&duplicates, i, 0);
				id = strtoul(c->txt, NULL, 10);

				/* Add object open button */
				DYNBUF_ADD_STR(form->html, "<tr>");
				if(qry_obj_idfield(cntxt, &data, id, 0) ||
					ctrl_add_symbol_btn(cntxt, ctrl, c, &data, 0, "", "SYMBOL+LABEL+OBJNOTES"))
					STACK_ERROR;

				/* Add object creation infos */
				if(form_eval_fieldexpr(cntxt, &data, 0, id, add_sz_str("_EVA_FORMSTAMP.DateCr"), NULL, 0)) STACK_ERROR;
				datetxt_to_format(cntxt, datecr, dyntab_val(&data, 0, 0), "");
				if(*datecr)
				{
					if(form_eval_fieldexpr(cntxt, &data, 0, id, add_sz_str("_EVA_FORMSTAMP.IdWhoCr->_EVA_USERNAME"), NULL, 0)) STACK_ERROR;
					dynbuf_print2(form->html, "</td><td valign=top><font size=-1><i>Fiche créée le %s par %s</i></font></td><td>",
									datecr, dyntab_val(&data, 0, 0));
				}

				/* Add associated keys */
				id = 0;
				for(j = 0; j < keyfields.nbrows; j++)
				{
					c = dyntab_cell(&duplicates, i, j);
					if(c && c->IdObj)
					{
						if(id) DYNBUF_ADD_STR(form->html, " + ");
						id = dyntab_cell(&keyfields, j, 0)->col;
						dynbuf_print(form->html, "<font color=%s><b>", id & 4 ? "FF0000" : "FF8844");
						DYNBUF_ADD_CELL(form->html, &keymsg, j, 1, TO_HTML)
						DYNBUF_ADD_STR(form->html, "</b></font>");
					}
				}
				DYNBUF_ADD_STR(form->html, "</td></tr>");
			}
			DYNBUF_ADD_STR(form->html, "</table>");
		}
	}
	form->html = html;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_save
** Description : save current form data 
*********************************************************************/
#define ERR_FUNCTION "form_save"
#define ERR_CLEANUP	DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(cgival); \
					DYNTAB_FREE(srcctrl); \
					M_FREE(buf)
int form_save(					/* return : 0 on success, other on error */
	EVA_context *cntxt			/* in/out : execution context data */
){
	EVA_form *form = cntxt->form;
	unsigned long idform = DYNTAB_TOUL(&cntxt->form->id_form);
	unsigned long idobj = DYNTAB_TOUL(&cntxt->form->id_obj);
	DynTable cgival = {0};
	DynTable id_obj = {0};
	DynTable srcctrl = {0};
	DynBuffer *buf = NULL;

	/* Call handler in save mode for all controls & add form stamp */
	form->step = FormSave;
	if(ctrl_add_child(cntxt, 0, NULL)) STACK_ERROR;
	if(form->id_formsave)
	{
		char txt[32];
		if(qry_add_obj_field_val(cntxt, &form->id_obj, "_EVA_FORMSTAMP",
								txt, snprintf(add_sz_str(txt), "%lu", form->id_formsave), 1, 0)) STACK_ERROR;
	}
	idobj = DYNTAB_TOUL(&cntxt->form->id_obj);

	/* Handle new obj storage in caller : look for storage field */
	if(form->b_newobj)
	{
		/* Retrieve caller data */
		CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
		int b_alt = !dyntab_cmp(&cntxt->alt_form, 0, 0, &cntxt->form->id_form, 0, 0) &&
					DYNTAB_TOUL(&cntxt->alt_obj) == 0;
		unsigned long idobj0, idform0, idctrl, num = 0, line = 0;
		char *ctrltyp;
		dyntab_from_list(&cgival, DYNTAB_VAL_SZ(&form->call_data, 0, 0), "/", 0, 6);
		idform0 = DYNTAB_TOULRC(&cgival, 0, 0);
		idobj0 = DYNTAB_TOULRC(&cgival, 1, 0);
		idctrl = DYNTAB_TOULRC(&cgival, 2, 0);
		sscanf(dyntab_val(&cgival, 3, 0), "%lu.%lu", &num, &line);

		/* Look for control type */
		if(qry_cache_idobj_field(cntxt, &srcctrl, idctrl, NULL, 0) || ctrl_read_baseobj(cntxt, &srcctrl)) STACK_ERROR;
		ctrltyp = DYNTAB_FIELD_VAL(&srcctrl, CONTROL);
		if(!strcmp(ctrltyp, "_EVA_INPUT"))
		{
			/* Relation : add new value (for relations direct add function) */
			DYNTAB_FIELD(&cgival, &srcctrl, FIELD);
		}
		else if(!strcmp(ctrltyp, "_EVA_BUTTON"))
		{
			/* Button : look for storage field for new objects */
			DYNTAB_FIELD(&cgival, &srcctrl, NEWOBJ_FIELD);
		}
		else
			dyntab_free(&cgival);

		/* Storage field found : add new value with object id */
		if(dyntab_sz(&cgival, 0, 0))
		{
			DynTableCell *fld = dyntab_cell(&cgival, 0, 0);
			DynTableCell *c;
			DYNTAB_ADD_INT(&id_obj, 0, 0, idobj);
			c = dyntab_cell(&id_obj, 0, 0);
			c->Line = line;
			c->Num = num;
			if(cgi_set_field_values(cntxt, idform0, idobj0, fld->txt, fld->len, &id_obj, NULL, 0))
				STACK_ERROR;
			cgi = cntxt->cgi + cntxt->cgibtn;

			/* Handle control status : close search */
			if(cgi_filter_values(cntxt, &cgival, 'D', ~0UL,  idform0, idobj0, fld->txt, "STATUS", 0, 0)) STACK_ERROR;
			if(cgival.nbrows)
			{
				unsigned long sts = 0;
				int len = 0;
				sscanf(cgival.cell->txt, "%lx%n", &sts, &len);
				snprintf(cgival.cell->txt, len, "%lx", sts & ~(TblCtrl_opensearch | TblCtrl_opensel));
			}
		}

		/* Update new object id in context (main form or alt) */
		DYNTAB_ADD_CELL(b_alt ? &cntxt->alt_obj : &cntxt->id_obj, 0, 0, &form->id_obj, 0, 0);
		if(cgi->name[0] == 'B' && !cgi->IdObj && cgi->IdForm == idform) cgi->IdObj = idobj;
	}

	/* Clear CGI form inputs */
	cgi_clear_form_inputs(cntxt, idform, form->b_newobj ? 0 : idobj, form->b_newobj ? 0 : 1);

	/* Handle new object */
	if(form->b_newobj)
	{
		/* Rebuild CGI name && clear variables temp values */
		unsigned long i;
		for(i = 0; i < form->nb_ctrl; i++)
			if(form->ctrl[i].cginame && 
				cgi_build_basename(cntxt, &form->ctrl[i].cginame, i, *form->ctrl[i].cginame->data))
				STACK_ERROR;
		DYNTAB_FREE(form->varsvalues);
	}
	else
		qry_uncache_idobj(cntxt, idobj);
	form->b_reload = 1;

	/* Keep selected tab & form status */
	if(form_tab_selection(cntxt, 0)) STACK_ERROR;
	form->step = HtmlEdit;
	dyntab_free(&form->dlg_ctrl);
	if(form_status_data(cntxt, &buf)) STACK_ERROR;
	if(cgi_add_input(cntxt, NULL, DYNBUF_VAL_SZ(form->ctrl->cginame), DYNBUF_VAL_SZ(buf))) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_save_dialog_end
** Description : handle a Save / Restore / Cancel dialog 
*********************************************************************/
#define ERR_FUNCTION "form_save_dialog_end"
#define ERR_CLEANUP	M_FREE(buf)
int form_save_dialog_end(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	int actionbtn					/* in : clicked button */
){
	EVA_form *form = cntxt->form;
	unsigned long i;
	DynBuffer *buf = NULL;

	/* Process clicked buttton */
	switch(actionbtn)
	{
	case BUTN_CLOSE:
		/* Close form - return to previous */
		if(form_close_nosave(cntxt, DYNTAB_TOUL(&form->id_form), 
									DYNTAB_TOUL(&form->id_obj),
									&form->call_data))
			STACK_ERROR;
		form_free(cntxt);
		break;

	case BUTN_RESTORE:
		/* Clear all CGI form data & set form reload flag */
		cgi_clear_form_inputs(cntxt, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), 1);
		form->b_reload = 1;

	default:
		/* Restore edit mode */
		form->nextstep = form->prevstep < HtmlView ? HtmlEdit : form->prevstep;
		form->step = form->nextstep;
		dyntab_free(&form->dlg_ctrl);
		form_free_html(cntxt);

		/* Update form status in CGI data */
		for(i = 0; i < cntxt->nb_cgi && (!form->ctrl->cginame ||
				strcmp(form->ctrl->cginame->data, cntxt->cgi[i].name)); i++);
		if(i < cntxt->nb_cgi)
		{
			CGIData *cgi = cntxt->cgi + i;
			if(form_status_data(cntxt, &buf)) STACK_ERROR;
			M_FREE(cgi->value);
			M_STRDUP(cgi->value, buf->data, buf->cnt);
			cgi->value_sz = buf->cnt;
		}
	}
	if(cntxt->cgi && cntxt->cgi[cntxt->cgibtn].subfield &&
		!strncmp(cntxt->cgi[cntxt->cgibtn].subfield, add_sz_str("FORM")))
			cntxt->cgi[cntxt->cgibtn].name[0] = 0;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_save_dialog_output
** Description : output Save / Restore / Cancel dialog 
*********************************************************************/
#define ERR_FUNCTION "form_save_dialog_output"
#define ERR_CLEANUP	M_FREE(name)
int form_save_dialog_output(		/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	int butns,						/* in : buttons to display */
	int *clicbtn					/* in : defaut button if no dialog needed */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *name = NULL;
	form->html = &form->html_top;

	/* Output dialog message header */
	DYNBUF_ADD3(form->html, 
			"<table border=0 cellspacing=0 cellpadding=0 width=100% bgcolor=#FFFFFF><tr>\n"
			"<td align=center><font size=-1>Vous avez cliqué sur <b><font color=#FFAA00>", ctrl->LABEL, 0, NO_CONV, "</font> - ");
	DYNBUF_ADD(form->html, (
				(clicbtn && *clicbtn & BUTN_SAVE) ? "Confirmation avant enregistrement" :
													"Confirmation avant fermeture"), 0, TO_HTML);
	DYNBUF_ADD_STR(form->html, "</td></tr></table>");
	DYNBUF_ADD3(form->html, 
			"<table border=0 cellspacing=0 cellpadding=5 width=100% bgcolor=#DDDDDD><tr>\n"
			"<td align=center><b><font size=+1 color=#", 
			(clicbtn && *clicbtn & BUTN_CLOSE) ? "000000>Fiche modifiée" :
			(form->error & 2) ? "FF0000>Erreur bloquante" :
			"FFAA00>Vérifiez les données",0, NO_CONV,
			"</b></font></td><td width=10></td><td align = center>");

	/* Add goback / close button */
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("FORMGOBACKOBJ"));
	if(put_html_button(cntxt, name->data, "Revenir", "_eva_btn_gobackobj_fr.gif", "_eva_btn_gobackobj_fr_s.gif", "Retourne à la page précédente", 0, 0)) STACK_ERROR;
	if(!butns || butns & BUTN_CLOSE)
	{
		DYNBUF_ADD_STR(form->html, "</td><td align = center>");
		if(form->b_modified)
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("FORMCLOSE"));
			if(put_html_button(cntxt, name->data, "Fermer sans enregistrer", "_eva_btn_closenosave_fr.gif", "_eva_btn_closenosave_fr_s.gif", "Ignore les modifications et retourne au formulaire précédent", 0, 0)) STACK_ERROR;
		}
		else
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("FORMCLOSE"));
			if(put_html_button(cntxt, name->data, "Fermer", "_eva_btn_close_fr.gif", "_eva_btn_close_fr_s.gif", "Retourne au formulaire précédent", 0, 0)) STACK_ERROR;
		}
	}
	DYNBUF_ADD_STR(form->html, "</td></tr></table>");

	/* Output complementary error message and form controls */
	DYNBUF_ADD_BUF(form->html, form->html_err, NO_CONV);
	if(form->html_tabs)
	{
		DYNBUF_ADD_STR(form->html, 
			"<table border=0 cellspacing=0 cellpadding=2 width=100% height=200><tr>")
		DYNBUF_ADD_BUF(form->html, form->html_tabs, NO_CONV);
		DYNBUF_ADD_STR(form->html, "</tr></table>\n");
		M_FREE(form->html_tabs);
	}
	form->nextstep = HtmlSaveDlg;					   

	/* Output bottom of form */
	DYNBUF_ADD_STR(form->html,
				"<table border=0 cellspacing=0 cellpadding=2 width=100%><tr>");
	
	/* Output error message if blocking error */
	if(form->error & form->savedlg_outmode & 2 || form->error & 4)
	{
		DYNBUF_ADD_STR(form->html,
		"<td align=center bgcolor=#DDDDDD colspan=4><b><font face=Arial>"
		"<font size=+1 color=#FF0000>Enregistrement refusé</font>");
		if(!(form->error & 4))
			DYNBUF_ADD_STR(form->html, " : corrigez les erreurs et essayez d'enregistrer à nouveau");
		DYNBUF_ADD_STR(form->html,
		"</font></b></td></tr><tr>");
	}

	/* Add save / restore buttons in form->html_tabs */
	DYNBUF_ADD_STR(form->html, "<td align = center>");
	if(butns & BUTN_RESTORE)
	{
		DYNBUF_ADD_STR(form->html, "</td><td align = center>");
		CTRL_CGINAMEBTN(&name, NULL, add_sz_str("FORMRESTORE"));
		if(put_html_button(cntxt, name->data, "Annuler", "_eva_btn_restore_fr.gif", "_eva_btn_restore_fr_s.gif", "Annule les modifications et retourne à la fiche originale", 0, 0)) STACK_ERROR;
	}
	if(form->error < 4)
	{
		if(butns & BUTN_SAVE_BACK)
		{
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("FORMSAVE_BACK"));
			DYNBUF_ADD_STR(form->html, "</td>\n<td align = center>");
			if(put_html_button(cntxt, name->data, "Enregistrer", "_eva_btn_save_fr.gif", "_eva_btn_save_fr_s.gif", "Enregistre les modifications et retourne à la fiche enregistrée", 0, 0)) STACK_ERROR;
		}
		if(butns & BUTN_SAVE_CLOSE)
		{
			DYNBUF_ADD_STR(form->html, "</td>\n<td align = center>");
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("FORMSAVE_CLOSE"));
			if(put_html_button(cntxt, name->data, "Enregistrer et fermer", "_eva_btn_saveclose_fr.gif", "_eva_btn_saveclose_fr_s.gif", "Enregistre les modifications et retourne au formulaire précédent", 0, 0)) STACK_ERROR;
		}
		if(butns & BUTN_SAVE_NEXT)
		{
			DYNBUF_ADD_STR(form->html, "</td>\n<td align = center>");
			CTRL_CGINAMEBTN(&name, NULL, add_sz_str("FORMSAVE_NEXT"));
			if(put_html_button(cntxt, name->data, "Enregistrer et continuer", "_eva_btn_savenext_fr.gif", "_eva_btn_savenext_fr_s.gif", "Enregistre les modifications et passe à l'étape suivante", 0, 0)) STACK_ERROR;
		}
	}

	/* Add Display changes checkbox */
	DYNBUF_ADD_STR(form->html, "</td><td><table border=0 cellpadding=0 cellspacing=0><tr><td align=right>");
	CTRL_CGINAMESUBFIELD(&name, NULL, "CHANGE");
	if(form->b_modified)
	{
		if(put_html_chkbox(cntxt, DYNBUF_VAL_SZ(name), "1", 1, form->savedlg_outmode & 4 ? 1 : 0, 1)) STACK_ERROR;
		DYNBUF_ADD_STR(form->html, "</td><td><font size=-1>Afficher les modifications");
	}
	else if(form->savedlg_outmode & 4)
		DYNBUF_ADD3_BUF(form->html, "<input type=hidden name='", name, NO_CONV, "' value=1>");

	/* Add Display warnings checkbox */
	DYNBUF_ADD_STR(form->html, "</td></tr><tr><td align=right>");
	CTRL_CGINAMESUBFIELD(&name, NULL, "WARN");
	if(form->error & 1 || form->error & 2 && !(form->savedlg_outmode & 2))
	{
		if(put_html_chkbox(cntxt, DYNBUF_VAL_SZ(name), "1", 1, form->savedlg_outmode & 1 ? 1 : 0, 1)) STACK_ERROR;
		DYNBUF_ADD_STR(form->html, "</td><td><font size=-1>Afficher les avertissements");
	}
	else if(form->savedlg_outmode & 1)
		DYNBUF_ADD3_BUF(form->html, "<input type=hidden name='", name, NO_CONV, "' value=1>");
	DYNBUF_ADD_STR(form->html, "</td></tr></table></td>");

	DYNBUF_ADD_STR(form->html, 
		"</tr></table>\n")

	/* Set form dialog control */
	DYNTAB_ADD_BUF(&form->dlg_ctrl, 0, 0, ctrl->cginame);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_save_dialog
** Description : handle a Save / Restore / Cancel dialog 
*********************************************************************/
#define ERR_FUNCTION "form_save_dialog"
#define ERR_CLEANUP	DYNTAB_FREE(cgival)
int form_save_dialog(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *confirm,					/* in : confirm mode */
	int butns,						/* in : buttons to display */
	int *clicbtn					/* in : defaut button if no dialog needed
									  out : clicked button if sucessfully processed
											0 if no answer (dialog displayed) */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = {0};
	int b_nodlg = 0, actionbtn = 0;
	int b_first = form->prevstep != HtmlSaveDlg;

	/* End dialog & return if no save */
 	if(!strcmp(confirm, "_EVA_NOSAVE") || (clicbtn && *clicbtn == BUTN_CLOSE && form->prevstep == HtmlView))
	{
		if(form_save_dialog_end(cntxt, clicbtn ? *clicbtn : BUTN_CLOSE)) STACK_ERROR;
		RETURN_OK;
	}

	form->error = 0;
	if(strcmp(confirm, "_EVA_NOSAVE"))
	{
		/* Check data for each input control */
		form->step = InputCheck;
		if(dyntab_sz(&form->id_obj, 0, 0)) form->b_modified = 0;
		if(ctrl_add_child(cntxt, 0, NULL)) STACK_ERROR;

		/* Check duplicates if no fatal error or close unchanged form */	
		if(form->error < 2 && !(b_first && !form->b_modified && clicbtn && *clicbtn & BUTN_CLOSE) &&
			form_check_keys(cntxt)) STACK_ERROR;
	}

	if(b_first)
	{
		/* First dialog display : Initialize default button */
		actionbtn = clicbtn ? *clicbtn : 0;

		/* Initialize displayed infos */
		form->savedlg_outmode = 
			(actionbtn & BUTN_CLOSE ? 0 : 2) | (
				!strcmp(confirm, "_EVA_CONFIRMNONE") ? 0 : 
				!strcmp(confirm, "_EVA_CONFIRMWARN") ?  actionbtn & BUTN_CLOSE ? 0 : 1:
				!strcmp(confirm, "_EVA_CONFIRMCHANGE") ? 4 : (actionbtn & BUTN_CLOSE ? 0 : 1) | 4);

		/* Determine if confirm dialog needed */
		b_nodlg =  
				!strcmp(confirm, "_EVA_CONFIRMNONE") ? form->error < 2 :
				!strcmp(confirm, "_EVA_CONFIRMWARN") ? !form->error :
				!strcmp(confirm, "_EVA_CONFIRMCHANGE") ? !form->b_modified :
										!form->error && !form->b_modified;
	}
	else 
	{
		/* Next dialogs : parse button click */
		char *btn = CGI_CLICK_BTN_SUBFIELD;
		if(*btn) actionbtn =
			!strcmp(btn, "FORMCLOSE") ? BUTN_CLOSE :
			!strcmp(btn, "FORMRESTORE") ? BUTN_RESTORE :
			!strcmp(btn, "FORMGOBACKOBJ") ? BUTN_GOBACKOBJ :
			!strcmp(btn, "FORMSAVE_NEXT") ? BUTN_SAVE_NEXT :
			!strcmp(btn, "FORMSAVE_CLOSE") ? BUTN_SAVE_CLOSE :
			!strcmp(btn, "FORMSAVE_BACK") ? BUTN_SAVE_BACK :
			0;

		/* Read displayed infos - always dislay error */
		form->savedlg_outmode = 2;
		CTRL_GET_CGI_SUBFIELD("CHANGE");
		if(cgival.nbrows) form->savedlg_outmode |= 4;
		CTRL_GET_CGI_SUBFIELD("WARN");
		if(cgival.nbrows) form->savedlg_outmode |= 1;

		/* Determine if confirm dialog needed */
		b_nodlg = (actionbtn || form->prevstep != HtmlSaveDlg) && form->error < 2;
	}

	/* Disable save if errors */
	if(form->error > 1)
	{
		if(!(actionbtn & BUTN_CLOSE)) b_nodlg = 0;
		if(actionbtn & BUTN_SAVE) actionbtn = 0;
	}

	/* If dialog applicable */
	form->step = HtmlSaveDlg;
	if(!b_nodlg)
	{
		/* Output selected controls in form->html_tabs */
		form->html = &form->html_tabs;
		if(ctrl_add_child(cntxt, 0, NULL)) STACK_ERROR;

		/* Handle if no selected control */
		if(!form->html_tabs || !form->html_tabs->data ||
			!strstr(form->html_tabs->data, "</td>"))
		{
			if(b_first && actionbtn & BUTN_CLOSE)
			{
				b_nodlg = 1;
				form->html = NULL;
			}
			else if(!form->html_err)
				DYNBUF_ADD3(form->html, "<td colspan=2 align=center><br>",
					form->error ? 
						"<font color=#FF8844><b>Fiche incomplète : il y a des avertissements</b></font>" :
						"<b>Fiche complète</b>", 0, NO_CONV, 
					"<br><br></td></tr><tr>");
		}
	}

	/* Handle action if applicable */
	if(b_first && !b_nodlg) actionbtn =0;
	switch(actionbtn)
	{
	case BUTN_CLOSE:
	case BUTN_RESTORE:
	case BUTN_GOBACKOBJ:
		if(form_save_dialog_end(cntxt, actionbtn)) STACK_ERROR;
		break;

	case BUTN_SAVE_CLOSE:
		if(form_save(cntxt)) STACK_ERROR;
		if(form_save_dialog_end(cntxt, BUTN_CLOSE)) STACK_ERROR;
		break;

	case BUTN_SAVE_BACK:
	case BUTN_SAVE_NEXT:
		if(form_save(cntxt)) STACK_ERROR;
		if(form_save_dialog_end(cntxt, 0)) STACK_ERROR;
		break;

	default:
		/* Output message and dialog buttons if applicable */
		if(form_save_dialog_output(cntxt, i_ctrl, butns, clicbtn)) STACK_ERROR;
	}
	if(clicbtn) *clicbtn = actionbtn;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
