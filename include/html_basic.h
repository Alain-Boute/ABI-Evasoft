/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : html_basic.h
** Description : these functions output HTML code for basic elements
**      Author : Alain BOUTE
**     Created : Feb 26 2002
**********************************************************************/

/*********************************************************************
** Function : put_html_table_header
** Description : output HTML header for a new table
*********************************************************************/
int put_html_table_header(
	EVA_context *cntxt,				/* in/out : execution context data */
	char *align,					/* in : horizontal position (center / left / right) */
	char *width,					/* in : widht in pixels or % of window width */
	char *height,					/* in : widht in pixels or % of window width */
	char *bgcolor,					/* in : background color (RGB / 6 chars hexa) */
	char *background,				/* in : background image filename (no path) */
	int cellspacing,				/* in : cell spacing in pixels */
	int cellpadding,				/* in : cell padding in pixels */
	int border,						/* in : border width in pixels */
	char *rules,					/* in : cells border drawing rule */
	char *cls,						/* in : CSS class */
	char *style						/* in : CSS style */
);

/*********************************************************************
** Function : put_html_format_pos
** Description : output HTML table line / column header & text format
*********************************************************************/
int put_html_format_pos(
	EVA_context *cntxt,				/* in/out : execution context data */
	char *position,					/* in : position in table (NewLine, NewColumn, SameCell, ...) */
	char *align,					/* in : horizontal position (center / left / right) */
	char *valign,					/* in : verical position (middle / top / left) */
	char *width,					/* in : widht in pixels or % of window width */
	char *height,					/* in : height in pixels or % window height */
	char *bgcolor,					/* in : background color (RGB / 6 chars hexa) */
	char *background,				/* in : background image filename (no path) */
	int colspan, int rowspan,		/* in : # of row & columns the cell (if any) will occupy */
	char *fontface,					/* in : font face (a font name) */
	char *fontsize,					/* in : font size may be absolute (size) or relative (+size / -size) */
	char *fontcolor,				/* in : font color (RGB / 6 chars hexa) */
	char *style,					/* in : CSS class style */
	int bold,						/* in : use bold text if 1 */
	int italic,						/* in : use italic text if 1 */
	int underline,					/* in : use underlined text if 1 */
	int nobreak,					/* in : do not break lines if 1 */
	int b_head						/* in : output header if 1, footer else */
);

/*********************************************************************
** Function : put_html_page_header
** Description : output HTML header code (direct to stdout)
*********************************************************************/
void put_html_page_header(EVA_context *cntxt, FILE *f, char *title, char *action, int steps);

/*********************************************************************
** Function : put_html_page_trailer
** Description : output signup & HTML footer code (direct to stdout)
*********************************************************************/
void put_html_page_trailer(EVA_context *cntxt, FILE *f);

/*********************************************************************
** Function : put_html_bold_substring
** Description : output text with the given substring bold
*********************************************************************/
int put_html_bold_substring(
	EVA_context *cntxt,					/* in/out : execution context data */
	DynBuffer **html,					/* in/out : buffer to add text to */
	char *txtout,	 					/* in : text to output to buffer */
	size_t sz,							/* string length of txtout, or 0 if unknown */
	DynTable *substr					/* in : list of substrings to output as bold text */
);
