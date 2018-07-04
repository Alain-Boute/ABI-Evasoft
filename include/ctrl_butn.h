/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_add_butn.h
** Description : handling fonctions for BUTTON controls
**      Author : Alain BOUTE
**     Created : March 27 2002
*********************************************************************/


/*********************************************************************
** Function : put_html_button_sz
** Description : output HTML code for a button control
*********************************************************************/
int put_html_button_sz(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name,	 						/* in : input name */
	char *value,					 	/* in : input click value */
	char *img,	 						/* in : image file */
	char *img1,							/* in : image displayed when mouse over  */
	char *alt,	 						/* in : tooltip text */
	char *style, 						/* in : CSS style class */
	unsigned long width,				/* in : image width */
	unsigned long height,				/* in : image height */
	int border,						 	/* in : border size */
	int mode						 	/* in : display mode - bit mask
											bit 0 : button disabled if set
											bit 1 : dont use JavaScript if set
											bit 2 : aligned on text if set
											bit 3 : output label if set
											bit 4 : label position is left if set
											bit 5 : omit image if set
											bit 6 : output anchor (use href) if set
											bit 7 : use checkbox handler if set */
);
#define put_html_button(cntxt, name, value, im, im1, alt, border, mode) \
	put_html_button_sz(cntxt, name, value, im, im1, alt, NULL, 0, 0, border, mode)

/*********************************************************************
** Function : ctrl_add_button
** Description : handles BUTTON controls
*********************************************************************/
int ctrl_add_button(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in/out : execution context data */
	unsigned long i_ctrl						/* in : control index in cntxt->form->ctrl */
);
