/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_chkbox.c
** Description : handling functions for CHECKBOX controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : ctrl_add_chkbox
** Description : handles INPUT controls of type CHECKBOX
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_chkbox"
#define ERR_CLEANUP DYNTAB_FREE(ctrltree); \
					M_FREE(name); \
					M_FREE(notes)
int ctrl_add_chkbox(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable ctrltree = { 0 };
	DynBuffer *name = NULL;
	DynBuffer *notes = NULL;
	char *img, *imgsel;
	unsigned long i;
	int b_3state = CTRL_ATTR_CELL(CHECKBOX_3STATE) != NULL;
	int tree = 1;
	switch(*dyntab_val(&ctrl->val, 0, 0))
	{
	case 0: tree = b_3state ? 0 : 2; break;
	case '0': tree = 2;
	}

	switch(form->step)
	{
	case CtrlRead:
		/* If checkbox clicked */
		if(form->i_ctrl_clic == i_ctrl && cntxt->cgi && cntxt->cgi[cntxt->cgibtn].name[0] == 'B')
		{
			/* Search / create corresponding value */
			CGIData *cgi = cntxt->cgi + cntxt->cgibtn;
			DynTableCell *val;
			for(i = 0; i < ctrl->val.nbrows; i++)
			{
				val = dyntab_cell(&ctrl->val, i, 0);
				if(val->Line == cgi->Line && val->Num == cgi->Num) break;
			}
			if(i == ctrl->val.nbrows)
			{
				DYNTAB_SET(&ctrl->val, i, 0, "");
				val = dyntab_cell(&ctrl->val, i, 0);
				val->Line = cgi->Line;
				val->Num = cgi->Num;
			}

			/* Handle value on checkbox click */
			switch(*dyntab_val(&ctrl->val, i, 0))
			{
			case 0:	/* Null Value : Switch to checked */
				DYNTAB_SET(&ctrl->val, i, 0, "1");
				break;
			case '0':	/* Unchecked value : Switch to null */
				DYNTAB_SET(&ctrl->val, i, 0, "");
				break;
			default:	/* Checked value : Switch to unchecked */
				if(b_3state) DYNTAB_SET(&ctrl->val, i, 0, "0")
				else DYNTAB_SET(&ctrl->val, i, 0, "");
			}
			val = dyntab_cell(&ctrl->val, i, 0);
			val->b_modified = 1;
			ctrl->b_modified = 1;
		}

		/* Read both branches */
		tree = 3;
		break;

	case InputCheck:
		/* Disable empty value check if not 3 state */
		if(!b_3state) ctrl->error = 0;
		break;

	case HtmlEdit:
	case HtmlPrint:
	case HtmlView:
		/* Break if no output */
		if(!form->html) break;

		/* Defaut is no option button */
		if(!ctrl->OPTIONBUTTON[0]) ctrl->OPTIONBUTTON = "_EVA_NONE";

		/* Handle checkbox tooltip */
		if(ctrl->NOTES && *ctrl->NOTES) DYNBUF_ADD3(&notes, "", ctrl->NOTES, 0, NO_CONV, "\n\n");

		/* Handle 3 states images */
		switch(tree)
		{
		case 0:	/* Null value */
			img = "_eva_checkbox_nul.gif";
			imgsel = "_eva_checkbox_nul_s.gif";
			DYNBUF_ADD_STR(&notes, "Etat : non renseigné");
			break;
		case 2:	/* Unchecked value */
			img = "_eva_checkbox_no.gif";
			imgsel = "_eva_checkbox_no_s.gif";
			DYNBUF_ADD_STR(&notes, "Etat : non coché");
			break;
		default:	/* Checked value */
			img = "_eva_checkbox_yes.gif";
			imgsel = "_eva_checkbox_yes_s.gif";
			DYNBUF_ADD_STR(&notes, "Etat : coché");
		}

		/* Add HTML code for control label & position */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;


		/* If edit mode */
		if(form->step == HtmlEdit)
		{
			char *onchange = CTRL_ATTR_VAL(ONCHANGE);

			/* Add hidden input to hold checkbox value */
			if(ctrl_put_hidden(cntxt, ctrl, 0)) STACK_ERROR;

			/* Add checkbox as button */
			CTRL_CGINAMEVAL(&name, 0);
			name->data[0] = 'B';
			if(put_html_button(cntxt, name->data, NULL, img, imgsel, notes ? notes->data : NULL, 0,
				(CTRL_ATTR_CELL(CTRLTREE) || CTRL_ATTR_CELL(CTRLTREE_ALT) || *onchange && strcmp(onchange, "_EVA_NONE")) ? 0 : (128 | (b_3state ? 256 : 0))))
				STACK_ERROR;
		}
		else
		{
			/* Else : Add checkbox as image */
			if(put_html_image(cntxt, ctrl->cginame->data, img, notes ? notes->data : NULL, NULL, NULL, 0))
				STACK_ERROR;
		}

		/* Add HTML code for the control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	}

	/* Add lower level controls depending on check status */
	if(tree == 1 || tree == 3)
	{
		/* Controls when selected */
		CTRL_ATTR(ctrltree, CTRLTREE);
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
	}
	if(tree == 2 || tree == 3)
	{
		/* Controls when not selected */
		CTRL_ATTR(ctrltree, CTRLTREE_ALT);
		CTRL_ADD_CHILD(i_ctrl, &ctrltree);
	}

	/* Add lower level controls */
	
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
