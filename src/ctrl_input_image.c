/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_image.c
** Description : Handling functions for image input controls
**      Author : Alain BOUTE
**     Created : Sept 5 2005
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Constant : spc_to_star
** Description : replace table for word search in files
*********************************************************************/
ReplaceTable spc_to_star[] = { { add_sz_str(" "), add_sz_str("*") },
								{ NULL } };
#define SPC_TO_STAR spc_to_star, 1

/*********************************************************************
** Function : ctrl_image_search
** Description : handle search dialog for images
*********************************************************************/
#define ERR_FUNCTION "ctrl_image_search"
#define ERR_CLEANUP	DYNTAB_FREE(cgival); \
					DYNTAB_FREE(files); \
					M_FREE(name); \
					M_FREE(srch)
int ctrl_image_search(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = {0};
	DynTable files = {0};
	DynBuffer *name = NULL;
	DynBuffer *srch = NULL;
	unsigned long i, szico;
	char szstr[8] = {0};

	/* Output dialog title */
	DYNBUF_ADD3(form->html, "<table border=1 rules=none width=100%><tr><td align=right>Chercher une image pour <b>",
							ctrl->LABEL, 0, TO_HTML, "</td>");

	/* Output file name search input & Ok button (preserve previous value) */
	CTRL_GET_CGI_SUBFIELD("FNAME");
	CTRL_CGINAMESUBFIELD(&name, NULL, "FNAME");
	DYNBUF_ADD3_BUF(form->html, "<td><input type=text name=", name, NO_CONV, " size=8 value=\"");
	DYNBUF_ADD_CELL(form->html, &cgival, 0, 0, NO_CONV);
	DYNBUF_ADD_STR(form->html, "\"><input type=submit value=Ok></td>");
	if(ctrl_autofocus(cntxt, DYNBUF_VAL_SZ(name), 1)) STACK_ERROR;

	/* Set search path */
	DYNBUF_ADD3_CELL(&srch, "../img/", &cgival, 0, 0, SPC_TO_STAR, "*");

	/* Output icons size input (preserve previous value - default 50) */
	CTRL_GET_CGI_SUBFIELD("SZICO");
	CTRL_CGINAMESUBFIELD(&name, NULL, "SZICO");
	szico = DYNTAB_TOUL(&cgival);
	if(!szico) szico = 50;
	snprintf(add_sz_str(szstr), "%lu", szico);
	DYNBUF_ADD3_BUF(form->html, "<td align=right>Taille icônes<td><input type=text name=", name, NO_CONV, " size=2 value=");
	DYNBUF_ADD_INT(form->html, szico);
	DYNBUF_ADD_STR(form->html, "> px</td></tr></table>");

	/* Handle input for seach string */
	if(srch && srch->cnt)
	{
		/* Read matching files */
		if(find_files(cntxt, &files, srch->data, 0)) STACK_ERROR;

		/* Output images */
		DYNBUF_ADD_STR(form->html, "<table border=1 rules=none width=100%><tr><td>");
		for(i = 0; i < files.nbrows; i++)
		{
			/* Build subfield with file name */
			srch->cnt = 0;
			DYNBUF_ADD3_CELL(&srch, "SEL", &files, i, 0, NO_CONV, "");
			CTRL_CGINAMEBTN1(&name, NULL, srch->data, srch->cnt);
			
			/* Build subfield with file name */
			if(put_html_image_dir(cntxt, name->data, NULL,
									dyntab_val(&files, i, 0),
									dyntab_val(&files, i, 0),
									szstr, szstr, 0, 0, NULL, NULL)) STACK_ERROR;
			DYNBUF_ADD_STR(form->html, " ");
		}
		DYNBUF_ADD_STR(form->html, "</tr></table>");
	}
	else
		DYNBUF_ADD_STR(form->html, "Entrer le texte à chercher");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_input_image
** Description : handles INPUT controls of type IMAGE
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_input_image"
#define ERR_CLEANUP	M_FREE(name)
int ctrl_add_input_image(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynBuffer *name = NULL;
	unsigned long i, k;
	int b_multiple = ctrl->MULTIPLE[0] != 0 && strcmp("No", ctrl->MULTIPLE);
	char *dir = CTRL_ATTR_VAL(IMAGE_DIR);
	char *imgdef = CTRL_ATTR_VAL(IMAGE_DEFAULT);
	char *w = CTRL_ATTR_VAL(IMG_WIDTH), *h = CTRL_ATTR_VAL(IMG_HEIGHT);
	
	switch(form->step)
	{
	case CtrlRead:
		/* Handle image search button click */
		if(form->i_ctrl_clic == i_ctrl)
		{
			CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
			char *btn = cgi->subfield;
			if(!strcmp(btn, "IMGSRCH"))
			{
				/* Search button clicked : initiate search dialog */
				DYNTAB_ADD_BUF(&form->dlg_ctrl, 0, 0, ctrl->cginame);
			}
			else if(!strncmp(btn, add_sz_str("SEL")))
			{
				/* Image selected : set input & terminate search dialog */
				DYNTAB_ADD(&ctrl->val, 0, 0, btn + 3, 0, NO_CONV);
				DYNTAB_FREE(form->dlg_ctrl);
			}
			else
				DYNTAB_FREE(form->dlg_ctrl);
		}
		break;

	case HtmlPrint:
	case HtmlView:
	case HtmlEdit:
		/* Handle image search dialog */
		if(!dyntab_txt_cmp(DYNBUF_VAL_SZ(ctrl->cginame), &form->dlg_ctrl, 0, 0))
		{
			if(ctrl_image_search(cntxt, i_ctrl)) STACK_ERROR;
			break;
		}

		if(!form->html) RETURN_OK;

		/* Handle default image size */
		if(!*w && !*h)
		{
			w = ctrl->WIDTH;
			h = ctrl->HEIGHT;
		}
		/* Output control header */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle empty value in view mode */
		if(form->step != HtmlEdit && !ctrl->val.nbrows && ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);

		/* Add HTML code for each value */
		k = ctrl->val.nbrows + ((form->step == HtmlEdit && b_multiple) ? 1 : 0);
		for(i = 0; !i || i < k; i++)
			if(form->step == HtmlEdit)
			{
				/* Edit mode : add image search button
				CTRL_CGINAMEBTN(&name, dyntab_cell(&ctrl->val, i, 0), add_sz_str("IMGSRCH"));
				if(put_html_button(cntxt, name->data, "Chercher", "_eva_search.gif", "_eva_search_s.gif", 
					"Cliquez pour chercher des images", 0, 0)) STACK_ERROR; */

				/* Separate multiple values with a line break */
				if(i) DYNBUF_ADD_STR(form->html, "<br>");

				/* Output text input */
				CTRL_CGINAMEVAL(&name, i);
				if(put_html_text_input(cntxt, DYNBUF_VAL_SZ(name), DYNTAB_VAL_SZ(&ctrl->val, i,0), 0, 0, 1, ctrl->COLUMNS, 0) ||
					ctrl_put_hidden_old(cntxt, ctrl, i, name, NULL, 0))
				STACK_ERROR;
			}
			else
			{
				/* View mode : select defaut image if none */
				char *img = dyntab_val(&ctrl->val, i, 0);
				if(!*img) img = imgdef;
				if(!*img) continue;

				/* Separate multiple values with a space */
				if(i) DYNBUF_ADD_STR(form->html, " ");

				/* Add image */
				if(put_html_image_dir(cntxt, NULL, img == imgdef ? NULL : dir, img, ctrl->NOTES, w, h, 0, 0, NULL, NULL)) STACK_ERROR;
			}

		/* Output control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
