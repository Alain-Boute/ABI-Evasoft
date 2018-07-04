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


/*********************************************************************
** Typedef : EVA_handler
** Description : control handler callback definition
*********************************************************************/
typedef int (* EVA_handler)(EVA_context *cntxt, unsigned long i_ctrl);

/*********************************************************************
** Typedef : EVA_handlers
** Description : handling functions table for controls
*********************************************************************/
typedef struct _EVA_handlers
{
	char *control;			/* control type */
	char *type;				/* control subtype */
	EVA_handler h0;			/* primary handler */
	EVA_handler h1;			/* secondary handler */
} 
	EVA_handlers;

/*********************************************************************
** Constant : ctrl_handlers
** Description : control handlers definition table for CONTROL & TYPE attributes
*********************************************************************/
extern EVA_handlers ctrl_handlers[];

/*********************************************************************
** Macro : CTRL_PRI_HDLR
** Description : call primary handler for a control
*********************************************************************/
#define CTRL_PRI_HDLR(i) if(i) { if(ctrl_primary_handler(cntxt, i)) STACK_ERROR; }

/*********************************************************************
** Macro : CTRL_SEC_HDLR
** Description : call secondary handler for a control
*********************************************************************/
#define CTRL_SEC_HDLR(i) if(i && ctrl_handlers[cntxt->form->ctrl[i].i_handler].h1 && \
							ctrl_handlers[cntxt->form->ctrl[i].i_handler].h1(cntxt, i)) STACK_ERROR

/*********************************************************************
** Macro : CTRL_ACT_HDLR
** Description : call action handler for a control
*********************************************************************/
#define CTRL_ACT_HDLR(ic) {  if(ctrl_call_act_handler(cntxt, ic)) STACK_ERROR; }

/*********************************************************************
** Constant : action_handlers
** Description : action handlers definition table for ACTION attribute
*********************************************************************/
extern EVA_handlers action_handlers[];

/*********************************************************************
** Function : ctrl_call_act_handler
** Description : handle form interaction
*********************************************************************/
int ctrl_call_act_handler(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl			/* in : control index in cntxt->form->ctrl */
);
