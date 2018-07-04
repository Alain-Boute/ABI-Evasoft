/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : form_io.c
** Description : form load & save functions
**      Author : Alain BOUTE
**     Created : Apr 8 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : form_load
** Description : load a form in cntxt->form
*********************************************************************/
#define ERR_FUNCTION "form_load"
#define ERR_CLEANUP if(cntxt->err.text) form_free(cntxt); \
					DYNTAB_FREE(ctrltree)
int form_load(
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_form,				/* in : form to load (IdObj) */		
	DynTable *id_obj,				/* in : object to load in form (IdObj) */		
	int mode						/* in : display mode
											0 : automatic
											1 : edit
											2 : read only
											3 : print
											4 : footer */
){
	EVA_form *form = NULL;
 	EVA_ctrl *ctrl;
	DynTable ctrltree = { 0 };
	unsigned long idobj = DYNTAB_TOUL(id_obj);
	unsigned long idform = DYNTAB_TOUL(id_form);
	int b_noacc = 0;
	int b_opttab = 0;

	/* Alloc & intialize form structure */
	M_ALLOC(form, sizeof(EVA_form));
	DYNTAB_ADD_INT(&form->id_form, 0, 0, idform);
	form_free(cntxt);
	cntxt->form = form;

	/* Store object id if not null */
	if(idobj) DYNTAB_ADD_INT(&form->id_obj, 0, 0, idobj);
	form->b_newobj = idobj == 0;

	/* Read current object data if object exists */
	if(idobj && qry_obj_idfield(cntxt, &form->objdata, idobj, 0)) STACK_ERROR;

	/* Create new control for form */
	form->step = CtrlRead;
	if(ctrl_add_new(cntxt, idform, NULL)) STACK_ERROR;

	/* Set form params : cginame, previous context, access */
	ctrl = form->ctrl;
	if(cgi_build_basename(cntxt, &ctrl->cginame, 0, 'F') ||
		cgi_get_values(cntxt, &ctrl->cgival, form->ctrl->cginame->data, 0))	STACK_ERROR;
	if(dyntab_from_list(&ctrltree, DYNTAB_VAL_SZ(&ctrl->cgival, 0, 0), "§", 0, 4)) RETURN_ERR_MEMORY;
	if(dyntab_sz(&ctrltree, 0, 0)) DYNTAB_ADD_CELL(&form->call_data, 0, 0, &ctrltree, 0, 0);
	form->prevstep = atoi(dyntab_val(&ctrltree, 2, 0));
	if(dyntab_sz(&ctrltree, 3, 0)) DYNTAB_ADD_CELL(&form->dlg_ctrl, 0, 0, &ctrltree, 3, 0);

	/* Check if control is a form */
	if(!strcmp(ctrl->CONTROL, "_EVA_FORM") || mode == 4)
	{
		/* Handle save as form */
		if(!dyntab_sz(&form->id_obj, 0, 0) || !CTRL_ATTR_CELL(NO_FORMSTAMP))
		{
			form->id_formsave = strtoul(CTRL_ATTR_VAL(FORMSAVEAS), NULL, 10);
			if(!form->id_formsave) form->id_formsave = DYNTAB_TOUL(&form->id_form);
		}
		b_opttab = *CTRL_ATTR_VAL(OPTIMIZE_TAB) == '1';

		/* Calc output mode : check access, new object & edit on open in user preferences */
		if(ctrl_check_access(cntxt, form->ctrl)) STACK_ERROR;
		form->nextstep = mode == 1 ? HtmlEdit : (mode == 2 || mode == 4) ? HtmlView : mode == 3 ? HtmlPrint :
					form->prevstep > HtmlSaveDlg ? form->prevstep :
					(DYNTAB_FIELD_CELL(&cntxt->user_data, OPEN_MODE_EDIT) || !idobj) ? 
						HtmlEdit : HtmlView;
		if(idobj || !CTRL_ATTR_CELL(DISPLAYFIELDS))
		{
			form->step = form->nextstep;
			if(form->step && ctrl_check_access(cntxt, form->ctrl)) STACK_ERROR;
			form->step = CtrlRead;
		}

		/* Check if user has access rights to the form */
		b_noacc = (idobj ? ctrl->access & (AccessEdit | AccessView) :
						(!CTRL_ATTR_CELL(DISPLAYFIELDS) && (ctrl->access & AccessView) ||
							(ctrl->access & AccessCreate))) ? 0 : 1;
	}
	else
		b_noacc = 1;

	/* Access granted */
	if(!b_noacc)
	{
		/* Set View mode if no edit access */
		if(dyntab_sz(&form->id_obj, 0, 0) && form->nextstep == HtmlEdit && !(ctrl->access & AccessEdit))
			form->nextstep = HtmlView;

		/* Handle tab selection */
		if(b_opttab)
		{
			CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
			if(cgi->IdForm == idform && cgi->IdObj == idobj && cgi->name[0] == 'T')
				form->opttabid = cgi->IdCtrl;
			if(!form->opttabid)
			{
				if(cgi_filter_values(cntxt, &ctrltree,
					'D', ~0UL, idform, idobj, "_EVA_SELTAB", "", 0, 0)) STACK_ERROR;
				form->opttabid = DYNTAB_TOUL(&ctrltree);
			}
			if(!form->opttabid) form->opttabid = ~0UL;
		}

		/* Read form controls */
		CTRL_ATTR(ctrltree, CTRLTREE);
		if(ctrl_add_child(cntxt, 0, &ctrltree)) STACK_ERROR;
		ctrl = form->ctrl;
		if(idobj)
		{
			form->step = HtmlView;
			if(ctrl_check_access(cntxt, ctrl)) STACK_ERROR;
			b_noacc = (idobj ? ctrl->access & (AccessEdit | AccessView) : ctrl->access & AccessCreate) ? 0 : 2;
			form->step = CtrlRead;
		}

		/* Handle tab selection */
		if(form_tab_selection(cntxt, 1)) STACK_ERROR;
	}

	/* Access not granted */
	else if(dyntab_cmp(id_form, 0, 0, &cntxt->menubar, 0, 0))
	{
		/* Output error message for identified user */
		form->html = &form->html_top;
		if(cntxt->b_identified)
		{
			DYNBUF_ADD_STR(form->html, 
				"<td align=center>"
				"<font size=+1><b><br><br>Accès non autorisé<br><br></b></font>");
			if(idobj)
 				DYNBUF_ADD_STR(form->html, "L'accès à cette fiche ")
			else if(CTRL_ATTR_CELL(DISPLAYFIELDS) && !strcmp(ctrl->CONTROL, "_EVA_FORM"))
 				DYNBUF_ADD_STR(form->html, "L'ajout de fiches")
			else
 				DYNBUF_ADD_STR(form->html, "L'accès au formulaire");
			DYNBUF_ADD3(form->html, " [", ctrl->LABEL, 0, TO_HTML, 
				"] vous est refusé<br><br>"
				"Contactez votre administateur si vous avez besoin de cet accès<br><br>");
			if(put_html_button(cntxt, "B$#.CLOSE", "Revenir", "_eva_btn_gobackobj_fr.gif", "_eva_btn_gobackobj_fr_s.gif",
									"Retourne à la page précédente", 0, 0)) STACK_ERROR;
			DYNBUF_ADD_STR(form->html, "<br><br></td>");
		}
		/* Output login message for non identified user */
		else
		{
			int b_showid = strtoul(DYNTAB_FIELD_VAL(&cntxt->cnf_data, ALLOW_CLEAR_LOGIN), NULL, 10);
			DYNBUF_ADD3(form->html, 
				"<td align=center>"
				"<table noborder width=100% bgcolor=#FFFFFF><tr><td align=center>"
				"<b>Vous devez vous identifier pour accéder à cette fiche</b>"
				"</td></tr></table><hr>"
				"<table noborder width=100% bgcolor=#FFFFFF><tr><td>"
				"</td></tr><tr><td align=right width=50%><font size=-1 color=#0000AA>"
					"Identifiant</font></td><td><input type=", b_showid ? "text" : "password", 0, NO_CONV," name='D$#_EVA_LOGINSUBMIT'></td></tr>"
				"<tr><td align=right><font size=-1 color=#0000AA>"
					"Mot de passe</font></td><td><input type=password name='D$#_EVA_LOGINSUBMITP'>"
				"</td></tr></table><hr>"
				"<table noborder width=100% height=50 bgcolor=#FFFFFF><tr><td align=center>"
				"<input type=submit value=\"Valider l'identification\"></td></tr></table><hr>");
			if(cntxt->jsenabled) DYNBUF_ADD_STR(form->html,
				"<script>document.mainform['D$#_EVA_LOGINSUBMIT'].focus();</script>");
		}

		/* Add auto title */
		CTRL_SETATTRDEF("1", AUTO_TITLEBAR);
	}
	
	/* Restore used database if extern form */
	if(form->b_extern && sql_use_db(cntxt, cntxt->dbname)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_load_from_ctrl
** Description : open form & object and memorize caller data
*********************************************************************/
#define ERR_FUNCTION "form_load_from_ctrl"
#define ERR_CLEANUP DYNTAB_FREE(idform); \
					DYNTAB_FREE(sqlres); \
					DYNTAB_FREE(data); \
					M_FREE(call_data)

int form_load_from_ctrl(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *id_form,				/* in : form to load */		
	DynTable *id_obj,				/* in : object to load */		
	int loc,						/* in : where to place new form
											0 : same as caller
											1 : top
											2 : bottom (alt form)
											3 : replace caller form */
	int mode						/* in : display mode
											0 : automatic
											1 : edit
											2 : read only
											3 : print */
){
	EVA_form *form = cntxt->form;
	DynTable idform = { 0 };
	DynTable sqlres = { 0 };
	DynTable data = { 0 };
	DynBuffer *call_data = NULL;
	CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
	unsigned long idform0 = form ? DYNTAB_TOUL(&form->id_form) : 0;
	unsigned long idobj0 = form ? DYNTAB_TOUL(&form->id_obj) : 0;
	int b_reuse = loc == 3;
	if(b_reuse) loc = 0;

	/* Check params */
	if(!dyntab_sz(id_form, 0, 0) && !dyntab_sz(id_obj, 0, 0)) RETURN_OK;

	/* Set placement if same as caller */
	if(!loc) loc = (!form || !dyntab_sz(&cntxt->alt_form, 0, 0) || 
		dyntab_cmp(&form->id_obj, 0, 0, &cntxt->alt_obj, 0, 0) || 
		dyntab_cmp(&form->id_form, 0, 0, &cntxt->alt_form, 0, 0)) ? 1 : 2;

	/* If no form given */
	if(!dyntab_sz(id_form, 0, 0))
	{
		/* Read object formstamp */
		if(qry_obj_idfield(cntxt, &sqlres, DYNTAB_TOUL(id_obj), cntxt->val_FORMSTAMP)) STACK_ERROR;

		/* TODO - handle form access - take first available */
		DYNTAB_ADD_CELL(&idform, 0, 0, &sqlres, 0, 0);
	}
	else
		DYNTAB_ADD_CELL(&idform, 0, 0, id_form, 0, 0);

	/* If no form to open : return */
	if(!dyntab_sz(&idform, 0, 0)) RETURN_OK;

	/* reuse caller data if reuse form */
	if(b_reuse)
	{
		if(!form)
		{
			idform0 = cgi->IdForm;
			idobj0 = cgi->IdObj;
			if(cgi_filter_values(cntxt, &sqlres, 'F', ~0UL, idform0, idobj0, NULL, NULL, 0, 0)) STACK_ERROR;
			if(sqlres.nbrows && sqlres.cell->len)
			{
				size_t sz = strcspn(sqlres.cell->txt, "§");
				DYNBUF_ADD(&call_data, sqlres.cell->txt, sz, NO_CONV);
			}
		}
		else
			DYNBUF_ADD_CELL(&call_data, &form->call_data, 0, 0, NO_CONV);
	}
	else
	{
		/* Memorise caller data : id_form/id_obj/id_ctrl/Num.Line  */
		if(form)
		{
			DYNBUF_ADD_CELL(&call_data, &form->id_form, 0, 0, NO_CONV);
			DYNBUF_ADD3_CELL(&call_data, "/", &form->id_obj, 0, 0, NO_CONV, "/");
		}
		else
		{
			DYNBUF_ADD_INT(&call_data, cgi->IdForm);
			DYNBUF_ADD3_INT(&call_data, "/", cgi->IdObj, "/");
		}
		DYNBUF_ADD_INT(&call_data, cgi->IdCtrl);
		DYNBUF_ADD3_INT(&call_data, "/", cgi->Num, ".");
		DYNBUF_ADD_INT(&call_data, cgi->Line);
	}

	/* Set next opened form & obj */
	DYNTAB_ADD_CELL(loc == 2 ? &cntxt->alt_form: &cntxt->id_form, 0, 0, &idform, 0, 0);
	DYNTAB_ADD_CELL(loc == 2 ? &cntxt->alt_obj : &cntxt->id_obj, 0, 0, id_obj, 0, 0);

	/* Clear previous form is reused */
	if(b_reuse) cgi_clear_form_inputs(cntxt, idform0, idobj0, 0);

	/* Load new form & set caller data */
	if(form_load(cntxt, &idform, id_obj, mode)) STACK_ERROR;
	if(call_data && !cntxt->form->call_data.nbrows)
		DYNTAB_ADD_BUF(&cntxt->form->call_data, 0, 0, call_data);
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_close_nosave
** Description : close the given form & obj without saving
*********************************************************************/
#define ERR_FUNCTION "form_close_nosave"
#define ERR_CLEANUP	DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(id_form); \
					DYNTAB_FREE(id_ctrl); \
					DYNTAB_FREE(call_data1); \
					DYNTAB_FREE(field); \
					M_FREE(name)
int form_close_nosave(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context field */
	unsigned long idform,			/* in : form id to search for close  */
	unsigned long idobj,			/* in : object id to search for close */
	DynTable *call_data				/* in : caller data for form close */
){
	DynTable id_obj = { 0 };
	DynTable id_form = { 0 };
	DynTable id_ctrl = { 0 };
	DynTable call_data1 = { 0 };
	DynTable field = { 0 };
	DynBuffer *name = NULL;
	unsigned long idobj0 = DYNTAB_TOUL(&cntxt->id_obj);
	unsigned long idform0 = DYNTAB_TOUL(&cntxt->id_form);
	unsigned long idobj1 = DYNTAB_TOUL(&cntxt->alt_obj);
	unsigned long idform1 = DYNTAB_TOUL(&cntxt->alt_form);
	unsigned long homepage = DYNTAB_TOUL(&cntxt->homepage);
	unsigned long menubar = DYNTAB_TOUL(&cntxt->menubar);
	int b_alt = idform1 == idform && idobj1 == idobj;
	DynTable *dest_form = b_alt ? &cntxt->alt_form : &cntxt->id_form;
	DynTable *dest_obj = b_alt ? &cntxt->alt_obj : &cntxt->id_obj;
	unsigned long i;

	/* Clear current form position */
	dyntab_free(dest_form);
	dyntab_free(dest_obj);

	/* If no caller field given */
	if(!call_data)
	{
		/* Search for form field in CGI input */
		for(i = 0; i < cntxt->nb_cgi && !(cntxt->cgi[i].name[0] == 'F' &&
					cntxt->cgi[i].IdObj == idobj && cntxt->cgi[i].IdForm == idform); i++);
		if(i < cntxt->nb_cgi)
		{
			call_data = &call_data1;
			if(cgi_extract(cntxt, cntxt->cgi[i].value, '§', call_data, NULL))
				STACK_ERROR;
		}
	}

	/* Parse caller field */
	if(cgi_extract(cntxt, dyntab_val(call_data, 0, 0), '/', &id_form, &id_obj, &id_ctrl, &field, NULL))
		STACK_ERROR;

	/* If caller field available */
	if(dyntab_sz(&id_form, 0, 0))
	{
		unsigned long idobj2 = DYNTAB_TOUL(&id_obj);
		unsigned long idform2 = DYNTAB_TOUL(&id_form);

		/* Put caller form & obj in destination location */
		DYNTAB_ADD_CELL(dest_form, 0, 0, &id_form, 0, 0);
		dyntab_free(dest_obj);
		if(idobj2) DYNTAB_ADD_CELL(dest_obj, 0, 0, &id_obj, 0, 0);

		/* Clear form position if home page or menubar or already in page */
		if(idform2 == homepage) form_set_homepage(cntxt);
		else if(idform2 == menubar ||
			(idform2 == idform0 && idobj2 == idobj0) ||
			(idform2 == idform1 && idobj2 == idobj1))
		{
			dyntab_free(dest_form);
			dyntab_free(dest_obj);
		}
	}

	/* If main form closed : place alt form as top form */
	if(!dyntab_cmp(&cntxt->alt_obj, 0, 0, &cntxt->id_obj, 0, 0) &&
		!dyntab_cmp(&cntxt->alt_form, 0, 0, &cntxt->id_form, 0, 0))
	{
		DYNTAB_FREE(cntxt->alt_form);
		DYNTAB_FREE(cntxt->alt_obj);
	}
	if(!dyntab_sz(&cntxt->id_form, 0, 0) && dyntab_sz(&cntxt->alt_form, 0, 0))
	{
		DYNTAB_FREE(cntxt->id_obj);
		DYNTAB_ADD_CELL(&cntxt->id_form, 0, 0, &cntxt->alt_form, 0, 0);
		if(dyntab_sz(&cntxt->alt_obj, 0, 0)) DYNTAB_ADD_CELL(&cntxt->id_obj, 0, 0, &cntxt->alt_obj, 0, 0);
		DYNTAB_FREE(cntxt->alt_form);
		DYNTAB_FREE(cntxt->alt_obj);
	}

	/* Clear form data in CGI inputs */
	cgi_clear_form_inputs(cntxt, idform, idobj, 0);

	/* If no form to display */
	if(!dyntab_sz(&cntxt->id_form, 0, 0))
	{
		/* Look for next opened form in CGI inputs */
		unsigned long i = 0;
		if(cntxt->nb_cgi) for(i = 1; i < cntxt->nb_cgi; i++)
			if(cntxt->cgi[i].name[0] == 'F' && cntxt->cgi[i].IdForm &&
				cntxt->cgi[i].IdForm != homepage &&
				cntxt->cgi[i].IdForm != menubar)
				break;
		if(i < cntxt->nb_cgi)
		{
			DYNTAB_ADD_INT(&cntxt->id_form, 0, 0, cntxt->cgi[i].IdForm);
			DYNTAB_FREE(cntxt->id_obj);
			if(cntxt->cgi[i].IdObj) DYNTAB_ADD_INT(&cntxt->id_obj, 0, 0, cntxt->cgi[i].IdObj);
		}				   
	}
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_output
** Description : output the loaded form in cntxt->html or cntxt->html1
*********************************************************************/
#define ERR_FUNCTION "form_output"
#define ERR_CLEANUP DYNTAB_FREE(ctrltree)
int form_output(					/* return : 0 on success, other on error */
	EVA_context *cntxt				/* in/out : execution context data */
){
	EVA_form *form = cntxt->form;
	DynTable ctrltree = { 0 };
	int b_alt = form && 
				!dyntab_cmp(&cntxt->alt_form, 0, 0, &form->id_form, 0, 0) &&
				!dyntab_cmp(&cntxt->alt_obj, 0, 0, &form->id_obj, 0, 0);
	DynTable *id_form = b_alt ? &cntxt->alt_form : &cntxt->id_form;
	DynTable *id_obj = b_alt ? &cntxt->alt_obj : &cntxt->id_obj;
	int b_done = 0;
	if(!form) RETURN_OK;

	/* If dialog mode not processed : call control action handler */
	if(!form->html && dyntab_sz(&form->dlg_ctrl, 0, 0))
	{
		/* Get dialog control index */
		unsigned long i = ctrl_from_cginame(cntxt, DYNTAB_VAL_SZ(&form->dlg_ctrl, 0, 0));
		if(i) 
		{
			/* Control index found : use control handler */
			if(!strcmp(form->ctrl[i].CONTROL, "_EVA_BUTTON"))
			{
				/* Button dialog : call action handler */
				CTRL_ACT_HDLR(i);
			}
			else
			{
				/* Other controls : call secondary handler */
				form->step = form->nextstep;
				form->html = &form->html_top;
				CTRL_SEC_HDLR(i);
			}
		}
		else
			/* Control index not found : simulate close button */
			if(form_save_dialog(cntxt, 0, "_EVA_CONFIRMCHANGE", BUTN_CLOSE | BUTN_SAVE_CLOSE, &b_done)) STACK_ERROR;

		/* Do not output form if cleared */
		if(!cntxt->form) RETURN_OK;
	}

	/* Reload form if applicable */
	if(form->b_reload)
	{
		EVA_execmode nextstep = form->nextstep;
		if(form_load(cntxt, id_form, id_obj, 0)) STACK_ERROR;
		form = cntxt->form;
		form->nextstep = nextstep;
	}

	/* If no output prepared : prepare form HTML output */
	if(!form->html)
	{
		/* Set display mode */
		if(form->nextstep < HtmlSaveDlg || form->nextstep > FormValues)
			form->nextstep = HtmlView;
		else if(dyntab_sz(&form->id_obj, 0, 0) &&
				form->nextstep == HtmlEdit &&
				!(form->ctrl->access & AccessEdit))
			form->nextstep = HtmlView;

		/* Prepare form buffers */
		ATTR_OPTIONAL(ctrltree, CTRLTREE, form->ctrl);
		form_free_html(cntxt);
		switch(form->nextstep)
		{
		case HtmlEdit:
			/* Check input if edit mode */
			form->step = InputCheck;
			if(ctrl_add_child(cntxt, 0, &ctrltree)) STACK_ERROR;
			
		default:
			form->step = form->nextstep;
			form->html = &form->html_top;
			if(ctrl_add_child(cntxt, 0, &ctrltree)) STACK_ERROR;
		}
	}

	/* Output form */
	if(cntxt->form && form_put_html(cntxt)) STACK_ERROR;

	/* Switch to alt buffer if form displayed in bottom position */
	if(b_alt)
	{
		M_FREE(cntxt->html1);
		cntxt->html1 = cntxt->html;
		cntxt->html = NULL;
	}
	form_free(cntxt);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : form_get_html
** Description : open form & produce HTML output
*********************************************************************/
#define ERR_FUNCTION "form_get_html"
#define ERR_CLEANUP DYNTAB_FREE(idform); \
					DYNTAB_FREE(idobj); \
					DYNTAB_FREE(ctrltree)

int form_get_html(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **res,				/* in/out : buffer to output form HTML */		
	unsigned long id_form,			/* in : form to load */		
	unsigned long id_obj			/* in : object to load */		
){
	DynTable idform = { 0 };
	DynTable idobj = { 0 };
	DynTable ctrltree = { 0 };

	/* Load form */
	DYNTAB_ADD_INT(&idform, 0, 0, id_form);
	DYNTAB_ADD_INT(&idobj, 0, 0, id_obj);
	if(form_load(cntxt, &idform, &idobj, 4)) STACK_ERROR;
	ATTR_OPTIONAL(ctrltree, CTRLTREE, cntxt->form->ctrl);

	/* Send output to result */
	cntxt->form->html = res;
	cntxt->form->step = HtmlView;
	cntxt->form->ctrl->POSITION = "_EVA_SameCell";
	if(ctrl_primary_handler(cntxt, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
