/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_sequence.c
** Description : button handler for executing sequences of actions
**      Author : Alain BOUTE
**     Created : June 10 2005
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : action_sequence_exec
** Description : exectute given actions sequence on current object
*********************************************************************/
#define ERR_FUNCTION "action_sequence_exec"
#define ERR_CLEANUP	M_FREE(buf); \
					DYNTAB_FREE(seq); \
					DYNTAB_FREE(data)
int action_sequence_exec(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *buf = NULL;
	DynTable seq = {0};
	DynTable data = {0};
	unsigned long i;
	int top = 
		!form || !dyntab_sz(&cntxt->alt_form, 0, 0) || 
		dyntab_cmp(&form->id_obj, 0, 0, &cntxt->alt_obj, 0, 0) || 
		dyntab_cmp(&form->id_form, 0, 0, &cntxt->alt_form, 0, 0);
	DynTable *idform = top ? &cntxt->id_form : &cntxt->alt_form;
	DynTable *idobj = top ? &cntxt->id_obj : &cntxt->alt_obj;

	/* Prepare actions list */
	DYNTAB_FIELD1(&seq, &ctrl->attr, CTRLTREE);
	if(!seq.nbrows) RETURN_OK;

	/* Process each action */
	for(i = 0; i < seq.nbrows; i++)
	{
		unsigned long i_btn = 1;

		/* Switch to homepage if no form selected */
		if(!dyntab_sz(idform, 0, 0))
		{
			if(form_set_homepage(cntxt)) STACK_ERROR;
			idform = &cntxt->id_form;
			idobj = &cntxt->id_obj;
		}

		/* Load current form if applicable */
		if((!cntxt->form || 
			dyntab_cmp(&form->id_obj, 0, 0, idobj, 0, 0) || 
			dyntab_cmp(&form->id_form, 0, 0, idform, 0, 0)) &&
			form_load(cntxt, idform, idobj, 0))
			STACK_ERROR;
		form = cntxt->form;

		/* Reload form if applicable */
		if(form->b_reload && form_load(cntxt, idform, idobj, 0)) STACK_ERROR;
		form = cntxt->form;

		/* Check if button in form */
		while(i_btn < form->nb_ctrl && dyntab_cmp(&form->ctrl[i_btn].id, 0, 0, &seq, i, 0)) i_btn++;

		/* Button not in form : read button in first available control */
		if(i_btn >= cntxt->form->nb_ctrl)
		{
			if(ctrl_add_new(cntxt, DYNTAB_TOULRC(&seq, i, 0), &i_btn)) STACK_ERROR;
			if(cgi_build_basename(cntxt, &cntxt->form->ctrl[i_btn].cginame, i_btn, 'B')) STACK_ERROR;
			CTRL_PRI_HDLR(i_btn);
		}

		/* Call action handler */
		form->ctrl[i_btn].CONTROL = "_EVA_BUTTON";
		CTRL_ACT_HDLR(i_btn);
		form = cntxt->form;

		/* Break loop if dialog mode */
		if(form && (form->nextstep == HtmlSaveDlg || form->html_top || form->html_tab))
		{
			DYNBUF_ADD_STR(&cntxt->debug_html, "<br>*** Dialog stopped action sequence !<br>");
			DYNTAB_FREE(form->dlg_ctrl);
			break;
		}
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_sequence_loop
** Description : execute action sequence on a list of objects
*********************************************************************/
#define ERR_FUNCTION "action_sequence_loop"
#define ERR_CLEANUP	DYNTAB_FREE(seq); \
					DYNTAB_FREE(listobj); \
					DYNTAB_FREE(updobj); \
					DYNTAB_FREE(id_obj); \
					DYNTAB_FREE(id_form)
int action_sequence_loop(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *updobjtyp,				/* in : type of source object */
	DynTable *updfield				/* in : fields to eval on source objects */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable seq = {0};
	DynTable listobj = {0};
	DynTable updobj = {0};
	DynTable id_obj = {0};
	DynTable id_form = {0};
	unsigned long i, j, cnt = 0, b_pbar;
	unsigned long pbar_minobj = strtoul(CTRL_ATTR_VAL(PROGBAR_MINOBJ), NULL, 10);

	/* Prepare actions list */
	DYNTAB_FIELD1(&seq, &ctrl->attr, CTRLTREE);
	if(!seq.nbrows) RETURN_OK;

	/* Prepare objects list */
	if(!strcmp(updobjtyp, "_EVA_FILTER"))
	{
		CTRL_ATTR(listobj, FILTERS);
		if(!listobj.nbrows) RETURN_WARNING("No filter given : button ignored");
	}
	else CTRL_ATTR(listobj, LISTOBJ);
	if(ctrl_eval_valtyp(cntxt, ctrl, &updobj, updobjtyp, &listobj, updfield)) STACK_ERROR;
	if(!updobj.nbrows) RETURN_OK;

	/* Display progress bar if applicable */
	b_pbar = updobj.nbrows > pbar_minobj;
	if(b_pbar)
	{
		put_html_page_header(cntxt, NULL, ctrl->LABEL, NULL, 3);
		printf("<hr><b>%s</b><br><pre>%s", ctrl->LABEL, WAIT_TXTSCALE);
		fflush(stdout);
	}

	/* Process each object */
	for(j = 0; j < updobj.nbrows; j++)
	{
		int b_err = 0;

		/* Display progress bar */
		if(b_pbar)
		{
			while((j + 1) * 60 / updobj.nbrows >= cnt)
			{
				putc('*', stdout);
				fflush(stdout);
				cnt++;
			}
		}

		/* Load object in base form */
		DYNTAB_ADD_CELL(&id_obj, 0, 0, &updobj, j, 0);
		if(qry_listobj_field(cntxt, &id_form, &id_obj, "_EVA_FORMSTAMP")) STACK_ERROR;
		if(id_form.nbrows > 1) id_form.nbrows = 1;
		if(form_load(cntxt, &id_form, &id_obj, 0)) STACK_ERROR;
		form = cntxt->form;

		/* Process each action */
		for(i = 0; i < seq.nbrows; i++)
		{
			unsigned long i_btn = 1;

			/* Reload form if applicable */
			if(form->b_reload && form_load(cntxt, &form->id_form, &form->id_obj, 0)) STACK_ERROR;
			form = cntxt->form;

			/* Check if button in form */
			while(i_btn < form->nb_ctrl && dyntab_cmp(&form->ctrl[i_btn].id, 0, 0, &seq, i, 0)) i_btn++;

			/* Button not in form : read button in first available control */
			if(i_btn >= cntxt->form->nb_ctrl)
			{
				if(ctrl_add_new(cntxt, DYNTAB_TOULRC(&seq, i, 0), &i_btn)) STACK_ERROR;
				if(cgi_build_basename(cntxt, &cntxt->form->ctrl[i_btn].cginame, i_btn, 'B')) STACK_ERROR;
				CTRL_PRI_HDLR(i_btn);
			}

			/* Call action handler for button */
			form->ctrl[i_btn].CONTROL = "_EVA_BUTTON";
			if(ctrl_call_act_handler(cntxt, i_btn)) { b_err = 1; CLEAR_ERROR; }
			form = cntxt->form;
			b_err |= form && (form->nextstep == HtmlSaveDlg || form->html_top || form->html_tab);

			/* Break loop on error */
			if(b_err) break;
		}

		/* Break loop & display message on error */
		if(b_err)
		{
			if(b_pbar)
			{
				puts("<hr>*** Dialog or error stopped action sequence !<br>");
				if(form->nextstep == HtmlSaveDlg) puts("*** Save Dialog<br>");
				if(form->html_top) printf("*** Form top dialog<br>%s", form->html_top->data);
				if(form->html_tab) printf("*** Form tab dialog<br>%s", form->html_tab->data);
				if(cntxt->debug_html) printf("*** Error<br>%s", cntxt->debug_html->data);
			}
			else
				RETURN_WARNING("*** Dialog or error stopped action sequence !");
			break;
		}
	}

	/* End progress bar */
	if(b_pbar)
	{
		printf("<br><hr><br><center>"
				"<input type=image name='B$#NOP' src='../img/_eva_btn_gobackobj_fr.gif'></center><br>\n");
		fflush(stdout);
		cntxt->b_terminate = 15;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : taskplan_sequence
** Description : handler for scheduled actions
*********************************************************************/
#define ERR_FUNCTION "taskplan_sequence"
#define ERR_CLEANUP	form_free(cntxt); \
					DYNTAB_FREE(actions); \
					M_FREE(html)
int taskplan_sequence(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	char *task						/* in : task type (DayTask / HourTask) */
){
	EVA_form *form = NULL;
	DynTable actions = {0};
	DynBuffer *html = NULL;
	unsigned long i, j = 0;
	time_t t;

	/* Initialize application */
	cntxt->b_task = 1;
	if(cgi_init_call(cntxt)) RETURN_OK;

	/* Set scheduled actions user */
	DYNTAB_FIELD(&cntxt->id_user, &cntxt->cnf_data, USER_TASKPLAN);
	if(!cntxt->id_user.nbrows) RETURN_OK;

	/* Read actions to execute */
	if(dyntab_filter_field(&actions, 0, &cntxt->cnf_data, !strcmp(task, "DayTask") ? "_EVA_ACTIONS_DAYTASK" : "_EVA_ACTIONS_HOURTASK", ~0UL, NULL))
		RETURN_ERR_MEMORY;

	/* Execute actions - handle stop on error */
	time(&t);
	printf("<html><head><title>%s - %s</title></head>\n<body>\n", task, asctime(localtime(&t)));
	printf("<hr>Processing %s - %s<hr>\n", task, asctime(localtime(&t)));
	for(i = 0; i < actions.nbrows; i++)
	{
		/* Set current form as home page */
		M_ALLOC(cntxt->form, sizeof(EVA_form));
		form = cntxt->form;
		DYNTAB_ADD_CELL(&form->id_form, 0, 0, &cntxt->homepage, 0, 0);
		j = 0;
		if(ctrl_add_new(cntxt, DYNTAB_TOULRC(&cntxt->homepage, 0, 0), &j)) STACK_ERROR;

		/* Load action button in form & alloc space for extra controls */
		if(ctrl_add_new(cntxt, DYNTAB_TOULRC(&actions, i, 0), &j)) STACK_ERROR;

		/* Call action handler for button & handle error */
		printf("\n<b>==> [%s]</b><hr>", form->ctrl[j].LABEL);
		if(ctrl_call_act_handler(cntxt, j))
		{
			printf("\n<br><b>*** Erreur de traitement</b><br>\n");
			CLEAR_ERROR;
		}

		/* Output action messages */
		form = cntxt->form;
		if(form->html_top) printf("%s<br>\n", form->html_top->data);
		if(form->html_tab) printf("%s<br>\n", form->html_tab->data);
		if(form->html_err) printf("%s<br>\n", form->html_err->data);
		put_debug_msg(cntxt, NULL);
		form_free(cntxt);
	}
	printf("</body></html>\n");
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_sequence
** Description : button click handler for ACTION_SEQ
*********************************************************************/
#define ERR_FUNCTION "action_sequence"
#define ERR_CLEANUP	DYNTAB_FREE(updfield)
int action_sequence(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable updfield = {0};
	char *confirm = CTRL_ATTR_VAL(CONFIRM);
	char *updobjtyp = CTRL_ATTR_VAL(UPDATE);
	if(!*updobjtyp) updobjtyp = "_EVA_CURRENTOBJ";

	/* Handle save before */
	if(*confirm && strcmp(confirm, "_EVA_NOSAVE"))
	{
		int b_done = BUTN_SAVE_NEXT;
		if(form_save_dialog(cntxt, i_ctrl, confirm, BUTN_SAVE_NEXT, &b_done)) STACK_ERROR;
		if(b_done != BUTN_SAVE_NEXT) RETURN_OK;
	}

	/* Handle objects loop */
	CTRL_ATTR(updfield, UPDATE_OBJFIELD);
	if(updfield.nbrows || strcmp(updobjtyp, "_EVA_CURRENTOBJ"))
	{
		if(action_sequence_loop(cntxt, i_ctrl, updobjtyp, &updfield)) STACK_ERROR;
	}
	else
	{
		if(action_sequence_exec(cntxt, i_ctrl)) STACK_ERROR;
	}
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
