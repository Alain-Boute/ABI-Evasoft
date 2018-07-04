/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_input_fileupload.c
** Description : handling functions for file upload input controls
**      Author : Alain BOUTE
**     Created : Aug 18 2001
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_add_fileupload
** Description : handles INPUT controls of type FILEUPLOAD
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_fileupload"
#define ERR_CLEANUP DYNTAB_FREE(cgival); \
					M_FREE(name)
int ctrl_add_fileupload(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	DynTable cgival = {0};
	DynBuffer *name = NULL;
	unsigned long i;
	char *tablesearch = CTRL_ATTR_VAL(TABLESEARCH);
	int b_search = strcmp(tablesearch, "_EVA_NONE");
	int b_multiple = ctrl->MULTIPLE[0] != 0 && strcmp("No", ctrl->MULTIPLE);
	int b_loadfile = cntxt->cgiencodemultipart && (!b_search || form->i_ctrl_clic == i_ctrl);

	switch(form->step)
	{
	case CtrlRead:
		if(ctrl->storage) ctrl->storage = 3;

		/* If not transmit mode : handle files list */
		if(!b_loadfile)
		{
			/* Read table format data if needed */
			if(!CTRL_ATTR_CELL(DISPLAYFIELDS))
			{
				if(!cntxt->objdata_FORMLIST_FILTER.nbrows)
				{
					DynTableCell *list = DYNTAB_FIELD_CELL(&cntxt->cnf_data, FORMLIST_FILTER);
					unsigned long idlist = (list && list->txt) ? atoi(list->txt) : 0;
					if(qry_cache_idobj(&cntxt->objdata_FORMLIST_FILTER, idlist)) STACK_ERROR;
				}
				if(qry_complete_data(cntxt, &ctrl->attr, &cntxt->objdata_FORMLIST_FILTER, NULL, NULL)) STACK_ERROR;
			}

			/* Handle table buttons clics */
			if(table_read_controls(cntxt, i_ctrl, NULL) ||
				table_process_controls(cntxt, i_ctrl)) STACK_ERROR;
		}
		for(i = 0; i < ctrl->val.nbrows; i++) dyntab_cell(&ctrl->val, i, 0)->b_relation = 1;
		break;

	case HtmlEdit:
		/* Read control status */
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;

		/* Output control values */
		if(!b_loadfile)
		{
			/* Output relation table if applicable */
			if(ctrl_relation_put_table(cntxt, i_ctrl, table_put_html_obj_list)) STACK_ERROR;
		}
		else
			/* Output as hidden inputs if no table */
			for(i = 0; i < ctrl->val.nbrows; i++)
				if(ctrl_put_hidden(cntxt, ctrl, i)) STACK_ERROR;

		/* If new value allowed */
		if(form->step == HtmlEdit && (b_loadfile || b_multiple || !ctrl->val.nbrows))
		{
			int columns = ctrl->COLUMNS;
			if(!columns) columns = 5;

			/* Handle transmit mode : Add HTML input of type file + upload button + description input */
			if(b_loadfile)
			{
				/* Add HTML input of type file */
				CTRL_CGINAMEVAL(&name, ctrl->val.nbrows);
				DYNBUF_ADD3_BUF(form->html, "<input type=file name='", name, NO_CONV, "'");
				DYNBUF_ADD3_INT(form->html, " size=", columns, ">\n");
				if(ctrl_put_hidden_old(cntxt, ctrl, ctrl->val.nbrows, name, NULL, 0)) STACK_ERROR;

				/* Add upload button + name & description inputs */
				CTRL_CGINAMEBTN(&name, NULL, add_sz_str("Nop"));
				if(put_html_button(cntxt, name->data, "Lancer", 
									NULL, NULL,
									"Lance la transmission du fichier\n\n"
									"Cliquez d'abord sur le bouton 'Parcourir ...' puis choisissez le fichier à transmettre (un seul fichier à la fois)\n\n"
									"La transmission peut prendre un certain temps si le fichier est important\n\n"
									"Note : le fichier est aussi transmis quand vous cliquez un autre bouton ou si vous changez une valeur dynamique\n\n"
									"Si vous avez utilisé Parcourir et que vous ne voulez pas transmettre le fichier, effacez le champ à gauche du bouton Parcourir"
									,0, 0))
					STACK_ERROR;

				/* Output file description input if applicable */
				if(*CTRL_ATTR_VAL(INPUT_DESCRIPTION))
				{
					CTRL_CGINAMESUBFIELD(&name, NULL, "FILE_DESCRIPTION");
					if(put_html_text_input(cntxt, name->data, name->cnt, 
										NULL, 0, 0, 0, 3, 30, 128))
						STACK_ERROR;
				}
			}
			else
			{
				/* Add open upload dialog button */
				CTRL_CGINAMEBTN(&name, NULL, add_sz_str("TRANSMIT_FILE"));
				if(put_html_button(cntxt, name->data, "Transmettre ...", 
									NULL, NULL, "Transmettre un fichier au serveur", 0, 0))
					STACK_ERROR;
			}
		}
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
		break;

	case HtmlPrint:
	case HtmlView:
		if(ctrl_format_pos(cntxt, ctrl, 1)) STACK_ERROR;
		if(ctrl_relation_put_table(cntxt, i_ctrl, table_put_html_obj_list)) STACK_ERROR;
		if(ctrl_format_pos(cntxt, ctrl, 0)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

