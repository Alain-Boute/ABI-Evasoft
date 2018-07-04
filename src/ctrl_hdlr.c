/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_hdlr.c
** Description : controls handlers definition table 
**      Author : Alain BOUTE
**     Created : Mar 9 2002
*********************************************************************/
																																					   
#include "eva.h"

/*********************************************************************
** Constant : ctrl_handlers
** Description : control handlers definition table for CONTROL & TYPE attributes
*********************************************************************/
EVA_handlers ctrl_handlers[] = 
	{
#ifdef _EVA_DLL
	NULL
#else 
	/*	CONTROL					TYPE					Primary handler				Secondary handler */
		{ "_EVA_OPTION",		NULL,					ctrl_add_option,			NULL },
		{ "_EVA_BUTTON",		NULL,					ctrl_add_button,			NULL },
		{ "_EVA_INPUT",			"_EVA_RELATION",		ctrl_add_input,				ctrl_add_relation },
		{ "_EVA_INPUT",			"_EVA_LIST",			ctrl_add_input,				ctrl_add_input_list },
		{ "_EVA_INPUT",			"_EVA_CHECKBOX",		ctrl_add_input,				ctrl_add_chkbox },
		{ "_EVA_INPUT",			"_EVA_DATE",			ctrl_add_input,				ctrl_add_date },
		{ "_EVA_INPUT",			"_EVA_HOUR",			ctrl_add_input,				ctrl_add_hour },
		{ "_EVA_INPUT",			"_EVA_NUMBER",			ctrl_add_input,				ctrl_add_number },
		{ "_EVA_INPUT",			"_EVA_FILEUPLOAD",		ctrl_add_input,				ctrl_add_fileupload },
		{ "_EVA_INPUT",			"_EVA_HTTPLINK",		ctrl_add_input,				ctrl_add_httplink },
		{ "_EVA_INPUT",			"_EVA_EMAIL",			ctrl_add_input,				ctrl_add_input_email },
		{ "_EVA_INPUT",			"_EVA_TELEPHONE",		ctrl_add_input,				ctrl_add_input_tel },
		{ "_EVA_INPUT",			"_EVA_COLOR",			ctrl_add_input,				ctrl_add_input_color },
		{ "_EVA_INPUT",			"_EVA_FIELD",			ctrl_add_input,				ctrl_add_input_field },
		{ "_EVA_INPUT",			"_EVA_IMAGE",			ctrl_add_input,				ctrl_add_input_image },
		{ "_EVA_INPUT",			NULL,					ctrl_add_input,				ctrl_add_text },
		{ "_EVA_CONTAINER",		"_EVA_TAB",				ctrl_add_tab,				NULL },
		{ "_EVA_CONTAINER",		"_EVA_GROUP",			ctrl_add_group,				NULL },
		{ "_EVA_CONTAINER",		"_EVA_TITLE",			ctrl_add_title,				NULL },
		{ "_EVA_CONTAINER",		"_EVA_MENUBAR",			ctrl_add_menubar,			NULL },
		{ "_EVA_CONTAINER",		"_EVA_TABLE",			ctrl_add_table	,			NULL },
		{ "_EVA_CONTAINER",		"_EVA_PACKAGE",			ctrl_add_pack	,			NULL },
		{ "_EVA_CONTAINER",		NULL,					ctrl_add_pack	,			NULL },
		{ "_EVA_OUTPUT",		"_EVA_RELTREE",			ctrl_add_reltree,			NULL },
		{ "_EVA_OUTPUT",		"_EVA_LIST",			ctrl_add_output_list_simple,NULL },
		{ "_EVA_OUTPUT",		"_EVA_LIST_SIMPLE",		ctrl_add_output_list_simple,NULL },
		{ "_EVA_OUTPUT",		"_EVA_STATLIST",		ctrl_add_output_stats,		NULL },
		{ "_EVA_OUTPUT",		"_EVA_CALENDAR",		ctrl_add_output_calendar,	NULL },
		{ "_EVA_OUTPUT",		"_EVA_PLANNING",		ctrl_add_output_planning,	NULL },
		{ "_EVA_OUTPUT",		"_EVA_CATALOG",			ctrl_add_output_catalog,	NULL },
		{ "_EVA_STATICTEXT",	NULL,					ctrl_add_statictext,		NULL },
		{ "_EVA_IMAGE",			NULL,					ctrl_add_image,				NULL },
		{ "_EVA_CALC",			NULL,					ctrl_add_exprval,			NULL },
		{ "_EVA_ANCHOR",		NULL,					ctrl_add_anchor,			NULL },
		{ NULL,					NULL,					ctrl_add_unknown,			NULL } 
#endif /* _EVA_DLL */
	};

/*********************************************************************
** Constant : action_handlers
** Description : action handlers definition table for ACTION attribute
*********************************************************************/
EVA_handlers action_handlers[] = 
	{
#ifdef _EVA_DLL
	NULL
#else 
	/*	ACTION							Handler */
		{ "_EVA_OPENFORM",			NULL,	action_openform },
		{ "_EVA_LAUNCHPROC",		NULL,	action_launchproc },
		{ "_EVA_NEWOBJ",			NULL,	action_formbuttons },
		{ "_EVA_SAVE",				NULL,	action_formbuttons },
		{ "_EVA_SAVECLOSE",			NULL,	action_formbuttons },
		{ "_EVA_CLOSE",				NULL,	action_formbuttons },
		{ "_EVA_PRINTMODE",			NULL,	action_formbuttons },
		{ "_EVA_EDITMODE",			NULL,	action_formbuttons },
		{ "_EVA_VIEWMODE",			NULL,	action_formbuttons },
		{ "_EVA_LISTMODE",			NULL,	action_formbuttons },
		{ "_EVA_STATSMODE",			NULL,	action_formbuttons },
		{ "_EVA_VALUESMODE",		NULL,	action_formbuttons },
		{ "_EVA_SEARCHMODE",		NULL,	action_formbuttons },
		{ "_EVA_CLOSEALL",			NULL,	action_closeall },
		{ "_EVA_EXPORT",			NULL,	action_export },
		{ "_EVA_CREATE",			NULL,	action_create_objects },
		{ "_EVA_IMPORT",			NULL,	action_import },
		{ "_EVA_DELETE",			NULL,	action_delete },
		{ "_EVA_COPYOBJ",			NULL,	action_copyobj },
		{ "_EVA_END_SESSION",		NULL,	action_end_session },
		{ "_EVA_UPDATE",			NULL,	action_update },
		{ "_EVA_OPENEDFORMS_DLG",	NULL,	dlg_openedforms },
		{ "_EVA_SQL_SCRIPT",		NULL,	action_sql_script },
		{ "_EVA_EMAIL",				NULL,	action_email },
		{ "_EVA_ACTION_SEQ",		NULL,	action_sequence },
		{ "_EVA_PAY_SITE",			NULL,	action_pay_site },
		{ NULL,						NULL,	action_default } 
#endif /* _EVA_DLL */
	};

/*********************************************************************
** Function : ctrl_call_act_handler
** Description : handle buttons clic
*********************************************************************/
#define ERR_FUNCTION "ctrl_call_act_handler"
#define ERR_CLEANUP 
int ctrl_call_act_handler(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	unsigned long i = 0; 
	char *action = strcmp(ctrl->CONTROL, "_EVA_BUTTON") ? "" : CTRL_ATTR_VAL(ACTION); 

	while(action_handlers[i].control && strcmp(action_handlers[i].control, action)) i++; 
	if(action_handlers[i].h0 && action_handlers[i].h0(cntxt, i_ctrl)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
