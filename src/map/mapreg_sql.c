/**
 * This file is part of Hercules.
 * http://herc.ws - http://github.com/HerculesWS/Hercules
 *
 * Copyright (C) 2012-2015  Hercules Dev Team
 * Copyright (C)  Athena Dev Teams
 *
 * Hercules is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "stdafx.h"

CMapreg mapreg_s;
CMapreg *mapreg;

#define MAPREG_AUTOSAVE_INTERVAL (300*1000)

/**
 * Looks up the value of an integer variable using its uid.
 *
 * @param uid variable's unique identifier.
 * @return variable's integer value
 */
int CMapreg::readreg(int64 uid) {
	struct mapreg_save *m = (struct mapreg_save *) i64db_get(mapreg->regs.vars, uid);
	return m?m->u.i:0;
}

/**
 * Looks up the value of a string variable using its uid.
 *
 * @param uid variable's unique identifier
 * @return variable's string value
 */
char* CMapreg::readregstr(int64 uid) {
	struct mapreg_save *m = (struct mapreg_save *) i64db_get(mapreg->regs.vars, uid);
	return m?m->u.str:NULL;
}

/**
 * Modifies the value of an integer variable.
 *
 * @param uid variable's unique identifier
 * @param val new value
 * @retval true value was successfully set
 */
bool CMapreg::setreg(int64 uid, int val) {
	struct mapreg_save *m;
	int num = script_getvarid(uid);
	unsigned int i = script_getvaridx(uid);
	const char* name = script->get_str(num);

	if( val != 0 ) {
		if( (m = (struct mapreg_save *)i64db_get(mapreg->regs.vars, uid)) ) {
			m->u.i = val;
			if(name[1] != '@') {
				m->save = true;
				mapreg->dirty = true;
			}
		} else {
			if( i )
				script->array_update(&mapreg->regs, uid, false);

			m = ers_alloc(mapreg->ers, struct mapreg_save);

			m->u.i = val;
			m->uid = uid;
			m->save = false;
			m->is_string = false;

			if (name[1] != '@' && !mapreg->skip_insert) {// write new variable to database
				char tmp_str[(SCRIPT_VARNAME_LENGTH+1)*2+1];
				SQL->EscapeStringLen(map->mysql_handle, tmp_str, name, strnlen(name, SCRIPT_VARNAME_LENGTH+1));
				if( SQL_ERROR == SQL->Query(map->mysql_handle, "INSERT INTO `%s`(`varname`,`index`,`value`) VALUES ('%s','%d','%d')", mapreg->table, tmp_str, i, val) )
					Sql_ShowDebug(map->mysql_handle);
			}
			i64db_put(mapreg->regs.vars, uid, m);
		}
	} else { // val == 0
		if( i )
			script->array_update(&mapreg->regs, uid, true);
		if( (m = (struct mapreg_save *) i64db_get(mapreg->regs.vars, uid)) ) {
			ers_free(mapreg->ers, m);
		}
		i64db_remove(mapreg->regs.vars, uid);

		if( name[1] != '@' ) {// Remove from database because it is unused.
			if( SQL_ERROR == SQL->Query(map->mysql_handle, "DELETE FROM `%s` WHERE `varname`='%s' AND `index`='%d'", mapreg->table, name, i) )
				Sql_ShowDebug(map->mysql_handle);
		}
	}

	return true;
}

/**
 * Modifies the value of a string variable.
 *
 * @param uid variable's unique identifier
 * @param str new value
 * @retval true value was successfully set
 */
bool CMapreg::setregstr(int64 uid, const char* str) {
	struct mapreg_save *m;
	int num = script_getvarid(uid);
	unsigned int i   = script_getvaridx(uid);
	const char* name = script->get_str(num);

	if( str == NULL || *str == 0 ) {
		if( i )
			script->array_update(&mapreg->regs, uid, true);
		if(name[1] != '@') {
			if( SQL_ERROR == SQL->Query(map->mysql_handle, "DELETE FROM `%s` WHERE `varname`='%s' AND `index`='%d'", mapreg->table, name, i) )
				Sql_ShowDebug(map->mysql_handle);
		}
		if( (m = (struct mapreg_save *)i64db_get(mapreg->regs.vars, uid)) ) {
			if( m->u.str != NULL )
				aFree(m->u.str);
			ers_free(mapreg->ers, m);
		}
		i64db_remove(mapreg->regs.vars, uid);
	} else {
		if( (m = (struct mapreg_save *)i64db_get(mapreg->regs.vars, uid)) ) {
			if( m->u.str != NULL )
				aFree(m->u.str);
			m->u.str = aStrdup(str);
			if(name[1] != '@') {
				mapreg->dirty = true;
				m->save = true;
			}
		} else {
			if( i )
				script->array_update(&mapreg->regs, uid, false);

			m = ers_alloc(mapreg->ers, struct mapreg_save);

			m->uid = uid;
			m->u.str = aStrdup(str);
			m->save = false;
			m->is_string = true;

			if(name[1] != '@' && !mapreg->skip_insert) { //put returned null, so we must insert.
				char tmp_str[(SCRIPT_VARNAME_LENGTH+1)*2+1];
				char tmp_str2[255*2+1];
				SQL->EscapeStringLen(map->mysql_handle, tmp_str, name, strnlen(name, SCRIPT_VARNAME_LENGTH+1));
				SQL->EscapeStringLen(map->mysql_handle, tmp_str2, str, strnlen(str, 255));
				if( SQL_ERROR == SQL->Query(map->mysql_handle, "INSERT INTO `%s`(`varname`,`index`,`value`) VALUES ('%s','%d','%s')", mapreg->table, tmp_str, i, tmp_str2) )
					Sql_ShowDebug(map->mysql_handle);
			}
			i64db_put(mapreg->regs.vars, uid, m);
		}
	}

	return true;
}

/**
 * Loads permanent variables from database.
 */
void CMapreg::load(void) {
	/*
	        0        1       2
	   +-------------------------+
	   | varname | index | value |
	   +-------------------------+
	                                */
	SqlStmt* stmt = SQL->StmtMalloc(map->mysql_handle);
	char varname[SCRIPT_VARNAME_LENGTH+1];
	int index;
	char value[255+1];
	uint32 length;

	if ( SQL_ERROR == SQL->StmtPrepare(stmt, "SELECT `varname`, `index`, `value` FROM `%s`", mapreg->table)
	  || SQL_ERROR == SQL->StmtExecute(stmt)
	  ) {
		SqlStmt_ShowDebug(stmt);
		SQL->StmtFree(stmt);
		return;
	}

	mapreg->skip_insert = true;

	SQL->StmtBindColumn(stmt, 0, SQLDT_STRING, &varname[0], sizeof(varname), &length, NULL);
	SQL->StmtBindColumn(stmt, 1, SQLDT_INT, &index, 0, NULL, NULL);
	SQL->StmtBindColumn(stmt, 2, SQLDT_STRING, &value[0], sizeof(value), NULL, NULL);

	while ( SQL_SUCCESS == SQL->StmtNextRow(stmt) ) {
		int s = script->add_str(varname);
		int i = index;


		if( i64db_exists(mapreg->regs.vars, reference_uid(s, i)) ) {
			ShowWarning("load_mapreg: duplicate! '%s' => '%s' skipping...\n",varname,value);
			continue;
		}
		if( varname[length-1] == '$' ) {
			mapreg->setregstr(reference_uid(s, i),value);
		} else {
			mapreg->setreg(reference_uid(s, i),atoi(value));
		}
	}

	SQL->StmtFree(stmt);

	mapreg->skip_insert = false;

	mapreg->dirty = false;
}

/**
 * Saves permanent variables to database.
 */
void CMapreg::save(void) {
	if (mapreg->dirty) {
		DBIterator *iter = db_iterator(mapreg->regs.vars);
		struct mapreg_save *m;
		for (m = (struct mapreg_save *)dbi_first(iter); dbi_exists(iter); m = (struct mapreg_save *)dbi_next(iter)) {
			if (m->save) {
				int num = script_getvarid(m->uid);
				int i   = script_getvaridx(m->uid);
				const char* name = script->get_str(num);
				if (!m->is_string) {
					if( SQL_ERROR == SQL->Query(map->mysql_handle, "UPDATE `%s` SET `value`='%d' WHERE `varname`='%s' AND `index`='%d' LIMIT 1", mapreg->table, m->u.i, name, i) )
						Sql_ShowDebug(map->mysql_handle);
				} else {
					char tmp_str2[2*255+1];
					SQL->EscapeStringLen(map->mysql_handle, tmp_str2, m->u.str, safestrnlen(m->u.str, 255));
					if( SQL_ERROR == SQL->Query(map->mysql_handle, "UPDATE `%s` SET `value`='%s' WHERE `varname`='%s' AND `index`='%d' LIMIT 1", mapreg->table, tmp_str2, name, i) )
						Sql_ShowDebug(map->mysql_handle);
				}
				m->save = false;
			}
		}
		dbi_destroy(iter);
		mapreg->dirty = false;
	}
}

/**
 * Timer event to auto-save permanent variables.
 *
 * @see timer->do_timer
 */
int CMapreg::save_timer(int tid, int64 tick, int id, intptr_t data) {
	mapreg->save();
	return 0;
}

/**
 * Destroys a mapreg_save structure, freeing the contained string, if any.
 *
 * @see DBApply
 */
int CMapreg::destroyreg(DBKey key, DBData *data, va_list ap) {
	struct mapreg_save *m = NULL;

	if (data->type != DB_DATA_PTR) // Sanity check
		return 0;

	m = (struct mapreg_save *)DB->data2ptr(data);

	if (m->is_string) {
		if (m->u.str)
			aFree(m->u.str);
	}
	ers_free(mapreg->ers, m);

	return 0;
}

/**
 * Reloads mapregs, saving to database beforehand.
 *
 * This has the effect of clearing the temporary variables, and
 * reloading the permanent ones.
 */
void CMapreg::reload(void) {
	mapreg->save();

	mapreg->regs.vars->clear(mapreg->regs.vars, mapreg->destroyreg);

	if( mapreg->regs.arrays ) {
		mapreg->regs.arrays->destroy(mapreg->regs.arrays, script->array_free_db);
		mapreg->regs.arrays = NULL;
	}

	mapreg->load();
}

/**
 * Finalizer.
 */
void CMapreg::final(void) {
	mapreg->save();

	mapreg->regs.vars->destroy(mapreg->regs.vars, mapreg->destroyreg);

	ers_destroy(mapreg->ers);

	if( mapreg->regs.arrays )
		mapreg->regs.arrays->destroy(mapreg->regs.arrays, script->array_free_db);
}

/**
 * Initializer.
 */
void CMapreg::init(void) {
	mapreg->regs.vars = i64db_alloc(DB_OPT_BASE);
	mapreg->ers = ers_new(sizeof(struct mapreg_save), "mapreg_sql.c::mapreg_ers", ERS_OPT_CLEAN);

	mapreg->load();

	timer->add_func_list(mapreg->save_timer, "mapreg_script_autosave_mapreg");
	timer->add_interval(timer->gettick() + MAPREG_AUTOSAVE_INTERVAL, mapreg->save_timer, 0, 0, MAPREG_AUTOSAVE_INTERVAL);
}

/**
 * Loads the mapreg configuration file.
 */
bool CMapreg::config_read(const char* w1, const char* w2) {
	if(!strcmpi(w1, "mapreg_db"))
		safestrncpy(mapreg->table, w2, sizeof(mapreg->table));
	else
		return false;

	return true;
}

/**
 * Interface defaults initializer.
 */
void mapreg_defaults(void) {
	mapreg = &mapreg_s;

	/* */
	mapreg->regs.vars = NULL;
	mapreg->ers = NULL;
	mapreg->skip_insert = false;

	safestrncpy(mapreg->table, "mapreg", sizeof(mapreg->table));
	mapreg->dirty = false;

	/* */
	mapreg->regs.arrays = NULL;



}
