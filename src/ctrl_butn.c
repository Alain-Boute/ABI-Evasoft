/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_butn.c
** Description : handling fonctions for BUTTON controls
**      Author : Alain BOUTE
**     Created : March 27 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : put_html_button_sz
** Description : output HTML code for a button control
*********************************************************************/
#define ERR_FUNCTION "put_html_button_sz"
#define ERR_CLEANUP M_FREE(img); \
					M_FREE(img1); \
					M_FREE(tmp)
int put_html_button_sz(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name,	 						/* in : input name or href location */
	char *label,					 	/* in : button label */
	char *im,	 						/* in : image file */
	char *im1,							/* in : image displayed when mouse over  */
	char *tooltip,					 	/* in : first line for tooltip */
	char *notes,	 					/* in : tooltip text */
	char *errmsg,	 					/* in : tooltip error message */
	char *style, 						/* in : CSS style class */
	unsigned long width,				/* in : image width */
	unsigned long height,				/* in : image height */
	int border,						 	/* in : border size */
	unsigned long id_obj,				/* in : object Id to include in tooltip */
	int mode						 	/* in : display mode - bit mask
											bit 0 : button disabled if set (1)
											bit 1 : dont use JavaScript if set (2)
											bit 2 : aligned on text if set (4)
											bit 3 : output label if set (8)
											bit 4 : label position is left if set (16)
											bit 5 : output anchor style button (omit image) if set (32)
											bit 6 : output anchor (use href) if set (64)
											bit 7 : use checkbox handler if set (128)
											bit 8 : label is HTML if set (256)
											bit 9 : use INPUT submit with background for images if set (512)
											bit 10: use DIV onclic with background for images if set (1024)
											bit 11: do not output label in tooltip if set (2048)
											*/
){
	char *img = NULL;
	char *img1 = NULL;
	DynBuffer *tmp = NULL;
	CHECK_HTML_STATUS;
	if(im1 && *im1 && (!im || !*im)) { im = im1; im1 = NULL; }
	img = (mode & 32) ? NULL : get_image_file(cntxt, im, &width, &height);
	img1 = img ? get_image_file(cntxt, im1, 0, 0) : NULL;

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(html, "\n<!--- Button Start -->\n");

	/* Handle button output mode */
	if(mode & 128 && !(mode & 3) && (im && *im && im1 && *im1 || mode & 32))
	{
		/* Output javascript for button */
		EVA_form *form = cntxt->form;
		DynBuffer *htm = *html;

		/* Optimize if previous command is javascript */
		if(htm && htm->cnt > 9 && !strcmp(htm->data + htm->cnt - 9, "</script>"))
			htm->cnt -= 9;
		else
			DYNBUF_ADD_STR(html, "<script>");
		
		/* Use javascript ob function */
		DYNBUF_ADD3(html, "ob(\"", name, 0, NO_CONV, "\"");
		DYNBUF_ADD3(html, ",\"", img, 0, NO_CONV, "\"");
		DYNBUF_ADD3(html, ",\"", img1, 0, NO_CONV, "\"");
		DYNBUF_ADD3(html, ",\"", label, 0, TO_JS_STRING, "\"");
		DYNBUF_ADD3(html, ",\"", notes, 0, TO_JS_STRING, "\"");
		DYNBUF_ADD3_INT(html, ",", border, "");
		DYNBUF_ADD3_INT(html, ",", mode, "");
		DYNBUF_ADD3_INT(html, ",", width, "");
		DYNBUF_ADD3_INT(html, ",", height, ",'");
		if(form->nb_ctrl && cntxt->opt_btn_mode > OptBtn_Help)
		{
			DYNBUF_ADD3_CELL(html, "I", &(form->ctrl[form->i_ctrl].id), 0, 0, NO_CONV, "/");
			DYNBUF_ADD_CELL(html, &form->id_form, 0, 0, NO_CONV);
			DYNBUF_ADD3_CELL(html, "$", &form->id_obj, 0, 0, NO_CONV, "#");
		}
		DYNBUF_ADD_STR(html, ".');</script>");
	}
	else /* Output HTML code for standard button */
	{
		int b_tooltip = tooltip || id_obj != ~0UL || notes || errmsg;
		
		/* Handle button mode */
		char *tag = ((mode & 1024) && label) ? "div" :
					((mode & (32 | 256)) && label) ? "a" :
					((mode & 1) && img) ? "img" :
					(!(mode & 512) && img) ? "input type=image" : "input type=submit";
		int b_image = !strcmp(tag, "img") || !strcmp(tag, "input type=image");

		/* Output tag start */
		DYNBUF_ADD3(html, "<", tag, 0, NO_CONV, "");

		/* Output image info depending on tag */
		if(b_image)
		{
			/* Output image attributes for image tags */
			DYNBUF_ADD3(html, " alt='", label, 0, HTML_NO_QUOTE, "'")
			DYNBUF_ADD3(html, " src='", img, 0, NO_CONV, "'");
			if(width) DYNBUF_ADD3_INT(html, " width=", width, "");
			if(height) DYNBUF_ADD3_INT(html, " height=", height, "");
			if(mode & 4) DYNBUF_ADD_STR(html, " align=absmiddle");
			DYNBUF_ADD3_INT(html, " border=", border, "");
		}
		else if(img)
		{
			/* Output background info for non image tags */
			DYNBUF_ADD3(html, " style='background-image:url(\"", img, 0, NO_CONV, "\");'");
		}

		/* Output alternate image info if needed */
		if(img && img1)
		{
			DYNBUF_ADD3(html, " imgstd='", img, 0, HTML_NO_QUOTE, "'");
			DYNBUF_ADD3(html, " imghov='", img1, 0, HTML_NO_QUOTE, "'");
			style = dynbuf_concat_ws(&tmp, b_image ? "EVA_SwapImg" : "EVA_SwapBgImg", " ", style);
		}

		/* Output action info depending on tag */
		if(*tag == 'd')
		{
			/* Output DIV attributes */
			DYNBUF_ADD3(html, " onclick=cb('", name, 0, NO_CONV, "')");
		}
		else if(*tag == 'a')
		{
			/* Output A attributes */
			if(mode & 64)
				DYNBUF_ADD3(html, " href='", name, 0, NO_CONV, "'")
			else
				DYNBUF_ADD3(html, " href=\"javascript:cb('", name, 0, NO_CONV, "')\"");			
		}
		else
		{
			/* Output INPUT attributes */
			DYNBUF_ADD3(html, " name='", name, 0, NO_CONV, "'");
			if(label && *label) DYNBUF_ADD3(html, " value='", label, 0, HTML_NO_QUOTE, "'");
		}

		/* Output common attributes */
		if(style && *style) DYNBUF_ADD3(html, " class='", style, 0, HTML_NO_QUOTE, "'");
		if(b_tooltip) DYNBUF_ADD_STR(html, " onMouseOver=EVA_ShowTooltip(this) onMouseOut=EVA_HideToolTip(this)");
		if(mode & 1 && !strncmp(tag, "in", 2)) DYNBUF_ADD_STR(html, " disabled");
		DYNBUF_ADD_STR(html, ">");

		/* Output label between tags when closing tag */
		if(*tag != 'i')
		{
			if(mode & 256)	DYNBUF_ADD(html, label, 0, NO_CONV)
			else			DYNBUF_ADD(html, label, 0, TO_HTML);
			DYNBUF_ADD3(html, "</", tag, 0, NO_CONV, ">");
		}

		/* Output tooltip if applicable */
		if(b_tooltip)
		{
			EVA_form *form = cntxt->form;

			/* Output SPAN for tooltip */
			DYNBUF_ADD_STR(html, "<span class=EVA_ToolTip>");

			/* Output IMG.EVA_EditBtnInfo for edit button if applicable */
			if(form && form->nb_ctrl && cntxt->opt_btn_mode > OptBtn_Help)
			{
				EVA_ctrl *ctrl = form->ctrl + form->i_ctrl;
				DYNBUF_ADD3_CELL(html, "<img class=EVA_EditBtnInfo onclick=\"javascript:cb('I", &ctrl->id, 0, 0, NO_CONV, "/");
				DYNBUF_ADD_CELL(html, &form->id_form, 0, 0, NO_CONV);
				DYNBUF_ADD3_CELL(html, "$", &form->id_obj, 0, 0, NO_CONV, "#')\" title='Modifier le contrôle' src='../img/btn_edit_small.gif'>");
			}

			/* Output DIV.EVA_TipIcon for tooltip title & icon */
			DYNBUF_ADD_STR(html, "<div class=EVA_TipIcon><img src='../img/info.gif'>");
			DYNBUF_ADD3(html, "<span>", tooltip ? tooltip : (label && !(mode & 256)) ? label : "Informations", 0, TO_HTML, "</span>");

			/* Output object Id if given */
			if(id_obj != ~0UL)
			{
				DYNBUF_ADD_STR(html, "<span>");
				if(!id_obj) DYNBUF_ADD_STR(html, "Nouvelle fiche")
				else DYNBUF_ADD3_INT(html, "Fiche n° ", id_obj, "");
				DYNBUF_ADD_STR(html, "</span>");
			}
			DYNBUF_ADD_STR(html, "</div>");

			/* Add P.EVA_ErrMsg for error message */
			if(errmsg) DYNBUF_ADD3(html, "<p class=EVA_ErrMsg>", errmsg, 0, TO_HTML, "</p>");

			/* Add P.EVA_LabelInfo for label if applicable */
			if(tooltip && label && !(mode & (32 | 256 | 512 | 2048)))
				DYNBUF_ADD3(html, "<p class=EVA_LabelInfo>", label, 0, TO_HTML, "</p>");

			/* Add P.EVA_NotesInfo for notes */
			if(notes) DYNBUF_ADD3(html, "<p class=EVA_NotesInfo>", notes, 0, TO_HTML, "</p>");
			DYNBUF_ADD_STR(html, "</span>");
		}
		if(img && mode & 8 && !(mode & 16)) DYNBUF_ADD(html, label, 0, TO_HTML);
	}

	if(cntxt->debug & DEBUG_HTML) DYNBUF_ADD_STR(cntxt->form->html, "\n<!--- Button End -->\n");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_put_button
** Description : output BUTTON controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_put_button"
#define ERR_CLEANUP	DYNTAB_FREE(img); \
					DYNTAB_FREE(imgsel); \
					DYNTAB_FREE(cond); \
					DYNTAB_FREE(data); \
					DYNTAB_FREE(condlbl); \
					M_FREE(tooltip); \
					M_FREE(jslink); \
					M_FREE(tmp)
int ctrl_put_button(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable img = {0};
	DynTable imgsel = {0};
	DynTable cond = {0};
	DynTable condlbl = {0};
	DynTable data = {0};
	DynBuffer *tooltip = NULL;
	DynBuffer *jslink = NULL;
	int b_formbtn = 0;
	int b_formbtnsel = 0;
	char *action = CTRL_ATTR_VAL(ACTION);
	char *showmode = CTRL_ATTR_VAL(SHOWMODE);
	char *update = CTRL_ATTR_VAL(UPDATE);
	char *access_edit = CTRL_ATTR_VAL(ACCES_EDIT);
	int b_updatecurobj = (!*update || !strcmp(update, "_EVA_CURRENTOBJ")) && !CTRL_ATTR_CELL(UPDATE_OBJFIELD);
	int b_link = !strcmp(action, "_EVA_HTTPLINK");
	char *link = CTRL_ATTR_VAL(URL);
	char *btntyp = CTRL_ATTR_VAL(BTN_TYPE);
	unsigned long i;
	DynBuffer *tmp = NULL;
	int b_submitbtn = !strcmp(btntyp, "_EVA_BTN_DIV") ? 2 : !strcmp(btntyp, "_EVA_BTN") ? 1 : 0;

	/* Handle specific buttons for selected mode */
#define CHECK_FOR(mode, tag) if(!strcmp(action, "_EVA_"#tag)) { \
								b_formbtn = 1; \
								b_formbtnsel = form->step == mode; }
	CHECK_FOR(FormList, LISTMODE)
	else CHECK_FOR(FormSearch, SEARCHMODE)
	else CHECK_FOR(FormStats, STATSMODE)
	else CHECK_FOR(FormValues, VALUESMODE)
	else CHECK_FOR(HtmlEdit, EDITMODE)
	else CHECK_FOR(HtmlView, VIEWMODE)
	else CHECK_FOR(HtmlPrint, PRINTMODE)
#undef CHECK_FOR

	/* Do not output buttons depending on mode & status */
#define ACTION_IS(tag) (!strcmp(action, "_EVA_"#tag))
	if( /* Only print button in print mode */
		form->step == HtmlPrint && !ACTION_IS(PRINTMODE) ||
		/* No edit / save / saveclose button if no edit access for form */
		!(form->ctrl->access & (form->b_newobj ? AccessCreate : AccessEdit)) &&
			(ACTION_IS(EDITMODE) || ACTION_IS(SAVE) || ACTION_IS(SAVECLOSE)) ||
		/* No save or ungranted update buttons if not editing */
		form->step != HtmlEdit && (ACTION_IS(SAVE) || ACTION_IS(SAVECLOSE) ||
			(ACTION_IS(UPDATE) && b_updatecurobj && !*access_edit || 
			ACTION_IS(OPENFORM) && CTRL_ATTR_CELL(NEWOBJ_FIELD)) &&
				!(form->ctrl->access & (form->b_newobj ? AccessCreate : AccessEdit)) &&
				strcmp(access_edit, "_EVA_ALWAYS")) ||
		/* No new & copy buttons if new object */
		!form->id_obj.nbrows && (ACTION_IS(NEWOBJ) || ACTION_IS(COPYOBJ)) ||
		/* Hide selected mode button if HIDESELECTED */
		b_formbtnsel && !strcmp(showmode, "_EVA_HIDESELECTED") ||
		/* No buttons other than form buttons if mode other than view or edit */
		!b_formbtn && !ACTION_IS(CLOSE) && form->step != HtmlEdit && form->step != HtmlView
		)
		RETURN_OK;
#undef ACTION_IS
	
	/* Set button default attributes values */
	if(!ctrl->POSITION[0]) ctrl->POSITION = "_EVA_NewColumn";
	if(!ctrl->LABELPOS[0] || b_submitbtn) ctrl->LABELPOS = "_EVA_NONE";
	if(!ctrl->OPTIONBUTTON[0]) ctrl->OPTIONBUTTON = "_EVA_NONE";
	if(!ctrl->COLSPAN && b_submitbtn) ctrl->COLSPAN = 2;
 
	/* Switch images if mode selected and display mode is SHOWSELECT */
	if(imgsel.nbrows && img.nbrows && b_formbtnsel && !strcmp(showmode, "_EVA_SHOWSELECTED"))
	{
		CTRL_ATTR_TAB(img, IMAGESEL);
		CTRL_ATTR_TAB(imgsel, IMAGE);
	}
	else
	{
		CTRL_ATTR_TAB(img, IMAGE);
		CTRL_ATTR_TAB(imgsel, IMAGESEL);
	}

	/* Get conditional images */
	CTRL_ATTR_TAB(cond, IMAGE_FILTER);
	CTRL_ATTR_TAB(condlbl, IMAGE_LABEL);
	for(i = 0; i < img.nbrows; i++)
	{
		char *c = dyntab_val(&cond, i, 0);
		int b_match = !*c;
		if(!b_match)
		{
			unsigned long k;
			if(ctrl_eval_fieldexpr(cntxt, ctrl, &data, c)) CLEAR_ERROR;
			for(k = 0; k < data.nbrows && !b_match; k++)
				b_match = dyntab_sz(&data, k, 0) && strcmp("0", dyntab_val(&data, k, 0));
		}
		if(b_match) break;
	}

	/* Handle tooltip */
	DYNBUF_ADD3(&tooltip, "Bouton [", ctrl->LABEL, 0, NO_CONV, "]");

	/* Output button */
	if(ctrl_format_pos(cntxt, ctrl, 1) ||
		put_html_button_sz(cntxt, 
							b_link ? link : ctrl->cginame->data,
							ctrl->LABEL,
							dyntab_val(&img, i, 0),
							dyntab_val(&imgsel, i, 0),
							tooltip->data, ctrl->NOTES,
							dyntab_sz(&condlbl, i, 0) ? dyntab_cell(&condlbl, i, 0)->txt : NULL,
							"EVA_Btn", 0, 0, 0, ~0UL,
							/* Button mode : handle link & submit types */
							(b_link ? 64 : 0) | (b_submitbtn * 512) | 2048) ||
		ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_button
** Description : handles BUTTON controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_button"
#define ERR_CLEANUP
int ctrl_add_button(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;

	switch(form->step)
	{
	case CtrlRead:
		/* Set CGI name type */
		ctrl->cginame->data[0] = 'B';
		RETURN_OK;

	case HtmlEdit:
	case HtmlView:	 
	case HtmlPrint:	 
	case FormList: 
	case FormSearch:
	case FormStats: 
	case FormValues:
		if(ctrl_put_button(cntxt, i_ctrl)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

