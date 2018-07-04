/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : form_save.h
** Description : forms save & close functions
**      Author : Alain BOUTE
**     Created : Jan 11 2003
*********************************************************************/


#define BUTN_SAVE_NEXT		0x1
#define BUTN_SAVE_CLOSE		0x2
#define BUTN_SAVE_BACK		0x4
#define BUTN_SAVE			(BUTN_SAVE_NEXT | BUTN_SAVE_CLOSE | BUTN_SAVE_BACK)
#define BUTN_CLOSE			0x8
#define BUTN_RESTORE		0x10
#define BUTN_GOBACKOBJ		0x20

/*********************************************************************
** Function : form_save_dialog
** Description : handle a Save / Restore / Cancel dialog 
*********************************************************************/
int form_save_dialog(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl,			/* in : control index in cntxt->form->ctrl */
	char *confirm,					/* in : confirm mode */
	int butns,						/* in : buttons to display */
	int *b_done						/* out : 1 if object is saved */
);

