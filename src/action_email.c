/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : action_email.c
** Description : button handler for sending email
**      Author : Alain BOUTE
**     Created : June 10 2005
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Typedef : EVA_sendmail
** Description : data for send email
*********************************************************************/
typedef struct _EVA_sendmail
{
	unsigned long i_ctrl;			/* control index in cntxt->form->ctrl */
	DynTableCell *from;				/* sender email adress */
	DynTable dst;					/* destination email adresses */
	DynTable cpy;					/* copy email adresses */
	DynTable hid;					/* hidden copy email adresses */
	DynTable att;					/* attached files */
	DynBuffer *txtbody;				/* mesage body */
	DynBuffer *txtsubj;				/* mesage subject */
	DynBuffer *cmd;					/* BLAT shell command */
	int b_html;						/* txtbody is HTML format if not 0 */
	int b_done;						/* email was succesfully sent if not 0 */
	char *err;						/* error message */
	char *err_file;					/* file to include in error message */
}
	EVA_sendmail;

/*********************************************************************
** Function : sendmail_free
** Description : free data & clear temporary files for email
*********************************************************************/
void sendmail_free(
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_sendmail *sm				/* in/out : send mail data */
){
	DYNTAB_FREE(sm->dst);
	DYNTAB_FREE(sm->cpy);
	DYNTAB_FREE(sm->hid);
	DYNTAB_FREE(sm->att);
	M_FREE(sm->txtsubj);
	M_FREE(sm->txtbody);
	M_FREE(sm->cmd);
	if(!sm->err) {
		chdir_user_doc(cntxt);
		remove("dst.txt");
		remove("cpy.txt");
		remove("hid.txt");
		remove("body.txt");
		remove("err.txt");
		remove("res.txt");
	}
}

/*********************************************************************
** Function : mailmerge_brackets
** Description : evaluate bracketed SQL instructions in a string
*********************************************************************/
#define ERR_FUNCTION "mailmerge_brackets"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					M_FREE(expr)
int mailmerge_brackets(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **res,				/* out : result string (brackets replaced with result) */
	char *str, size_t sz_str,		/* in : string with brackets */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl (for variables) */
	int conv						/* in : conversion mode for brackets result
										0 : NO_CONV
										1 : TO_HTML
										2 : NO_CONV if result starts with '<', TO_HTML else */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	char *start = str;
	DynTable data = {0};
	DynBuffer *expr = NULL;
	unsigned long i;

	/* Init */
	if(!res) RETURN_OK;

	/* For each SQL command */
	while(start && *start && (size_t)(start - str) < sz_str)
	{
		/* Locate start & end of brackets */
		char *next = strchr(start, '{');
		char *end = next ? strstr(next, "}") : NULL;
		if(!next) next = str + sz_str;
		if(!end) end = str + sz_str;

		/* Output text before brackets */
		if(next > start) DYNBUF_ADD(res, start, next - start, NO_CONV);

		/* Evaluate SQL expression on current object */
		DYNTAB_FREE(data);
		if(end > next + 1)
		{
			M_FREE(expr);
			DYNBUF_ADD(&expr, next + 1, end - next - 1, NO_CONV);
			if(ctrl_eval_fieldexpr(cntxt, ctrl, &data, expr->data)) STACK_ERROR;
		}

		/* Output results in place of brackets */
		for(i = 0; i < data.nbrows; i++)
		{
			char *txt = dyntab_val(&data, i, 0);
			ReplaceTable *sr = (conv == 1 || conv == 2 && *txt != '<') ? plain_to_html : NULL;
			if(i) DYNBUF_ADD_STR(res, ", ");
			if(dynbuf_add(res, txt, dyntab_sz(&data, i, 0), sr, sr ? 1 : 0)) RETURN_ERR_MEMORY;
		}
		start = end + 1;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : mail_file_link
** Description : output HTML link to a file for a mail
*********************************************************************/
#define ERR_FUNCTION "mail_file_link"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					M_FREE(buf)
int mail_file_link(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **res,				/* out : result string */
	unsigned long idobj,			/* in : file object to output */
	int b_html						/* in : output HTML code if set */
){
	DynTable data = {0};
	DynBuffer *buf = NULL;
	DynTableCell *fname, *name;
	char *size;
	if(qry_obj_field(cntxt, &data, idobj, NULL)) STACK_ERROR;
	name = DYNTAB_FIELD_CELL(&data, FILE_CLIENT_PATH);
	size = human_filesize(strtoul(DYNTAB_FIELD_VAL(&data, FILE_SIZE), NULL, 10));
	fname = DYNTAB_FIELD_CELL(&data, FILE_SERVER_NAME);
	if(!fname) RETURN_OK;
	DYNBUF_ADD3(&buf, "http://", cntxt->srvname, 0, NO_CONV, "/docs/");
	DYNBUF_ADD(&buf, cntxt->dbname, 0, NO_CONV);
	DYNBUF_ADD_STR(&buf, "/");
	if(b_html)
	{
		DYNBUF_ADD_CELLP(&buf, fname, HTML_NO_QUOTE);
		DYNBUF_ADD3_BUF(res, "<a href='", buf, NO_CONV, "' target=_blank>");
 		DYNBUF_ADD3(res, "", basename(name->txt, name->len), 0, TO_HTML, "</a> - ");
		DYNBUF_ADD(res, size, 0, NO_CONV);
	}
	else
	{
		if(cgi_output_string(cntxt, &buf, fname->txt, fname->len)) STACK_ERROR;
		DYNBUF_ADD(res, basename(name->txt, name->len), 0, NO_CONV);
		DYNBUF_ADD3(res, " - ", size, 0, NO_CONV, "\n\t");
		DYNBUF_ADD_BUF(res, buf, NO_CONV);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : attach_list
** Description : output attachement list
*********************************************************************/
#define ERR_FUNCTION "attach_list"
#define ERR_CLEANUP	
int attach_list(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_sendmail *sm				/* in/out : send mail data */
){
	DynBuffer **html = &sm->txtbody;
	unsigned long i;
	if(!sm->att.nbrows) RETURN_OK;

	/* Add attachements to body */
	if(sm->b_html)
	{
		DYNBUF_ADD_STR(html, "<font face=Arial><b>Pièces jointes :</b>");
		for(i = 0; i < sm->att.nbrows; i++)
		{
			DYNBUF_ADD3_INT(html, "<br>&nbsp;&nbsp;", i + 1, ") ");
			if(mail_file_link(cntxt, html, DYNTAB_TOULRC(&sm->att, i, 0), 1)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(html, "</font><br>");
	}
	else
	{
		DYNBUF_ADD_STR(html, "\nPièces jointes :\n");
		for(i = 0; i < sm->att.nbrows; i++)
		{
			DYNBUF_ADD3_INT(html, "\n  ", i + 1, ") ");
			if(mail_file_link(cntxt, html, DYNTAB_TOULRC(&sm->att, i, 0), 0)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(html, "\n");
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : show_adr_dest
** Description : output email adresses for email confirmation
*********************************************************************/
#define ERR_FUNCTION "show_adr_dest"
#define ERR_CLEANUP
int show_adr_dest(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynTable *dst,					/* in : email adresses */
	char *title, size_t sz_title	/* in : email title */
){
	DynBuffer **html = cntxt->form->html;
	if(!dst->nbrows) RETURN_OK;

	DYNBUF_ADD3(html, "<tr><td align=right valign=top><nobr><b>", title, sz_title, TO_HTML, " :</nobr></td><td>");
	if(dyntab_to_dynbuf(dst, html, add_sz_str(","), add_sz_str(", "), TO_HTML)) RETURN_ERR_MEMORY;
	DYNBUF_ADD_STR(html, "</td></tr>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : file_adr_dest
** Description : create file with email adresses & set command line
*********************************************************************/
#define ERR_FUNCTION "file_adr_dest"
#define ERR_CLEANUP
int file_adr_dest(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	DynBuffer **cmd,				/* in/out : send mail command line */
	DynTable *dst,					/* in : email adresses */
	char *fname,					/* in : output file name for adresses */
	char *opt						/* in : send mail command option */
){
	FILE *f = NULL;
	unsigned long i;

	/* Return if email empty */
	if(!dyntab_sz(dst, 0, 0)) RETURN_OK;

	/* Output adresses to file */
	f = fopen(fname, "wc");
	if(!f) RETURN_ERR_DIRECTORY(1);
	for(i = 0; i < dst->nbrows; i++) fprintf(f, "%s%s", i ? "," : "", dyntab_val(dst, i, 0));
	fclose(f);

	/* Add file to send mail command line */
	DYNBUF_ADD3(cmd, " -", opt, 2, NO_CONV, " ");
	DYNBUF_ADD(cmd, fname, 7, NO_CONV);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : send_mail
** Description : actually send a prepared email
*********************************************************************/
#define ERR_FUNCTION "send_mail"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					DYNTAB_FREE(res)
int send_mail(						/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_sendmail *sm				/* in/out : send mail data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + sm->i_ctrl;
	DynTable data = {0};
	DynTable res = {0};
	FILE *f = NULL;
	struct stat fs;
	int err;

	/* Create body text file */
	f = fopen("body.txt", "wc");
	if(!f) RETURN_ERR_DIRECTORY(1);
	if(sm->txtbody) fputs(sm->txtbody->data, f);

	/* If HTML mode & object exists : add invisible image for read confirmation */
	if(sm->b_html && dyntab_sz(&form->id_obj, 0, 0))
	{
		fprintf(f, "<img src='http://%s%s?MailRead=%s' width=1 height=1>",
					cntxt->srvname, cntxt->requri, dyntab_val(&form->id_obj, 0, 0));
	}
	fclose(f);

	/* Build BLAT command : exe, body & sender */
	DYNBUF_ADD3(&sm->cmd, "", cntxt->path, 0, NO_CONV, DD "BLAT.EXE body.txt -debug -noh2");
	if(sm->b_html) DYNBUF_ADD_STR(&sm->cmd, " -html");
	DYNBUF_ADD3_CELLP(&sm->cmd, " -mailfrom ", sm->from, NO_CONV, "");

	/* Build BLAT command : files with destination emails */
	if( file_adr_dest(cntxt, &sm->cmd, &sm->dst, "dst.txt", "tf") ||
		file_adr_dest(cntxt, &sm->cmd, &sm->cpy, "cpy.txt", "cf") ||
		file_adr_dest(cntxt, &sm->cmd, &sm->hid, "hid.txt", "bf"))
		STACK_ERROR;

	/* Build BLAT command : subject */
	DYNBUF_ADD3_BUF(&sm->cmd, " -s \"", sm->txtsubj, TO_SYSCMD, "\" >res.txt 2>err.txt");

	/* Launch BLAT command */
	err = system(sm->cmd->data);
	stat("err.txt", &fs);

	/* Handle error */
	if(fs.st_size)
	{
		sm->err = "Erreur lors du lancement de la commande d'envoi du message";
		sm->err_file = "err.txt";
	}
	else if(err)
	{
		sm->err = "Erreur lors de la transmission au serveur de messagerie";
		sm->err_file = "res.txt";
	}
	else
	{
		/* No error : handle sent storage */
		DynTableCell *c;
		CTRL_ATTR(data, STORE_SENT);
		if(data.nbrows)
		{
			/* Eval storage field if given */
			if(ctrl_eval_valtyp(cntxt, ctrl, &res, "_EVA_CURRENTOBJ", NULL, &data)) CLEAR_ERROR;
		}
		else
			/* Use sender email as default value to store message sent */
			DYNTAB_ADD_CELLP(&res, 0, 0, sm->from);

		/* Store message sent if applicable */
		c = dyntab_cell(&res, 0, 0);
		if(c)
		{
			/* Check if message sent info is an IdObj */
			char *end = NULL;
			unsigned long idsent = c->txt ? strtoul(c->txt, &end, 10) : 0;
			if(idsent && !*end)
			{
				c->IdValue = idsent;
				c->b_relation = 1;
			}

			/* Prepare message sent record */
			c->IdObj = DYNTAB_TOUL(&form->id_obj);
			if(!c->b_dontfreefield) M_FREE(c->field);
			c->b_dontfreefield = 1;
			c->field = "_EVA_MAIL_SENT_FROM";
			c->IdField = 0;
			if(qry_add_val(cntxt, c, 0, NULL)) STACK_ERROR;
		}
		sm->b_done = 1;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : preview_mail
** Description : display email before sending
*********************************************************************/
#define ERR_FUNCTION "preview_mail"
#define ERR_CLEANUP	M_FREE(name)
int preview_mail(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_sendmail *sm				/* in/out : send mail data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + sm->i_ctrl;
	DynBuffer *name = NULL;
	DynBuffer **html = form->html;

	/* Message header */
	DYNBUF_ADD3_CELLP(html, "<table border=0 bgcolor=#ffffff width=100%><tr><td>"
		"<table border=0><tr>"
		"<td align=right><b>Expéditeur :</td><td>", sm->from, NO_CONV, "</td></tr>");
	if(show_adr_dest(cntxt, &sm->dst, add_sz_str("Destinataire"))) STACK_ERROR;
	if(show_adr_dest(cntxt, &sm->cpy, add_sz_str("Copie à"))) STACK_ERROR;
	if(show_adr_dest(cntxt, &sm->hid, add_sz_str("Copie cachée"))) STACK_ERROR;
	DYNBUF_ADD3_BUF(html, "<tr><td align=right valign=top>"
							"<b>Sujet :</td><td>", sm->txtsubj, TO_HTML, "</td>"
							"</tr></table></td></tr><tr><td><hr>");
	if(sm->b_html)
		DYNBUF_ADD_BUF(html, sm->txtbody, NO_CONV)
	else
		DYNBUF_ADD3_BUF(html, "<font face=courier size=-1>", sm->txtbody, TO_HTML, "</font>");
	DYNBUF_ADD_STR(html, "<hr></td></tr></table><center>");
	CTRL_CGINAMEBTN(&name, NULL, add_sz_str("SENDMAIL"));
	if(put_html_button(cntxt, name->data, "Envoyer le message", NULL, NULL, NULL, 0, 0)) STACK_ERROR;
	DYNBUF_ADD_STR(html, "</center>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : prepare_mail
** Description : prepare email data for a control
*********************************************************************/
#define ERR_FUNCTION "prepare_mail"
#define ERR_CLEANUP	DYNTAB_FREE(data)
int prepare_mail(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	EVA_sendmail *sm				/* in/out : send mail data */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + sm->i_ctrl;
	DynTable data = {0};
	DynTableCell *c;
	char *link_pos = CTRL_ATTR_VAL(DOC_LINK_POS);

	/* Build destination adress list */
	CTRL_ATTR(data, EMAIL_DEST);
	if(data.nbrows && ctrl_eval_valtyp(cntxt, ctrl, &sm->dst, "_EVA_CURRENTOBJ", NULL, &data)) CLEAR_ERROR;
	CTRL_ATTR(data, EMAIL_DESTCOPY);
	if(data.nbrows && ctrl_eval_valtyp(cntxt, ctrl, &sm->cpy, "_EVA_CURRENTOBJ", NULL, &data)) CLEAR_ERROR;
	CTRL_ATTR(data, EMAIL_DESTHIDDEN);
	if(data.nbrows && ctrl_eval_valtyp(cntxt, ctrl, &sm->hid, "_EVA_CURRENTOBJ", NULL, &data)) CLEAR_ERROR;

	/* Return error if no destination adress */
	if(sm->dst.nbrows + sm->cpy.nbrows + sm->hid.nbrows == 0) 
	{
		sm->err = "Le message n'a pas de destinataire";
		RETURN_OK;
	}

	/* Buil subject */
	c = CTRL_ATTR_CELL(EMAIL_SUBJECT);
	if(c && mailmerge_brackets(cntxt, &sm->txtsubj, c->txt, c->len, sm->i_ctrl, 0)) CLEAR_ERROR;

	/* Return error if no subject */
	if(!sm->txtsubj)
	{
		sm->err = "Le message n'a pas de sujet";
		RETURN_OK;
	}

	/* Limit subject length */
	if(sm->txtsubj && sm->txtsubj->cnt > 100)
	{
		sprintf(sm->txtsubj->data + 97, "...");
		sm->txtsubj->cnt = 100;
	}

	/* Check HTML format */
	c = CTRL_ATTR_CELL(EMAIL_BODY);
	sm->b_html = c && c->txt[0] == '<';

	/* Build attachements list & output to top of body if applicable */
	CTRL_ATTR(data, EMAIL_ATTACH);
	if(data.nbrows && ctrl_eval_valtyp(cntxt, ctrl, &sm->att, "_EVA_CURRENTOBJ", NULL, &data)) CLEAR_ERROR;
	if(*link_pos != '1' && sm->att.nbrows)
	{
		if(attach_list(cntxt, sm)) STACK_ERROR;
		DYNBUF_ADD(&sm->txtbody, sm->b_html ? "<hr>" : "\n", 0, NO_CONV);
	}

	/* Build body */
	if(mailmerge_brackets(cntxt, &sm->txtbody, c->txt, c->len, sm->i_ctrl, sm->b_html ? 2 : 0)) CLEAR_ERROR;

	/* Output attachements list to end of body if applicable */
	if(*link_pos != '0' && sm->att.nbrows)
	{
		DYNBUF_ADD(&sm->txtbody, sm->b_html ? "<hr>" : "\n", 0, NO_CONV);
		if(attach_list(cntxt, sm)) STACK_ERROR;
	}

	/* Handle from adress */
	sm->from = dyntab_field_cell(&ctrl->attr, "_EVA_EMAIL_SENDER", ~0UL, 0);
	if(!sm->from) sm->from = dyntab_field_cell(&cntxt->user_data, "EMAIL", ~0UL, 0);
	if(!sm->from) sm->from = dyntab_cell(&cntxt->mail_admin, 0, 0);

	/* Handle copy to sender if applicable */
	if(*CTRL_ATTR_VAL(NO_SENDER_COPY) != '1')
	{
		c = dyntab_field_cell(&cntxt->user_data, "EMAIL", ~0UL, 0);
		if(!c) c = sm->from;
		DYNTAB_ADD_CELLP(&sm->hid, sm->hid.nbrows, 0, c);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : action_email
** Description : button click handler for EMAIL
*********************************************************************/
#define ERR_FUNCTION "action_email"
#define ERR_CLEANUP	M_FREE(errfile);\
					sendmail_free(cntxt, &sm)
int action_email(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	EVA_sendmail sm = {0};
	DynBuffer *errfile = NULL;
	char *dlgmode = CTRL_ATTR_VAL(EMAIL_DIALOG);
	unsigned long b_dlg = strcmp(dlgmode, "_EVA_NONE");
	unsigned long b_send = !strcmp(CGI_CLICK_BTN_SUBFIELD, "SENDMAIL");
	unsigned long b_err = 0;

	/* Handle save before */
	if(!b_send)
	{
		int b_done = BUTN_SAVE_NEXT;
		if(form_save_dialog(cntxt, i_ctrl, CTRL_ATTR_VAL(CONFIRM), BUTN_SAVE_NEXT, &b_done)) STACK_ERROR;
		if(b_done != BUTN_SAVE_NEXT) RETURN_OK;
	}

	/* Prepare message */
	sendmail_free(cntxt, &sm);
	sm.i_ctrl = i_ctrl;
	b_err = prepare_mail(cntxt, &sm) || sm.err;
	
	/* Send mail if applicable */
	if(!b_err && (b_send || !b_dlg || !strcmp(dlgmode, "_EVA_AFTER")))
	{
		b_err = send_mail(cntxt, &sm) || sm.err;
		b_dlg = !*dlgmode || !strcmp(dlgmode, "_EVA_AFTER") || !strcmp(dlgmode, "_EVA_ALWAYS");
	}

	/* Handle message dialog */
	if(b_err || b_dlg)
	{
		/* Set buffer & output dialog header */
		DynBuffer **html = &form->html_top;
		form->html = html;
		DYNBUF_ADD_STR(html, "<font face=Arial><center><b>Envoi d'un email</b></center><hr>");

		if(b_err)
		{
			/* Display error for unsent message */
			if(!sm.err) sm.err = cntxt->err.text;
			DYNBUF_ADD3(html, "<center>Le message n'a pas été envoyé pour la raison suivante :<br><br><b>",
								sm.err, strlen(sm.err), TO_HTML, "</b></center><br>");
			if(sm.err_file)
			{
				if(file_to_dynbuf(cntxt, &errfile, sm.err_file)) STACK_ERROR;
				DYNBUF_ADD3_BUF(html, "Détail de l'erreur :<hr>", errfile, TO_HTML, "<br>");
			}
		}
		else if(sm.b_done)
		{
			/* Confirm after message sent */
			DYNBUF_ADD_STR(html, "<center><b>Le message a bien été envoyé</b></center>");
		}
		else
		{
			/* Preview message */
			if(preview_mail(cntxt, &sm)) STACK_ERROR;
		}

		/* Output page footer */
		DYNBUF_ADD_STR(html, "<hr><br><table width=100%><tr><td align=center>");
		if(put_html_button(cntxt, "NOP", "Revenir", "_eva_btn_gobackobj_fr.gif", "_eva_btn_gobackobj_fr_s.gif",
								"Retourne à la page précédente", 0, 0)) STACK_ERROR;
		if(sm.b_done)
		{
			DYNBUF_ADD_STR(html, "</td><td align=center>");
			if(put_html_button(cntxt, "B$#.CLOSE", "Fermer", "_eva_btn_close_fr.gif", "_eva_btn_close_fr_s.gif",
								"Retourne au formulaire précédent", 0, 0)) STACK_ERROR;
		}
		DYNBUF_ADD_STR(html, "</td></tr></table><br>");
		cntxt->b_terminate = 15;
		cntxt->html = *form->html;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
