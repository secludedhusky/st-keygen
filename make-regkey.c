/*
 * Registration key maker for Stereo Tool
 * Copyright (C) 2021 Anthony96922
 *
 * This program is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

/* max name length */
#define MAXLEN		108

/* set to 1 for event FM license (3 days) */
#define EVENT_FM	0

/* known features */
#define FEATURE_FM_PROC		(0x00000001 | 0x00000004 | 0x00000008)
#define FEATURE_ADV_CLIPPER	0x00000002
#define FEATURE_ADVANCED_RDS	0x00000010
#define FEATURE_FILE_POLLING	0x00000040
#define FEATURE_LOW_LAT_MON	0x00000080
#define FEATURE_DECLIPPER	0x00000800 /* also enables nat dynamics */
#define FEATURE_DECLIPPER_2H	0x00001000 /* also enables nat dynamics */
#define FEATURE_NAT_DYN_ONLY	0x00004000 /* natural dynamics only */
#define FEATURE_EVENT_FM_PROC	0x00008000
#define FEATURE_COMP_CLIP	0x00010000
#define FEATURE_COMP_CLIP_EVENT	0x00020000
#define FEATURE_DELOSSIFIER	0x00040000
#define FEATURE_UMPX		0x00080000 /* disabled when FM and this are set */
#define FEATURE_AGC34_AEQ	0x00200000
#define FEATURE_DYN_SPEEDS	0x00400000
#define FEATURE_BIMP		0x00800000
#define FEATURE_UMPX_SFN_GPS	0x01000000
#define FEATURE_UMPXP		0x10000000 /* disabled when FM and this are set */
#define FEATURE_PPM_WTRMRKNG	0x40000000

/* ST-Enterprise */
#define STE_PROC		0x08000000

#if EVENT_FM
#define FEATURE_FM	FEATURE_EVENT_FM_PROC | \
			FEATURE_ADVANCED_RDS | \
			FEATURE_COMP_CLIP | \
			FEATURE_COMP_CLIP_EVENT | \
			FEATURE_UMPX_SFN_GPS | \
			FEATURE_PPM_WTRMRKNG
#else
#define FEATURE_FM	FEATURE_FM_PROC | \
			FEATURE_ADVANCED_RDS | \
			FEATURE_COMP_CLIP | \
			FEATURE_UMPX_SFN_GPS | \
			FEATURE_PPM_WTRMRKNG
#endif

/* feature mask */
#define FEATURES	FEATURE_ADV_CLIPPER | FEATURE_FILE_POLLING | \
			FEATURE_LOW_LAT_MON | FEATURE_FM | \
			FEATURE_DECLIPPER | FEATURE_DELOSSIFIER | \
			FEATURE_AGC34_AEQ | FEATURE_DYN_SPEEDS | \
			FEATURE_BIMP | STE_PROC

static void show_features(unsigned int feat) {
#define SHOW_FEATURE(a, b) \
	if ((feat & a) == a) \
		printf("\t* " b "\n");

#define SHOW_FEATURE_INVERSE(a, b, c) \
	if (feat & a) \
		printf((feat & b) == b ? "\t* " c " disabled\n" : "\t* " c "\n");

#define SHOW_FEATURE_ONLY(a, b, c) \
	if (!(feat & a)) \
		printf((feat & b) == b ? "\t* " c " only\n" : "\t* " c "\n");

#define SHOW_FEATURE_ALWAYS(a) \
	if (feat) \
		printf("\t* " a "\n");

	printf("License: 0x%08x\n", feat);
	SHOW_FEATURE_ALWAYS(						"Dehummer");
	SHOW_FEATURE(FEATURE_FM_PROC,					"FM Processing");
	SHOW_FEATURE(FEATURE_ADV_CLIPPER,				"Advanced Clipper");
	SHOW_FEATURE(FEATURE_ADVANCED_RDS,				"Advanced RDS");
	SHOW_FEATURE(FEATURE_FILE_POLLING,				"File Polling");
	SHOW_FEATURE(FEATURE_LOW_LAT_MON,				"Low Latency Monitoring");
	SHOW_FEATURE(FEATURE_DECLIPPER,					"Declipper & Natural Dynamics");
	SHOW_FEATURE(FEATURE_DECLIPPER_2H,				"Declipper (2 hour limit)");
	SHOW_FEATURE_ONLY(FEATURE_DECLIPPER,	FEATURE_NAT_DYN_ONLY,	"Natural Dynamics");
	SHOW_FEATURE(FEATURE_EVENT_FM_PROC,				"Event FM (3 days)");
	SHOW_FEATURE(FEATURE_COMP_CLIP,					"Composite Clipper");
	SHOW_FEATURE(FEATURE_COMP_CLIP_EVENT,				"Composite Clipper (Event FM)");
	SHOW_FEATURE(FEATURE_DELOSSIFIER,				"Delossifier");
	SHOW_FEATURE_INVERSE(FEATURE_FM_PROC,	FEATURE_UMPX,		"uMPX");
	SHOW_FEATURE(FEATURE_AGC34_AEQ,					"3/4 AGC & Auto EQ");
	SHOW_FEATURE(FEATURE_DYN_SPEEDS,				"Dynamic Speeds");
	SHOW_FEATURE(FEATURE_BIMP,					"BIMP");
	SHOW_FEATURE(FEATURE_UMPX_SFN_GPS,				"uMPX SFN GPS");
	SHOW_FEATURE_INVERSE(FEATURE_FM_PROC,	FEATURE_UMPXP,		"uMPX+");
	SHOW_FEATURE(FEATURE_PPM_WTRMRKNG,				"Nielsen PPM watermarking");
	SHOW_FEATURE(STE_PROC,						"ST-Enterprise");
}

static void scramble(unsigned char *key, size_t length) {
	unsigned char in, out;

	for (size_t i = 0; i < length; i++) {
		in = key[i] ^ (-1 - i - (1 << (1 << (i & 31) & 7)));
		out = 0;
		for (int j = 0; j < 8; j++) {
			out <<= 1;
			out |= in & 1;
			in >>= 1;
		}
		key[i] = out;
	}
}

static void calc_name_check(unsigned char *trailer, char *name) {
	/* the algorithm as found on ghidra */
	trailer[0] = (((name[0] | name[1]) ^ ((name[2] | name[3]) + name[4])) & 0xf) << 4;
	trailer[0] |= (name[0] ^ name[1] ^ name[2] ^ name[3] ^ name[4]) & 0xf;
	trailer[1] = (((name[0] * name[1]) / ((name[2] - name[3]) + 1) - name[4]) & 0xf) << 4;
	trailer[1] |= ((name[0] * name[1]) / ((name[2] - name[3]) + 1) * name[4]) & 0xf;
	trailer[2] = (((name[2] + name[3]) * (name[0] - name[1]) ^ ~name[4]) & 0xf) << 4;
	trailer[2] |= ((name[2] - name[3]) * (name[0] + name[1]) ^ name[4]) & 0xf;
	trailer[3] = ((((name[0] ^ name[1]) + (name[2] ^ name[3])) ^ name[4]) & 0xf) << 4;
	trailer[3] |= (name[0] + name[1] + name[2] - name[3] - name[4]) & 0xf;

	/* reserved */
	trailer[4] = 0;
	trailer[5] = 0;
	trailer[6] = 0;
	trailer[7] = 0;
}

static int calc_checksum(unsigned char *key, size_t length) {
	int checksum = 0;

	for (unsigned int i = 0; i < length; i++) {
		checksum = key[i] * 0x11121 + (checksum << 3);
		checksum += checksum >> 26;
	}
	return checksum;
}

int main(int argc, char *argv[]) {
	int opt;
	unsigned int features = FEATURES;
	int name_len;
	int key_len;
	/* key checksum */
	int checksum;
	char name[MAXLEN + 1];
	unsigned char key[9 + MAXLEN + 1 + 8];
	char out_key_text[(9 + MAXLEN + 1 + 8) * 2];

	const char *short_opt = "f:";
	const struct option long_opt[] = {
		{"features",	required_argument,	NULL,	'f'},
		{0,		0,			0,	0}
	};

	memset(name, 0, MAXLEN + 1);

keep_parsing_opts:

	opt = getopt_long(argc, argv, short_opt, long_opt, NULL);
	if (opt == -1) goto done_parsing_opts;

	switch (opt) {
		case 'f':
			features = strtoul(optarg, NULL, 16);
			break;

		default:
			printf("Usage: %s [ -f features (hex) ] NAME\n",
				argv[0]);
			return 1;
	}

	goto keep_parsing_opts;

done_parsing_opts:

	if (optind < argc) {
		name_len = strlen(argv[optind]);
		if (name_len > MAXLEN) {
			printf("Name is too long.\n");
			return 1;
		}
		name[MAXLEN] = 0;
		strncpy(name, argv[optind], MAXLEN);
	}

	if (!name[0]) {
		printf("Please enter a name.\n");
		return 1;
	}

	/* input validation */

	/* pad the name with spaces if it is shorter than 5 chars */
	if (name_len < 5) {
		for (int i = 0; i < 5 - name_len; i++)
			name[name_len + i] = ' ';
		name[5] = 0;
		name_len = 5;
	}

	/* make sure we don't try to divide by 0 */
	if ((name[2] - name[3]) + 1 == 0) {
		/* we can't divide by 0 */
		printf("Invalid name.\n");
		return 1;
	}

	/*
	 * 18 = the stuff before and after the key (112233445566778899<name>00aabbccddeeffaabb)
	 * 14 (9 + name_len + 1 + 4) is the bare minimum
	 */
	key_len = 9 + name_len + 1 /* null terminator for name string */ + 8;

	*key = 1; /* doesn't seem to affect anything */

	/* registered options */
	memcpy(key + 1, &features, 4);

	/* copy name to key */
	memcpy(key + 9, name, name_len);

	/* add terminator */
	*(key + 9 + name_len) = 0;

	/* add name check trailer */
	calc_name_check(key + 9 + name_len + 1, name);

	/* clear checksum field */
	memset(key + 5, 0, 4);

	/* calculate the checksum */
	checksum = calc_checksum(key, key_len);

	/* copy the checksum */
	memcpy(key + 5, &checksum, 4);

	/* scramble the key */
	scramble(key, key_len);

	for (int i = 0; i < key_len; i++)
		sprintf(out_key_text + i * 2, "%02x", key[i]);

	/* output */
	printf("\n");
	printf("==========================================\n");
	printf("Name\t\t: %s\n", name);
	printf("Features\t: 0x%08x\n", features);
	printf("Calc'd checksum\t: 0x%08x\n", checksum);
	printf("==========================================\n");
	printf("\n");
	show_features(features);
	printf("\n");
	printf("<%s>\n", out_key_text);
	printf("\n");

	return 0;
}
