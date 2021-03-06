/*
 *  CUPS add-on module for Canon Inkjet Printer.
 *  Copyright CANON INC. 2001-2021
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "com_def.h"
#include "paramlist.h"

#ifdef __x86_64__
		__asm__(".symver memcpy, memcpy@GLIBC_2.2.5");
#endif

ParamList *param_list_find(ParamList *pl, const char *key)
{
	ParamList *curs;

	for( curs = pl ; curs != NULL ; curs = curs->next )
	{
		if( !strcmp(curs->key, key) )
			return curs;
	}
	return NULL;
}

void param_list_delete(ParamList **root, const char *key)
{
	ParamList *curs;
	ParamList *prev = NULL;

	for( curs = *root ; curs != NULL ; prev = curs, curs = curs->next )
	{
		if( !strcmp(curs->key, key) )
		{
			if( prev != NULL )
				prev->next = curs->next;
			else
				*root = curs->next;

			free(curs->key);
			free(curs->value);
			free(curs);
			break;
		}
	}
}

void param_list_add_multi(ParamList **root,
		const char *key, const char *value, int value_size, int multi_key)
{

	if(multi_key || param_list_find(*root, key) == NULL){
		ParamList *pl = (ParamList*)malloc(sizeof(ParamList));
		int key_len = strlen(key);

		param_list_delete(root, key);

		pl->next = *root;
		pl->key = malloc(key_len + 1);
		memcpy(pl->key, key, key_len + 1);

		pl->value = malloc(value_size);
		memcpy(pl->value, value, value_size);
		pl->value_size = value_size;
		*root = pl;
	}

}

void param_list_free(ParamList *pl)
{
	ParamList *next;

	for( ; pl != NULL ; pl = next )
	{
		next = pl->next;

		free(pl->key);
		free(pl->value);
		free(pl);
	}
}

int param_list_num(ParamList *pl)
{
	int num = 0;

	for( ; pl != NULL ; pl = pl->next )
		num++;
	return num;
}

void param_list_print(ParamList *pl)
{
	FILE *fp = fopen("/tmp/ijexec.log", "a");

	if( fp )
	{
		fprintf(fp, "---- param_list_print ----\n");
		for( ; pl != NULL ; pl = pl->next )
		{
			char value_buf[MAX_VALUE_LEN + 1];
			memcpy(value_buf, pl->value, pl->value_size);
			value_buf[pl->value_size] = 0;
			fprintf(fp, "key=%s, value=%s\n", pl->key, value_buf);
		}
		fclose(fp);
	}
}

char *ref_value_from_list( ParamList *root, const char *key )
{
	ParamList *curs;
	char *result = NULL;

	for( curs = root ; curs != NULL ; curs = curs->next ) {
		DEBUG_PRINT2( "DEBUG:[rastertocanonij(paramlist.c)] key : %s\n", curs->key );
		if( !strcmp(curs->key, key) ) {
			result = curs->value;
			break;
		}
	}

	return result;
}
