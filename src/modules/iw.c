/* iw.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2008 Sascha Wessel <sawe@users.sf.net>
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

#define _GNU_SOURCE

#include <xsysguard.h>
#include <math.h>
#include <iwlib.h>

#ifndef IW_QUAL_RCPI
# define IW_QUAL_RCPI 0x80
#endif

/******************************************************************************/

static int skfd = -1;

static char buffer[4096];
static struct iwreq wrq;
static struct iw_range range;
static struct iw_statistics stats;

static xsg_list_t *device_list = NULL;

/******************************************************************************/

static int
handle_iw(int fd, char *ifname, char *args[], int count)
{
	/* if no wireless name: no wireless extensions */
	if (iw_get_ext(fd, ifname, SIOCGIWNAME, &wrq) >= 0) {
		device_list = xsg_list_append(device_list,
				(void *) xsg_strdup(ifname));
	}

	return 0;
}

static void
get_iw_device_list(void){
	if (device_list == NULL) {
		iw_enum_devices(skfd, handle_iw, NULL, 0);
	}
}

/******************************************************************************/

static double
dbm2mw(double dbm)
{
	return pow(10.0, dbm / 10.0);
}

/******************************************************************************/

static const char *
get_name(void *arg)
{
	static char name[IFNAMSIZ + 1];

	if (iw_get_ext(skfd, arg, SIOCGIWNAME, &wrq) < 0) {
		xsg_debug("get_name: UNKNOWN");
		return NULL;
	}

	strncpy(name, wrq.u.name, IFNAMSIZ);
	name[IFNAMSIZ] = '\0';

	xsg_debug("get_name: \"%s\"", name);
	return name;
}

static double
get_nwid(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWNWID, &wrq) < 0) {
		xsg_debug("get_nwid: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_nwid: %f", (double) wrq.u.nwid.value);
	return (double) wrq.u.nwid.value;
}

static double
get_freq_number(void *arg)
{
	double freq;

	if (iw_get_ext(skfd, arg, SIOCGIWFREQ, &wrq) < 0) {
		xsg_debug("get_freq_number: UNKNOWN");
		return DNAN;
	}

	freq = iw_freq2float(&(wrq.u.freq));

	if (freq < KILO) {
		if (iw_get_range_info(skfd, arg, &range) >= 0) {
			iw_channel_to_freq((int) freq, &freq, &range);
		}
	}

	xsg_debug("get_freq_number: %f", freq);
	return freq;
}

static const char *
get_freq_string(void *arg)
{
	double freq;

	freq = get_freq_number(arg);

	if (isnan(freq)) {
		xsg_debug("get_freq_string: UNKNOWN");
		return NULL;
	}

	iw_print_freq_value(buffer, sizeof(buffer), freq);

	xsg_debug("get_freq_string: \"%s\"", buffer);
	return buffer;
}

static double
get_channel(void *arg)
{
	double freq;

	if (iw_get_ext(skfd, arg, SIOCGIWFREQ, &wrq) < 0) {
		xsg_debug("get_channel: UNKNOWN");
		return DNAN;
	}

	freq = iw_freq2float(&(wrq.u.freq));

	if (freq >= KILO) {
		if (iw_get_range_info(skfd, arg, &range) >= 0) {
			return (double) iw_freq_to_channel(freq, &range);
		}
	}

	xsg_debug("get_channel: %f", freq);
	return freq;
}

static const char *
get_key(void *arg)
{
	unsigned char key[IW_ENCODING_TOKEN_MAX];
	int key_size, key_flags;

	wrq.u.data.pointer = (caddr_t) key;
	wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
	wrq.u.data.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWENCODE, &wrq) < 0) {
		xsg_debug("get_key: UNKNOWN");
		return NULL;
	}

	key_size = wrq.u.data.length;
	key_flags = wrq.u.data.flags;

	if ((key_flags & IW_ENCODE_DISABLED) || (key_size == 0)) {
		xsg_debug("get_key: \"off\"");
		return "off";
	}

	iw_print_key(buffer, sizeof(buffer), key, key_size, key_flags);

	xsg_debug("get_key: \"%s\"", buffer);
	return buffer;
}

static double
get_keyid(void *arg)
{
	unsigned char key[IW_ENCODING_TOKEN_MAX];
	int key_size, key_flags;

	wrq.u.data.pointer = (caddr_t) key;
	wrq.u.data.length = IW_ENCODING_TOKEN_MAX;
	wrq.u.data.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWENCODE, &wrq) < 0) {
		xsg_debug("get_keyid: UNKNOWN");
		return DNAN;
	}

	key_size = wrq.u.data.length;
	key_flags = wrq.u.data.flags;

	if ((key_flags & IW_ENCODE_DISABLED) || (key_size == 0)) {
		xsg_debug("get_keyid: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_keyid: %f", (double) (key_flags & IW_ENCODE_INDEX));
	return (double) (key_flags & IW_ENCODE_INDEX);
}

static const char *
get_essid(void *arg)
{
	static char essid[IW_ESSID_MAX_SIZE + 1];
	int essid_on;

	wrq.u.essid.pointer = (caddr_t) essid;
	wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWESSID, &wrq) < 0) {
		xsg_debug("get_essid: UNKNOWN");
		return NULL;
	}

	essid_on = wrq.u.data.flags;

	if (essid_on) {
		xsg_debug("get_essid: \"%s\"", essid);
		return essid;
	} else {
		xsg_debug("get_essid: off/any");
		return "off/any";
	}
}

static double
get_mode_number(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWMODE, &wrq) < 0) {
		xsg_debug("get_mode_number: UNKNOWN");
		return DNAN;
	}

	if (wrq.u.mode < IW_NUM_OPER_MODE) {
		xsg_debug("get_mode_number: %f", (double) wrq.u.mode);
		return (double) wrq.u.mode;
	} else {
		xsg_debug("get_mode_number: %f", (double) IW_NUM_OPER_MODE);
		return (double) IW_NUM_OPER_MODE;
	}
}

static const char *
get_mode_string(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWMODE, &wrq) < 0) {
		xsg_debug("get_mode_string: UNKNOWN");
		return NULL;
	}

	if (wrq.u.mode < IW_NUM_OPER_MODE) {
		strncpy(buffer, iw_operation_mode[wrq.u.mode],
				sizeof(buffer) - 1);
	} else {
		strncpy(buffer, iw_operation_mode[IW_NUM_OPER_MODE],
				sizeof(buffer) - 1);
	}

	buffer[sizeof(buffer) - 1] = '\0';

	xsg_debug("get_mode_string: \"%s\"", buffer);
	return buffer;
}

static double
get_sens(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWSENS, &wrq) < 0) {
		xsg_debug("get_sens: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_sens: %f", (double) wrq.u.sens.value);
	return (double) wrq.u.sens.value;
}

static const char *
get_nickname(void *arg)
{
	static char nickname[IW_ESSID_MAX_SIZE + 1];

	wrq.u.essid.pointer = (caddr_t) nickname;
	wrq.u.essid.length = IW_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWNICKN, &wrq) < 0) {
		xsg_debug("get_nickname: UNKNOWN");
		return NULL;
	}

	if (wrq.u.essid.length <= 1) {
		xsg_debug("get_nickname: UNKNOWN");
		return NULL;
	}

	xsg_debug("get_nickname: \"%s\"", nickname);
	return nickname;
}

static const char *
get_ap(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWAP, &wrq) < 0) {
		xsg_debug("get_ap: UNKNOWN");
		return NULL;
	}

	iw_sawap_ntop(&(wrq.u.ap_addr), buffer);

	xsg_debug("get_ap: \"%s\"", buffer);
	return buffer;
}

static double
get_bitrate_number(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWRATE, &wrq) < 0) {
		xsg_debug("get_bitrate_number: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_bitrate_number: %f", (double) wrq.u.bitrate.value);
	return (double) wrq.u.bitrate.value;
}

static const char *
get_bitrate_string(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWRATE, &wrq) < 0) {
		xsg_debug("get_bitrate_string: UNKNOWN");
		return NULL;
	}

	iw_print_bitrate(buffer, sizeof(buffer), wrq.u.bitrate.value);

	xsg_debug("get_bitrate_string: \"%s\"", buffer);
	return buffer;
}

static double
get_rts_number(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWRTS, &wrq) < 0) {
		xsg_debug("get_rts_number: UNKNOWN");
		return DNAN;
	}

	if (wrq.u.rts.disabled) {
		xsg_debug("get_rts_number: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_rts_number: %f", (double) wrq.u.rts.value);
	return (double) wrq.u.rts.value;
}

static const char *
get_rts_string(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWRTS, &wrq) < 0) {
		xsg_debug("get_rts_string: UNKNOWN");
		return NULL;
	}

	if (wrq.u.rts.disabled) {
		xsg_debug("get_rts_string: \"off\"");
		return "off";
	}

	snprintf(buffer, sizeof(buffer), "%d B", wrq.u.rts.value);

	xsg_debug("get_rts_string: \"%s\"", buffer);
	return buffer;
}

static double
get_frag_number(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWFRAG, &wrq) < 0) {
		xsg_debug("get_frag_number: UNKNOWN");
		return DNAN;
	}

	if (wrq.u.frag.disabled) {
		xsg_debug("get_frag_number: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_frag_number: %f", (double) wrq.u.frag.value);
	return (double) wrq.u.frag.value;
}

static const char *
get_frag_string(void *arg)
{
	if (iw_get_ext(skfd, arg, SIOCGIWFRAG, &wrq) < 0) {
		xsg_debug("get_frag_string: UNKNOWN");
		return NULL;
	}

	if (wrq.u.frag.disabled) {
		xsg_debug("get_frag_string: \"off\"");
		return "off";
	}

	snprintf(buffer, sizeof(buffer), "%d B", wrq.u.frag.value);

	xsg_debug("get_frag_string: \"%s\"", buffer);
	return buffer;
}

static const char *
get_power_management(void *arg)
{
	static xsg_string_t *string = NULL;
	wrq.u.power.flags = 0;

	if (iw_get_ext(skfd, arg, SIOCGIWPOWER, &wrq) < 0) {
		xsg_debug("get_power_management: UNKNOWN");
		return NULL;
	}

	if (wrq.u.power.disabled) {
		xsg_debug("get_power_management: \"off\"");
		return "off";
	}

	if (string == NULL) {
		string = xsg_string_new(NULL);
	}

	if (wrq.u.power.flags & IW_POWER_TYPE) {
		if (iw_get_range_info(skfd, arg, &range) >= 0) {
			iw_print_pm_value(buffer, sizeof(buffer),
					wrq.u.power.value, wrq.u.power.flags,
					range.we_version_compiled);
			xsg_string_append_len(string, buffer, -1);
		}
	}

	iw_print_pm_mode(buffer, sizeof(buffer), wrq.u.power.flags);
	xsg_string_append_len(string, buffer, -1);

	if (wrq.u.power.flags == IW_POWER_ON) {
		xsg_string_append_len(string, "on", -1);
	}

	if (string->str[0] == ' ') {
		xsg_debug("get_power_management: \"%s\"", string->str + 1);
		return string->str + 1;
	} else {
		xsg_debug("get_power_management: \"%s\"", string->str);
		return string->str;
	}
}

static double
get_txpower_dbm(void *arg)
{
	if (iw_get_range_info(skfd, arg, &range) < 0) {
		xsg_debug("get_txpower_dbm: UNKNOWN");
		return DNAN;
	}

	if (range.we_version_compiled <= 9) {
		xsg_debug("get_txpower_dbm: UNKNOWN");
		return DNAN;
	}

	if (iw_get_ext(skfd, arg, SIOCGIWTXPOW, &wrq) < 0) {
		xsg_debug("get_txpower_dbm: UNKNOWN");
		return DNAN;
	}

	if (wrq.u.txpower.disabled) {
		xsg_debug("get_txpower_dbm: UNKNOWN");
		return DNAN;
	}

	if (wrq.u.txpower.flags & IW_TXPOW_RELATIVE) {
		xsg_debug("get_txpower_dbm: UNKNOWN");
		return DNAN;
	}

	if (wrq.u.txpower.flags & IW_TXPOW_MWATT) {
		xsg_debug("get_txpower_dbm: %f", (double) iw_mwatt2dbm(wrq.u.txpower.value));
		return (double) iw_mwatt2dbm(wrq.u.txpower.value);
	}

	xsg_debug("get_txpower_dbm: %f", (double) wrq.u.txpower.value);
	return (double) wrq.u.txpower.value;
}

static double
get_txpower_mw(void *arg)
{
	double mw = dbm2mw(get_txpower_dbm(arg));
	xsg_debug("get_txpower_mw: %f", mw);
	return mw;
}

static const char *
get_retry(void *arg)
{
	if (iw_get_range_info(skfd, arg, &range) < 0) {
		xsg_debug("get_retry: UNKNOWN");
		return NULL;
	}

	if (range.we_version_compiled <= 10) {
		xsg_debug("get_retry: UNKNOWN");
		return NULL;
	}

	if (iw_get_ext(skfd, arg, SIOCGIWRETRY, &wrq) < 0) {
		xsg_debug("get_retry: UNKNOWN");
		return NULL;
	}

	if (wrq.u.retry.disabled) {
		xsg_debug("get_retry: \"off\"");
		return "off";
	}

	if (wrq.u.retry.flags == IW_RETRY_ON) {
		xsg_debug("get_retry: \"on\"");
		return "on";
	}

	if (wrq.u.retry.flags & IW_RETRY_TYPE) {
		iw_print_retry_value(buffer, sizeof(buffer),
				wrq.u.retry.value, wrq.u.retry.flags,
				range.we_version_compiled);

		if (buffer[0] == ' ') {
			xsg_debug("get_retry: \"%s\"", buffer + 1);
			return buffer + 1;
		} else {
			xsg_debug("get_retry: \"%s\"", buffer);
			return buffer;
		}
	}

	xsg_debug("get_retry: UNKNOWN");
	return NULL;
}

static double
get_stats_quality_quality(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		xsg_debug("get_stats_quality_quality: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_stats_quality_quality: %f", (double) stats.qual.qual);
	return (double) stats.qual.qual;
}

static double
get_stats_quality_signal(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		xsg_debug("get_stats_quality_signal: UNKNOWN");
		return DNAN;
	}

	if (has_range
	 && ((stats.qual.level != 0)
	  || (stats.qual.updated & (IW_QUAL_DBM | IW_QUAL_RCPI)))) {
		if (stats.qual.updated & IW_QUAL_RCPI) {
			if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)) {
				xsg_debug("get_stats_quality_signal: %f",
					(stats.qual.level / 2.0) - 110.0);
				return (stats.qual.level / 2.0) - 110.0;
			}
		} else if ((stats.qual.updated & IW_QUAL_DBM)
			|| (stats.qual.level > range.max_qual.level)) {
			if (!(stats.qual.updated & IW_QUAL_LEVEL_INVALID)) {
				int dblevel = stats.qual.level;

				if (stats.qual.level >= 64) {
					dblevel -= 0x100;
				}

				xsg_debug("get_stats_quality_signal: %f",
						(double) dblevel);
				return (double) dblevel;
			}
		}
	}

	xsg_debug("get_stats_quality_signal: UNKNOWN");
	return DNAN;
}

static double
get_stats_quality_noise(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		xsg_debug("get_stats_quality_noise: UNKNOWN");
		return DNAN;
	}

	if (has_range
	 && ((stats.qual.level != 0)
	  || (stats.qual.updated & (IW_QUAL_DBM | IW_QUAL_RCPI)))) {
		if (stats.qual.updated & IW_QUAL_RCPI) {
			if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID)) {
				xsg_debug("get_stats_quality_noise: %f",
					(stats.qual.noise / 2.0) - 110.0);
				return (stats.qual.noise / 2.0) - 110.0;
			}
		} else if ((stats.qual.updated & IW_QUAL_DBM)
			|| (stats.qual.level > range.max_qual.level)) {
			if (!(stats.qual.updated & IW_QUAL_NOISE_INVALID)) {
				int dbnoise = stats.qual.noise;

				if (stats.qual.noise >= 64) {
					dbnoise -= 0x100;
				}

				xsg_debug("get_stats_quality_noise: %f",
						(double) dbnoise);
				return (double) dbnoise;
			}
		}
	}

	xsg_debug("get_stats_quality_noise: UNKNOWN");
	return DNAN;
}

static double
get_stats_discarded_nwid(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		return DNAN;
	}

	return (double) stats.discard.nwid;
}

static double
get_stats_discarded_code(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		xsg_debug("get_stats_discarded_code: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_stats_discarded_code: %f",
			(double) stats.discard.code);
	return (double) stats.discard.code;
}

static double
get_stats_discarded_fragment(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		xsg_debug("get_stats_discarded_fragment: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_stats_discarded_fragment: %f",
			(double) stats.discard.fragment);
	return (double) stats.discard.fragment;
}

static double
get_stats_discarded_retries(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (range.we_version_compiled <= 11) {
		xsg_debug("get_stats_discarded_retries: UNKNOWN");
		return DNAN;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		xsg_debug("get_stats_discarded_retries: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_stats_discarded_retries: %f",
			(double) stats.discard.retries);
	return (double) stats.discard.retries;
}

static double
get_stats_discarded_misc(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		xsg_debug("get_stats_discarded_misc: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_stats_discarded_misc: %f",
			(double) stats.discard.misc);
	return (double) stats.discard.misc;
}

static double
get_stats_missed_beacon(void *arg)
{
	int has_range = 1;

	if (iw_get_range_info(skfd, arg, &range) < 0) {
		has_range = 0;
	}

	if (range.we_version_compiled <= 11) {
		xsg_debug("get_stats_missed_beacon: UNKNOWN");
		return DNAN;
	}

	if (iw_get_stats(skfd, arg, &stats, &range, has_range) < 0) {
		xsg_debug("get_stats_missed_beacon: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_stats_missed_beacon: %f",
			(double) stats.miss.beacon);
	return (double) stats.miss.beacon;
}

static double
get_range_sensitivity(void *arg)
{
	if (iw_get_range_info(skfd, arg, &range) < 0) {
		xsg_debug("get_range_sensitivity: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_range_sensitivity: %f",
			(double) range.sensitivity);
	return (double) range.sensitivity;
}

static double
get_range_max_quality_quality(void *arg)
{
	if (iw_get_range_info(skfd, arg, &range) < 0) {
		xsg_debug("get_range_max_quality_quality: UNKNOWN");
		return DNAN;
	}

	xsg_debug("get_range_max_quality_quality: %f",
			(double) range.max_qual.qual);
	return (double) range.max_qual.qual;
}

/******************************************************************************/

static void
init_iw(void)
{
	if (skfd < 0) {
		skfd = iw_sockets_open();
	}
	if (skfd < 0) {
		xsg_warning("iw_socket_open() failed");
	} else {
		xsg_set_cloexec_flag(skfd, TRUE);
	}
}

static void
shutdown_iw(void)
{
	iw_sockets_close(skfd);
}

/******************************************************************************/

static void
parse_iw(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	char *ifname;

	ifname = xsg_conf_read_string();

	if (strcmp(ifname, "*") == 0) {
		xsg_free(ifname);

		init_iw();
		get_iw_device_list();

		if (device_list == NULL) {
			xsg_conf_warning("no devices with wireless "
					"extensions found");
			ifname = xsg_strdup("");
		} else {
			ifname = xsg_strdup(device_list->data);
		}
	}

	*arg = (void *) ifname;

	if (xsg_conf_find_command("config")) {
		if (xsg_conf_find_command("name")) {
			*str = get_name;
		} else if (xsg_conf_find_command("nwid")) {
			*num = get_nwid;
		} else if (xsg_conf_find_command("freq")) {
			*num = get_freq_number;
			*str = get_freq_string;
		} else if (xsg_conf_find_command("channel")) {
			*num = get_channel;
		} else if (xsg_conf_find_command("key")) {
			*str = get_key;
		} else if (xsg_conf_find_command("keyid")) {
			*num = get_keyid;
		} else if (xsg_conf_find_command("essid")) {
			*str = get_essid;
		} else if (xsg_conf_find_command("mode")) {
			*num = get_mode_number;
			*str = get_mode_string;
		} else {
			xsg_conf_error("name, nwid, freq, channel, key, keyid, "
					"essid or mode expected");
		}
	} else if (xsg_conf_find_command("info")) {
		if (xsg_conf_find_command("sensitivity")) {
			*num = get_sens;
		} else if (xsg_conf_find_command("nickname")) {
			*str = get_nickname;
		} else if (xsg_conf_find_command("access_point")) {
			*str = get_ap;
		} else if (xsg_conf_find_command("bitrate")) {
			*num = get_bitrate_number;
			*str = get_bitrate_string;
		} else if (xsg_conf_find_command("rts")) {
			*num = get_rts_number;
			*str = get_rts_string;
		} else if (xsg_conf_find_command("fragment")) {
			*num = get_frag_number;
			*str = get_frag_string;
		} else if (xsg_conf_find_command("power_management")) {
			*str = get_power_management;
		} else if (xsg_conf_find_command("txpower")) {
			if (xsg_conf_find_command("dbm")) {
				*num = get_txpower_dbm;
			} else if (xsg_conf_find_command("mw")) {
				*num = get_txpower_mw;
			} else {
				xsg_conf_error("dbm or mw expected");
			}
		} else if (xsg_conf_find_command("retry")) {
			*str = get_retry;
		} else {
			xsg_conf_error("sensitivity, nickname, access_point, "
					"bitrate, rts, fragment, "
					"power_management, txpower or retry "
					"expected");
		}
	} else if (xsg_conf_find_command("stats")) {
		if (xsg_conf_find_command("quality")) {
			if (xsg_conf_find_command("quality")) {
				*num = get_stats_quality_quality;
			} else if (xsg_conf_find_command("signal")) {
				*num = get_stats_quality_signal;
			} else if (xsg_conf_find_command("noise")) {
				*num = get_stats_quality_noise;
			} else {
				xsg_conf_error("quality, signal or noise "
						"expected");
			}
		} else if (xsg_conf_find_command("discarded")) {
			if (xsg_conf_find_command("nwid")) {
				*num = get_stats_discarded_nwid;
			} else if (xsg_conf_find_command("code")) {
				*num = get_stats_discarded_code;
			} else if (xsg_conf_find_command("fragment")) {
				*num = get_stats_discarded_fragment;
			} else if (xsg_conf_find_command("retries")) {
				*num = get_stats_discarded_retries;
			} else if (xsg_conf_find_command("misc")) {
				*num = get_stats_discarded_misc;
			} else {
				xsg_conf_error("nwid, code, fragment, retries "
						"or misc expected");
			}
		} else if (xsg_conf_find_command("missed")) {
			if (xsg_conf_find_command("beacon")) {
				*num = get_stats_missed_beacon;
			} else {
				xsg_conf_error("beacon expected");
			}
		} else {
			xsg_conf_error("quality, discarded or missed expected");
		}
	} else if (xsg_conf_find_command("range")) {
		if (xsg_conf_find_command("sensitivity")) {
			*num = get_range_sensitivity;
		} else if (xsg_conf_find_command("max_quality")) {
			if (xsg_conf_find_command("quality")) {
				*num = get_range_max_quality_quality;
			} else {
				xsg_conf_error("quality expected");
			}
		} else {
			xsg_conf_error("sensitivity or max_quality expected");
		}
	} else {
		xsg_conf_error("config, info, stats or range expected");
	}

	xsg_main_add_init_func(init_iw);
	xsg_main_add_shutdown_func(shutdown_iw);
}

static const char *
help_iw(void)
{
	static xsg_string_t *string = NULL;
	xsg_list_t *l;
	int i = 0;

	init_iw();

	if (skfd < 0) {
		return NULL;
	}

	if (string == NULL) {
		string = xsg_string_new(NULL);
	} else {
		xsg_string_truncate(string, 0);
	}

	get_iw_device_list();

	for (l = device_list; l; l = l->next) {
		char name[128];

		snprintf(name, sizeof(name), "IW_INTERFACE%d", i++);
		xsg_setenv(name, (char *) device_list->data, TRUE);

		xsg_string_append_printf(string, "%s:   %s\n", name,
				(char *) device_list->data);
	}

	xsg_string_append(string, "\n");

	for (l = device_list; l; l = l->next) {
		char *ifname = (char *) l->data;

		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "config:name",
				get_name(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "config:nwid",
				get_nwid(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "config:freq",
				get_freq_number(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "config:freq",
				get_freq_string(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "config:channel",
				get_channel(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "config:key",
				get_key(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "config:keyid",
				get_keyid(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "config:essid",
				get_essid(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "config:mode",
				get_mode_number(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "config:mode",
				get_mode_string(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "info:sensitivity",
				get_sens(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "info:nickname",
				get_nickname(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "info:access_point",
				get_ap(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "info:bitrate",
				get_bitrate_number(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "info:bitrate",
				get_bitrate_string(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "info:rts",
				get_rts_number(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "info:rts",
				get_rts_string(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "info:fragment",
				get_frag_number(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "info:fragment",
				get_frag_string(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "info:power_management",
				get_power_management(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "info:txpower:dbm",
				get_txpower_dbm(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.2f\n",
				XSG_MODULE_NAME, ifname, "info:txpower:mw",
				get_txpower_mw(ifname));
		xsg_string_append_printf(string, "S %s:%s:%-36s%s\n",
				XSG_MODULE_NAME, ifname, "info:retry",
				get_retry(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:quality:quality",
				get_stats_quality_quality(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:quality:signal",
				get_stats_quality_signal(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:quality:noise",
				get_stats_quality_noise(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:discarded:nwid",
				get_stats_discarded_nwid(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:discarded:code",
				get_stats_discarded_code(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:discarded:fragment",
				get_stats_discarded_fragment(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:discarded:retries",
				get_stats_discarded_retries(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:discarded:misc",
				get_stats_discarded_misc(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "stats:missed:beacon",
				get_stats_missed_beacon(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "range:sensitivity",
				get_range_sensitivity(ifname));
		xsg_string_append_printf(string, "N %s:%s:%-36s%.0f\n",
				XSG_MODULE_NAME, ifname, "range:max_quality:quality",
				get_range_max_quality_quality(ifname));
	}

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_iw, help_iw, "libiw (wireless extension library)");

