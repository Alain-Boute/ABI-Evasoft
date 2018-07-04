/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : file_image_utils.c
** Description : image files handling utilities - includes GD library
**      Author : Alain BOUTE
**     Created : Jan 9 2010
*********************************************************************/

#include "eva.h"
#include "gd.h"

/*********************************************************************
** Typedef : imgType
** Description : image type
*********************************************************************/
typedef enum { imgUnkonwn, imgGIF, imgPNG, imgBMP, imgJPG } imgType;

/*********************************************************************
** Fonction : get_image_size_head
** Description : scan image size based on headers
*********************************************************************/
int get_image_size_head(	/* return : 1 if size found, 0 else */
	char *header,			/* in : image data (64 first bytes are enought but extra bytes may be used) */
	size_t sz,				/* in : image data size */
	unsigned long *width,	/* out : image width - 0 if not found */
	unsigned long *height,	/* out : image height - 0 if not found */
	imgType *imgtyp			/* out : image type */
){
	/* Check / init params - return 0 on error */
	if(imgtyp) *imgtyp = imgUnkonwn;
	if(!header || !sz) return 0;

#define BYTES_TO_WORD(c, l, m) ((c[l] & 0xFF) | (c[m] & 0xFF) << 8)

	/* Check image type */
	if(!strncmp(header, add_sz_str("GIF")))
	{
		/* Read GIF image size */
		if(imgtyp) *imgtyp = imgGIF;
		if(width) *width = BYTES_TO_WORD(header, 6, 7);
		if(height) *height = BYTES_TO_WORD(header, 8, 9);
	}
	else if(*header == 137 && !strncmp(header+1, add_sz_str("PNG")))
	{
		/* Read PNG image size */
		if(imgtyp) *imgtyp = imgPNG;
		if(width) *width = BYTES_TO_WORD(header, 18, 19);
		if(height) *height = BYTES_TO_WORD(header, 22, 23);
	}
	else if(!strncmp(header+1, add_sz_str("BMP")))
	{
		/* Read BMP image size */
		if(imgtyp) *imgtyp = imgBMP;
		if(width) *width = BYTES_TO_WORD(header, 19, 18);
		if(height) *height = BYTES_TO_WORD(header, 23, 22);
	}
	else 
	{
		/* Check for JPEG image markers FF/D8/FF*/
		size_t sz1;
		char *mark = header - 1;
		if(sz > 10) sz -= 10;
		do mark = memchr(mark + 1, 0xFF, sz - (size_t)(mark - header));
		while(mark && (size_t)(mark - header) < sz && !(mark[1] == '\xD8' && mark[2] == '\xFF'));
		if(mark) mark += 2;

		/* JPEG marker found : search for image marker FF/C0 & others */
		while(mark && (size_t)(mark - header) < sz)
		{
			while(*mark != '\xFF' && (size_t)(mark - header) < sz) mark++;
			if(*mark != '\xFF') break;
			mark++;
			if(*mark >= '\xC0' && *mark <= '\xCF' && *mark != '\xC4' && *mark != '\xC8' && *mark != '\xCC')
			{
				/* Read JPG image size */
				if(imgtyp) *imgtyp = imgJPG;
				if(width) *width = BYTES_TO_WORD(mark, 7, 6);
				if(height) *height = BYTES_TO_WORD(mark, 5, 4);
				break;
			}
			sz1 = BYTES_TO_WORD(mark, 2, 1);
			mark = (mark - header + sz1 >= sz) ? NULL : mark + sz1 + 1;
		}
	}
	return (!width || *width) && (!height || *height);
}

/*********************************************************************
** Fonction : get_image_size_fast
** Description : scan image size from a file - fast version base on headers
*********************************************************************/
int get_image_size_fast(	/* return : 1 if size found, 0 else */
	char *imgpath,			/* in : image file path */
	unsigned long *width,	/* out : image width - 0 if not found */
	unsigned long *height,	/* out : image height - 0 if not found */
	imgType *imgtyp			/* out : image type */
){
	char header[0x40];
	FILE *f;
	size_t sz;

	/* Open file & read header - return 0 on error */
	f = fopen (imgpath, "rb");
	if(!f) return 0;
	sz = fread(header, 1, sizeof(header), f);
	fclose(f);

	return get_image_size_head(header, sz, width, height, imgtyp);
}

/*********************************************************************
** Fonction : get_image_data
** Description : read image data from a file
*********************************************************************/
char *get_image_data(		/* return : image data (alloc-ed memory), NULL on error */
	char *imgpath,			/* in : image file path */
	size_t sz,				/* in : image file size */
	gdImagePtr *img,		/* out : GD image pointer */
	imgType *imgtyp			/* out : image type */
){
	FILE *f;
	char *data;

	/* Alloc memory for data - return on error */
	if(imgtyp) *imgtyp = imgUnkonwn;
	data = malloc(sz);
	if(!data) return NULL;

	/* Open file - skip processing on error */
	f = fopen(imgpath, "rb");
	if(f)
	{
		/* Read data */
		fread(data, 1, sz, f);
		fclose(f);

		/* Handle image type - call specific GD function */
		if(!strncmp(data, add_sz_str("GIF")))
		{
			if(imgtyp) *imgtyp = imgGIF;
			*img = gdImageCreateFromGifPtr(sz, data);
		}
		else if(!strncmp(data+1, add_sz_str("PNG")))
		{
			if(imgtyp) *imgtyp = imgPNG;
			*img = gdImageCreateFromPngPtr(sz, data);
		}
		else if(!strncmp(data+1, add_sz_str("BMP")))
		{
			if(imgtyp) *imgtyp = imgBMP;
			*img = gdImageCreateFromWBMPPtr(sz, data);
		}
		else 
		{
			if(imgtyp) *imgtyp = imgJPG;
			*img = gdImageCreateFromJpegPtr(sz, data);
		}
	}
	else
	{
		free(data);
		return NULL;
	}

	return data;
}

/*********************************************************************
** Fonction : get_image_size
** Description : scan image size from a file
*********************************************************************/
int get_image_size(			/* return : 0 if file found & params correct, 1 else */
	char *imgpath,			/* in : image file path */
	unsigned long *width,	/* out : image width - 0 if not found */
	unsigned long *height	/* out : image height - 0 if not found */
){
	struct stat fs = {0};
	char *data = NULL;
	gdImagePtr img = NULL;
	imgType imgtyp = imgUnkonwn;

	/* Init params - return on error */
	if(!imgpath || !*imgpath || !height && !width) return 1;
	if(width) *width = 0;
	if(height) *height = 0;

	/* Check file status - return error if not found */
	if(stat(imgpath, &fs)) return 1;

	/* Try fast mode - return on success */
	if(get_image_size_fast(imgpath, width, height, &imgtyp)) return 0;

	/* Open file - skip processing on error */
	data = get_image_data(imgpath, fs.st_size, &img, &imgtyp);
	if(data)
	{
		/* Set dimensions if found */
		if(img)
		{
			if(width) *width = img->sx;
			if(height) *height = img->sy;
			gdImageDestroy(img);
		}

		/* Free alloc-ed data */
		free(data);
	}

	return !(data && img);
}

/*********************************************************************
** Fonction : get_image_thumb
** Description : return thumbnail file for the given image - build if necessary
*********************************************************************/
char *get_image_thumb(		/* return : image file path (alloc-ed memory), NULL if not found */
	EVA_context *cntxt,		/* in/out : execution context data */
	char *imgpath,			/* in : image file path */
	size_t imgpath_sz,		/* in : image file path size */
	unsigned long dw,		/* in : desired image width - 0 if no constraint */
	unsigned long dh,		/* in : desired image height - 0 if no constraint */
	unsigned long *ow,		/* out : thumbnail width - 0 if not available */
	unsigned long *oh		/* out : thumbnail height - 0 if not available */
){
	FILE *f;
	struct stat fs = {0};
	char *path = NULL;
	char *data = NULL;
	gdImagePtr img = NULL;
	unsigned long w = 0, h = 0;
	char *imgfname = basename(imgpath, imgpath_sz);
	size_t sz, imgsz, p0 = imgfname - imgpath;
	int b_noresize = 1;
	imgType imgtyp = imgUnkonwn;

	/* Init params - return NULL on error */
	if(!imgpath || !*imgpath || (!dw && !dh)) return NULL;
	if(ow) *ow = 0;
	if(oh) *oh = 0;

	/* Check original file status - return NULL if not found */
	if(stat(imgpath, &fs)) return NULL;
	imgsz = fs.st_size;

	/* Alloc memory for path - return NULL on error */
	sz = imgpath_sz + 64;
	path = malloc(sz);
	if(!path) return NULL;

	/* Check if thumbnail exist - return path if so */
	memcpy(path, imgpath, p0);
	p0 += snprintf(path + p0, sz - p0, "cache");
	chdir(cntxt->path); mkdir(path);
	p0 += snprintf(path + p0, sz - p0, "/T%lux%lu-%s", dw, dh, imgfname);
	if(!stat(path, &fs))
	{
		if(ow || oh) get_image_size(path, ow, oh);
		return path;
	}

	/* Read image - skip processing on error */
	data = get_image_data(imgpath, imgsz, &img, &imgtyp);
	if(data) 
	{
		/* Handle image if it was read with GD */
		if(img)
		{
			gdImagePtr thumb = NULL;
			char *thumbdata = NULL;
			int thumbdatasz = 0;

			/* Compute resize factors & new dimensions */
			double rfw = !dw ? 1. : 1. * img->sx / dw;
			double rfh = !dh ? 1. : 1. * img->sy / dh;
			if(rfw && rfh && (rfw > 1 || rfh > 1))
			{
				/* Handle resize if needed */
				b_noresize = 0;
				if(rfw > rfh)
				{
					w = dw;
					h = (unsigned long)(0.5 + img->sy / rfw);
				}
				else
				{
					w = (unsigned long)(0.5 + img->sx / rfh);
					h = dh;
				}

				/* Handle image type */
				if(imgtyp == imgGIF)
				{
					/* Resize GIF image */
					thumb = gdImageCreate(w, h);
					gdImageCopyResized(thumb, img, 0, 0, 0, 0, w, h, img->sx, img->sy);
					thumbdata = gdImageGifPtr(thumb, &thumbdatasz);
				}
				else if(imgtyp == imgPNG)
				{
					/* Resize PNG image */
					thumb = gdImageCreate(w, h);
					gdImageCopyResized(thumb, img, 0, 0, 0, 0, w, h, img->sx, img->sy);
					thumbdata = gdImagePngPtr(thumb, &thumbdatasz);
				}
				else if(imgtyp == imgBMP)
				{
					/* Resize BMP image */
					thumb = gdImageCreate(w, h);
					gdImageCopyResized(thumb, img, 0, 0, 0, 0, w, h, img->sx, img->sy);
					thumbdata = gdImageWBMPPtr(thumb, &thumbdatasz, gdImageColorAllocate(thumb, 0, 0, 0));
				}
				else if(imgtyp == imgJPG)
				{
					/* Resize JPEG image */
					thumb = gdImageCreateTrueColor(w, h);
					gdImageCopyResized(thumb, img, 0, 0, 0, 0, w, h, img->sx, img->sy);
					thumbdata = gdImageJpegPtr(thumb, &thumbdatasz, -1);
				}

				/* Output thumb file */
				f = fopen(path, "wb");
				if(f)
				{
					if(thumbdata) fwrite(thumbdata, 1, thumbdatasz, f);
					fclose(f);
				}
				gdImageDestroy(img);
				gdFree(thumbdata);
				gdImageDestroy(thumb);
			}
			else
			{
				w = img->sx;
				h = img->sy;
			}
		}
		else
		{
			/* Image could not be read by GD : check alternate size read method */
			if(get_image_size_head(data, imgsz, &w, &h, NULL))
			{
				/* Size available with alternate method : use display resize */
				double rfw = !dw ? 1. : 1. * w / dw;
				double rfh = !dh ? 1. : 1. * h / dh;
				if(rfw && rfh && (rfw > 1 || rfh > 1))
				{
					/* Handle display resize if needed */
					if(rfw > rfh)
					{
						w = dw;
						h = (unsigned long)(0.5 + h / rfw);
					}
					else
					{
						w = (unsigned long)(0.5 + w / rfh);
						h = dh;
					}
				}
			}
		}

		/* Set return dimensions where applicable */
		if(ow) *ow = w;
		if(oh) *oh = h;

		/* Free alloc-ed data */
		free(data);
	}

	/* Handle no resize : free path & return NULL */
	if(b_noresize) M_FREE(path);

	return path;
}
