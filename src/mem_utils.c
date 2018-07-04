/*********************************************************************
** ---------------------- Copyright notice ---------------------------
** This source code is part of the EVASoft project
** It is property of Alain Boute Ingenierie - www.abing.fr and is
** distributed under the GNU Public Licence version 2
** Commercial use is submited to licencing - contact eva@abing.fr
** -------------------------------------------------------------------
**        File : mem_utils.c
** Description : memory (malloc/free) handling utility functions
**      Author : Alain BOUTE
**     Created : Jan 3 2003
*********************************************************************/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>

/*********************************************************************
** Function : mem_trace
** Description : malloc / free trace utility
*********************************************************************/
#define MEM_TRACE(type, mem) /* mem_trace(type, mem) */
void mem_trace(
	char *type,					/* in : free / alloc or NULL for initialisation */
	void *mem					/* in : alloc-ed pointer */
){
	FILE *f;
	static clock_t t0;
	char *path = "D:\\Travail\\ABI\\ApacheWeb\\cgi\\memtrace.txt";


	if(type && !mem) return;
	if(!type)
	{
		t0 = clock(); 
		f = fopen(path, "wc");
		fprintf(f, "Clock\tType\tAdress\tSize\n");
	}
	else
	{
		f = fopen(path,"a");
		fprintf(f, "%lu\t%s\t0x%lx\t0x%lx\n", clock() - t0, type, mem, _msize(mem));
	}
	fclose(f);
}

/*********************************************************************
** Function : mem_alloc
** Description : malloc wrapper
*********************************************************************/
void *mem_alloc(				/* return : realloc(src, new_sz) */
	size_t sz					/* in : size increment */
){
	void *res;
	res = malloc(sz);
	if(res) memset(res, 0, sz);
	MEM_TRACE("alloc", res);
	return res;
}

/*********************************************************************
** Function : mem_strdup
** Description : duplicate a string
*********************************************************************/
char *mem_strdup(				/* return : duplicated (malloc) string */
	char *src					/* in : string to duplicate */
){
	size_t sz = src ? strlen(src) + 1 : 0;
	void *res = sz ? mem_alloc(sz) : NULL;
	if(res) memcpy(res, src, sz);
	return res;
}

/*********************************************************************
** Function : mem_free
** Description : free wrapper
*********************************************************************/
void mem_free(
	void *src					/* in : old pointer */
){
	MEM_TRACE("free", src);
	free(src);
}

/*********************************************************************
** Function : mem_realloc
** Description : realloc wrapper
*********************************************************************/
void *mem_realloc(				/* return : realloc(src, new_sz) */
	void *src,					/* in : old pointer */
	size_t old_sz,				/* in : old size */
	size_t incr					/* in : size increment */
){
	void *res;
	MEM_TRACE("free", src);
	res = realloc(src, old_sz + incr);
	MEM_TRACE("alloc", res);
	if(incr > 0) memset((char *)res + old_sz, 0, incr);
	return res;
}
