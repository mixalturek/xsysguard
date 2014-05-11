/* sensors.c
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

#include <xsysguard.h>
#include <string.h>
#include <sensors/sensors.h>

/******************************************************************************/

typedef struct _feature_t {
	sensors_chip_name *chip_name;
	int number;
} feature_t;

/******************************************************************************/

static bool
init_sensors(void)
{
	static bool initialized = FALSE;
	const char *config;
	FILE *f = NULL;

	if (initialized) {
		return TRUE;
	}

	config = xsg_getenv("XSYSGUARD_SENSORS_CONFIG");

	if (config != NULL) {
		if ((f = fopen(config, "r")) == NULL) {
			xsg_warning("cannot open configuration file: %s", config);
			return FALSE;
		}
	}

	// The function accepts NULL file, which is suggested for most applications
	if (sensors_init(f)) {
		if (f != NULL) {
			fclose(f);
		}

		xsg_warning("cannot initialize sensors: sensors_init failed");
		return FALSE;
	}

	if (f != NULL) {
		fclose(f);
	}

	initialized = TRUE;

	return TRUE;
}

/******************************************************************************/

static double
get_sensors_feature(void *arg)
{
	feature_t *feature = (feature_t *) arg;
	double value = DNAN;

	sensors_get_value(feature->chip_name, feature->number, &value);

	xsg_debug("get_sensors_feature: %f", value);

	return value;
}

/******************************************************************************/

static void
parse_sensors(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	const sensors_chip_name *chip_name;
	char *chip_name_prefix;
	int chip_nr = 0;

	if (!init_sensors()) {
		xsg_conf_error("cannot initialize sensors");
	}

	chip_name_prefix = xsg_conf_read_string();

	while ((chip_name = sensors_get_detected_chips(NULL, &chip_nr)) != NULL) {
		const sensors_feature *feature_data;
		const sensors_subfeature *subfeature_data;
		char *feature_data_name;
		int nr1 = 0, nr2 = 0;

		if (!chip_name->prefix
		 || strcmp(chip_name->prefix, chip_name_prefix) != 0) {
			if (strcmp("*", chip_name_prefix) != 0) {
				continue;
			}
		}

		feature_data_name = xsg_conf_read_string();

		while ((feature_data = sensors_get_features(chip_name, &nr1)) != NULL) {
			double value = DNAN;
			feature_t *feature;
			char *label = NULL;
			bool found = FALSE;

			while ((subfeature_data = sensors_get_all_subfeatures(chip_name, feature_data, &nr2)) != NULL) {
				if (sensors_get_value(chip_name, subfeature_data->number, &value) != 0) {
					continue;
				}

				if (subfeature_data->name && !strcmp(subfeature_data->name, feature_data_name)) {
					feature = xsg_new(feature_t, 1);
					feature->number = subfeature_data->number;
					feature->chip_name = xsg_new(sensors_chip_name, 1);
					memcpy((void *) feature->chip_name, (const void *) chip_name,
							sizeof(sensors_chip_name));

					num[0] = get_sensors_feature;
					arg[0] = (void *) feature;

					xsg_free(chip_name_prefix);
					xsg_free(feature_data_name);

					return;
				}
			}

			if (sensors_get_value(chip_name, feature_data->number, &value) != 0) {
				continue;
			}

			if ((label = sensors_get_label(chip_name, feature_data)) == NULL) {
				continue;
			}

			if (feature_data->name && !strcmp(feature_data->name, feature_data_name)) {
				found = TRUE;
			} else if (label && !strcmp(label, feature_data_name)) {
				found = TRUE;
			}

			if (label) {
				free(label);
			}
			if (!found) {
				continue;
			}

			feature = xsg_new(feature_t, 1);
			feature->number = feature_data->number;
			feature->chip_name = xsg_new(sensors_chip_name, 1);
			memcpy((void *) feature->chip_name, (const void *) chip_name,
					sizeof(sensors_chip_name));

			num[0] = get_sensors_feature;
			arg[0] = (void *) feature;

			xsg_free(chip_name_prefix);
			xsg_free(feature_data_name);

			return;
		}

		xsg_conf_error("cannot find feature: %s for %s",
				feature_data_name, chip_name_prefix);
	}

	xsg_conf_error("cannot find chip: %s", chip_name_prefix);
}

static const char *
help_sensors(void)
{
	static xsg_string_t *string = NULL;
	const sensors_chip_name *chip_name;
	int chip_nr;
	int i = 0;

	if (!init_sensors()) {
		return NULL;
	}

	if (string == NULL) {
		string = xsg_string_new(NULL);
	} else {
		xsg_string_truncate(string, 0);
	}

	chip_nr = 0;
	while ((chip_name = sensors_get_detected_chips(NULL, &chip_nr)) != NULL) {
		char name[128];

		snprintf(name, sizeof(name), "SENSORS_CHIP%d", i++);
		xsg_setenv(name, chip_name->prefix, TRUE);

		xsg_string_append_printf(string, "%s:   %s\n", name,
				chip_name->prefix);
	}

	xsg_string_append(string, "\n");

	chip_nr = 0;
	while ((chip_name = sensors_get_detected_chips(NULL, &chip_nr)) != NULL) {
		const sensors_feature *feature_data;
		const sensors_subfeature *subfeature_data;
		int nr1 = 0, nr2 = 0;

		while ((feature_data = sensors_get_features(chip_name, &nr1)) != NULL) {
			double value = DNAN;
			char *label = NULL;

			while ((subfeature_data = sensors_get_all_subfeatures(chip_name, feature_data, &nr2)) != NULL) {
				if (sensors_get_value(chip_name, subfeature_data->number, &value) != 0) {
					continue;
				}

				xsg_string_append_printf(string,
						"N %s:%s:%-32s %6.2f\n",
						XSG_MODULE_NAME,
						chip_name->prefix,
						subfeature_data->name, value);
			}

			if (sensors_get_value(chip_name, feature_data->number, &value) != 0) {
				continue;
			}

			if ((label = sensors_get_label(chip_name, feature_data)) == NULL) {
				continue;
			}

			if (label && feature_data->name && strcmp(label, feature_data->name)) {
				xsg_string_append_printf(string,
						"N %s:%s:%-32s %6.2f  %s\n",
						XSG_MODULE_NAME,
						chip_name->prefix,
						feature_data->name,
						value, label);
			} else {
				xsg_string_append_printf(string,
						"N %s:%s:%-32s %6.2f\n",
						XSG_MODULE_NAME,
						chip_name->prefix,
						feature_data->name, value);
			}

			if (label) {
				free(label);
			}
		}
	}

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_sensors, help_sensors, "libsensors (lm-sensors - Linux hardware monitoring)");

