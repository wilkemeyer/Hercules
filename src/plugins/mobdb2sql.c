// Copyright (c) Hercules Dev Team, licensed under GNU GPL.
// See the LICENSE file

#include "config/core.h"

#include "common/hercules.h"
#include "common/cbasetypes.h"
#include "common/conf.h"
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/nullpo.h"
#include "common/strlib.h"
#include "map/battle.h"
#include "map/itemdb.h"
#include "map/mob.h"
#include "map/map.h"

#include "common/HPMDataCheck.h"

#include <stdio.h>
#include <stdlib.h>

HPExport struct hplugin_info pinfo = {
	"ItemDB2SQL",    // Plugin name
	SERVER_TYPE_MAP, // Which server types this plugin works with?
	"0.6",           // Plugin version
	HPM_VERSION,     // HPM Version (don't change, macro is automatically updated)
};

/// Conversion state tracking.
struct {
	FILE *fp; ///< Currently open file pointer
	struct {
		char *p;    ///< Buffer pointer
		size_t len; ///< Buffer length
	} buf[4]; ///< Output buffer
	char *db_name; ///< Database table name
} tosql;

/// Whether the plugin will automatically run.
bool mobdb2sql_torun = false;

/// Backup of the original function pointer.
int (*mob_read_db_sub) (config_setting_t *it, int n, const char *source);


/**
 * Normalizes and appends a string to the output buffer.
 *
 * @param str The string to append.
 */
void hstr(const char *str)
{
	if (strlen(str) > tosql.buf[3].len) {
		tosql.buf[3].len = tosql.buf[3].len + strlen(str) + 1000;
		RECREATE(tosql.buf[3].p,char,tosql.buf[3].len);
	}
	safestrncpy(tosql.buf[3].p,str,strlen(str));
	normalize_name(tosql.buf[3].p,"\t\n ");
}

/**
 * Converts an Item DB entry to SQL.
 *
 * @see mobdb_readdb_libconfig_sub.
 */
int mobdb2sql_sub(config_setting_t *mobt, int n, const char *source)
{
	struct mob_db *md = NULL;
	nullpo_ret(mobt);

	if ((md = mob->db(mob_read_db_sub(mobt, n, source))) != mob->dummy) {
		char e_name[NAME_LENGTH*2+1];
		StringBuf buf;
		int card_idx = 9, i;

		StrBuf->Init(&buf);

		// id
		StrBuf->Printf(&buf, "%u,", md->mob_id);

		// Sprite
		SQL->EscapeString(NULL, e_name, md->sprite);
		StrBuf->Printf(&buf, "'%s',", e_name);

		// kName
		SQL->EscapeString(NULL, e_name, md->name);
		StrBuf->Printf(&buf, "'%s',", e_name);

		// iName
		SQL->EscapeString(NULL, e_name, md->jname);
		StrBuf->Printf(&buf, "'%s',", e_name);

		// LV
		StrBuf->Printf(&buf, "%u,", md->lv);

		// HP
		StrBuf->Printf(&buf, "%u,", md->status.max_hp);

		// SP
		StrBuf->Printf(&buf, "%u,", md->status.max_sp);
		
		// EXP
		StrBuf->Printf(&buf, "%u,", md->base_exp);

		// JEXP
		StrBuf->Printf(&buf, "%u,", md->job_exp);

		// Range1
		StrBuf->Printf(&buf, "%u,", md->status.rhw.range);

		// ATK1
		StrBuf->Printf(&buf, "%u,", md->status.rhw.atk);

		// ATK2
		StrBuf->Printf(&buf, "%u,", md->status.rhw.atk2);

		// DEF
		StrBuf->Printf(&buf, "%u,", md->status.def);

		// MDEF
		StrBuf->Printf(&buf, "%u,", md->status.mdef);

		// STR
		StrBuf->Printf(&buf, "%u,", md->status.str);

		// AGI
		StrBuf->Printf(&buf, "%u,", md->status.agi);

		// VIT
		StrBuf->Printf(&buf, "%u,", md->status.vit);

		// INT
		StrBuf->Printf(&buf, "%u,", md->status.int_);

		// DEX
		StrBuf->Printf(&buf, "%u,", md->status.dex);

		// LUK
		StrBuf->Printf(&buf, "%u,", md->status.luk);

		// Range2
		StrBuf->Printf(&buf, "%u,", md->range2);

		// Range3
		StrBuf->Printf(&buf, "%u,", md->range3);

		// Scale
		StrBuf->Printf(&buf, "%u,", md->status.size);

		// Race
		StrBuf->Printf(&buf, "%u,", md->status.race);

		// Element
		StrBuf->Printf(&buf, "%u,", md->status.def_ele + 10 * (1+md->status.ele_lv));

		// Mode
		StrBuf->Printf(&buf, "0x%X,", md->status.mode);

		// Speed
		StrBuf->Printf(&buf, "%u,", md->status.speed);

		// aDelay
		StrBuf->Printf(&buf, "%u,", md->status.adelay);

		// aMotion
		StrBuf->Printf(&buf, "%u,", md->status.amotion);

		// dMotion
		StrBuf->Printf(&buf, "%u,", md->status.dmotion);

		// MEXP
		StrBuf->Printf(&buf, "%u,", md->mexp);

		for (i = 0; i < 3; i++) {
			// MVP{i}id
			StrBuf->Printf(&buf, "%u,", md->mvpitem[i].nameid);
			// MVP{i}per
			StrBuf->Printf(&buf, "%u,", md->mvpitem[i].p);
		}

		// Scan for cards
		for (i = 0; i < 10; i++) {
			struct item_data *it = NULL;
			if (md->dropitem[i].nameid != 0 && (it = itemdb->exists(md->dropitem[i].nameid)) != NULL && it->type == IT_CARD)
				card_idx = i;
		}

		for (i = 0; i < 10; i++) {
			if (card_idx == i)
				continue;
			// Drop{i}id
			StrBuf->Printf(&buf, "%u,", md->dropitem[i].nameid);
			// Drop{i}per
			StrBuf->Printf(&buf, "%u,", md->dropitem[i].p);
		}

		// DropCardid
		StrBuf->Printf(&buf, "%u,", md->dropitem[card_idx].nameid);
		// DropCardper
		StrBuf->Printf(&buf, "%u", md->dropitem[card_idx].p);

		fprintf(tosql.fp, "REPLACE INTO `%s` VALUES (%s);\n", tosql.db_name, StrBuf->Value(&buf));

		StrBuf->Destroy(&buf);
	}

	return md ? md->mob_id : 0;
}

/**
 * Prints a SQL table header for the current table.
 */
void mobdb2sql_tableheader(void)
{
	fprintf(tosql.fp,
			"-- NOTE: This file was auto-generated and should never be manually edited,\n"
			"--       as it will get overwritten. If you need to modify this file,\n"
			"--       please consider modifying the corresponding .conf file inside\n"
			"--       the db folder, and then re-run the mobdb2sql plugin.\n"
			"\n"
			"--\n"
			"-- Table structure for table `%s`\n"
			"--\n"
			"\n"
			"DROP TABLE IF EXISTS `%s`;\n"
			"CREATE TABLE `%s` (\n"
			"  `ID` MEDIUMINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Sprite` TEXT NOT NULL,\n"
			"  `kName` TEXT NOT NULL,\n"
			"  `iName` TEXT NOT NULL,\n"
			"  `LV` TINYINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `HP` INT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `SP` MEDIUMINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `EXP` MEDIUMINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `JEXP` MEDIUMINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Range1` TINYINT(4) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `ATK1` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `ATK2` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `DEF` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `MDEF` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `STR` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `AGI` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `VIT` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `INT` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `DEX` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `LUK` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Range2` TINYINT(4) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Range3` TINYINT(4) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Scale` TINYINT(4) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Race` TINYINT(4) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Element` TINYINT(4) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Mode` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Speed` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `aDelay` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `aMotion` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `dMotion` SMALLINT(6) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `MEXP` MEDIUMINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `MVP1id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `MVP1per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `MVP2id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `MVP2per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `MVP3id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `MVP3per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop1id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop1per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop2id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop2per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop3id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop3per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop4id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop4per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop5id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop5per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop6id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop6per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop7id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop7per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop8id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop8per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop9id` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `Drop9per` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `DropCardid` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  `DropCardper` SMALLINT(9) UNSIGNED NOT NULL DEFAULT '0',\n"
			"  PRIMARY KEY (`ID`)\n"
			") ENGINE=MyISAM;\n"
			"\n",tosql.db_name,tosql.db_name,tosql.db_name);
}

/**
 * Plugin main function.
 *
 * Converts Renewal, Pre-Renewal Item DB and Item DB2 to SQL scripts.
 */
void do_mobdb2sql(void)
{
	int i;

	/* link */
	mob_read_db_sub = mob->read_db_sub;
	mob->read_db_sub = mobdb2sql_sub;

	if (map->minimal) {
		// Set up modifiers
		battle->config_set_defaults();
	}

	memset(&tosql.buf, 0, sizeof(tosql.buf));

	/* Process Renewal Item DB */
	if ((tosql.fp = fopen("sql-files/mob_db_re.sql", "wt+")) == NULL) {
		ShowError("mobdb_tosql: File not found \"%s\".\n", "sql-files/mob_db_re.sql");
		return;
	}
	tosql.db_name = "mob_db";
	mobdb2sql_tableheader();

	itemdb->clear(false);
	itemdb->readdb_libconfig("re/item_db.conf");
	mob->read_libconfig("re/mob_db.conf", false);

	fclose(tosql.fp);

	/* Process Pre-Renewal Item DB */
	if ((tosql.fp = fopen("sql-files/mob_db.sql", "wt+")) == NULL) {
		ShowError("mobdb_tosql: File not found \"%s\".\n", "sql-files/mob_db.sql");
		return;
	}
	tosql.db_name = "mob_db";
	mobdb2sql_tableheader();

	itemdb->clear(false);
	itemdb->readdb_libconfig("pre-re/item_db.conf");
	mob->read_libconfig("pre-re/mob_db.conf", false);

	fclose(tosql.fp);

	/* Process Item DB2 */
	if ((tosql.fp = fopen("sql-files/mob_db2.sql", "wt+")) == NULL) {
		ShowError("mobdb_tosql: File not found \"%s\".\n", "sql-files/mob_db2.sql");
		return;
	}
	tosql.db_name = "mob_db2";
	mobdb2sql_tableheader();

	mob->read_libconfig("mob_db2.conf", true);

	fclose(tosql.fp);

	/* unlink */
	mob->read_db_sub = mob_read_db_sub;

	for (i = 0; i < ARRAYLENGTH(tosql.buf); i++) {
		if (tosql.buf[i].p)
			aFree(tosql.buf[i].p);
	}
}

CPCMD(mobdb2sql)
{
	do_mobdb2sql();
}

CMDLINEARG(mobdb2sql)
{
	map->minimal = true;
	mobdb2sql_torun = true;
	return true;
}

HPExport void server_preinit(void)
{

	addArg("--mobdb2sql", false, mobdb2sql, NULL);
}

HPExport void plugin_init(void)
{
	addCPCommand("server:tools:mobdb2sql",mobdb2sql);
}

HPExport void server_online(void)
{
	if (mobdb2sql_torun)
		do_mobdb2sql();
}
