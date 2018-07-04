/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_misc.c
** Description : handling fonctions for misceleanous buttons & actions
**      Author : Alain BOUTE
**     Created : March 27 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : action_openform
** Description : button click handler for OPENFORM
*********************************************************************/
#define ERR_FUNCTION "action_openform"
#define ERR_CLEANUP DYNTAB_FREE(id_form); \
					DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(destfield); \
					DYNTAB_FREE(srctype); \
					DYNTAB_FREE(srcobj); \
					DYNTAB_FREE(srcval); \
					DYNTAB_FREE(setmode); \
					DYNTAB_FREE(lineobj); \
					DYNTAB_FREE(debug); \
					M_FREE(expr)
int action_openform(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable id_form = { 0 };
	DynTable id_obj = { 0 };
	DynTable destfield = { 0 };
	DynTable srctype = { 0 };
	DynTable srcobj = { 0 };
	DynTable srcval = { 0 };
	DynTable setmode = { 0 };
	DynTable lineobj = { 0 };
	DynTable lineval = { 0 };
	DynTable debug = { 0 };
	DynBuffer *expr = NULL;
	char *formtype = CTRL_ATTR_VAL(OPENFORM_TYPE);
	char *objtype = CTRL_ATTR_VAL(OPENOBJ_TYPE);
	char *altform = CTRL_ATTR_VAL(ALTFORM);
	unsigned long i, j;
	int loc = 0, b_edit, openmode = atoi(CTRL_ATTR_VAL(OPEN_MODE));

	/* Handle save before */
	int b_done = 0;
	if(form_btn_do_save(cntxt, i_ctrl, &b_done)) STACK_ERROR;
	if(!b_done) RETURN_OK;
	form = cntxt->form;
	ctrl = form->ctrl + i_ctrl;

	/* Select opened form */
	if(!strcmp("_EVA_CURRENT", formtype))
	{
		char *srcfield = CTRL_ATTR_VAL(SRCFIELD);
		if(*srcfield)
		{
			if(ctrl_eval_fieldexpr(cntxt, ctrl, &id_form, srcfield)) CLEAR_ERROR;
		}
		else
			DYNTAB_ADD_CELL(&id_form, 0, 0, &form->id_obj, 0, 0);
	}
	else
	{
		CTRL_ATTR(id_form, OPENFORM);
	}

	/* Select opened object */
	if(*objtype)
	{
		CTRL_ATTR(srcval, OPENFIELD);
		CTRL_ATTR(srcobj, LISTOBJ);
		if(ctrl_eval_valtyp(cntxt, ctrl, &id_obj, objtype, &srcobj, &srcval)) CLEAR_ERROR;
		if(id_obj.nbrows > 1) id_obj.nbrows = 1;
		if(!DYNTAB_TOUL(&id_obj)) DYNTAB_FREE(id_obj);
	}

	/* Return if no form for object */
	if(!dyntab_sz(&id_obj, 0, 0) && !dyntab_sz(&id_form, 0, 0)) RETURN_OK;

	/* Set default form if none specified */
	if(!id_form.nbrows && qry_obj_field(cntxt, &id_form, DYNTAB_TOUL(&id_obj), "_EVA_FORMSTAMP")) STACK_ERROR;

	/* Set initial values */
	CTRL_ATTR_TAB(destfield, DESTFIELD);
	b_edit = destfield.nbrows > 0;
	CTRL_ATTR_TAB(srctype, SRCTYPE);
	CTRL_ATTR_TAB(srcobj, SRCLISTOBJ);
	CTRL_ATTR_TAB(srcval, SRCFIELD);
	CTRL_ATTR_TAB(setmode, SET_MODE);
	CTRL_ATTR_TAB(debug, DEBUG_SQL_EXPR);
	for(i = 0; i < destfield.nbrows; i++)
		if(dyntab_sz(&destfield, i, 0))
		{
			DynTableCell *c;
			int sql_trace = cntxt->sql_trace;
			dyntab_free(&lineobj);
			for(j = 0; j < srcobj.nbcols; j++) if(dyntab_sz(&srcobj, i, j))
			{
				unsigned long row = lineobj.nbrows;
				DYNTAB_SET_CELL(&lineobj, row, 0, &srcobj, i, j);
				c = dyntab_cell(&lineobj, row, 0);
				c->Num = 0;
				c->Line = 0;
			}
			M_FREE(expr);
			if(qry_eval_sqlexpr_var(cntxt, &expr, dyntab_val(&srcval, i, 0), &ctrl->attr, NULL)) STACK_ERROR;
			DYNTAB_ADD_BUF(&lineval, 0, 0, expr);
			if(*dyntab_val(&debug, i, 0)) cntxt->sql_trace = DEBUG_SQL_RES;
			if(form_set_values(cntxt,	
					DYNTAB_TOUL(&id_form),
					DYNTAB_TOUL(&id_obj),
					DYNTAB_VAL_SZ(&destfield, i, 0),
					dyntab_val(&srctype, i, 0),
					&lineobj,
					&lineval,
					dyntab_val(&setmode, i, 0)))
				CLEAR_ERROR;
			cntxt->sql_trace = sql_trace;
		}

	/* Transfer fields from current object */
	CTRL_ATTR_TAB(destfield, COPY_FIELDS);
	for(i = 0; i < destfield.nbrows; i++)
	{
		unsigned long idfield = DYNTAB_TOULRC(&destfield, i, 0);
		if(qry_cache_idobj(&lineobj, idfield) || ctrl_read_baseobj(cntxt,  &lineobj)) STACK_ERROR;
		DYNTAB_FIELD(&srcval, &lineobj, FIELD);
		if(form_set_values(cntxt,	
				DYNTAB_TOUL(&id_form),
				DYNTAB_TOUL(&id_obj),
				DYNTAB_VAL_SZ(&srcval, 0, 0),
				"_EVA_CURRENTOBJ",
				NULL,
				&srcval,
				"_EVA_REPLACE"))
			CLEAR_ERROR;
	}

	/* Select form location */
	if(!strcmp("_EVA_BOTTOM", altform)) loc = 2;
	else if(!strcmp("_EVA_TOP", altform)) loc = 1;
	else if(!strcmp("_EVA_REUSE", altform)) loc = 3;

	/* Open form with given object */
	if(form_load_from_ctrl(cntxt, &id_form, &id_obj, loc, 0)) STACK_ERROR;

	/* Set edit mode if enougth rights & data modified */
	form = cntxt->form;
	if(openmode)  form->nextstep = openmode == 3 ? HtmlPrint :
									(openmode == 1 && (form->ctrl->access & AccessEdit)) ? HtmlEdit : HtmlView;
	else if((b_edit || openmode == 1) && form->ctrl->access & AccessEdit) form->nextstep = HtmlEdit;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_formbuttons
** Description : button click handler for PRINT, SAVE, CLOSE, EDIT, ...
*********************************************************************/
#define ERR_FUNCTION "action_formbuttons"
#define ERR_CLEANUP	
int action_formbuttons(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	char *action = CTRL_ATTR_VAL(ACTION);
	char *confirm = CTRL_ATTR_VAL(CONFIRM);
	int buttons = 0, mode = 0;

	/* Handle close button on unchanged object */
	if(!strcmp(action, "_EVA_CLOSE") && (
		!dyntab_sz(&form->id_obj, 0, 0) && !form->b_modified ||
		form->prevstep == HtmlView))
	{
		if(form_close_nosave(cntxt, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), &form->call_data)) STACK_ERROR;
		RETURN_OK;
	}
	
	/* Handle button action */
#define DEFACTION(t, b, m)	if(!strcmp(action, "_EVA_"#t)) { buttons = b; mode = m; }
		 DEFACTION(SAVE, BUTN_SAVE_BACK, 0)
	else DEFACTION(CLOSE, BUTN_CLOSE, 0)
	else DEFACTION(SAVECLOSE, BUTN_SAVE_CLOSE, 0)
	else DEFACTION(PRINTMODE, (form->prevstep == HtmlPrint ? 0 : BUTN_SAVE_NEXT),
							(form->prevstep == HtmlPrint ? HtmlView : HtmlPrint))
	else DEFACTION(LISTMODE, 0, FormList)
	else DEFACTION(STATSMODE, BUTN_SAVE_NEXT, FormStats)
	else DEFACTION(VALUESMODE, 0, FormValues)
	else DEFACTION(EDITMODE, 0, HtmlEdit)
	else DEFACTION(VIEWMODE, BUTN_SAVE_NEXT, HtmlView)
	else DEFACTION(SEARCHMODE, 0, FormSearch)
	else DEFACTION(NEWOBJ, BUTN_SAVE_NEXT, HtmlEdit)
#undef DEFACTION

	/* Call save dialog */
	if(buttons)
	{
		int btn = buttons;
		if(form_save_dialog(cntxt, i_ctrl, confirm, buttons | BUTN_SAVE_CLOSE | BUTN_CLOSE | BUTN_RESTORE, &btn)) 
			STACK_ERROR;

		/* Set form mode if dialog succeded */
		if(btn == buttons && mode && form)
		{
			form->nextstep = mode;
		}
	}
	else 
	{
		form->nextstep = mode;
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_closeall
** Description : button click handler for CLOSEALL
*********************************************************************/
#define ERR_FUNCTION "action_closeall"
#define ERR_CLEANUP 
int action_closeall(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;

	/* TODO */
	ctrl = 0;

	DYNTAB_FREE(cntxt->id_form);
	DYNTAB_FREE(cntxt->id_obj);
	DYNTAB_FREE(cntxt->alt_form);
	DYNTAB_FREE(cntxt->alt_obj);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_end_session
** Description : button click handler for END_SESSION
*********************************************************************/
#define ERR_FUNCTION "action_end_session"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					DYNTAB_FREE(cgival); \
					M_FREE(name)
int action_end_session(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = { 0 };
	DynTable data = { 0 };
	DynBuffer *name = NULL;
	ObjTableFormat tbl = {0};
	unsigned long i, j;

	/* End session if confirm button clicked */
	if(!strcmp(CGI_CLICK_BTN_SUBFIELD, "ENDSESSION"))
	{
		if(action_closeall(cntxt, i_ctrl)) STACK_ERROR;
		M_FREE(cntxt->html);
		M_FREE(cntxt->html0);
		M_FREE(cntxt->html1);
		form_free(cntxt);
		cgi_free(cntxt);
		cntxt->b_identified= 0;
		if(qry_add_obj_field_val(cntxt, &cntxt->session, "_EVA_TERMINATE", add_sz_str("1"), 0, 1)) STACK_ERROR;
		DYNTAB_FREE(cntxt->session);
		RETURN_OK;
	}

	/* Output page header */
	put_html_page_header(cntxt, NULL, "Fin de session", NULL, 3);
	form->html = &form->html_top;
	cntxt->b_terminate = 15;
	fflush(stdout);
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("ENDSESSION"));
	printf(
		"<font face=Arial><br>"
		"<center><font size=+1><b>Terminer la session</b></font></center><br><hr><br>"
		"<u>Avertissement concernant la sécurité : </u>"
		"la plupart des navigateurs disposent d'une fonction 'Page précédente' qui permet "
		"de visualiser votre travail si vous laissez cette fenêtre ouverte. "
		"Il est donc fortement recommandé de fermer la fenêtre pour éviter toute indiscrétion, "
		"sauf si vous souhaitez vous identifier sous un autre nom. Dans ce cas utilisez le bouton ci dessous."
		"<br><br><center><input type=submit value='Changer d&#39;identification' name='%s'><br></center><hr>",
		name->data);

	/* Output forms with modified data in CGI */
	for(i = cntxt->nb_cgi - 1, j = 0; i; i--) if(cntxt->cgi[i].name[0] == 'F')
	{
		/* Check if form was modified */
		CGIData *cgi = cntxt->cgi + i;
		char *bgcolor = table_row_bgcolor(cntxt, &tbl, j, NULL);
		if(cgi_check_form_change(cntxt, cgi->IdForm, cgi->IdObj) != 1) continue;
		j++;

		/* Read object data */
		if(cgi_filter_values(cntxt, &cgival, 'D', ~0UL, 
								cgi->IdObj ? ~0UL : cgi->IdForm,
								cgi->IdObj, NULL, "", 0, 0)) STACK_ERROR;
		if(cgi->IdObj)
		{
			unsigned long k;
			if(qry_cache_idobj(&data, cgi->IdObj)) STACK_ERROR;
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
		DYNBUF_ADD_STR(form->html, "<tr>");
		if(ctrl_add_symbol_btn(cntxt, NULL, NULL, &cgival, 0, bgcolor, "SYMBOL+NAME+LABEL+OBJNOTES"))
			CLEAR_ERROR;
		DYNBUF_ADD_STR(form->html, "</tr>");
	}

	/* Output message & open buttons for modified forms if applicable */
	if(form->html_top)
	{
		printf("%s%s%s",
			"<u>Attention : vous avez modifié les formulaires suivants sans les enregistrer</u><br><br>"
			"<table border=0 width=100% bgcolor=#EEEEEE cellspacing=0 cellpadding=2><tr>",
			form->html_top->data,
			"</tr></table><br><center>"
			"<li>si voulez enregistrer vos modifications, ouvrez chaque fiche modifiée et enregistrez-la<br><br>"
			"<li>si vous ne voulez pas enregistrer vos modifications, fermez cette fenêtre</li>"
			"<br><a href='javascript:window.close()' title='Ferme cette fenêtre'>"
			"<br><br><img src='../img/_eva_btn_closenosave_fr.gif' border=0></a><br><br></center>");
	}
	else
	{	
		/* Close window */
		printf("<script>window.close();</script>");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_delete
** Description : button click handler for DELETE
*********************************************************************/
#define ERR_FUNCTION "action_delete"
#define ERR_CLEANUP	DYNTAB_FREE(t_keepobj); \
					DYNTAB_FREE(objdata); \
					DYNTAB_FREE(linkdata); \
					M_FREE(sql)
int action_delete(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i, j;
	EVA_ctrl *ctrl_delobj = 0;
	EVA_ctrl *ctrl_delmode = 0;
	unsigned long keepobj = 0;
	DynTable t_keepobj = {0};
	DynTable objdata = {0};
	DynTable linkdata = {0};
	DynBuffer *sql = NULL;

	/* TODO */
	ctrl = 0;

	/* Retrieve delete parameters controls index */
	for(i = 0; i < form->nb_ctrl; i++)
		if(!strcmp(form->ctrl[i].FIELD, "_EVA_DELOBJ")) ctrl_delobj = form->ctrl + i;
		else if(!strcmp(form->ctrl[i].FIELD, "_EVA_DELETE_TYPE")) ctrl_delmode = form->ctrl + i;
	if(!ctrl_delobj) RETURN_OK;

	/* If not delete all mode */
	if(!ctrl_delmode || strcmp(dyntab_val(&ctrl_delmode->val, 0, 0), "_EVA_DELETE_ALL"))
	{
		/* Keep 1st object : save id & remove from delete list */
		if(ctrl_delobj->val.nbrows < 2) RETURN_OK;
		keepobj = DYNTAB_TOUL(&ctrl_delobj->val);
		DYNTAB_ADD_CELL(&t_keepobj, 0, 0, &ctrl_delobj->val, 0, 0);
		dyntab_del_rows(&ctrl_delobj->val, 0, 1);

		/* Read links to be transfered to kept object */
		DYNBUF_ADD_STR(&sql, "(DateDel IS NULL AND IdRelObj IN (");
		if(qry_values_list(cntxt, &ctrl_delobj->val, 0, &sql)) STACK_ERROR;
		DYNBUF_ADD3_INT(&sql, ") AND IdObj NOT IN (", keepobj, ",");
		if(qry_values_list(cntxt, &ctrl_delobj->val, 0, &sql)) STACK_ERROR;
		DYNBUF_ADD_STR(&sql, "))");
		if(qry_get_obj_data(cntxt, &linkdata, sql->data)) STACK_ERROR;
		M_FREE(sql);

		/* If fusion mode */
		if(!ctrl_delmode || !strcmp(dyntab_val(&ctrl_delmode->val, 0, 0), "_EVA_DELETE_KEEPALL"))
		{
			/* Read objects data if fusion mode */
			DYNBUF_ADD_STR(&sql, "(DateDel IS NULL AND IdObj IN (");
			if(qry_values_list(cntxt, &ctrl_delobj->val, 0, &sql)) STACK_ERROR;
			DYNBUF_ADD3_INT(&sql, ") AND IF(ISNULL(IdRelObj),1,IdRelObj NOT IN (", keepobj, ",");
			if(qry_values_list(cntxt, &ctrl_delobj->val, 0, &sql)) STACK_ERROR;
			DYNBUF_ADD_STR(&sql, ")))");
			if(qry_get_obj_data(cntxt, &objdata, sql->data)) STACK_ERROR;
			M_FREE(sql);

			/* Copy other objects data to kept object */
			for(i = 0; i < objdata.nbrows; i++)
			{
				DynTableCell *c = dyntab_cell(&objdata, i, 0);
				c->IdObj = keepobj;
				if(qry_add_val(cntxt, c, 1, NULL)) STACK_ERROR;
			}
		}

		/* Copy links to kept object */
		for(i = 0; i < linkdata.nbrows; i++)
		{
			DynTableCell *c = dyntab_cell(&linkdata, i, 0);
			M_FREE(c->txt);
			c->b_dontfreetxt = 1;
			c->txt = dyntab_val(&t_keepobj, 0, 0);
			c->len = dyntab_sz(&t_keepobj, 0, 0);
			if(qry_add_val(cntxt, c, 1, NULL)) STACK_ERROR;
		}

		/* Handle IdWhoCr */
		DYNBUF_ADD3_INT(&sql, "-- Handle IdWhoCr\n"
			"UPDATE TLink SET IdWhoCr=", keepobj, "\nWHERE IdWhoCr IN(");
		if(qry_values_list(cntxt, &ctrl_delobj->val, 0, &sql)) STACK_ERROR;
		DYNBUF_ADD_STR(&sql, ")");
		if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
		M_FREE(sql);

		/* Handle IdWhoDel */
		DYNBUF_ADD3_INT(&sql, "-- Handle IdWhoDel\n"
			"UPDATE TLink SET IdWhoDel=", keepobj, "\nWHERE IdWhoDel IN(");
		if(qry_values_list(cntxt, &ctrl_delobj->val, 0, &sql)) STACK_ERROR;
		DYNBUF_ADD_STR(&sql, ")");
		if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
		M_FREE(sql);
	}

	/* Delete objects */
	DYNBUF_ADD3(&sql, "-- Delete objects & links\n"
		"UPDATE TLink SET DateDel='", cntxt->timestamp, 0, NO_CONV, "'");
	DYNBUF_ADD3_CELL(&sql, ",IdWhoDel=", &cntxt->id_user, 0, 0, NO_CONV, "\nWHERE DateDel IS NULL AND IdObj IN(");
	if(qry_values_list(cntxt, &ctrl_delobj->val, 0, &sql)) STACK_ERROR;
	DYNBUF_ADD_STR(&sql, ")");
	if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
	M_FREE(sql);

	/* Delete links to objects */
	DYNBUF_ADD3(&sql, "-- Delete objects & links\n"
		"UPDATE TLink SET DateDel='", cntxt->timestamp, 0, NO_CONV, "'");
	DYNBUF_ADD3_CELL(&sql, ",IdWhoDel=", &cntxt->id_user, 0, 0, NO_CONV, "\nWHERE DateDel IS NULL AND IdRelObj IN(");
	if(qry_values_list(cntxt, &ctrl_delobj->val, 0, &sql)) STACK_ERROR;
	DYNBUF_ADD_STR(&sql, ")");
	if(sql_exec_query(cntxt, sql->data)) STACK_ERROR;
	M_FREE(sql);

	/* Close forms containg deleted objects */
	if(cntxt->cgi)
	{
		unsigned long idform = DYNTAB_TOUL(&form->id_form);
		for(i = 0; i < ctrl_delobj->val.nbrows; i++)
		{
			unsigned long idobj = DYNTAB_TOULRC(&ctrl_delobj->val, i, 0);
			for(j = 0; j < cntxt->nb_cgi; j++)
			{
				CGIData *cgi = cntxt->cgi + j;
				if(cgi->IdObj == idobj || cgi->IdForm == idform) cgi->name[0] = 0;
			}
		}
	}
	form_close_nosave(cntxt, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), NULL);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_copyobj
** Description : button click handler for COPYOBJ
*********************************************************************/
#define ERR_FUNCTION "action_copyobj"
#define ERR_CLEANUP M_FREE(name)
int action_copyobj(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *name = NULL;

	/* Clear current clipboard values if applicable */
	if(*CTRL_ATTR_VAL(ADD) != '1')
	{
		unsigned long i;
		if(cgi_filter_values(cntxt, &ctrl->cgival, 'D', 0, 0, 0, "CB", NULL, 0, 0)) STACK_ERROR;
		for(i = 0; i < ctrl->cgival.nbrows; i++) cgi_value_setkeep(cntxt, &ctrl->cgival, i, 2);
	}

	/* Add new values */
	DYNBUF_ADD_STR(&name, "D0$0#CB$1");
	if(cgi_add_input(cntxt, NULL, DYNBUF_VAL_SZ(name), DYNTAB_VAL_SZ(&form->id_obj, 0, 0))) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_default
** Description : default handler for form clicks
*********************************************************************/
#define ERR_FUNCTION "action_default"
#define ERR_CLEANUP	DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(id_form); \
					DYNTAB_FREE(alt)
int action_default(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	DynTable id_obj = { 0 };
	DynTable id_form = { 0 };
	DynTable alt = { 0 };
	CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
	char *btn = CGI_CLICK_BTN_SUBFIELD;
	char *equal = btn ? strchr(btn, '=') : NULL;

	/* Switch on button type */
	switch(cgi->name[0])
	{
	case 'I':	/* Info button : display control info */
		/* Load alternate form */
		DYNTAB_ADD_INT(&id_form, 0, 0, OBJ_FORM_CONTROL);
		DYNTAB_ADD_INT(&id_obj, 0, 0, cgi->IdCtrl);
		if((cntxt->opt_btn_mode == OptBtn_OpenSame || cntxt->opt_btn_mode ==  OptBtn_OpenBottom) && 
			form_load_from_ctrl(cntxt, &id_form, &id_obj, cntxt->opt_btn_mode == OptBtn_OpenSame ? 0 : 2, 0)) STACK_ERROR;
		break;

	case 'T':	/* TAB button : memorize selected tab */
		form->seltab = i_ctrl;
		break;

	default:
		/* Check for program specific actions */
		if(equal)
		{
			/* Get parameters : IdObj,IdForm,Location,Mode */
			unsigned long idobj = 0, idform = 0;
			int loc = 0, mode = 0;

			if(!strncmp(btn, add_sz_str("OPENOBJ")))
			{
				/* Open object in given form */
				if(sscanf(equal + 1, "%lu,%lu,%d,%d", &idobj, &idform, &loc, &mode) > 0)
				{
					if(idform) DYNTAB_ADD_INT(&id_form, 0, 0, idform);
					if(idobj) DYNTAB_ADD_INT(&id_obj, 0, 0, idobj);
					if(form_load_from_ctrl(cntxt, &id_form, &id_obj, loc, mode)) STACK_ERROR;
				}
			}
			else if(!strncmp(btn, add_sz_str("CLOSESETVAL")))
			{
				char *sep = strchr(equal + 1, '=');
				dyntab_from_list(&id_obj, DYNTAB_VAL_SZ(&form->call_data, 0, 0), "/", 0, 6);
				idform = DYNTAB_TOUL(&id_obj);
				idobj = DYNTAB_TOULRC(&id_obj, 1, 0);
				form_close_nosave(cntxt, DYNTAB_TOUL(&form->id_form), DYNTAB_TOUL(&form->id_obj), NULL);
				if(sep && sep - equal > 1 && idform)
				{
					DYNTAB_ADD(&id_form, 0, 0, sep + 1, 0, NO_CONV);
					if(cgi_set_field_values(cntxt, idform, idobj, equal + 1, sep - equal - 1, &id_form, "_EVA_REPLACE", 0)) STACK_ERROR;
				}
			}
			else if(!strncmp(btn, add_sz_str("CLOSE")))
			{
				form_close_nosave(cntxt, idform, idobj, NULL);
			}
			else if(!strncmp(btn, add_sz_str("DELOBJLIB")))
			{
				if(!chdir(cntxt->rootdir) && !chdir("objlib"))
					remove(equal + 1);
			}
		}
		else if(!strcmp(btn, "CLOSE")) 
		{
			form_close_nosave(cntxt, DYNTAB_TOUL(form ? &form->id_form : &cntxt->id_form), 
										DYNTAB_TOUL(form ? &form->id_obj : &cntxt->id_obj),
										NULL);
		}
		break;
	}


	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
#undef CHECK_ACTION
