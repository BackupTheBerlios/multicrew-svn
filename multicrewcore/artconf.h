/*  artCcnf
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

#if !defined(__ARTCONF_H)
#define __ARTCONF_H

#ifdef __cplusplus
extern "C" {
#endif


struct conf_insgroup {
 char *nameitem, *value;
 struct conf_insgroup *ig_next;
};

typedef struct conf_insgroup conf_sinsgroup;
typedef conf_sinsgroup *conf_pinsgroup;

struct conf_group {
 char *namegroup;
 conf_pinsgroup inside;
 struct conf_group *g_next;
};

typedef struct conf_group conf_sgroup;
typedef conf_sgroup *conf_pgroup;

struct conf_struct {
 char *namearch;
 conf_pgroup group;
 conf_pgroup actual_group;
};

typedef struct conf_struct conf_sstruct;
typedef conf_sstruct *conf_pstruct;



conf_pstruct conf_init(const char *name);
int conf_end(conf_pstruct kkstruct);
int conf_savetodisk(conf_pstruct kkstruct);

int conf_togroup(const char *name, conf_pstruct kkstruct);
char *conf_getstring(const char *nameitem, conf_pstruct kkstruct);
long conf_getnumber(const char *nameitem, conf_pstruct kkstruct);

int conf_newgroup(const char *name, conf_pstruct kkstruct);
int conf_newitem(const char *itemname, const char *value, conf_pstruct kkstruct);
int conf_newstring(const char *itemname, const char *value, conf_pstruct kkstruct);
int conf_newnumber(const char *itemname, const long thefnumber, conf_pstruct kkstruct);

int conf_delgroup(const char *name, conf_pstruct kkstruct);
int conf_delitem(const char *itemname, conf_pstruct kkstruct);

int mostrar_todo(conf_pstruct kkstruct);
char *conf_p2d(const char *strkk, const char tofind);
char *conf_getany(const char *nameitem, conf_pstruct kkstruct);


#ifdef __cplusplus
}
#endif

#endif
