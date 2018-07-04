/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : html_table.h
** Description : these functions output HTML code for formatted tables
**      Author : Alain BOUTE
**     Created : Feb 26 2002
**********************************************************************/



/*********************************************************************
** Constants group : TblCtrl_...
** Description : bitmasks used to specify controls displayed in a table
*********************************************************************/
#define TblCtrl_sort		0x1			/* Sort arrows */
#define TblCtrl_title		0x2			/* Title row (1st row) */
#define TblCtrl_search		0x10
#define TblCtrl_opensearch	0x20
#define TblCtrl_addnew		0x40
#define TblCtrl_openaddnew	0x80
#define TblCtrl_sel			0x100
#define TblCtrl_opensel		0x200
#define TblCtrl_ctrlsel		0x400
#define TblCtrl_openbtn		0x800
#define TblCtrl_printmode	0x1000
#define TblCtrl_extsearch	0x2000

/*********************************************************************
** Function : table_free
** Description : free ObjTableFormat structure data
*********************************************************************/
void table_free(ObjTableFormat *tbl);

/*********************************************************************
** Function : table_read_controls
** Description : read & intialize data for list & relations tables
*********************************************************************/
int table_read_controls(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	DynTable *data						/* in : table format data (from a list control) */
);

/*********************************************************************
** Function : table_process_controls
** Description : process button clicks & control inputs for list & relations tables
*********************************************************************/
int table_process_controls(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : table_export_list
** Description : export a table of objects
*********************************************************************/
int table_export_list(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : table_init_obj_list
** Description : read unsorted objects list for a table of objects
*********************************************************************/
int table_init_obj_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int *selobj							/* out : table displays control values if not 0 */
);

/*********************************************************************
** Function : table_sort_obj_list
** Description : read unsorted objects list for a table of objects
*********************************************************************/
int table_sort_obj_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int selobj							/* out : table displays control values if not 0 */
);

/*********************************************************************
** Function : table_read_obj_list
** Description : read table list of of objects
*********************************************************************/
int table_read_obj_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int b_selobj						/* in : use control values if not 0 */
);

/*********************************************************************
** Function : table_row_bgcolor
** Description : return row background color for a given row
*********************************************************************/
char *table_row_bgcolor(				/* return : RGB color code - empty string for transparent */
	EVA_context *cntxt,					/* in/out : execution context data */
	ObjTableFormat *tbl,				/* in : table to output */
	unsigned long i,					/* in : row index to get color for */
	char *colorfield					/* in : color expression */
);

/*********************************************************************
** Function : table_prepare_rows
** Description : prepare cell values for a table of objects
*********************************************************************/
int table_prepare_rows(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char *pfx							/* in : values prefix */
);

/*********************************************************************
** Function : table_word_search
** Description : return a list of objects matching control filter & a search string
*********************************************************************/
int table_word_search(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : table_put_openclose_btn
** Description : output open / close button
*********************************************************************/
int table_put_openclose_btn(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : table_put_closesearch_btn
** Description : output close search button
*********************************************************************/
int table_put_closesearch_btn(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : table_put_opensearch_btn
** Description : output open search button
*********************************************************************/
int table_put_opensearch_btn(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char *tooltip							/* in : button tooltip */
);

/*********************************************************************
** Function : table_put_addnew_btn
** Description : output insert button
*********************************************************************/
int table_put_addnew_btn(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char *bef, size_t bef_sz,			/* in : insert this text before button */
	char *aft, size_t aft_sz,			/* in : insert this text after button */
	int mode							/* in : button output mode */
);

/*********************************************************************
** Function : table_put_functions_btn
** Description : output open new object button
*********************************************************************/
int table_put_functions_btn(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	char *separ, size_t separ_sz		/* in : buttons separator */
);

/*********************************************************************
** Function : table_put_clear_btn
** Description : output clear values button
*********************************************************************/
int table_put_clear_btn(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : table_put_search_input
** Description : output text input for search & new object
*********************************************************************/
int table_put_search_input(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int *b_label						/* in : output label if not 0
										  out : 1 if HTML input was output */
);

/*********************************************************************
** Function : table_put_filter_list
** Description : output extra filters list
*********************************************************************/
int table_put_filter_list(				/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int *b_label						/* in : output label if not 0
										  out : 1 if HTML input was output */
);

/*********************************************************************
** Function : table_put_page_navigator
** Description : output a table's page navigation buttons
*********************************************************************/
int table_put_page_navigator(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl,				/* in : control index in cntxt->form->ctrl */
	int options							/* in : display options - bit mask
												bit 0 : use border around controls */
);

/*********************************************************************
** Function : table_put_status
** Description : output table status as hidden input
*********************************************************************/
int table_put_status(					/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);

/*********************************************************************
** Function : table_put_html_obj_list
** Description : output a HTML table of objects	(Ids are in IdList table)
*********************************************************************/
int table_put_html_obj_list(			/* return : 0 on success, other on error */
	EVA_context *cntxt,					/* in/out : execution context data */
	unsigned long i_ctrl				/* in : control index in cntxt->form->ctrl */
);
