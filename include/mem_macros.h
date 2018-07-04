/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : mem_macros.h
** Description : memory (malloc/free) handling utility macros
**      Author : Alain BOUTE
**     Created : Jan 3 2003
*********************************************************************/


/*********************************************************************
** Macro : MEM_TRACE
** Description : malloc / free trace utility
*********************************************************************/
#define MEM_TRACE(type, mem) /* mem_trace(type, mem) */

/*********************************************************************
** Macro : M_STRDUP
** Description : duplicate string with error handling
*********************************************************************/
#define M_STRDUP(dest, _src, sz) { size_t size = sz; char *src = _src;\
	M_ALLOC(dest, size + 1); if(src && size) memcpy(dest, src, size); }

/*********************************************************************
** Macro : M_ALLOC
** Description : malloc with error handling
*********************************************************************/
#define M_ALLOC(data, sz) {size_t size1 = sz;\
	data = mem_alloc(size1); \
	if(size1 && !data) RETURN_ERR_MEMORY(NULL); }

/*********************************************************************
** Macro : C_ALLOC
** Description : calloc with error handling
*********************************************************************/
#define C_ALLOC(data, size, num) {\
	data = calloc(size, num); \
	if(!data) RETURN_ERR_MEMORY(NULL); }

/*********************************************************************
** Macro : M_REALLOC
** Description : increment alloc-ed space of 1 element with error handling
*********************************************************************/
#define M_REALLOC(type, dest, nb) { \
	type *newdest = mem_realloc(dest, sizeof(type)*nb, sizeof(type)*2); \
	if(!newdest) RETURN_ERR_MEMORY(NULL); \
	dest = newdest; (nb)++; }

/*********************************************************************
** Macro : M_FREE
** Description : wrapper for free
*********************************************************************/
#define M_FREE(data)	{ mem_free(data); data = 0; }
