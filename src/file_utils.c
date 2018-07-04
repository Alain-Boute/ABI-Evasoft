	/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : file_utils.c
** Description : TAB/RC files handling utilities
**      Author : Alain BOUTE
**     Created : April 28 2002
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Fonction : dyntab_add_strip
** Description : store text data in a dyntable
*********************************************************************/
#define ERR_FUNCTION "dyntab_add_strip"
#define ERR_CLEANUP 
int dyntab_add_strip(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,				/* in/out : result table */
	unsigned long row,			/* in : cell to modify */
	unsigned long col,			/* in : cell to modify */
	char *v,					/* in : value to add */
	size_t s					/* in : size of value */
){
	STRIP_SPACES_AROUND(v, s);
	if(s) DYNTAB_ADD(res, row, col, v, s, IMPORT_TABRC);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_tabrc_to_dyntab
** Description : store tabulated text data in a dyntable
*********************************************************************/
#define ERR_FUNCTION "file_tabrc_to_dyntab"
#define ERR_CLEANUP 
int file_tabrc_to_dyntab(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : result table */
	char *data,						/* in : file data with \n & \t delimitors */
	size_t sz						/* in : size of data */
){
	unsigned long row = 0, col = 0, i, i0;

	/* Read records in DynTable */
	dyntab_free(res);
	for(i = 0, i0 = 0; i < sz; i++) 
	{
		switch(data[i])
		{
		case '\t':
			if(i > i0 && dyntab_add_strip(cntxt, res, row, col, data + i0, i - i0)) STACK_ERROR;
			col++;
			i0 = i+1;
			break;
		case '\n':
			if(i > i0 && dyntab_add_strip(cntxt, res, row, col, data + i0, i - i0)) STACK_ERROR;
			col = 0;
			row++;
			i0 = i+1;
			break;
		}
	}
	if(i > i0 && dyntab_add_strip(cntxt, res, row, col, data + i0, i - i0)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_to_dynbuf
** Description : read a file in a dynbuffer
*********************************************************************/
#define ERR_FUNCTION "file_to_dynbuf"
#define ERR_CLEANUP 
int file_to_dynbuf(					/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **res,				/* out : result buffer */
	char *file						/* in : input file name */
){
	struct stat sts;			/* for stat() */
	FILE *f = 0;				/* stdio file pointer */

	/* Read file status & alloc memory for file data */
	if(!res || !file || !*file) RETURN_OK;
	if(stat(file, &sts)) RETURN_ERROR("Fichier non trouvé", NULL);
	*res = dynbuf_init(sts.st_size+16);

	/* Open file & read data */
	f = fopen(file, "r");
	if(!f)  RETURN_ERROR("Impossible d'ouvrir le fichier", NULL);
	(*res)->cnt = fread((*res)->data, 1, sts.st_size+2, f);
	fclose(f);

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_read_tabrc
** Description : read a tabulated text file in a dyntable
*********************************************************************/
#define ERR_FUNCTION "file_read_tabrc"
#define ERR_CLEANUP M_FREE(fdata)
int file_read_tabrc(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *res,					/* out : result table */
	char *file						/* in : input file name or uploaded file IdObj */
){
	struct stat sts;			/* for stat() */
	FILE *f = 0;				/* stdio file pointer */
	size_t sz;					/* char counter */
	char fpath[2048];			/* file path if uploaded */
	char *fdata = NULL;			/* alloc-ed file data */
	unsigned long id;			/* uploaded file IdObj */
	char *end = NULL;			/* for strtoul */
	DynTable fname = {0};		/* uploaded file name */

	/* Check if file is IdObj */
	id = strtoul(file, &end, 10);
	if(id && (!end || !*end))
	{
		/* file is an uploaded object : get uploaded file path */
		if(qry_obj_field(cntxt, &fname, id, "_EVA_FILE_SERVER_NAME")) STACK_ERROR;
		snprintf(add_sz_str(fpath), "../docs/%s/%s", cntxt->dbname, dyntab_val(&fname, 0, 0));
		file = fpath;
	}

	/* Read file status & alloc memory for file data */
	if(!file || stat(file, &sts)) RETURN_ERROR("Fichier non trouvé", NULL);
	M_ALLOC(fdata, sts.st_size+16);

	/* Open file & read data */
	f = fopen(file, "r");
	if(!f)  RETURN_ERROR("Impossible d'ouvrir le fichier", NULL);
	sz = fread(fdata, 1, sts.st_size+2, f);
	fdata[sz] = 0;
	fclose(f);

	/* Read records in DynTable */
	if(file_tabrc_to_dyntab(cntxt, res, fdata, sz)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_read_config
** Description : return file name from file name or file IdObj
*********************************************************************/
#define ERR_FUNCTION "file_read_config"
#define ERR_CLEANUP
int file_read_config(				/* return : 0 on success, other on error */
	EVA_context *cntxt				/* in/out : execution context data */
){
	chdir(cntxt->path);
	if(file_read_tabrc(cntxt, &cntxt->cnf_server, "serverconfig.conf"))
	{
		DYNTAB_SET(&cntxt->cnf_server, 0, 0, "C:\\Program Files\\MSOfficeEVA");
		cntxt->dbpwd = "";
		CLEAR_ERROR_NOWARN;
	}
	else
	{
		cntxt->dbpwd = dyntab_val(&cntxt->cnf_server, 1, 0);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_from_idobj
** Description : return file name from file name or file IdObj
*********************************************************************/
#define ERR_FUNCTION "file_from_idobj"
#define ERR_CLEANUP
int file_from_idobj(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	char **file,					/* in : file name or object Id
									   out : file name */
	DynTable *data					/* out : temporary data (free after file is used) */
){
	unsigned long idobj = (file && *file) ? strtoul(*file, NULL, 10) : 0;
	if(idobj)
	{
		if(qry_obj_field(cntxt, data, idobj, "_EVA_FILE_SERVER_NAME")) STACK_ERROR;
		if(data->nbrows) *file = data->cell->txt;
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_read_import_data
** Description : read object format file into forms & object data dyntables
*********************************************************************/
#define ERR_FUNCTION "file_read_import_data"
#define ERR_CLEANUP if(f) fclose(f); \
					M_FREE(fdata); \
					DYNTAB_FREE(data)
int file_read_import_data(			/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *forms,				/* out : forms referenced by imported objects */
	DynTable *objdata,				/* out : imported objects data */
	char *file						/* in : input file name */
){
	DynTable data = { 0 };
	struct stat sts;			/* for stat() */
	FILE *f = 0;				/* stdio file pointer */
	size_t sz;					/* char counter */
	char *fdata = NULL;			/* alloc-ed file data */
	char *p1;

	/* Handle file object */
	if(!file || !*file) RETURN_ERROR("Import file is NULL", NULL);
	if(file_from_idobj(cntxt, &file, &data)) STACK_ERROR;

	/* Read file status & alloc memory for file data */
	if(!file || stat(file, &sts)) RETURN_ERROR("Import file not found", NULL);
	M_ALLOC(fdata, sts.st_size+2);

	/* Open file & read data */
	f = fopen(file, "r");
	if(!f)  RETURN_ERROR("Cannot open import file", NULL);
	sz = fread(fdata, 1, sts.st_size + 2, f);
	if(!sz)  RETURN_ERROR("Cannot read import file", NULL);
	fdata[sz] = 0;
	fclose(f); f = NULL;

	/* Find end of forms marker : two consecutive \n */
	p1 = strstr(fdata, "\n\n");
	if(strncmp(fdata, add_sz_str("IdObj	Label\n")) ||
		!p1 || strncmp(p1 + 2, add_sz_str("IdObj	Field	Value	Type	Num	Line\n")))
		RETURN_ERROR("Invalid format for import file", NULL);
	*p1 = 0;

	/* Read forms & objdata in DynTable */
	if(file_tabrc_to_dyntab(cntxt, forms, fdata, p1 - fdata)) STACK_ERROR;
	if(file_tabrc_to_dyntab(cntxt, objdata, p1 + 2, sz - (p1 - fdata) - 2)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : chdir_user_doc
** Description : change current directory to user temporary documents
*********************************************************************/
#define ERR_FUNCTION "chdir_user_doc"
#define ERR_CLEANUP
int chdir_user_doc(				/* return : 0 on success, other on error */
	EVA_context *cntxt			/* in : execution context data */
){
	if(chdir_db_doc(cntxt)) STACK_ERROR;
	mkdir(dyntab_val(&cntxt->id_user, 0, 0));
	if(chdir(dyntab_val(&cntxt->id_user, 0, 0))) RETURN_ERR_DIRECTORY(1);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : chdir_db_doc
** Description : change current directory to database documents
*********************************************************************/
#define ERR_FUNCTION "chdir_db_doc"
#define ERR_CLEANUP
int chdir_db_doc(				/* return : 0 on success, other on error */
	EVA_context *cntxt			/* in : execution context data */
){
	if(chdir(cntxt->rootdir)) RETURN_ERR_DIRECTORY(1);
	mkdir(DIRECTORY_DOCS);
	if(chdir(DIRECTORY_DOCS)) RETURN_ERR_DIRECTORY(1);
	mkdir(cntxt->dbname);
	if(chdir(cntxt->dbname)) RETURN_ERR_DIRECTORY(1);
	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : FILE_write_tabrc
** Description : write a table in a tabulated text file
*********************************************************************/
#define ERR_FUNCTION "FILE_write_tabrc"
#define ERR_CLEANUP M_FREE(buf)
int FILE_write_tabrc(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *data,					/* in : table to write */
	FILE *f							/* in : output file stream */
){
	DynBuffer *buf = NULL;
	unsigned long i, j;

	if(!data || !f) RETURN_OK;

	/* Output objects data */
	for(i = 0; i < data->nbrows; i++) 
		for(j = 0; j < data->nbcols; j++)
		{
			DYNBUF_ADD_CELL(&buf, data, i, j, EXPORT_TABRC);
			fprintf(f, "%s%s", buf->data, j < data->nbcols-1 ?  "\t" : "\n");
			M_FREE(buf);
		}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_write_tabrc
** Description : write a table in a tabulated text file
*********************************************************************/
#define ERR_FUNCTION "file_write_tabrc"
#define ERR_CLEANUP if(f) fclose(f); \
					M_FREE(buf)
int file_write_tabrc(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynTable *data,					/* in : table to write */
	char *file						/* in : output file name */
){
	DynBuffer *buf = NULL;
	FILE *f = NULL;

	if(!data) RETURN_OK;

	/* Open export file */
	f = fopen(file, "w");
	if(!f) RETURN_ERROR("Impossible d'écrire dans le fichier", NULL);

	if(!FILE_write_tabrc(cntxt, data, f)) STACK_ERROR;

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_compatible_name
** Description : modify a file name to avoid system compatibility problems
*********************************************************************/
size_t file_compatible_name(		/* return : name final length */
	char *name						/* in/out : file name */
){
	unsigned long i, j;
	char *a = LOCASE_ACCENT UPCASE_ACCENT;
	char *n = LOCASE_NO_ACC UPCASE_NO_ACC;

	/* For each char in string */
	for(i = 0, j = 0; name[j]; j++)
	{
		char u = name[j];
		/* Check if char is alphanumeric */
		if(!(u >= 'a' && u <= 'z' || u >= 'A' && u <= 'Z' || u >= '0' && u <= '9'))
		{
			/* Search for accentuated char & replace whith non accentuated */
			char *c = strchr(a, u);
			if(c) u = n[c - a];
			/* Replace non aplanumeric chars with dot */
			else u = '.';
		}
		/* Ignore multiple dots or dot as first char */
		if(u == '.' && (!i || name[i - 1] == '.')) continue;
		name[i] = u;
		i++;
	}
	name[i] = 0;

	return i;
}

/*********************************************************************
** Fonction : setcase
** Description : case function handling accents
*********************************************************************/
char *setcase(				/* return : string overwriten */
	int mode,				/* in : conversion mode
								bit0 : convert to uppercase if set lowercase else
								bit 1 : no accents in output if set
								bit 2 : no case convert if set - just remove accents (bits 0 & 1 ignored) */
	char *txt,				/* in/out : string to convert */
	size_t len				/* in : max # of char to convert */
){
	unsigned long i;
	char *src = mode & 4 ? LOCASE_ACCENT UPCASE_ACCENT :
				mode & 1 ? LOCASE_ACCENT : UPCASE_ACCENT;
	char *dst = mode & 4 ? LOCASE_NO_ACC UPCASE_NO_ACC :
				mode & 1 ? 
				(mode & 2 ? UPCASE_NO_ACC : UPCASE_ACCENT) :
				(mode & 2 ? LOCASE_NO_ACC : LOCASE_ACCENT);

	/* Check params */
	if(!txt || !*txt) return txt;

	/* No size limit if len is 0 */
	if(!len) len = ~0UL;

	/* Convert each char */
	for(i = 0; txt[i] && i < len; i++)
	{
		char *t = txt + i;
		char *c = strchr(src, *t);
		if(c)
			*t = dst[c - src];
		else if(!(mode & 4) && *t >= 'A' && *t <= 'z')
			*t = (char)((mode & 1) ? toupper(*t) : tolower(*t));
	}

	return txt;
}

/*********************************************************************
** Function : get_image_file
** Description : find the file path & size given the filename
*********************************************************************/
char *get_image_file(					/* return : image file path (alloc-ed memory), NULL if not found */
	EVA_context *cntxt,					/* in/out : execution context data */
	char *img,	 						/* in : image file name (find in img using preferences) or path */
	unsigned long *width,				/* out : image width - 0 if not found */
	unsigned long *height				/* out : image height - 0 if not found */
){
	struct stat fs = {0};
	size_t sz = (img && *img) ? strlen(img) : 0, sz1 = 0;
	unsigned long i;
	char *path;
	int b_found = 0;

	/* Initialize size & check params (no quotes in filenames) */
	if(!sz || strpbrk(img, "'\"")) return NULL;
	chdir(cntxt->path);

	/* If path is given */
	if(strchr("/.", *img))
	{
		/* Copy path & check existence */
		path = mystrdup(img, strlen(img));
		if(!path) return NULL;
		b_found = !stat(path, &fs);
	}
	else
	{
		/* Path not given : search in prefered img subfolders */
		for(i = 0; i < cntxt->imgpath.nbrows; i++)
		{
			size_t len = dyntab_cell(&cntxt->imgpath, i, 0)->len;
			if(len > sz1) sz1 = len;
		}
		path = mem_alloc(sz1 + sz + 32);
		if(!path) return NULL;

		/* Look for image file in cntxt->imgpath folders */
		for(i = 0; i < cntxt->imgpath.nbrows && !b_found; i++)
		{
			sprintf(path, "../img/%s/%s", dyntab_val(&cntxt->imgpath, i, 0), img);
			b_found = !stat(path, &fs);
		}
		if(!b_found)
		{
			/* Look for image file in root path */
			sprintf(path, "../img/%s", img);
			b_found = !stat(path, &fs);
		}
	}

	/* Get image size */
	if(b_found && width && height) get_image_size(path, width, height);
	return path;
}

