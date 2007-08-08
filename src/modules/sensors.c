/* sensors.c
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

#include <xsysguard.h>
#include <string.h>
#include <sensors/sensors.h>

#define DEFAULT_SENSORS_CONF "/etc/sensors.conf"

/******************************************************************************/

typedef struct _feature_t {
	const sensors_chip_name *chip_name;
	int number;
} feature_t;

/******************************************************************************/

static bool init(void) {
	char *config;
	FILE *f;

	config = getenv("XSYSGUARD_SENSORS_CONFIG");

	if (config == NULL)
		config = DEFAULT_SENSORS_CONF;

	if ((f = fopen(config, "r")) == NULL) {
		xsg_warning("Cannot open configuration file: %s", config);
		return FALSE;
	}

	if (sensors_init(f)) {
		fclose(f);
		xsg_warning("Cannot initialize sensors: sensors_init failed");
		return FALSE;
	}

	fclose(f);

	return TRUE;
}

/******************************************************************************/

static double get_sensors_feature(void *arg) {
	feature_t *feature = (feature_t *) arg;
	double value = DNAN;

	sensors_get_feature(*feature->chip_name, feature->number, &value);

	xsg_debug("get_sensors_feature: %f", value);

	return value;
}

/******************************************************************************/

void parse(uint64_t update, xsg_var_t **var, double (**num)(void *), char *(**str)(void *), void **arg, uint32_t n) {
	static bool initialized = FALSE;
	const sensors_chip_name *chip_name;
	char *chip_name_prefix;
	int chip_nr = 0;

	if (n > 1)
		xsg_conf_error("Past values not supported by sensors module");

	if (!initialized) {
		if (!init())
			xsg_conf_error("Cannot initialize sensors");
		initialized = TRUE;
	}

	chip_name_prefix = xsg_conf_read_string();

	while ((chip_name = sensors_get_detected_chips(&chip_nr)) != NULL) {
		const sensors_feature_data *feature_data;
		char *feature_data_name;
		int nr1 = 0, nr2 = 0;

		if (!chip_name->prefix || strcmp(chip_name->prefix, chip_name_prefix) != 0)
			if (strcmp("*", chip_name_prefix) != 0)
				continue;

		feature_data_name = xsg_conf_read_string();

		while ((feature_data = sensors_get_all_features(*chip_name, &nr1, &nr2)) != NULL) {
			double value = DNAN;
			feature_t *feature;

			//if (feature_data->mapping != SENSORS_NO_MAPPING)
			//	continue;

			if (sensors_get_feature(*chip_name, feature_data->number, &value) != 0)
				continue;

			if (!feature_data->name || strcmp(feature_data->name, feature_data_name) != 0)
				continue;

			feature = xsg_new(feature_t, 1);
			feature->number = feature_data->number;
			feature->chip_name = xsg_new(sensors_chip_name, 1);
			memcpy((void *) feature->chip_name, (void *) chip_name, sizeof(sensors_chip_name));

			num[0] = get_sensors_feature;
			arg[0] = (void *) feature;

			xsg_free(chip_name_prefix);
			xsg_free(feature_data_name);

			return;
		}

		xsg_conf_error("Cannot find feature: %s for %s", feature_data_name, chip_name_prefix);
	}

	xsg_conf_error("Cannot find chip: %s", chip_name_prefix);
}

char *info(void) {
	return "interface for lm-sensors <http://www.lm-sensors.org>";
}

char *help(void) {
	xsg_string_t *string;
	const sensors_chip_name *chip_name;
	int chip_nr = 0;

	if (!init())
		return NULL;

	string = xsg_string_new("Available features:\n");

	while ((chip_name = sensors_get_detected_chips(&chip_nr)) != NULL) {
		const sensors_feature_data *feature_data;
		int nr1 = 0, nr2 = 0;

		while ((feature_data = sensors_get_all_features(*chip_name, &nr1, &nr2)) != NULL) {
			double value = DNAN;

			//if (feature_data->mapping != SENSORS_NO_MAPPING)
			//	continue;

			if (sensors_get_feature(*chip_name, feature_data->number, &value) != 0)
				continue;

			xsg_string_append_printf(string, ":%s:%-32s %6.2f\n", chip_name->prefix, feature_data->name, value);
		}
	}

	return xsg_string_free(string, FALSE);
}

