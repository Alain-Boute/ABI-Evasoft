/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_email.c
** Description : handling functions for INPUT controls of type EMAIL
**      Author : Alain BOUTE
**     Created : Sept 22 2003
*********************************************************************/

#include "eva.h"


/*********************************************************************
** Function : ctrl_add_input_email
** Description : handles INPUT controls of type EMAIL
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_input_email"
#define ERR_CLEANUP
int ctrl_add_input_email(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i;
	size_t tot_char = 0;
	int b_stripped;
	
	switch(form->step)
	{
	case InputCheck:
		/* Check each value */
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			DynTableCell *c = dyntab_cell(&ctrl->val, i, 0);
			char *val = c->txt, *in = val, *out = val, *err, *old;
			char *at = strchr(val, '@');
			char *at2 = at ? strchr(at + 1, '@') : NULL;
			char *dot = at ? strchr(at, '.') : NULL;
			if(!val || !c->len) continue;
			M_STRDUP(old, c->txt, c->len);

			/* Remove unwanted chars for email */
			setcase(4, val, 0);
			while(*in)
			{
				if(isalnum(*in) || strchr("._-@", *in)) 
				{
					*out = *in; out++;
				}
				in++;
			}
			*out = 0;
			c->len = out - val;

			/* Handle value modification */
			if(strcmp(old, c->txt))
			{
				c->b_modified = 1;
				ctrl->b_modified = 1;
			}
			M_FREE(old);

			/* Check email validity */
			err = !at ?  " @ absent" :
				(at == val) ?  "pas de nom avant @" :
				at2 ?  "plusieurs @" :
				!dot ?  ". absent après @" :
				(dot - at < 2) ?  "pas de nom entre @ et ." :
				!dot[1] ?  "pas de nom après ." :
				NULL;
			if(err)
			{
				ctrl->error |= 2;
				if(ctrl->errmsg) DYNBUF_ADD_STR(&ctrl->errmsg, "\n");
				DYNBUF_ADD3_CELL(&ctrl->errmsg, "L'adresse <", &ctrl->val, i, 0, NO_CONV, "> n'est pas valide : ");
				DYNBUF_ADD(&ctrl->errmsg, err, 0, NO_CONV);
			}
		}
		break;

	case HtmlPrint:
	case HtmlView:
		/* Add HTML code for control label & position */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Handle empty value */
		if(!ctrl->val.nbrows && *ctrl->LABEL_NOSEL) DYNBUF_ADD(form->html, ctrl->LABEL_NOSEL, 0, TO_HTML);

		/* Add HTML code for each value */
		for(i = 0; i < ctrl->val.nbrows; i++)
		{
			/* Separate multiple values with a line break */
			if(i) DYNBUF_ADD_STR(form->html, "<br>");

			/* Add text control */
			if(html_put_value_fmt(cntxt, ctrl, &ctrl->val, i, "_EVA_EMAIL", 0, &tot_char, &b_stripped)) CLEAR_ERROR;
		}

		/* Add HTML code for the control footer */
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	default:
		/* Handle as text control */
		if(ctrl_add_text(cntxt, i_ctrl)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
