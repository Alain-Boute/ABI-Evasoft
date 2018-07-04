/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dlg_openedforms.c
** Description : handling fonctions for opened forms list dialog
**      Author : Alain BOUTE
**     Created : March 4 2003
*********************************************************************/

#include "eva.h"

#define NAME_BUTTON(btnname) NAME_BUTTON_SZ(btnname, sizeof(btnname) - 1)
#define NAME_BUTTON_SZ(txt, len) NAME_SZ(txt, len, 'B')
#define NAME_VALUE(btnname) NAME_SZ(btnname, sizeof(btnname) - 1, 'C')
#define NAME_SZ(txt, len, typ) { M_FREE(name); DYNBUF_ADD_BUF(&name, cginame, NO_CONV); \
								if(cgi_build_finalname(cntxt, &name, typ, txt, len, 0, 0, 0)) STACK_ERROR; }


/*********************************************************************
** Function : dlg_openedforms_output
** Description : output list of opened forms
*********************************************************************/
#define ERR_FUNCTION "dlg_openedforms_output"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					DYNTAB_FREE(cgival); \
					M_FREE(name); \
					cntxt->form = NULL; \
					table_free(&tbl)
int dlg_openedforms_output(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer *cginame,				/* in : base name for buttons */
	int b_confirm					/* in : confirm on close if not 0 */
){
	EVA_form _form = {0};
	unsigned long i, j;
	unsigned long idobj0 = DYNTAB_TOUL(&cntxt->id_obj);
	unsigned long idform0 = DYNTAB_TOUL(&cntxt->id_form);
	unsigned long idobj1 = DYNTAB_TOUL(&cntxt->alt_obj);
	unsigned long idform1 = DYNTAB_TOUL(&cntxt->alt_form);
	char *bgcolor;
	DynTable cgival = { 0 };
	DynTable data = { 0 };
	DynBuffer **html;
	DynBuffer *name = NULL;
	int b_modif = 0;
	ObjTableFormat tbl = {0};

	/* Output main table header for opened forms list */
	if(!cntxt->form) cntxt->form = &_form;
	html = &cntxt->html;
	cntxt->form->html = html;
	DYNBUF_ADD_STR(html,
		"<font face=Arial>"
		"<table border=0 width=100% bgcolor=#EEEEEE><tr>"
		"<td align=center><font size=+1><b>Liste des formulaires ouverts</b></font></td>");

	/* Output goback & menu buttons */
	if(dyntab_cmp(&cntxt->id_form, 0, 0, &cntxt->menubar, 0, 0) && 
		dyntab_cmp(&cntxt->id_form, 0, 0, &cntxt->homepage, 0, 0))
	{
		DYNBUF_ADD_STR(html, "<td align=center>")
		NAME_BUTTON("VIEW");
		if(put_html_button(cntxt, name->data, NULL, "_eva_btn_2forms_fr.gif", "_eva_btn_2forms_fr_s.gif", 
			"Afficher les formulaires sélectionnés dans les colonnes Haut et bas\n\n"
			"Vous pouvez afficher deux formulaires simultanément", 0, 0)) STACK_ERROR; 
		DYNBUF_ADD_STR(html, "</td>")
	}
	DYNBUF_ADD_STR(html, "<td align=right>")
	NAME_BUTTON("MENU");
	if(put_html_button(cntxt, name->data, NULL, "_eva_menu.gif", "_eva_menu_s.gif", 
		"Retour à la page d'accueil", 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html,
		"</td></tr></table>"
		"<table border=1 cellspacing=0 rules=rows bgcolor=#EEEEEE width=100%><tr>"
		"<td></td><td></td><td align=center><font size=-2>Haut</font></td><td align=center><font size=-2>Bas</font></td><td></td></tr>")

	/* For each form found in CGI data */
	for(i = cntxt->nb_cgi - 1, j = 0; i; i--) if(cntxt->cgi[i].name[0] == 'F')
	{
		CGIData *cgi = cntxt->cgi + i;
		char objref[64];
		int b_modified = cgi_check_form_change(cntxt, cgi->IdForm, cgi->IdObj);
		if(b_modified == 2) continue;
		b_modif |= b_modified;
		bgcolor = table_row_bgcolor(cntxt, &tbl, j++, NULL);
		if(!bgcolor || !*bgcolor) bgcolor = "FFFFFF";

		/* Output row header */
		DYNBUF_ADD_STR(html, "<tr>")

		/* Read object data */
		if(cgi_filter_values(cntxt, &cgival, 'D', ~0UL, 
								cgi->IdObj ? ~0UL : cgi->IdForm,
								cgi->IdObj, NULL, "", 0, 0)) STACK_ERROR;
		if(cgi->IdObj)
		{
			unsigned long k;
			if(qry_obj_idfield(cntxt, &data, cgi->IdObj, 0)) STACK_ERROR;
			if(qry_complete_data(cntxt, &cgival, &data, NULL, NULL)) STACK_ERROR;
			for(k = cgival.nbrows; k; k--)
				if(!strcmp(dyntab_cell(&cgival, k - 1, 0)->field, "_EVA_FORMSTAMP"))
					dyntab_del_rows(&cgival, k - 1, 1);
		}
		DYNTAB_ADD_INT(&cgival, cgival.nbrows, 0, cgi->IdForm);
		{
			DynTableCell *c = dyntab_cell(&cgival, cgival.nbrows - 1, 0);
			c->field = "_EVA_FORMSTAMP";
			c->b_dontfreefield = 1;
			c->IdObj = cgi->IdObj;
		}

		/* Output open button */
		if(ctrl_add_symbol_btn(cntxt, NULL, NULL, &cgival, 0, bgcolor, "SYMBOL+NAME+LABEL+OBJNOTES"))
			CLEAR_ERROR;

		/* Output display position radio buttons */
		NAME_VALUE("TOPFRM")
		DYNBUF_ADD3(html, "<td align=center bgcolor=", bgcolor, 6, NO_CONV, ">")
		if(put_html_chkbox(cntxt, DYNBUF_VAL_SZ(name),
				objref, snprintf(add_sz_str(objref), "%lu,%lu", cgi->IdObj, cgi->IdForm), 
				2 | (cgi->IdForm == idform0 && cgi->IdObj == idobj0), 0)) STACK_ERROR;
		NAME_VALUE("BOTFRM")
		DYNBUF_ADD3(html, "</td><td align=center bgcolor=", bgcolor, 6, NO_CONV, ">")
		if(put_html_chkbox(cntxt, DYNBUF_VAL_SZ(name),
				objref, snprintf(add_sz_str(objref), "%lu,%lu", cgi->IdObj, cgi->IdForm), 
				2 | (cgi->IdForm == idform1 && cgi->IdObj == idobj1), 0)) STACK_ERROR;
 		DYNBUF_ADD_STR(html, "</td>")

		/* Output close button depending on modified status */
		NAME_BUTTON_SZ(objref, snprintf(objref, sizeof(objref), "CLOSE=%lu,%lu", cgi->IdObj, cgi->IdForm));
		DYNBUF_ADD3(html, "<td align=right bgcolor=", bgcolor, 6, NO_CONV, ">")
		if(b_modified)
		{
			if(put_html_button(cntxt, name->data, "Fermer", "_eva_btn_close1_fr.gif", "_eva_btn_close1_fr_s.gif",
				"Attention : fiche modifiée\n\n"
				"Un dialogue vous demande de confirmer la fermeture\n\n"
				"Décochez la case 'Confirmation' pour ne pas afficher de confirmation", 0, 0)) STACK_ERROR;
		}
		else
		{
			if(put_html_button(cntxt, name->data, "Fermer", "_eva_btn_close_fr.gif", "_eva_btn_close_fr_s.gif", NULL, 0, 0)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(html, "</td></tr>")
	}

	switch(j)
	{
	case 0:
		DYNBUF_ADD_STR(html, "<tr><td colspan=6 align=center bgcolor=#DDDDDD><b>Aucune fenêtre ouverte</b>");
		break;
	case 1: 
		/* Output Confirm input */
		NAME_VALUE("CONFIRM");
		DYNBUF_ADD3_BUF(html, "<input type=hidden name='", name, NO_CONV, "' value=1>");
		break;
	default:
		/* Output close all buttons */
		DYNBUF_ADD_STR(html, "<tr><td colspan=6><table width=100% height=50><tr>");
		if(b_modif)
		{
			/* Output close no save button */
			char printbuf[512];
			DYNBUF_ADD_STR(html, "<td align=center>");
			NAME_BUTTON("CLOSEALLNOSAVE");
			if(put_html_button(cntxt, name->data, NULL, "_eva_btn_closeall1_fr.gif", "_eva_btn_closeall1_fr_s.gif",
				"Ferme tous les formulaires ouverts et retourne à la page d'accueil\n\n"
				"Les fiches modifiées ne sont pas fermées si la case 'Confirmation' est cochée\n\n"
				"Dans le cas contraire, les données modifiées sont perdues", 0, 0)) STACK_ERROR;

			/* Output Confirm checkbox */
			NAME_VALUE("CONFIRM");
			dynbuf_print2(html, "</td><td align=center colspan=2><input type=checkbox name='%s' %s>Confirmation", 
				name->data, b_confirm ? "checked" : "");

			/* Output close unmodified forms button */
			DYNBUF_ADD_STR(html, "</td><td align=right>");
			NAME_BUTTON("CLOSEALL");
			if(put_html_button(cntxt, name->data, NULL, "_eva_btn_closeall_fr.gif", "_eva_btn_closeall_fr_s.gif",
				"Ferme tous les formulaires non modifiés et affiche le premier formulaire modifié", 0, 0)) STACK_ERROR;
		}else
		{
			DYNBUF_ADD_STR(html, "<td align=right>");
			NAME_BUTTON("CLOSEALL");
			if(put_html_button(cntxt, name->data, "Tout fermer", "_eva_btn_closeall_fr.gif", "_eva_btn_closeall_fr_s.gif",
				"Ferme tous les formulaires ouverts et retourne à la page d'accueil", 0, 0)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(html, "</td></tr></table>");
	}
	DYNBUF_ADD_STR(html, "</td></tr></table></font>");

	/* Clear displayed forms */
	dyntab_free(&cntxt->id_obj);
	dyntab_free(&cntxt->id_form);
	dyntab_free(&cntxt->alt_obj);
	dyntab_free(&cntxt->alt_form);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : dlg_openedforms
** Description : button click handler for OPENEDFORMS_DLG
*********************************************************************/
#define ERR_FUNCTION "dlg_openedforms"
#define ERR_CLEANUP	DYNTAB_FREE(cgival); \
					M_FREE(cginame); \
					M_FREE(name)
					
int dlg_openedforms(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i;
	DynTable cgival = { 0 };
	DynBuffer *cginame = NULL;
	DynBuffer *name = NULL;
	char *btn = CGI_CLICK_BTN_SUBFIELD;
	char *equal = strchr(btn, '=');
	unsigned long idobj = equal ? strtoul(equal + 1, NULL, 10) : 0;
	char *c = equal ? strchr(equal, ',') : NULL;
	unsigned long idform = c ? strtoul(c + 1, &c, 10) : 0;
	int b_confirm;

	/* Memorize calling control cginame */
	DYNBUF_ADD_BUF(&cginame, ctrl->cginame, NO_CONV);

	/* Handle selected main & alt form */
	NAME_VALUE("TOPFRM");
	if(cgi_get_values(cntxt, &cgival, name->data, 0)) STACK_ERROR;
	if(cgival.nbrows)
	{
		i = strtoul(cgival.cell->txt, &c, 10);
		if(i) DYNTAB_ADD_INT(&cntxt->id_obj, 0, 0, i);
		DYNTAB_ADD_INT(&cntxt->id_form, 0, 0, c ? strtoul(c + 1, &c, 10) : 0);
	}
	NAME_VALUE("BOTFRM");
	if(cgi_get_values(cntxt, &cgival, name->data, 0)) STACK_ERROR;
	if(cgival.nbrows)
	{
		i = strtoul(cgival.cell->txt, &c, 10);
		if(i) DYNTAB_ADD_INT(&cntxt->alt_obj, 0, 0, i);
		DYNTAB_ADD_INT(&cntxt->alt_form, 0, 0, c ? strtoul(c + 1, &c, 10) : 0);
	}

	/* Exit if view button clicked */
	if(!strncmp(btn, add_sz_str("VIEW"))) RETURN_OK;

	/* Read Confirm checkbox status */
	NAME_VALUE("CONFIRM");
	if(cgi_get_values(cntxt, &cgival, name->data, 0)) STACK_ERROR;
	b_confirm = cgival.nbrows > 0;
	DYNTAB_FREE(cgival);

	/* Handle close buttons */
	if(!strncmp(btn, add_sz_str("CLOSEALL"))) 
	{
		/* For each form found in CGI data */
		int b_exit = 1;
		for(i = 0; i < cntxt->nb_cgi; i++) if(cntxt->cgi[i].name[0] == 'F')
		{
			CGIData *cgi = cntxt->cgi + i;
			int b_modified = cgi_check_form_change(cntxt, cgi->IdForm, cgi->IdObj);
			if(b_modified == 2) continue;
			if(b_modified && (b_confirm || strcmp(btn, "CLOSEALLNOSAVE"))) { b_exit = 0; continue; }
			form_close_nosave(cntxt, cgi->IdForm, cgi->IdObj, &cgival);
		}
		if(b_exit) RETURN_OK;
	}
	else if(!strncmp(btn, add_sz_str("CLOSE")) && idform)
	{
		int b_modified = cgi_check_form_change(cntxt, idform, idobj);
		if(!b_modified || !b_confirm)
			form_close_nosave(cntxt, idform, idobj, &cgival);
		else
		{
			DYNTAB_ADD_INT(&cntxt->id_form, 0, 0, idform);
			DYNTAB_FREE(cntxt->id_obj);
			if(idobj) DYNTAB_ADD_INT(&cntxt->id_obj, 0, 0, idobj);
			if(form_load(cntxt, &cntxt->id_form, &cntxt->id_obj, 0) ||
				form_save_dialog(cntxt, 0, "_EVA_CONFIRMCHANGES", BUTN_SAVE_CLOSE | BUTN_CLOSE | BUTN_RESTORE, NULL)) STACK_ERROR;
			RETURN_OK;
		}
	}
	else if(!strcmp(btn, "MENU"))
	{
		DYNTAB_FREE(cntxt->id_form);
		DYNTAB_FREE(cntxt->id_obj);
		DYNTAB_FREE(cntxt->alt_form);
		DYNTAB_FREE(cntxt->alt_obj);
		RETURN_OK;
	}

	/* Output opened forms list */
	if(dlg_openedforms_output(cntxt, cginame, b_confirm)) STACK_ERROR;

	/* Do not display other forms */
	cntxt->b_terminate = 15;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
