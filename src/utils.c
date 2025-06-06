/*
 * utils.c
 * Miscellaneous utilities for string manipulation,
 * file I/O and plist helper.
 *
 * Copyright (c) 2014-2019 Nikias Bassen, All Rights Reserved.
 * Copyright (c) 2013-2014 Martin Szulecki, All Rights Reserved.
 * Copyright (c) 2013 Federico Mena Quintero
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _MSC_VER
#include <winsock2.h>
#define strdup _strdup
#else
#include <sys/time.h>
#endif
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>

#include "common.h"
#include "libimobiledevice-glue/utils.h"
#include <inttypes.h>

#ifndef HAVE_STPCPY
#undef stpcpy
char *stpcpy(char *s1, const char *s2);

/**
 * Copy characters from one string into another
 *
 * @note: The strings should not overlap, as the behavior is undefined.
 *
 * @s1: The source string.
 * @s2: The destination string.
 *
 * @return a pointer to the terminating `\0' character of @s1,
 * or NULL if @s1 or @s2 is NULL.
 */
char *stpcpy(char *s1, const char *s2)
{
	if (s1 == NULL || s2 == NULL)
		return NULL;

	strcpy(s1, s2);

	return s1 + strlen(s2);
}
#endif

/**
 * Concatenate strings into a newly allocated string
 *
 * @note: Specify NULL for the last string in the varargs list
 *
 * @str: The first string in the list
 * @...: Subsequent strings.  Use NULL for the last item.
 *
 * @return a newly allocated string, or NULL if @str is NULL.  This will also
 * return NULL and set errno to ENOMEM if memory is exhausted.
 */
LIBIMOBILEDEVICE_GLUE_API char *string_concat(const char *str, ...)
{
	size_t len;
	va_list args;
	char *s;
	char *result;
	char *dest;

	if (!str)
		return NULL;

	/* Compute final length */

	len = strlen(str) + 1; /* plus 1 for the null terminator */

	va_start(args, str);
	s = va_arg(args, char *);
	while (s) {
		len += strlen(s);
		s = va_arg(args, char*);
	}
	va_end(args);

	/* Concat each string */

	result = (char*)malloc(len);
	if (!result)
		return NULL; /* errno remains set */

	dest = result;

	dest = stpcpy(dest, str);

	va_start(args, str);
	s = va_arg(args, char *);
	while (s) {
		dest = stpcpy(dest, s);
		s = va_arg(args, char *);
	}
	va_end(args);

	return result;
}

LIBIMOBILEDEVICE_GLUE_API char *string_append(char* str, ...)
{
	size_t len = 0;
	size_t slen;
	va_list args;
	char *s;
	char *result;
	char *dest;

	/* Compute final length */

	if (str) {
		len = strlen(str);
	}
	slen = len;
	len++; /* plus 1 for the null terminator */

	va_start(args, str);
	s = va_arg(args, char *);
	while (s) {
		len += strlen(s);
		s = va_arg(args, char*);
	}
	va_end(args);

	result = (char*)realloc(str, len);
	if (!result)
		return NULL; /* errno remains set */

	dest = result + slen;

	/* Concat additional strings */

	va_start(args, str);
	s = va_arg(args, char *);
	while (s) {
		dest = stpcpy(dest, s);
		s = va_arg(args, char *);
	}
	va_end(args);

	return result;
}

LIBIMOBILEDEVICE_GLUE_API char *string_build_path(const char *elem, ...)
{
	if (!elem)
		return NULL;
	va_list args;
	int len = strlen(elem)+1;
	va_start(args, elem);
	char *arg = va_arg(args, char*);
	while (arg) {
		len += strlen(arg)+1;
		arg = va_arg(args, char*);
	}
	va_end(args);

	char* out = (char*)malloc(len);
	strcpy(out, elem);

	va_start(args, elem);
	arg = va_arg(args, char*);
	while (arg) {
		strcat(out, "\\");
		strcat(out, arg);
		arg = va_arg(args, char*);
	}
	va_end(args);
	return out;
}

LIBIMOBILEDEVICE_GLUE_API char *string_format_size(uint64_t size)
{
	char buf[80];
	double sz;
	if (size >= 1000000000000LL) {
		sz = ((double)size / 1000000000000.0F);
		sprintf(buf, "%0.1f TB", sz);
	} else if (size >= 1000000000LL) {
		sz = ((double)size / 1000000000.0F);
		sprintf(buf, "%0.1f GB", sz);
	} else if (size >= 1000000LL) {
		sz = ((double)size / 1000000.0F);
		sprintf(buf, "%0.1f MB", sz);
	} else if (size >= 1000LL) {
		sz = ((double)size / 1000.0F);
		sprintf(buf, "%0.1f KB", sz);
	} else {
		sprintf(buf, "%d Bytes", (int)size);
	}
	return strdup(buf);
}

LIBIMOBILEDEVICE_GLUE_API char *string_toupper(char* str)
{
	char *res = strdup(str);
	size_t i;
	for (i = 0; i < strlen(res); i++) {
		res[i] = toupper(res[i]);
	}
	return res;
}

static int get_rand(int min, int max)
{
	int retval = (rand() % (max - min)) + min;
	return retval;
}

LIBIMOBILEDEVICE_GLUE_API char *generate_uuid()
{
	const char *chars = "ABCDEF0123456789";
	int i = 0;
	char *uuid = (char *) malloc(sizeof(char) * 37);

	srand(time(NULL));

	for (i = 0; i < 36; i++) {
		if (i == 8 || i == 13 || i == 18 || i == 23) {
			uuid[i] = '-';
			continue;
		}
		uuid[i] = chars[get_rand(0, 16)];
	}

	/* make it a real string */
	uuid[36] = '\0';

	return uuid;
}

LIBIMOBILEDEVICE_GLUE_API int buffer_read_from_filename(const char *filename, char **buffer, uint64_t *length)
{
	FILE *f;
	uint64_t size;

	if (!filename || !buffer || !length) {
		return 0;
	}

	*length = 0;

	f = fopen(filename, "rb");
	if (!f) {
		return 0;
	}

	fseek(f, 0, SEEK_END);
	size = ftell(f);
	rewind(f);

	if (size == 0) {
		fclose(f);
		return 0;
	}

	*buffer = (char*)malloc(sizeof(char)*(size+1));

	if (*buffer == NULL) {
		fclose(f);
		return 0;
	}

	int ret = 1;
	if (fread(*buffer, sizeof(char), size, f) != size) {
		free(*buffer);
		ret = 0;
		errno = EIO;
	}
	fclose(f);

	*length = size;
	return ret;
}

LIBIMOBILEDEVICE_GLUE_API int buffer_write_to_filename(const char *filename, const char *buffer, uint64_t length)
{
	FILE *f;

	f = fopen(filename, "wb");
	if (f) {
		size_t written = fwrite(buffer, sizeof(char), length, f);
		fclose(f);

		if (written == length) {
			return 1;
		}
		else {
			// Not all data could be written.
			errno = EIO;
			return 0;
		}
	}
	else {
		// Failed to open the file, let the caller know.
		return 0;
	}
}

LIBIMOBILEDEVICE_GLUE_API int plist_read_from_filename(plist_t *plist, const char *filename)
{
	char *buffer = NULL;
	uint64_t length;

	if (!filename)
		return 0;

	if (!buffer_read_from_filename(filename, &buffer, &length)) {
		return 0;
	}

	plist_from_memory(buffer, length, plist);

	free(buffer);

	return 1;
}

LIBIMOBILEDEVICE_GLUE_API int plist_write_to_filename(plist_t plist, const char *filename, enum plist_format_t format)
{
	char *buffer = NULL;
	uint32_t length;

	if (!plist || !filename)
		return 0;

	if (format == PLIST_FORMAT_XML)
		plist_to_xml(plist, &buffer, &length);
	else if (format == PLIST_FORMAT_BINARY)
		plist_to_bin(plist, &buffer, &length);
	else
		return 0;

	int res = buffer_write_to_filename(filename, buffer, length);

	free(buffer);

	return res;
}

static const char base64_str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64_pad = '=';

static char *base64encode(const unsigned char *buf, size_t size)
{
	if (!buf || !(size > 0)) return NULL;
	int outlen = (size / 3) * 4;
	char *outbuf = (char*)malloc(outlen+5); // 4 spare bytes + 1 for '\0'
	size_t n = 0;
	size_t m = 0;
	unsigned char input[3];
	unsigned int output[4];
	while (n < size) {
		input[0] = buf[n];
		input[1] = (n+1 < size) ? buf[n+1] : 0;
		input[2] = (n+2 < size) ? buf[n+2] : 0;
		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 3) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 15) << 2) + (input[2] >> 6);
		output[3] = input[2] & 63;
		outbuf[m++] = base64_str[(int)output[0]];
		outbuf[m++] = base64_str[(int)output[1]];
		outbuf[m++] = (n+1 < size) ? base64_str[(int)output[2]] : base64_pad;
		outbuf[m++] = (n+2 < size) ? base64_str[(int)output[3]] : base64_pad;
		n+=3;
	}
	outbuf[m] = 0; // 0-termination!
	return outbuf;
}

static void plist_node_print_to_stream(plist_t node, int* indent_level, FILE* stream);

static void plist_array_print_to_stream(plist_t node, int* indent_level, FILE* stream)
{
	/* iterate over items */
	int i, count;
	plist_t subnode = NULL;

	count = plist_array_get_size(node);

	for (i = 0; i < count; i++) {
		subnode = plist_array_get_item(node, i);
		fprintf(stream, "%*s", *indent_level, "");
		fprintf(stream, "%d: ", i);
		plist_node_print_to_stream(subnode, indent_level, stream);
	}
}

static void plist_dict_print_to_stream(plist_t node, int* indent_level, FILE* stream)
{
	/* iterate over key/value pairs */
	plist_dict_iter it = NULL;

	char* key = NULL;
	plist_t subnode = NULL;
	plist_dict_new_iter(node, &it);
	plist_dict_next_item(node, it, &key, &subnode);
	while (subnode)
	{
		fprintf(stream, "%*s", *indent_level, "");
		fprintf(stream, "%s", key);
		if (plist_get_node_type(subnode) == PLIST_ARRAY)
			fprintf(stream, "[%d]: ", plist_array_get_size(subnode));
		else
			fprintf(stream, ": ");
		free(key);
		key = NULL;
		plist_node_print_to_stream(subnode, indent_level, stream);
		plist_dict_next_item(node, it, &key, &subnode);
	}
	free(it);
}

static void plist_node_print_to_stream(plist_t node, int* indent_level, FILE* stream)
{
	char *s = NULL;
	char *data = NULL;
	double d;
	uint8_t b;
	uint64_t u = 0;
	struct timeval tv = { 0, 0 };

	plist_type t;

	if (!node)
		return;

	t = plist_get_node_type(node);

	switch (t) {
	case PLIST_BOOLEAN:
		plist_get_bool_val(node, &b);
		fprintf(stream, "%s\n", (b ? "true" : "false"));
		break;

	case PLIST_UINT:
		plist_get_uint_val(node, &u);
		fprintf(stream, "%" PRIu64 "\n", u);
		break;

	case PLIST_REAL:
		plist_get_real_val(node, &d);
		fprintf(stream, "%f\n", d);
		break;

	case PLIST_STRING:
		plist_get_string_val(node, &s);
		fprintf(stream, "%s\n", s);
		free(s);
		break;

	case PLIST_KEY:
		plist_get_key_val(node, &s);
		fprintf(stream, "%s: ", s);
		free(s);
		break;

	case PLIST_DATA:
		plist_get_data_val(node, &data, &u);
		if (u > 0) {
			s = base64encode((unsigned char*)data, u);
			free(data);
			if (s) {
				fprintf(stream, "%s\n", s);
				free(s);
			} else {
				fprintf(stream, "\n");
			}
		} else {
			fprintf(stream, "\n");
		}
		break;

	case PLIST_DATE:
		plist_get_date_val(node, (int32_t*)&tv.tv_sec, (int32_t*)&tv.tv_usec);
		{
			time_t ti = (time_t)tv.tv_sec + MAC_EPOCH;
			struct tm *btime = localtime(&ti);
			if (btime) {
				s = (char*)malloc(24);
 				memset(s, 0, 24);
				if (strftime(s, 24, "%Y-%m-%dT%H:%M:%SZ", btime) <= 0) {
					free (s);
					s = NULL;
				}
			}
		}
		if (s) {
			fprintf(stream, "%s\n", s);
			free(s);
		} else {
			fprintf(stream, "\n");
		}
		break;

	case PLIST_ARRAY:
		fprintf(stream, "\n");
		(*indent_level)++;
		plist_array_print_to_stream(node, indent_level, stream);
		(*indent_level)--;
		break;

	case PLIST_DICT:
		fprintf(stream, "\n");
		(*indent_level)++;
		plist_dict_print_to_stream(node, indent_level, stream);
		(*indent_level)--;
		break;

	default:
		break;
	}
}

LIBIMOBILEDEVICE_GLUE_API void plist_print_to_stream_with_indentation(plist_t plist, FILE* stream, unsigned int indentation)
{
	if (!plist || !stream)
		return;

	int indent = indentation;
	switch (plist_get_node_type(plist)) {
	case PLIST_DICT:
		plist_dict_print_to_stream(plist, &indent, stream);
		break;
	case PLIST_ARRAY:
		plist_array_print_to_stream(plist, &indent, stream);
		break;
	default:
		plist_node_print_to_stream(plist, &indent, stream);
	}
}

LIBIMOBILEDEVICE_GLUE_API void plist_print_to_stream(plist_t plist, FILE* stream)
{
	plist_print_to_stream_with_indentation(plist, stream, 0);
}
