/*  artConf
 *  Copyright (C) 2000 Ramiro Tasquer (tasquer@zuper.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "artconf.h"

enum ar_boolean { TRUE, FALSE };

int conf_savetodisk(conf_pstruct kkstruct)
{
	conf_pgroup gr_temp;
	conf_pinsgroup gri_temp;
	FILE *archive;
	archive = fopen(kkstruct->namearch, "w+");
	if( archive==0 ) return -1;
	gr_temp = kkstruct->group;
	while (gr_temp)
	{
		fprintf(archive,"[%s]",gr_temp->namegroup);
		gri_temp = gr_temp->inside;
		while (gri_temp)
		{
			fprintf(archive,"\n%s=%s",gri_temp->nameitem, gri_temp->value);
			gri_temp = gri_temp -> ig_next;
		}
		fprintf(archive,"\n\n");
		gr_temp = gr_temp -> g_next;
	}
	fclose(archive);
	return 0;
}

int conf_end(conf_pstruct kkstruct)
{
	conf_pgroup gr_temp, gr_temp1;
	conf_pinsgroup gri_temp, gri_temp1;
	gr_temp = kkstruct->group;
	while (gr_temp)
	{
		gr_temp1 = gr_temp -> g_next;
		gri_temp = gr_temp -> inside;
		while (gri_temp)
		{
			gri_temp1 = gri_temp -> ig_next;
			free(gri_temp -> nameitem);
			free(gri_temp -> value);
			free(gri_temp);
			gri_temp = gri_temp1;
		}
		free(gr_temp->namegroup);
		free(gr_temp);
		gr_temp = gr_temp1;
/*   conf_delgroup(gr_temp -> namegroup, kkstruct);
	 gr_temp = gr_temp -> g_next;*/
	}
	kkstruct->group = NULL;
	free(kkstruct->namearch);
	free(kkstruct);
	return 0;
}

int conf_delitem(const char *itemname, conf_pstruct kkstruct)
{
	conf_pinsgroup gri_temp, gri_temp1; 
	if (kkstruct -> actual_group)
		if ((gri_temp = kkstruct -> actual_group -> inside))
		{
			gri_temp1 = gri_temp;
			while (gri_temp)
			{
				if (stricmp(gri_temp -> nameitem, itemname) == 0)
				{
					if (gri_temp1 == gri_temp)
						kkstruct -> actual_group -> inside = gri_temp -> ig_next;
					else
						gri_temp1 -> ig_next = gri_temp -> ig_next;

					free(gri_temp -> nameitem);
					free(gri_temp -> value);
					free(gri_temp); 
					return 1;
				} /* if */
				gri_temp1 = gri_temp;
				gri_temp = gri_temp -> ig_next;
			} /* while */
		} /* if inside is not null */
	return 0;
}

int conf_delgroup(const char *name, conf_pstruct kkstruct)
{
	conf_pgroup gr_temp, gr1_temp; 
	conf_pinsgroup gri_temp, gri_temp1; 
	int ebool = FALSE;
	gr_temp = kkstruct->group;
	gr1_temp = gr_temp;
	while ((gr_temp)&&(ebool))
	{
		if (stricmp(gr_temp -> namegroup, name)==0)
		{ ebool = TRUE; }
		if (ebool) 
		{
			gr1_temp = gr_temp;
			gr_temp = gr_temp-> g_next;
		}
	}
	if (!ebool)
	{
		if (gr1_temp != gr_temp)
			gr1_temp -> g_next = gr_temp -> g_next; /* if its not the first one */
		else 
			kkstruct -> group = gr_temp -> g_next; /* if its the first one */
    
		if (gr_temp == kkstruct -> actual_group)
			kkstruct -> actual_group =  kkstruct->group; /* if you are erasing the actual group */

		gri_temp = gr_temp -> inside;
		while (gri_temp)
		{
			gri_temp1 = gri_temp -> ig_next;
			free(gri_temp -> nameitem);
			free(gri_temp -> value);
			free(gri_temp);
			gri_temp = gri_temp1;
		}
		free(gr_temp -> namegroup);
		free(gr_temp);
		return 0;
	}
	return 1;
}

int conf_newstring(const char *itemname, const char *value, conf_pstruct kkstruct)
{
	char str_ter[255];
	int ret;
	strcpy(str_ter, "\"");
	strcat(str_ter, value);
	strcat(str_ter, "\"");
	ret = conf_newitem(itemname, str_ter, kkstruct);
	return ret;
}

int conf_newnumber(const char *itemname, const long thefnumber, conf_pstruct kkstruct)
{
	char stch[255];
	sprintf(stch, "%lu", thefnumber);
	return conf_newitem(itemname, stch, kkstruct);
}

int conf_newitem(const char *itemname, const char *value, conf_pstruct kkstruct)
{
	conf_pinsgroup gri_temp, gri_temp1;
	int ebool = FALSE, tbool = FALSE, nochep = 0;

	if (kkstruct->actual_group) 
	{
		if (kkstruct->actual_group->inside)
		{
			gri_temp1 = kkstruct->actual_group->inside;
			do {
				if (stricmp(gri_temp1 -> nameitem, itemname)==0)
				{ ebool = tbool = TRUE; } /* if its the same item name */
				else
				{
					if (gri_temp1->ig_next!=NULL)
						gri_temp1 = gri_temp1 -> ig_next;
					else
						ebool = TRUE; /* the end of the search */
				} /* if not the same itemname */
			} while (ebool);
			if (!tbool)
			{
				free(gri_temp1->value);
				gri_temp1->value = strdup(value);
				return 1;
			} /* will be a modification */
			else
				nochep = 1; /* a new item */
		} /* not the first time :P */
		else
			nochep = 2; /* the first time */

		gri_temp = (conf_pinsgroup) malloc (sizeof(conf_sinsgroup));
		gri_temp -> nameitem = strdup(itemname);
		gri_temp -> value = strdup(value);
		gri_temp -> ig_next = NULL;
		if (nochep == 1)
			gri_temp1 -> ig_next = gri_temp; /* a new item */
		else 
			kkstruct->actual_group->inside = gri_temp; /* the first time */
		return 1;
	} /* this mean, we already have the group */
	return 0;
}

int conf_newgroup(const char *name, conf_pstruct kkstruct)
{
	conf_pgroup gr_temp,gr_temp1;
	if (!conf_togroup(name, kkstruct))
	{
		if ((gr_temp = (conf_pgroup) malloc (sizeof(conf_sgroup))) == NULL)
		{
			printf("Your memory is K.O., you lose, I WIN! :P (Loser)");
			return 0;
		}
		gr_temp -> namegroup = strdup(name);
		gr_temp -> inside = NULL;
		gr_temp -> g_next = NULL;
		if (kkstruct->group) /* kkstruct->group!=NULL */
		{
			gr_temp1 = kkstruct->group;
			while (gr_temp1->g_next)
				gr_temp1 = gr_temp1->g_next;
			gr_temp1->g_next = gr_temp;
			kkstruct->actual_group = gr_temp;
		} /* not the first group */
		else
		{
			kkstruct->group = gr_temp;
			kkstruct->actual_group = gr_temp;
		} /* the first group ever */  
	}
	return 1;
}

long conf_getnumber(const char *nameitem, conf_pstruct kkstruct)
{
	char *kk;
	if ((kk = conf_getany(nameitem, kkstruct)))
	{
		return atol(kk);
	}
	return 1;
}

char *conf_getstring(const char *nameitem, conf_pstruct kkstruct)
{
	char *kk, str[255];
	int cont;
	if ((kk = conf_getany(nameitem, kkstruct)))
	{
		cont = strlen(kk);
		memcpy(&str, kk+1, cont-2);
		str[cont-2]='\0';
		free(kk);
		return (char *) strdup (str);
	}
	return NULL;
}

char *conf_getany(const char *nameitem, conf_pstruct kkstruct)
{
	conf_pgroup gr_temp; 
	conf_pinsgroup gri_temp;

	if ((gr_temp = kkstruct -> actual_group))
	{
		gri_temp = gr_temp -> inside;
		while (gri_temp)
		{
			if (stricmp(gri_temp -> nameitem, nameitem)==0)
				return (char *) strdup(gri_temp ->value); /* to check if its the item :P */
			gri_temp = gri_temp -> ig_next;
		} /*  while (gri_temp!=NULL) */
	} /* to check if we already have at least one group */
	return NULL;
}

int conf_togroup(const char *name, conf_pstruct kkstruct)
{
	conf_pgroup gr_temp;
	gr_temp = kkstruct -> group;
	while (gr_temp)
	{
		if (stricmp(gr_temp -> namegroup, name)==0)
		{
			kkstruct -> actual_group = gr_temp;
			return 1;
		}
		gr_temp = gr_temp -> g_next;
	}
	return 0;
}

int mostrar_todo(conf_pstruct kkstruct)
{
	conf_pgroup gr_temp;
	conf_pinsgroup gri_temp;
	gr_temp = kkstruct->group;
	while (gr_temp)
	{
		printf("\n[%s]",gr_temp->namegroup);
		gri_temp = gr_temp->inside;
		while (gri_temp)
		{
			printf("\n %s=%s",gri_temp->nameitem, gri_temp->value);
			gri_temp = gri_temp -> ig_next;
		}
		gr_temp = gr_temp -> g_next;
	}
	return 0;
}

conf_pstruct conf_init(const char *name)
{
	conf_pstruct temp;
	conf_pgroup gr_temp, gr_temp1;
	conf_pinsgroup gri_temp, gri_temp1;
	int pos_str, tbool = FALSE, obool = FALSE, temp_str;
	int commentStarted = FALSE;
	char stch[255], *str_ret;
	FILE *archive;
	if ((temp = (conf_pstruct) malloc (sizeof(conf_sstruct))) == NULL)
	{
		printf("\nShit&Fuck! you dont have enought memory.. hey, i wont lend you some");
		return NULL;
	}
	temp->namearch = (char *) strdup(name);
	if ((archive = fopen(name, "r+")) == NULL)
	{
		archive = fopen(name, "w+");
	}
	temp->group = temp->actual_group = NULL;
	if( archive==NULL ) return temp;
	
	while(tbool)
	{
		pos_str = 0;  obool = FALSE;  commentStarted = FALSE;
		do {
			stch[pos_str] = fgetc(archive);
			if (stch[pos_str]=='/' && commentStarted==FALSE ) {
				commentStarted = TRUE;
				pos_str++;
			} else {
				if (stch[pos_str]==EOF)
				{ obool = tbool = TRUE; }
				if ((stch[pos_str]=='\n')||(pos_str>254))
					obool = TRUE;
				else if (stch[pos_str]=='/' && commentStarted==TRUE ) {
					while( fgetc(archive)!='\n' );
					pos_str--;
					obool = TRUE;
				} else
					pos_str++;
				
				commentStarted = FALSE;
			}
		} while (obool);

		while( pos_str>0 && stch[pos_str-1]==' ' ) pos_str--;

		if (pos_str == 255)
			printf("\nhey.. you just blow my string off.. snif.. long live to the char[255]");
/* if (stch[pos_str]==EOF)
   tbool = FALSE;*/
		if ((tbool)&&(pos_str<255))
		{
/*   pos_str = pos_str - 2;
     stch[pos_str+1]='\0';*/
			if ((stch[0]=='[')&&(stch[pos_str-1]==']'))
			{
				pos_str=0;
				while (stch[++pos_str]!=']')
					stch[pos_str-1]=stch[pos_str];
				stch[--pos_str]='\0';	
				gr_temp = (conf_pgroup) malloc (sizeof(conf_sgroup));
				gr_temp -> namegroup = (char *) strdup (stch);
				gr_temp -> inside = NULL;
				gr_temp -> g_next = NULL;
				if (temp->group == NULL)
				{ 
					temp->group = gr_temp; 
					temp->actual_group = gr_temp;
				} /* first time */
				else
					gr_temp1 -> g_next = gr_temp; /* not the first time.. :P */
				gr_temp1 = gr_temp;
			} /* if its a group */
			else
			{
				if (temp->group!=NULL)
				{ 
					stch[pos_str]='\0';
					/* check for spaces */
					temp_str = 0; pos_str=0;
					while(stch[temp_str++]==' '); temp_str--; /* make a function of this */
					while((stch[pos_str++]=stch[temp_str++])); /* change to memcpy */
					stch[pos_str+1]='\0';
					/* watch for a =, and if we find it, thats mean its a configuration item*/
					str_ret = conf_p2d(stch, '=');
					if (str_ret!=NULL)
					{
						pos_str = (strlen(str_ret)) + 1; temp_str = 0;
						/* now, copy the value part */
						while ((stch[temp_str++]=stch[pos_str++]));
						/* check for spaces in the value part */
						temp_str = 0; pos_str = 0;
						while(stch[temp_str++]==' '); temp_str--; /* make a function of this */
						while((stch[pos_str++]=stch[temp_str++])); /* change to memcpy */
						/* create the inside group */
						gri_temp = (conf_pinsgroup) malloc (sizeof(conf_sinsgroup));
						gri_temp->nameitem = str_ret;
						gri_temp->value = (char *) strdup(stch);
						gri_temp->ig_next = NULL;
						if ((gr_temp->inside)) /* gr_temp->inside != NULL */
							gri_temp1->ig_next = gri_temp;  /* not the first time.. damn it, i'm the whowilleverknow */
						else
							gr_temp->inside = gri_temp;  /* first time.. jeje */
						gri_temp1 = gri_temp;
					} /* if the string is part of a configuration */
				} /* if we already have at least one group */
			} /* else of '[' ']', if there is no group found in stch */
		} /* if tbool true and length of string is less than 255 */
	} /* while tbool not true*/
	fclose(archive);
	return temp;
}

char *conf_p2d(const char *strkk, const char tofind)
/* TODO : use strchr cuz' its so cool&hot... i can imagine myself using strchr
   WOW!! i can't wait.... mummy, can i use strchr now?
*/
{
	int cont=0;
	char str[255];
	while ((strkk[cont++]!='\0')&&(strkk[cont] != tofind));
	if (strkk[cont-1] == '\0')
		return NULL;
	else
	{
		memcpy(&str, strkk, cont);
		str[cont]='\0';
		return (char *) strdup (str);
	}
}
