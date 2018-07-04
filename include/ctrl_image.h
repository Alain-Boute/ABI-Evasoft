/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : ctrl_image.c
** Description : handling fonctions for static image controls
**      Author : Alain BOUTE
**     Created : Feb 17 2002
*********************************************************************/

/*********************************************************************
** Function : put_html_image_dir
** Description : output HTML code for an image
*********************************************************************/
int put_html_image_dir(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name,	 						/* in : image name */
	char *dir,							/* in : image directory */
	char *img,							/* in : image file */
	char *alt,							/* in : tooltip text */
	char *width,						/* in : image width */
	char *height,					 	/* in : image height */
	int border,						 	/* in : border size */
	unsigned long idobj,			 	/* in : object Id to open on click
											0 = open full size image for thumbnail
											1 = simple image (no anchor) */
	unsigned long *ow,					/* out : output image or thumb width */
	unsigned long *oh					/* out : output image or thumb height */
);

/*********************************************************************
** Function : put_html_img
** Description : output HTML code for an image control
*********************************************************************/
int put_html_img(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name,	 						/* in : image name */
	char *img,							/* in : image file */
	char *alt,							/* in : tooltip text */
	char *width,						/* in : image width */
	char *height,					 	/* in : image height */
	int border,						 	/* in : border size */
	char *extra						 	/* in : extra tags for image such as OnClick=... */
);
#define put_html_image(c, n, i, a, w, h, b) put_html_img(c, n, i, a, w, h, b, NULL)

/*********************************************************************
** Function : ctrl_add_image
** Description : handles IMAGE controls
*********************************************************************/
int ctrl_add_image(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl							/* in : control index in cntxt->form->ctrl */
);
