/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_attr.c
** Description : controls attributes handling utilities
**      Author : Alain BOUTE
**     Created : Sept 28 2005
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Function : ctrl_set_format_attr
** Description : read control format attributes in ctrl->attr or form / default form
*********************************************************************/
#define ERR_FUNCTION "ctrl_set_format_attr"
#define ERR_CLEANUP 
int ctrl_set_format_attr(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	EVA_ctrl *ctrl					/* in : control to process */
){
	/* Get server format defaults if applicable */
	if(!cntxt->srvfmt.attr.nbrows && cntxt->cnf_data.nbrows)
	{
		memcpy(&cntxt->srvfmt.attr, &cntxt->cnf_data, sizeof(cntxt->srvfmt.attr));
		if(ctrl_set_format_attr(cntxt, &cntxt->srvfmt)) STACK_ERROR;
	}

	/* Get control format */
	CTRL_ATTR_GET(LABELPOS);
	CTRL_ATTR_GET(POSITION);
	ctrl->COLSPAN = strtoul(CTRL_ATTR_VAL(COLSPAN), NULL, 10);
	ctrl->ROWSPAN = strtoul(CTRL_ATTR_VAL(ROWSPAN), NULL, 10);
	ctrl->LINES = strtoul(CTRL_ATTR_VAL(LINES), NULL, 10);
	ctrl->COLUMNS = strtoul(CTRL_ATTR_VAL(COLUMNS), NULL, 10);
	
	/* Get cell modifiers */
	CTRL_ATTR_GET(ALIGN);
	CTRL_ATTR_GET(VALIGN);
	CTRL_ATTR_GET(WIDTH);
	CTRL_ATTR_GET(HEIGHT);
	CTRL_ATTR_GET(NOBR);
	CTRL_ATTR_GETFORM(BGCOLOR);
	CTRL_ATTR_GETFORM(BACKGROUND);

	/* Get font modifiers */
	CTRL_ATTR_GETFORM(FONTFACE);
	CTRL_ATTR_GETFORM(FONTSIZE);
	CTRL_ATTR_GETFORM(FONTCOLOR);
	CTRL_ATTR_GETFORM(BOLD);
	CTRL_ATTR_GETFORM(ITALIC);
	CTRL_ATTR_GETFORM(UNDERLINE);

	/* Get style attributes */
	CTRL_ATTR_GET(CTRL_STYLE);
	CTRL_ATTR_GETFORM(CELL_STYLE);
	CTRL_ATTR_GETFORM(LABEL_STYLE);
	CTRL_ATTR_GET(TABLE_STYLE);

	/* Get label attributes */
	CTRL_ATTR_GET(LABEL);
	CTRL_ATTR_GET(LABELALIGN);
	CTRL_ATTR_GET(LABELVALIGN);
	CTRL_ATTR_GET(LABELWIDTH);
	CTRL_ATTR_GET(LABELHEIGHT);
	ctrl->LABELCOLSPAN = strtoul(CTRL_ATTR_VAL(LABELCOLSPAN), NULL, 10);
	ctrl->LABELROWSPAN = strtoul(CTRL_ATTR_VAL(LABELROWSPAN), NULL, 10);
	CTRL_ATTR_GETFORM(LABELBGCOLOR);
	CTRL_ATTR_GETFORM(LABELBACKGROUND);

	/* Get label font modifiers */
	CTRL_ATTR_GETFORM(LABELFONTFACE);
	CTRL_ATTR_GETFORM(LABELFONTSIZE);
	CTRL_ATTR_GETFORM(LABELFONTCOLOR);
	CTRL_ATTR_GETFORM(LABELBOLD);
	CTRL_ATTR_GETFORM(LABELITALIC);
	CTRL_ATTR_GETFORM(LABELUNDERLINE);

	/* Get table attributes */
	CTRL_ATTR_GET(TABLEWIDTH);
	CTRL_ATTR_GET(TABLEHEIGHT);
	CTRL_ATTR_GET(TABLERULES);
	CTRL_ATTR_GET(TABLEBGCOLOR);
	CTRL_ATTR_GET(TABLEBACKGROUND);
	ctrl->BORDER = strtoul(CTRL_ATTR_VAL(BORDER), NULL, 10);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_set_common_attr
** Description : read control attributes in ctrl->attr or form / default form
*********************************************************************/
#define ERR_FUNCTION "ctrl_set_common_attr"
#define ERR_CLEANUP 
int ctrl_set_common_attr(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	EVA_ctrl *ctrl					/* in : control to process */
){

	/* Get control attributes */
	CTRL_ATTR_GET(CONTROL);
	CTRL_ATTR_GET(TYPE);
	CTRL_ATTR_GET(FIELD);
	CTRL_ATTR_GET(NOTES);
	CTRL_ATTR_GET(OPTIONBUTTON);
	CTRL_ATTR_GET(ALLOWEMPTY);
	CTRL_ATTR_GET(MULTIPLE);
	CTRL_ATTR_GET(LABEL_NOSEL);
	if(ctrl_set_format_attr(cntxt, ctrl)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP


/*********************************************************************
** Function : ctrl_read_baseobj
** Description : complete derived control with base objects data
*********************************************************************/
#define ERR_FUNCTION "ctrl_read_baseobj"
#define ERR_CLEANUP DYNTAB_FREE(baseobj); \
					DYNTAB_FREE(data)
int ctrl_read_baseobj(				/* return : control index in cntxt->form->ctrl or 0 if no match */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *ctrldata				/* in/out : control data to complete */
){
	DynTable baseobj = {0};
	DynTable data = {0};
	unsigned long i;

	DYNTAB_FIELD(&baseobj, ctrldata, BASE_STYLE);
	for(i = 0; i < baseobj.nbrows; i++)
	{
		if(qry_cache_idobj(&data, DYNTAB_TOULRC(&baseobj, i, 0)) ||
			qry_complete_data(cntxt, ctrldata, &data, NULL, NULL)) STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
