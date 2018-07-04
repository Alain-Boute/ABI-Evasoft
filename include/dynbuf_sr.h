/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : dynbuf_sr.c
** Description : constants definitions for search & replace functions
**      Author : Alain BOUTE
**     Created : Mar 9 2002
*********************************************************************/


/*********************************************************************
** Typedef : ReplaceTable
** Description : table of character entities
*********************************************************************/
typedef struct _ReplaceTable
{
	char *search;				/* search string */
	size_t sz_search;			/* search length */
	char *replace;				/* replace string */
	size_t sz_replace;			/* replace length */
}
	ReplaceTable;

/*********************************************************************
** Constant : plain_to_html
** Description : minimal conversion table between plain text & HTML
*********************************************************************/
extern ReplaceTable plain_to_html[];

/*********************************************************************
** Macros : FROM_HTML / TO_HTML
** Description : conversion specifications between plain text & HTML
*********************************************************************/
#define TO_HTML plain_to_html, 1
#define FROM_HTML plain_to_html, -1

/*********************************************************************
** Constant : plain_to_xml
** Description : minimal conversion table between plain text & XML
*********************************************************************/
extern ReplaceTable plain_to_xml[];

/*********************************************************************
** Macros : FROM_XML / TO_XML
** Description : conversion specifications between plain text & XML
*********************************************************************/
#define TO_XML plain_to_xml, 1

/*********************************************************************
** Macros : NO_TABCR
** Description : conversion specifications between plain text & HTML
*********************************************************************/
extern ReplaceTable no_tab_cr[];
#define NO_TABCR no_tab_cr, 1

/*********************************************************************
** Constant : html_tooltip
** Description : conversion table for tooltip strings
*********************************************************************/
extern ReplaceTable html_tooltip[];

/*********************************************************************
** Macros : HTML_TOOLTIP
** Description : conversion specifications between plain text & XML
*********************************************************************/
#define HTML_TOOLTIP html_tooltip, 1

/*********************************************************************
** Constant : no_quote
** Description : conversion table for single quote
*********************************************************************/
extern ReplaceTable no_quote[];
extern ReplaceTable no_par[];

/*********************************************************************
** Macro : HTML_NO_QUOTE
** Description : conversion specification for XML with double quotes
*********************************************************************/
#define HTML_NO_QUOTE no_quote, 1
#define NO_PARAGRAPH no_par, 1

/*********************************************************************
** Macro : TO_JS_STRING
** Description : conversion for javascript strings
*********************************************************************/
extern ReplaceTable javascript_string[];
#define TO_JS_STRING javascript_string, 1

/*********************************************************************
** Macro : TO_SYSCMD
** Description : conversion table for double quoted system comnands
*********************************************************************/
extern ReplaceTable syscmd_string[];
#define TO_SYSCMD syscmd_string, 1

/*********************************************************************
** Constant : sql_no_quote
** Description : conversion table for SQL quote & backslash
*********************************************************************/
extern ReplaceTable sql_no_quote[];

/*********************************************************************
** Macro : SQL_NO_QUOTE
** Description : conversion specification for SQL quote & backslash
*********************************************************************/
#define SQL_NO_QUOTE sql_no_quote, 1

/*********************************************************************
** Constant : sql_no_like_ops
** Description : conversion table for SQL quote & backslash
*********************************************************************/
extern ReplaceTable sql_no_like_ops[];

/*********************************************************************
** Macro : SQL_NO_LIKE_OPS
** Description : conversion specification for SQL quote & backslash
*********************************************************************/
#define SQL_NO_LIKE_OPS sql_no_like_ops, 1

/*********************************************************************
** Constant : export_tabrc
** Description : conversion table for TAB/RC files
*********************************************************************/
extern ReplaceTable export_tabrc[];

/*********************************************************************
** Macros : EXPORT_TABRC / IMPORT_TABRC
** Description : conversion specifications for TAB/RC files
*********************************************************************/
#define EXPORT_TABRC export_tabrc, 1
#define IMPORT_TABRC export_tabrc, -1

/*********************************************************************
** Constant : crlf_to_lf
** Description : conversion table for CR <-> CR/LF
*********************************************************************/
extern ReplaceTable crlf_to_lf[];

/*********************************************************************
** Macros : CRLF_TO_LF / LF_TO_CRLF
** Description : conversion specifications for CR <-> CR/LF
*********************************************************************/
#define CRLF_TO_LF crlf_to_lf, 1
#define LF_TO_CRLF crlf_to_lf, -1


/*********************************************************************
** Macro : NO_CONV
** Description : conversion specification for no conversion
*********************************************************************/
#define NO_CONV NULL, 0
