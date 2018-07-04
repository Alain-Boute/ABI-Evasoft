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

#include "eva.h"

/*********************************************************************
** Function : put_html_image_dir
** Description : output HTML code for an image or thumbnail
*********************************************************************/
#define ERR_FUNCTION "put_html_image_dir"
#define ERR_CLEANUP	DYNTAB_FREE(data); \
					M_FREE(thumb)
int put_html_image_dir(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name,	 						/* in : image name */
	char *dir,							/* in : image directory */
	char *img,							/* in : image file or IdObj of a file object */
	char *alt,							/* in : tooltip text */
	char *width,						/* in : image width  */
	char *height,					 	/* in : image height */
	int border,						 	/* in : border size */
	unsigned long idobj,			 	/* in : object Id to open on click
											0 = open full size image for thumbnail
											1 = simple image (no anchor) */
	unsigned long *ow,					/* out : output image or thumb width */
	unsigned long *oh					/* out : output image or thumb height */
){
	DynTable data = {0};
	char path[4096] = {0};
	char bigpath[4096] = {0};
	size_t sz = 0;
	struct stat fs = {0};
	int b_anchor = 0, b_doc = 0, b_thumb = 0;
	unsigned long dw = 0, dh = 0, w = 0, h = 0;
	char *thumb = NULL, wstr[16], hstr[16], *end = "";
	unsigned long idimg;
	
	/* Handle thumbnails - check integer for desired size */
	if(width) dw = strtoul(width, &end, 10);
	if(*end) dw = 0;
	if(height) dh = strtoul(height, &end, 10);
	if(*end) dh = 0;
	b_thumb = dw || dh;

	/* Get image file name for a relation */
	idimg = img ? strtoul(img, &end, 10) : 0;
	if(idimg && *end) idimg = 0; 
	if(idimg)
	{
		if(qry_obj_field(cntxt, &data, idimg, "_EVA_FILE_SERVER_NAME")) STACK_ERROR;
		img = dyntab_val(&data, 0, 0);
		b_doc = 1;
	}

	/* Build full path with given dir */
	if(dir && *dir)
	{
		if(strchr("./", *dir))
			sz = snprintf(add_sz_str(path), "%s/%s", dir, img);
		else
		{
			snprintf(add_sz_str(path), "../docs/%s/%s/%s", cntxt->dbname, dir, img);
			sz = snprintf(add_sz_str(bigpath), "../docs/%s/%s/big/%s", cntxt->dbname, dir, img);
			b_thumb = 0;
		}
	}
	else if(b_doc)
		sz = snprintf(add_sz_str(path), "../docs/%s/%s", cntxt->dbname, img);
	else
		sz = snprintf(add_sz_str(path), "../img/%s", img);
	
	/* Handle thumbnails - create if needed */
	if(b_thumb)
	{
		/* Create thumbnail in cache */
		thumb = get_image_thumb(cntxt, path, strlen(path), dw, dh, &w, &h);
		if(thumb)
		{
			/* Use thumbnail for image tag if created */
			sz = snprintf(add_sz_str(bigpath), "%s", path);
			snprintf(add_sz_str(path), "%s", thumb);
		}

		/* Set actual image dimensions */
		if(w)
		{
			snprintf(add_sz_str(wstr), "%lu", w);
			width = wstr;
		}
		if(h)
		{
			snprintf(add_sz_str(hstr), "%lu", h);
			height = hstr;
		}
	}
	else if(ow || oh)
		get_image_size(path, &w, &h);

	/* Handle return dimensions if applicable */
	if(ow) *ow = w;
	if(oh) *oh = h;

	/* Add link to image or object where applicable */
	if(name && *name || idobj && idobj != DYNTAB_TOUL(&cntxt->form->id_obj))
	{
		/* Output link to object */
		char opname[64];
		size_t opname_sz = (name && *name) ? strlen(name) : build_open_btn_name(cntxt, opname, 0, idobj, 0, 0);
		if(!name || !*name) name = opname;
		DYNBUF_ADD3(cntxt->form->html, "<a href=javascript:cb('", name, opname_sz, NO_CONV, "')>");
		b_anchor = 1;
	}
	else if(bigpath && *bigpath && !stat(bigpath, &fs) != 0)
	{
		/* Output link to big image */
		DYNBUF_ADD3(cntxt->form->html, "<a href='", bigpath, sz, NO_CONV, "' target=_blank>");
		b_anchor = 1;
	}
	
	/* Default tooltip if no set */
	if(!alt && b_anchor) alt = (name && *name || idobj) ? "Ouvrir la fiche contenant l'image" : "Afficher l'image en haute résolution";

	/* Add image */
	if(put_html_image(cntxt, name, path, alt, width, height, border)) STACK_ERROR;
	if(b_anchor) DYNBUF_ADD_STR(cntxt->form->html, "</a>");
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : put_html_img
** Description : output HTML code for an image
*********************************************************************/
#define ERR_FUNCTION "put_html_img"
#define ERR_CLEANUP M_FREE(img)
int put_html_img(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *name,	 						/* in : image name */
	char *im,							/* in : image file */
	char *alt,							/* in : tooltip text */
	char *width,						/* in : image width */
	char *height,					 	/* in : image height */
	int border,						 	/* in : border size */
	char *extra						 	/* in : extra tags for image such as OnClick=... */
){
	char *img;
	unsigned long w = width ? strtoul(width, NULL, 10) : 0;
	unsigned long h = height ? strtoul(height, NULL, 10) : 0;
	CHECK_HTML_STATUS;

	img = get_image_file(cntxt, im, &w, &h);
	if(!img) RETURN_OK;

	DYNBUF_ADD_STR(html, "<img");
	if(name && *name) DYNBUF_ADD3(html, " name='", name, 0, NO_CONV, "'");
	if(alt && *alt) 
	{
		DYNBUF_ADD3(html, " title='", alt, 0, HTML_TOOLTIP, "'");
		if(put_showhelp(cntxt, html)) STACK_ERROR;
	}
	if(width && *width) DYNBUF_ADD3(html, " width='", width, 0, NO_CONV, "'")
	if(height && *height) DYNBUF_ADD3(html, " height='", height, 0, NO_CONV, "'")
	if(!(width && *width || height && *height))
	{
		if(w) DYNBUF_ADD3_INT(html, " width=", w, "");
		if(h) DYNBUF_ADD3_INT(html, " height=", h, "");
	}
	DYNBUF_ADD3_INT(html, " border=", border, "")
	DYNBUF_ADD3(html, " src='", img, 0, NO_CONV, "'");
	if(extra && *extra) DYNBUF_ADD3(html, " ", extra, 0, NO_CONV, "");
	DYNBUF_ADD_STR(html, " align=absmiddle>");
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Function : ctrl_add_image
** Description : handles IMAGE controls
*********************************************************************/
#define ERR_FUNCTION "ctrl_add_image"
#define ERR_CLEANUP	M_FREE(img)
int ctrl_add_image(						/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
){
	EVA_form *form = cntxt->form;
	EVA_ctrl *ctrl = form->ctrl + i_ctrl;
	char *animation;
	char *image, *img = NULL;

	switch(form->step)
	{
	case HtmlPrint:
	case HtmlView:
	case HtmlEdit:
		/* Set IMAGE default attributes values */
		if(!ctrl->LABELPOS[0]) ctrl->LABELPOS = "_EVA_NONE";
		if(!ctrl->OPTIONBUTTON[0]) ctrl->OPTIONBUTTON = "_EVA_NONE";
		image = CTRL_ATTR_VAL(IMAGE);

		/* Handle JavaScript animation during page load */
		if(cntxt->jsenabled)
		{
			animation = CTRL_ATTR_VAL(IMAGE_PAGELOAD);
			if(*animation)
			{
				img = get_image_file(cntxt, image, 0, 0);
				image = img ? img : "../img/_eva_nop.gif";
				DYNBUF_ADD3_BUF(&cntxt->endjs, "document.mainform[\"", ctrl->cginame, NO_CONV, "\"].src='");
				DYNBUF_ADD(&cntxt->endjs, image, 0, NO_CONV);
				DYNBUF_ADD_STR(&cntxt->endjs, "';");
				image = animation;
			}
		}

		/* Output image */
		if(ctrl_format_pos(cntxt, ctrl, 1) ||
			put_html_image(cntxt, ctrl->cginame->data, 
							image, 
							ctrl->LABEL,
							CTRL_ATTR_VAL(IMGWIDTH),
							CTRL_ATTR_VAL(IMGHEIGHT),
							atoi(CTRL_ATTR_VAL(IMGBORDER))) ||
			ctrl_format_pos(cntxt, ctrl, 0)) 
			STACK_ERROR;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP
