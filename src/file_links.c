/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : file_links.c
** Description : output HTML links to downloadable files
**      Author : Alain BOUTE
**     Created : January 16 2004
*********************************************************************/

#include "eva.h"

/*********************************************************************
** Fonction : cgi_output_string
** Description : output string with CGI escape chars
*********************************************************************/
#define ERR_FUNCTION "cgi_output_string"
#define ERR_CLEANUP 
int cgi_output_string(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **html,				/* in/out : output buffer */
	char *txt, size_t sz			/* in : string to output */
){
	size_t i;
	if(!html || !txt || !*txt || !sz) RETURN_OK;

	for(i = 0; i < sz; i++)
	{
		if(!isalnum(txt[i]) && !strchr(".-_" LOCASE_ACCENT UPCASE_ACCENT, txt[i])) 
		{
			char tmp[8];
			sprintf(tmp, "%%%02X", txt[i] & 0xFF);
			DYNBUF_ADD(html, tmp, 3, NO_CONV);
		}
		else
			DYNBUF_ADD(html, txt + i, 1, NO_CONV);
	}

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

/*********************************************************************
** Fonction : file_output_link
** Description : output HTML link to the given file
*********************************************************************/
#define ERR_FUNCTION "file_output_link"
#define ERR_CLEANUP 
int file_output_link(				/* return : 0 on success, other on error */
	EVA_context *cntxt,				/* in : execution context data */
	DynBuffer **html,				/* in/out : output buffer */
	char *title, size_t sz_title,	/* in : file title (not in link - html format) */
	char *name, size_t sz_name,		/* in : file user name */
	char *fname, size_t sz_fname,	/* in : file server name */
	char *fdir,						/* in : file directory (from root) or special name
										#user = user directory
										#docs = site docs directory */
	char *notes, size_t sz_notes,	/* in : file comments (for tooltip) */
	char *msg, size_t sz_msg,		/* in : message between file & buttons */
	int options						/* in : bit mask for display options
										bit 0 : output help header if set
										bit 1 : prepare zip for big files if set
										bit 2 : output return button if set
										bit 3 : output close button if set
										*/
){
	char path[4096];
	char dir[1024] = {0};
	size_t sz_dir = 0;
	char loc[1024] = {0};
	size_t sz_loc = 0;
	struct stat fs = {0};

	/* Output help if applicable */
	if(options) DYNBUF_ADD_STR(html, "<center>");
	if(options & 1) DYNBUF_ADD_STR(html,
		"Cliquez sur le lien ci-dessous pour télécharger le fichier<br><br>"
		"<font size=-1>Si vous utilisez Internet Explorer, le fichier s'ouvrira dans une nouvelle fenêtre<br>"
		"Pour enregistrer le fichier sur votre ordinateur, cliquez avec le bouton de droite et choisissez Enregistrer la cible sous ...</font><br><br>");
	
	/* Handle directories */
	if(fdir)
	{
		if(!strcmp("#user", fdir))
		{
			sz_loc = snprintf(add_sz_str(loc), "/" DIRECTORY_DOCS "/%s/%s/", cntxt->dbname, dyntab_val(&cntxt->id_user, 0, 0));
			sz_dir = snprintf(add_sz_str(dir), "%s" DIRECTORY_DOCS "\\%s\\%s\\", cntxt->rootdir, cntxt->dbname, dyntab_val(&cntxt->id_user, 0, 0));
		}
		else if(!strcmp("#docs", fdir))
		{
			sz_loc = snprintf(add_sz_str(loc), "/" DIRECTORY_DOCS "/%s/", cntxt->dbname);
			sz_dir = snprintf(add_sz_str(dir), "%s" DIRECTORY_DOCS "\\%s\\", cntxt->rootdir, cntxt->dbname);
		}
		else
		{
			sz_loc = snprintf(add_sz_str(loc), "/%s/", fdir);
			sz_dir = snprintf(add_sz_str(dir), "%s%s\\", cntxt->rootdir, fdir);
		}
	}
	else
	{
		sz_loc = snprintf(add_sz_str(loc), "/");
		sz_dir = snprintf(add_sz_str(dir), "%s", cntxt->rootdir);
	}
	if(sz_dir) snprintf(add_sz_str(path), "%s%s", dir, fname);

	/* Output title if present */
	if(title && *title && sz_title) DYNBUF_ADD(html, title, sz_title, NO_CONV);

	/* Check file status */
	if(stat(path, &fs)) 
	{
		DYNBUF_ADD3(html, "<font color=#FF0000><b>File not found</b> - <u>", name, sz_name, TO_HTML, "</u></font>");
		RETURN_OK;
	}
	if(!fs.st_size) 
	{
		DYNBUF_ADD3(html, "<font color=#FF0000><b>Empty file</b> - <u>", name, sz_name, TO_HTML, "</u></font>");
		RETURN_OK;
	}

	/* Output link to file - always use a new window */
	DYNBUF_ADD_STR(html, "<a target=_blank");

	/* Output notes if present */
	if(notes && *notes && sz_notes)
	{
		DYNBUF_ADD3(html, " title='", notes, sz_notes, HTML_TOOLTIP, "'");
		if(put_showhelp(cntxt, html)) STACK_ERROR;
	}

	/* Output link reference & file name / size */
	DYNBUF_ADD3(html, " href='", loc, sz_loc, NO_CONV, "");
	DYNBUF_ADD(html, fname, sz_fname, HTML_NO_QUOTE);
	DYNBUF_ADD3(html, "'>", name, sz_name, TO_HTML, "</a> - ");
	DYNBUF_ADD(html, human_filesize(fs.st_size), 0, NO_CONV);


	/* Zip result file if larger than 50k & output link to zip */
	if(fs.st_size > 50000L && options & 2 && sz_fname > 4 && stricmp(fname + sz_fname - 4, ".zip"))
	{
		char zipcmd[4096] = {0};
		char cwd[4096] = {0};
		char zipfile[4096] = {0};
		size_t sz_zipfile;
		getcwd(add_sz_str(cwd));
		if(chdir_user_doc(cntxt)) STACK_ERROR;
		sz_zipfile = snprintf(add_sz_str(zipfile), "%s.zip", fname);
		snprintf(add_sz_str(zipcmd), "copy \"%s%s\" . > msg.txt 2>err.txt", dir, fname);
		system(zipcmd);
		snprintf(add_sz_str(zipcmd), "%szip.exe -j -9 tmp.zip \"%s\" > msg.txt 2>err.txt", cntxt->path, fname);
		remove("tmp.zip");
		system(zipcmd);
		remove(zipfile);
		rename("tmp.zip", zipfile);
		remove("err.txt");
		remove("msg.txt");
		if(*cwd) chdir(cwd);
		if(file_output_link(cntxt, html,
								add_sz_str("<br><font size=-1>Ficher zippé : "),
								name, sz_name, zipfile, sz_zipfile,
								"#user",
								add_sz_str("Ce fichier est compressé pour réduire le temps de transfert"),
								NULL, 0, 0)) STACK_ERROR;
		DYNBUF_ADD_STR(html, "</font>");
	}

	/* Output message */
	if(options) DYNBUF_ADD_STR(html, "<hr>");
	if(msg && sz_msg) DYNBUF_ADD3(html, "</center>", msg, sz_msg, NO_CONV, "<hr><center>");

	/* Output return & close buttons */
	if(options & 12)
	{
		DynBuffer **h = cntxt->form->html;
		cntxt->form->html = html;
		DYNBUF_ADD_STR(html, "<br>");
		if(options & 4 &&
			put_html_button(cntxt, "NOP", "Revenir à la fiche",
									"_eva_btn_gobackobj_fr.gif", "_eva_btn_gobackobj_fr_s.gif",
									"Retourne au formulaire de saisie", 0, 0)) CLEAR_ERROR;
		if((options & 12) == 12) DYNBUF_ADD_STR(html, "<img src=/img/_eva_nop.gif width=100 height=1>");
		if(options & 8 &&
			put_html_button(cntxt, "B$#.CLOSE", "Fermer", "_eva_btn_close_fr.gif", "_eva_btn_close_fr_s.gif",
							"Retourne au formulaire précédent", 0, 0)) STACK_ERROR;
		cntxt->form->html = h;
	}
	if(options) DYNBUF_ADD_STR(html, "</center>");

	RETURN_OK_CLEANUP;
}
#undef ERR_FUNCTION
#undef ERR_CLEANUP

