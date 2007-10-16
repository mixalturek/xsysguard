/* iw.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2007 Sascha Wessel <sawe@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * TODO: wireless_scan
 *
 */

#include <xsysguard.h>
#include <math.h>
#include <iwlib.h>

/******************************************************************************/

static int skfd = -1;

static char buffer[4096];
static struct iwreq wrq;
static struct iw_range range;
static struct iw_statistics stats;

static xsg_list_t *device_list = NULL;

/******************************************************************************/

static int handler(int skfd, char *ifname, char *args[], int count) {
	/* if no wireless name: no wireless extensions */
	if (iw_get_ext(skfd, ifname, SIOCGIWNAME, &wrq) >= 0)
		device_list = xsg_list_append(device_list, (void *) xsg_strdup(ifname));

	return 0;
}

static void get_device_list(void) {
	if (device_list == NULL)
		iw_enum_devices(skfd, handler, NULL, 0);
}

/******************************************************************************/

static double dbm2mw(double dbm) {
	return pow(10.0, dbm / 10.0);
}

/******************************************************************************/

static char *get_name(void *arg) {
	static char name[IFNAMSIZ + 1];

	if (iw_get_ext(skfd, arg, SIOCGIWNAME, &wrq) < 0)
		return "";

	strncpy(name, wrq.u.name, IFNAMSIZ);
	name[IFNAMSIZ] = '\0';

	return name;
}

static double get_nwid(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWNWID, &wrq) < 0)
		return DNAN;

	return (double) wrq.u.nwid.value;
}

static double get_freq_number(void *arg) {
	double freq;

	if (iw_get_ext(skfd, arg, SIOCGIWFREQ, &wrq) < 0)
		return DNAN;

	freq = iw_freq2float(&(wrq.u.freq));

	if (freq < KILO) {
		if (iw_get_range_info(skfd, arg, &range) >= 0)
			iw_channel_to_freq((int) freq, &freq, &range);
	}

	return freq;
}

static char *get_freq_string(void *arg) {
	double freq;

	freq = get_freq_number(arg);

	if (isnan(freq))
		return "";

	iw_print_freq_value(buffer, sizeof(buffer), freq);
	return buffer;
}

static double get_channel(void *arg) {
	double freq;

	if (iw_get_ext(skfd, arg, SIOCGIWFREQ, &wrq) < 0)
		return DNAN;

	freq = iw_freq2float(&(wrq.u.freq));

	if (freq >= KILO) {
		if (iw_get_range_info(skfd, arg, &range) >= 0)
			return (double) iw_freq_to_channel(freq, &range);
	}

	return freq;
}

static char *get_key(void *arg) {
	unsigned char key[IW_ENCODING_TOKEN_MAX];
	int key_size, key_flags;

	wrq.u.data.pointer = (caddr_t) key;
	wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
	wrq.u.data.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWENCODE, &wrq) < 0)
		return "";

	key_size = wrq.u.data.length;
	key_flags = wrq.u.data.flags;

	if ((key_flags & IW_ENCODE_DISABLED) || (key_size == 0))
		return "off";

	iw_print_key(buffer, sizeof(buffer), key, key_size, key_flags);
	return buffer;
}

static double get_keyid(void *arg) {
	unsigned char key[IW_ENCODING_TOKEN_MAX];
	int key_size, key_flags;

	wrq.u.data.pointer = (caddr_t) key;
	wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
	wrq.u.data.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWENCODE, &wrq) < 0)
		return DNAN;

	key_size = wrq.u.data.length;
	key_flags = wrq.u.data.flags;

	if ((key_flags & IW_ENCODE_DISABLED) || (key_size == 0))
		return DNAN;

	return (double) (key_flags & IW_ENCODE_INDEX);
}

static char *get_essid(void *arg) {
	static char essid[IW_ESSID_MAX_SIZE + 1];
	int essid_on;

	wrq.u.essid.pointer = (caddr_t) essid;
	wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWESSID, &wrq) < 0)
		return "";

	essid_on = wrq.u.data.flags;

	if (essid_on)
		return essid;
	else
		return "off/any";
}

static double get_mode_number(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWMODE, &wrq) < 0)
		return DNAN;

	if (wrq.u.mode < IW_NUM_OPER_MODE)
		return (double) wrq.u.mode;
	else
		return (double) IW_NUM_OPER_MODE;
}

static char *get_mode_string(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWMODE, &wrq) < 0)
		return "";

	if (wrq.u.mode < IW_NUM_OPER_MODE)
		strncpy(buffer, iw_operation_mode[wrq.u.mode], sizeof(buffer) - 1);
	else
		strncpy(buffer, iw_operation_mode[IW_NUM_OPER_MODE], sizeof(buffer) - 1);

	buffer[sizeof(buffer) - 1] = '\0';

	return buffer;
}

static double get_sens(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWSENS, &wrq) < 0)
		return DNAN;

	return (double) wrq.u.sens.value;
}

static char *get_nickname(void *arg) {
	static char nickname[IW_ESSID_MAX_SIZE + 1];

	wrq.u.essid.pointer = (caddr_t) nickname;
	wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWNICKN, &wrq) < 0)
		return "";

	if (wrq.u.essid.length <= 1)
		return "";

	return nickname;
}

static char *get_ap(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWAP, &wrq) < 0)
		return "";

	iw_sawap_ntop(&(wrq.u.ap_addr), buffer);

	return buffer;
}

static double get_bitrate_number(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWRATE, &wrq) < 0)
		return DNAN;

	return (double) wrq.u.bitrate.value;
}

static char *get_bitrate_string(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWRATE, &wrq) < 0)
		return "";

	iw_print_bitrate(buffer, sizeof(buffer), wrq.u.bitrate.value);
	return buffer;
}

static double get_rts_number(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWRTS, &wrq) < 0)
		return DNAN;

	if (wrq.u.rts.disabled)
		return DNAN;

	return (double) wrq.u.rts.value;
}

static char *get_rts_string(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWRTS, &wrq) < 0)
		return "";

	if (wrq.u.rts.disabled)
		return "off";

	snprintf(buffer, sizeof(buffer), "%d B", wrq.u.rts.value);

	return buffer;
}

static double get_frag_number(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWFRAG, &wrq) < 0)
		return DNAN;

	if (wrq.u.frag.disabled)
		return DNAN;

	return (double) wrq.u.frag.value;
}

static char *get_frag_string(void *arg) {
	if (iw_get_ext(skfd, arg, SIOCGIWFRAG, &wrq) < 0)
		return "";

	if (wrq.u.frag.disabled)
		return "off";

	snprintf(buffer, sizeof(buffer), "%d B", wrq.u.frag.value);

	return buffer;
}

static char *get_power_management(void *arg) {
	static xsg_string_t *string = NULL;
	wrq.u.power.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWPOWER, &wrq) < 0)
		return "";

	if (wrq.u.power.disabled)
		return "off";

	if (string == NULL)
		string = xsg_string_new(NULL);

	if (wrq.u.power.flags & IW_POWER_TYPE) {
		if (iw_get_range_info(skfd, arg, &range) >= 0) {
			iw_print_pm_value(buffer, sizeof(buffer), wrq.u.power.value, wrq.u.power.flags, range.we_version_compiled);
			string = xsg_string_append_len(string, buffer, -1);
		}
	}

	iw_print_pm_mode(buffer, sizeof(buffer), wrq.u.power.flags);
	string = xsg_string_append_len(string, buffer, -1);

	if (wrq.u.power.flags == IW_POWER_ON)
		string = xsg_string_append_len(string, "on", -1);

	if (string->str[0] == ' ')
		return string->str + 1;
	else
		return string->str;
}

static double get_txpower_dbm(void *arg) {
	if (iw_get_range_info(skfd, arg, &range) < 0)
		return DNAN;

	if (range.we_version_compiled <= 9)
		return DNAN;

	if (iw_get_ext(skfd, arg, SIOCGIWTXPOW, &wrq) < 0)
		return DNAN;

	if (wrq.u.txpower.disabled)
		return DNAN;

	if (wrq.u.txpower.flags & IW_TXPOW_RELATIVE)
		return DNAN;

	if (wrq.u.txpower.flags & IW_TXPOW_MWATT)
		return (double) iw_mwatt2dbm(wrq.u.txpower.value);

	return (double) wrq.u.txpower.value;
}

static double get_txpower_mw(void *arg) {
	return dbm2mw(get_txpower_dbm(arg));
}

static char *get_retry(void *arg) {
	if (iw_get_range_info(skfd, arg, &range) < 0)
		return "";

	if (range.we_version_compiled <= 10)
		return "";

	if (iw_get_ext(skfd, arg, SIOCGIWRETRY, &wrq) < 0)
		return "";

	if (wrq.u.retry.disabled)
		return "off";

	if (wrq.u.retry.flags == IW_RETRY_ON)
		return "on";

	if (wrq.u.retry.flags & IW_RETRY_TYPE) {
		iw_print_retry_value(buffer, sizeof(buffer), wrq.u.retry.value, wrq.u.retry.flags, range.we_version_compiled);

		if (buffer[0] == ' ')
			return buffer + 1;
		else
			return buffer;
	}

	return "";
}

static double get_stats_quality_quality(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	return (double) stats.qual.qual;
}

static double get_stats_quality_signal_dbm(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	if (has_range && ((stats.qual.level != 0) || (stats.qual.updated & (IW_QUAL_DBM | IW_QUAL_RCPI)))) {
		if (stats.qual.updated & IW_QUAL_RCPI) {
			if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID))
				return (stats.qual.level / 2.0) - 110.0;
		} else if ((stats.qual.updated & IW_QUAL_DBM) || (stats.qual.level > range.max_qual.level)) {
			if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)) {
				int dblevel = stats.qual.level;

				if (stats.qual.level >= 64)
					dblevel -= 0x100;

				return (double) dblevel;
			}
		}
	}

	return DNAN;
}

static double get_stats_quality_signal_mw(void *arg) {
	return dbm2mw(get_stats_quality_signal_dbm(arg));
}

static double get_stats_quality_noise_dbm(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	if (has_range && ((stats.qual.level != 0) || (stats.qual.updated & (IW_QUAL_DBM | IW_QUAL_RCPI)))) {
		if (stats.qual.updated & IW_QUAL_RCPI) {
			if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID))
				return (stats.qual.noise / 2.0) - 110.0;
		} else if ((stats.qual.updated & IW_QUAL_DBM) || (stats.qual.level > range.max_qual.level)) {
			if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID)) {
				int dbnoise = stats.qual.noise;

				if (stats.qual.noise >= 64)
					dbnoise -= 0x100;

				return (double) dbnoise;
			}
		}
	}

	return DNAN;
}

static double get_stats_quality_noise_mw(void *arg) {
	return dbm2mw(get_stats_quality_noise_dbm(arg));
}

static double get_stats_discarded_nwid(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	return (double) stats.discard.nwid;
}

static double get_stats_discarded_code(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	return (double) stats.discard.code;
}

static double get_stats_discarded_fragment(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	return (double) stats.discard.fragment;
}

static double get_stats_discarded_retries(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (range.we_version_compiled <= 11)
		return DNAN;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	return (double) stats.discard.retries;
}

static double get_stats_discarded_misc(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	return (double) stats.discard.misc;
}

static double get_stats_missed_beacon(void *arg) {
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0)
		has_range = 0;

	if (range.we_version_compiled <= 11)
		return DNAN;

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0)
		return DNAN;

	return (double) stats.miss.beacon;
}

static double get_range_sensitivity(void *arg) {
	if (iw_get_range_info(skfd, arg, &range) < 0)
		return DNAN;

	return (double) range.sensitivity;
}

static double get_range_max_quality_quality(void *arg) {
	if (iw_get_range_info(skfd, arg, &range) < 0)
		return DNAN;

	return (double) range.max_qual.qual;
}

/******************************************************************************/

static void init(void) {
	if (skfd < 0)
		skfd = iw_sockets_open();
	if (skfd < 0)
		xsg_warning("iw_socket_open() failed");
}

static void shut_down(void) {
	iw_sockets_close(skfd);
}

/******************************************************************************/

static void parse(uint64_t update, xsg_var_t **var, double (**num)(void *), char *(**str)(void *), void **arg, uint32_t n) {
	char *ifname;

	if (n > 1)
		xsg_conf_error("Past values not supported");

	ifname = xsg_conf_read_string();

	if (strcmp(ifname, "*") == 0) {
		xsg_free(ifname);

		init();
		get_device_list();

		if (device_list == NULL) {
			xsg_conf_warning("No devices with wireless extensions found");
			ifname = "";
		} else {
			ifname = device_list->data;
		}
	}

	arg[0] = (void *) ifname;

	if (xsg_conf_find_command("config")) {
		if (xsg_conf_find_command("name")) {
			str[0] = get_name;
		} else if (xsg_conf_find_command("nwid")) {
			num[0] = get_nwid;
		} else if (xsg_conf_find_command("freq")) {
			if (xsg_conf_find_command("num"))
				num[0] = get_freq_number;
			else if (xsg_conf_find_command("str"))
				str[0] = get_freq_string;
			else
				xsg_conf_error("num or str expected");
		} else if (xsg_conf_find_command("channel")) {
			num[0] = get_channel;
		} else if (xsg_conf_find_command("key")) {
			str[0] = get_key;
		} else if (xsg_conf_find_command("keyid")) {
			num[0] = get_keyid;
		} else if (xsg_conf_find_command("essid")) {
			str[0] = get_essid;
		} else if (xsg_conf_find_command("mode")) {
			if (xsg_conf_find_command("num"))
				num[0] = get_mode_number;
			else if (xsg_conf_find_command("str"))
				str[0] = get_mode_string;
			else
				xsg_conf_error("num or str expected");
		} else {
			xsg_conf_error("name, nwid, freq, channel, key, keyid, essid or mode expected");
		}
	} else if (xsg_conf_find_command("info")) {
		if (xsg_conf_find_command("sensitivity")) {
			num[0] = get_sens;
		} else if (xsg_conf_find_command("nickname")) {
			str[0] = get_nickname;
		} else if (xsg_conf_find_command("access_point")) {
			str[0] = get_ap;
		} else if (xsg_conf_find_command("bitrate")) {
			if (xsg_conf_find_command("num"))
				num[0] = get_bitrate_number;
			else if (xsg_conf_find_command("str"))
				str[0] = get_bitrate_string;
			else
				xsg_conf_error("num or str expected");
		} else if (xsg_conf_find_command("rts")) {
			if (xsg_conf_find_command("num"))
				num[0] = get_rts_number;
			else if (xsg_conf_find_command("str"))
				str[0] = get_rts_string;
			else
				xsg_conf_error("num or str expected");
		} else if (xsg_conf_find_command("fragment")) {
			if (xsg_conf_find_command("num"))
				num[0] = get_frag_number;
			else if (xsg_conf_find_command("str"))
				str[0] = get_freq_string;
			else
				xsg_conf_error("num or str expected");
		} else if (xsg_conf_find_command("power_management")) {
			str[0] = get_power_management;
		} else if (xsg_conf_find_command("txpower")) {
			if (xsg_conf_find_command("dbm"))
				num[0] = get_txpower_dbm;
			else if (xsg_conf_find_command("mw"))
				num[0] = get_txpower_mw;
			else
				xsg_conf_error("dbm or mw expected");
		} else if (xsg_conf_find_command("retry")) {
			str[0] = get_retry;
		} else {
			xsg_conf_error("sensitivity, nickname, access_point, bitrate, rts, fragment, power_management, txpower or retry expected");
		}
	} else if (xsg_conf_find_command("stats")) {
		if (xsg_conf_find_command("quality")) {
			if (xsg_conf_find_command("quality")) {
				num[0] = get_stats_quality_quality;
			} else if (xsg_conf_find_command("signal")) {
				if (xsg_conf_find_command("dbm"))
					num[0] = get_stats_quality_signal_dbm;
				else if (xsg_conf_find_command("mw"))
					num[0] = get_stats_quality_signal_mw;
				else
					xsg_conf_error("dbm or mw expected");
			} else if (xsg_conf_find_command("noise")) {
				if (xsg_conf_find_command("dbm"))
					num[0] = get_stats_quality_noise_dbm;
				else if (xsg_conf_find_command("mw"))
					num[0] = get_stats_quality_noise_mw;
				else
					xsg_conf_error("dbm or mw expected");
			} else {
				xsg_conf_error("quality, signal or noise expected");
			}
		} else if (xsg_conf_find_command("discarded")) {
			if (xsg_conf_find_command("nwid"))
				num[0] = get_stats_discarded_nwid;
			else if (xsg_conf_find_command("code"))
				num[0] = get_stats_discarded_code;
			else if (xsg_conf_find_command("fragment"))
				num[0] = get_stats_discarded_fragment;
			else if (xsg_conf_find_command("retries"))
				num[0] = get_stats_discarded_retries;
			else if (xsg_conf_find_command("misc"))
				num[0] = get_stats_discarded_misc;
			else
				xsg_conf_error("nwid, code, fragment, retries or misc expected");
		} else if (xsg_conf_find_command("missed")) {
			if (xsg_conf_find_command("beacon"))
				num[0] = get_stats_missed_beacon;
			else
				xsg_conf_error("beacon expected");
		} else {
			xsg_conf_error("quality, discarded or missed expected");
		}
	} else if (xsg_conf_find_command("range")) {
		if (xsg_conf_find_command("sensitivity")) {
			num[0] = get_range_sensitivity;
		} else if (xsg_conf_find_command("max_quality")) {
			if (xsg_conf_find_command("quality"))
				num[0] = get_range_max_quality_quality;
			else
				xsg_conf_error("quality expected");
		} else {
			xsg_conf_error("sensitivity or max_quality expected");
		}
	} else {
		xsg_conf_error("config, info, stats or range expected");
	}

	xsg_main_add_init_func(init);
	xsg_main_add_shutdown_func(shut_down);
}

static char *help(void) {
	static xsg_string_t *string = NULL;
	xsg_list_t *l;

	init();

	if (skfd < 0)
		return NULL;

	if (string == NULL)
		string = xsg_string_new(NULL);

	get_device_list();

	xsg_string_append_printf(string, "XSG_IW_DEVICES: ");

	for (l = device_list; l; l = l->next) {
		char *filename = (char *) l->data;

		xsg_string_append_printf(string, "%s ", filename);
	}

	xsg_string_append_printf(string, "\n\n");

	for (l = device_list; l; l = l->next) {
		char *ifname = (char *) l->data;

		xsg_string_append_printf(string, "S iw:%s:config:name                         %s\n", ifname, get_name(ifname));
		xsg_string_append_printf(string, "N iw:%s:config:nwid                         %.0f\n", ifname, get_nwid(ifname));
		xsg_string_append_printf(string, "N iw:%s:config:freq:num                     %.0f\n", ifname, get_freq_number(ifname));
		xsg_string_append_printf(string, "S iw:%s:config:freq:str                     %s\n", ifname, get_freq_string(ifname));
		xsg_string_append_printf(string, "N iw:%s:config:channel                      %.0f\n", ifname, get_channel(ifname));
		xsg_string_append_printf(string, "S iw:%s:config:key                          %s\n", ifname, get_key(ifname));
		xsg_string_append_printf(string, "N iw:%s:config:keyid                        %.0f\n", ifname, get_keyid(ifname));
		xsg_string_append_printf(string, "S iw:%s:config:essid                        %s\n", ifname, get_essid(ifname));
		xsg_string_append_printf(string, "N iw:%s:config:mode:num                     %.0f\n", ifname, get_mode_number(ifname));
		xsg_string_append_printf(string, "S iw:%s:config:mode:str                     %s\n", ifname, get_mode_string(ifname));
		xsg_string_append_printf(string, "N iw:%s:info:sensitivity                    %.0f\n", ifname, get_sens(ifname));
		xsg_string_append_printf(string, "S iw:%s:info:nickname                       %s\n", ifname, get_nickname(ifname));
		xsg_string_append_printf(string, "S iw:%s:info:access_point                   %s\n", ifname, get_ap(ifname));
		xsg_string_append_printf(string, "N iw:%s:info:bitrate:num                    %.0f\n", ifname, get_bitrate_number(ifname));
		xsg_string_append_printf(string, "S iw:%s:info:bitrate:str                    %s\n", ifname, get_bitrate_string(ifname));
		xsg_string_append_printf(string, "N iw:%s:info:rts:num                        %.0f\n", ifname, get_rts_number(ifname));
		xsg_string_append_printf(string, "S iw:%s:info:rts:str                        %s\n", ifname, get_rts_string(ifname));
		xsg_string_append_printf(string, "N iw:%s:info:fragment:num                   %.0f\n", ifname, get_frag_number(ifname));
		xsg_string_append_printf(string, "S iw:%s:info:fragment:str                   %s\n", ifname, get_frag_string(ifname));
		xsg_string_append_printf(string, "S iw:%s:info:power_management               %s\n", ifname, get_power_management(ifname));
		xsg_string_append_printf(string, "N iw:%s:info:txpower:dbm                    %.0f\n", ifname, get_txpower_dbm(ifname));
		xsg_string_append_printf(string, "N iw:%s:info:txpower:mw                     %.2f\n", ifname, get_txpower_mw(ifname));
		xsg_string_append_printf(string, "S iw:%s:info:retry                          %s\n", ifname, get_retry(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:quality:quality               %.0f\n", ifname, get_stats_quality_quality(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:quality:signal:dbm            %.0f\n", ifname, get_stats_quality_signal_dbm(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:quality:signal:mw             %.6f\n", ifname, get_stats_quality_signal_mw(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:quality:noise:dbm             %.0f\n", ifname, get_stats_quality_noise_dbm(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:quality:noise:mw              %.6f\n", ifname, get_stats_quality_noise_mw(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:discarded:nwid                %.0f\n", ifname, get_stats_discarded_nwid(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:discarded:code                %.0f\n", ifname, get_stats_discarded_code(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:discarded:fragment            %.0f\n", ifname, get_stats_discarded_fragment(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:discarded:retries             %.0f\n", ifname, get_stats_discarded_retries(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:discarded:misc                %.0f\n", ifname, get_stats_discarded_misc(ifname));
		xsg_string_append_printf(string, "N iw:%s:stats:missed:beacon                 %.0f\n", ifname, get_stats_missed_beacon(ifname));
		xsg_string_append_printf(string, "N iw:%s:range:sensitivity                   %.0f\n", ifname, get_range_sensitivity(ifname));
		xsg_string_append_printf(string, "N iw:%s:range:max_quality:quality           %.0f\n", ifname, get_range_max_quality_quality(ifname));
	}

	return string->str;
}

xsg_module_t xsg_module = {
	parse, help, "libiw (wireless extension library) <http://www.hpl.hp.com/personal/Jean_Tourrilhes/Linux/Tools.html>"
};
