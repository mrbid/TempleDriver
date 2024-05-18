#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "haxstring.h"

int append(int output_fd, char byte, char *was_quoted) {
	switch(byte) {
	case 0:
		if (*was_quoted) {
			WRITES(output_fd, STRING("'"));
			*was_quoted = 0;
		}
		WRITES(output_fd, STRING(",0"));
		break;
	case '\n':
		if (*was_quoted) {
			WRITES(output_fd, STRING("'"));
			*was_quoted = 0;
		}
		WRITES(output_fd, STRING(",0Ah"));
		break;
	case '\r':
		if (*was_quoted) {
			WRITES(output_fd, STRING("'"));
			*was_quoted = 0;
		}
		WRITES(output_fd, STRING(",0Dh"));
		break;
	case 0x1A:
		if (*was_quoted) {
			WRITES(output_fd, STRING("'"));
			*was_quoted = 0;
		}
		WRITES(output_fd, STRING(",1Ah"));
		break;
	case '\'':
		if (*was_quoted) {
			WRITES(output_fd, STRING("'"));
			*was_quoted = 0;
		}
		WRITES(output_fd, STRING(",`'`"));
		break;
	default:
		if (!*was_quoted) {
			WRITES(output_fd, STRING(",'"));
			*was_quoted = 1;
		}
		write(output_fd, &byte, 1);
		break;
	}

	return 0; // TODO: return error if one of the writes failed?
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <filename.ply>\r\n", argv[0]);
		return 1;
	}

	size_t len = strlen(argv[1]);
	if (len < 4 || argv[1][len-4] != '.' || argv[1][len-3] != 'p' || argv[1][len-2] != 'l' || argv[1][len-1] != 'y') {
		fprintf(stderr, "This is not a .ply file.\r\n");
		return 1;
	}
	char writepath[len+1];
	memcpy(writepath, argv[1], len-3);
	writepath[len - 3] = 'a';
	writepath[len - 2] = 's';
	writepath[len - 1] = 'm';
	writepath[len - 0] = 0;

	int input_fd = open(argv[1], O_RDONLY, 0);
	if (input_fd == -1) {
		fprintf(stderr, "Cannot read `%s'.\r\n", argv[1]);
		return 1;
	}

	int output_fd = open(writepath, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (output_fd == -1) {
		fprintf(stderr, "Cannot write to `%s'.\r\n", writepath);
		return 1;
	}

	writepath[len - 3] = 'h';
	writepath[len - 2] = 0;

	int header_fd = open(writepath, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (header_fd == -1) {
		fprintf(stderr, "Cannot write to `%s'.\r\n", writepath);
		return 1;
	}

	writepath[len - 4] = 0;

	struct stat stat_struct;
	signed char retval = stat(argv[1], &stat_struct);
	if (retval == -1) {
		fprintf(stderr, "Cannot stat `%s'.\r\n", argv[1]);
		return 1;
	} else if (stat_struct.st_size < 0) {
		fprintf(stderr, "stat sucks\r\n");
		return 255;
	}

	size_t input_len = (size_t)stat_struct.st_size;

	char __attribute__((__may_alias__)) *buffer = mmap(0, input_len, PROT_READ, MAP_SHARED | MAP_POPULATE, input_fd, 0);
	if (buffer == 0) {
		fprintf(stderr, "Unable to mmap `%s'.\r\n", argv[1]);
		return 1;
	}

	close(input_fd);

	size_t offset = 0;
	while (offset < input_len && buffer[offset] != '\n') // skip first two lines (ply\nformat binary_little_endian 1.0\n)
		offset++;
	offset++;
	while (offset < input_len && buffer[offset] != '\n')
		offset++;
	offset++;

	unsigned char stage = 0;
	struct data_points {
		size_t x_len;
		size_t x_off;
		size_t y_len;
		size_t y_off;
		size_t z_len;
		size_t z_off;

		size_t nx_len;
		size_t nx_off;
		size_t ny_len;
		size_t ny_off;
		size_t nz_len;
		size_t nz_off;

		size_t r_len;
		size_t r_off;
		size_t g_len;
		size_t g_off;
		size_t b_len;
		size_t b_off;
		size_t a_len;
		size_t a_off;
	} data_points = {0};
	struct indices {
		size_t list_len;
		size_t individual_len;
	} indices = {0};

	size_t next = 0;
	size_t counts[2];
	while (1) {
		struct string line;
		line.data = &buffer[offset];

		while (offset < input_len && buffer[offset] != '\n')
			offset++;

		line.len = (size_t)(&buffer[offset] - line.data);
		offset++;

		size_t word_count = 1;
		for (size_t i = 0; i < line.len; i++)
			if (line.data[i] == ' ')
				word_count++;

		struct string words[word_count];

		size_t x = 0;
		size_t y = 0;
		while (1) {
			words[x].data = &line.data[y];
			while (y < line.len && line.data[y] != ' ')
				y++;

			words[x].len = (size_t)(&line.data[y] - words[x].data);
			y++;
			x++;

			if (y >= line.len)
				break;
		}

		if (STRING_EQ(words[0], STRING("end_header"))) {
			break;
		} else if (STRING_EQ(words[0], STRING("comment"))) {
			continue;
		} else if (STRING_EQ(words[0], STRING("element"))) {
			if (STRING_EQ(words[1], STRING("vertex"))) {
				stage = 0;
			} else if (STRING_EQ(words[1], STRING("face"))) {
				stage = 1;
			} else {
				WRITES(2, STRING("Unknown element type `"));
				WRITES(2, words[1]);
				WRITES(2, STRING("'.\r\n"));
				return 1;
			}
			char err;
			counts[stage] = str_to_unsigned(words[2], &err);
			if (err) {
				WRITES(2, STRING("element count `"));
				WRITES(2, words[2]);
				WRITES(2, STRING("' is not a valid number.\r\n"));
				return 1;
			}
		} else if (STRING_EQ(words[0], STRING("property"))) {
			size_t len;
			if (STRING_EQ(words[1], STRING("float"))) {
				len = sizeof(float);
			} else if (STRING_EQ(words[1], STRING("uchar"))) {
				len = sizeof(unsigned char);
			} else if (STRING_EQ(words[1], STRING("list"))) {
				if (stage == 0) {
					WRITES(2, STRING("List found in vertex element; this is not handled.\r\n"));
					return 1;
				}
				if (STRING_EQ(words[2], STRING("uchar"))) {
					indices.list_len = sizeof(unsigned char);
				} else {
					WRITES(2, STRING("Unknown type `"));
					WRITES(2, words[2]);
					WRITES(2, STRING("' for list count.\r\n"));
					return 1;
				}

				if (STRING_EQ(words[3], STRING("uint"))) {
					indices.individual_len = sizeof(unsigned int);
				} else {
					WRITES(2, STRING("Unknown type `"));
					WRITES(2, words[2]);
					WRITES(2, STRING("' for list type.\r\n"));
					return 1;
				}

				if ( ! STRING_EQ(words[4], STRING("vertex_indices"))) {
					WRITES(2, STRING("Unknown property `"));
					WRITES(2, words[4]);
					WRITES(2, STRING("' found in list.\r\n"));
				}
				continue;
			} else {
				WRITES(2, STRING("Unknown type `"));
				WRITES(2, words[1]);
				WRITES(2, STRING("'.\r\n"));
				return 1;
			}
			if (stage != 0) {
				WRITES(2, STRING("Non-list found in vertex element; this is not handled.\r\n"));
				return 1;
			}

			if (STRING_EQ(words[2], STRING("x"))) {
				data_points.x_len = len;
				data_points.x_off = next;
			} else if (STRING_EQ(words[2], STRING("y"))) {
				data_points.y_len = len;
				data_points.y_off = next;
			} else if (STRING_EQ(words[2], STRING("z"))) {
				data_points.z_len = len;
				data_points.z_off = next;
			} else if (STRING_EQ(words[2], STRING("nx"))) {
				data_points.nx_len = len;
				data_points.nx_off = next;
			} else if (STRING_EQ(words[2], STRING("ny"))) {
				data_points.ny_len = len;
				data_points.ny_off = next;
			} else if (STRING_EQ(words[2], STRING("nz"))) {
				data_points.nz_len = len;
				data_points.nz_off = next;
			} else if (STRING_EQ(words[2], STRING("red"))) {
				data_points.r_len = len;
				data_points.r_off = next;
			} else if (STRING_EQ(words[2], STRING("green"))) {
				data_points.g_len = len;
				data_points.g_off = next;
			} else if (STRING_EQ(words[2], STRING("blue"))) {
				data_points.b_len = len;
				data_points.b_off = next;
			} else if (STRING_EQ(words[2], STRING("alpha"))) {
				data_points.a_len = len;
				data_points.a_off = next;
			} else {
				WRITES(2, STRING("Unknown location `"));
				WRITES(2, words[2]);
				WRITES(2, STRING("'.\r\n"));
				return 1;
			}

			next += len;
		} else {
			WRITES(2, STRING("Unknown command `"));
			WRITES(2, words[0]);
			WRITES(2, STRING("'.\r\n"));
			return 1;
		}
	}
	size_t starting_offset = offset;

	WRITES(output_fd, STRING("global "));
	write(output_fd, writepath, strlen(writepath));
	WRITES(output_fd, STRING("_vertices\r\n"));
	write(output_fd, writepath, strlen(writepath));
	WRITES(output_fd, STRING("_vertices:\r\ndb '"));
	char was_string = 1;
	for (size_t i = 0; i < counts[0]; i++) {
		for (size_t n = 0; n < data_points.x_len; n++)
			append(output_fd, buffer[offset + n + data_points.x_off], &was_string);

		for (size_t n = 0; n < data_points.y_len; n++)
			append(output_fd, buffer[offset + n + data_points.y_off], &was_string);

		for (size_t n = 0; n < data_points.z_len; n++)
			append(output_fd, buffer[offset + n + data_points.z_off], &was_string);

		offset += next;
	}
	if (was_string)
		WRITES(output_fd, STRING("'"));

	WRITES(output_fd, STRING("\r\n"));

	offset = starting_offset;

	WRITES(output_fd, STRING("global "));
	write(output_fd, writepath, strlen(writepath));
	WRITES(output_fd, STRING("_normals\r\n"));
	write(output_fd, writepath, strlen(writepath));
	WRITES(output_fd, STRING("_normals:\r\ndb '"));
	was_string = 1;
	for (size_t i = 0; i < counts[0]; i++) {
		for (size_t n = 0; n < data_points.nx_len; n++)
			append(output_fd, buffer[offset + n + data_points.nx_off], &was_string);

		for (size_t n = 0; n < data_points.ny_len; n++)
			append(output_fd, buffer[offset + n + data_points.ny_off], &was_string);

		for (size_t n = 0; n < data_points.nz_len; n++)
			append(output_fd, buffer[offset + n + data_points.nz_off], &was_string);

		offset += next;
	}
	if (was_string)
		WRITES(output_fd, STRING("'"));

	WRITES(output_fd, STRING("\r\n"));

	offset = starting_offset;

	WRITES(output_fd, STRING("global "));
	write(output_fd, writepath, strlen(writepath));
	WRITES(output_fd, STRING("_colors\r\n"));
	write(output_fd, writepath, strlen(writepath));
	WRITES(output_fd, STRING("_colors:\r\ndb '"));
	was_string = 1;
	for (size_t i = 0; i < counts[0]; i++) {
		for (size_t n = 0; n < data_points.r_len; n++)
			append(output_fd, buffer[offset + n + data_points.r_off], &was_string);

		for (size_t n = 0; n < data_points.g_len; n++)
			append(output_fd, buffer[offset + n + data_points.g_off], &was_string);

		for (size_t n = 0; n < data_points.b_len; n++)
			append(output_fd, buffer[offset + n + data_points.b_off], &was_string);

		offset += next;
	}
	if (was_string)
		WRITES(output_fd, STRING("'"));

	WRITES(output_fd, STRING("\r\n"));

	// offset now aligns with the next element section
	starting_offset = offset;

	size_t max_index = 0;
	for (size_t i = 0; i < counts[1]; i++) {
		size_t len;
		switch(indices.list_len) {
		case 1:
			len = *((uint8_t*)(buffer + offset));
			offset += 1;
			break;
		case 2:
			len = *((uint16_t*)(buffer + offset));
			offset += 2;
			break;
		case 4:
			len = *((uint32_t*)(buffer + offset));
			offset += 4;
			break;
		case 8:
			len = *((uint64_t*)(buffer + offset));
			offset += 8;
			break;
		}

		if (len != 3) {
			WRITES(1, STRING("Indice count is not three, non-triangle detected, fix your ply.\r\n"));
			return 1;
		}

		for (size_t x = 0; x < len; x++) {
			switch(indices.individual_len) {
			case 1:
				if (max_index < *((uint8_t*)(buffer + offset + (x * indices.individual_len))))
					max_index = *((uint8_t*)(buffer + offset + (x * indices.individual_len)));
				break;
			case 2:
				if (max_index < *((uint16_t*)(buffer + offset + (x * indices.individual_len))))
					max_index = *((uint16_t*)(buffer + offset + (x * indices.individual_len)));
				break;
			case 4:
				if (max_index < *((uint32_t*)(buffer + offset + (x * indices.individual_len))))
					max_index = *((uint32_t*)(buffer + offset + (x * indices.individual_len)));
				break;
			case 8:
				if (max_index < *((uint64_t*)(buffer + offset + (x * indices.individual_len))))
					max_index = *((uint64_t*)(buffer + offset + (x * indices.individual_len)));
				break;
			}
		}
		offset += len * indices.individual_len;
	}
	size_t needed_len;
	if (max_index > 0xFFFFFFFFUL)
		needed_len = 8;
	else if (max_index > 0xFFFFUL)
		needed_len = 4;
	else if (max_index > 0xFFUL)
		needed_len = 2;
	else
		needed_len = 1;

	offset = starting_offset;

	WRITES(output_fd, STRING("global "));
	write(output_fd, writepath, strlen(writepath));
	WRITES(output_fd, STRING("_indices\r\n"));
	write(output_fd, writepath, strlen(writepath));
	WRITES(output_fd, STRING("_indices:\r\ndb '"));
	was_string = 1;

	for (size_t i = 0; i < counts[1]; i++) {
		offset += indices.list_len;
		for (size_t x = 0; x < 3; x++) {
			for (size_t y = 0; y < needed_len; y++)
				append(output_fd, buffer[offset + y], &was_string);

			offset += indices.individual_len;
		}
	}

	if (was_string)
		WRITES(output_fd, STRING("'"));

	WRITES(output_fd, STRING("\r\n"));

	WRITES(header_fd, STRING("#ifndef "));
	write(header_fd, writepath, strlen(writepath));
	WRITES(header_fd, STRING("_H\r\n#define "));
	write(header_fd, writepath, strlen(writepath));
	WRITES(header_fd, STRING("_H\r\n\r\nextern const GLfloat "));
	write(header_fd, writepath, strlen(writepath));
	WRITES(header_fd, STRING("_vertices[];\r\nextern const GLfloat "));
	write(header_fd, writepath, strlen(writepath));
	WRITES(header_fd, STRING("_normals[];\r\nextern const GLubyte "));
	write(header_fd, writepath, strlen(writepath));
	if (needed_len == 1)
		WRITES(header_fd, STRING("_colors[];\r\nextern const GLubyte "));
	else if (needed_len == 2)
		WRITES(header_fd, STRING("_colors[];\r\nextern const GLushort "));
	else if (needed_len == 4)
		WRITES(header_fd, STRING("_colors[];\r\nextern const GLuint "));
	else if (needed_len == 8)
		WRITES(header_fd, STRING("_colors[];\r\nextern const GLuint64 "));

	write(header_fd, writepath, strlen(writepath));
	WRITES(header_fd, STRING("_indices[];\r\nstatic const GLsizeiptr "));
	write(header_fd, writepath, strlen(writepath));
	WRITES(header_fd, STRING("_numind = "));
	dprintf(header_fd, "%lu", counts[1]);
	WRITES(header_fd, STRING(";\r\nstatic const GLsizeiptr "));
	write(header_fd, writepath, strlen(writepath));
	WRITES(header_fd, STRING("_numvert = "));
	dprintf(header_fd, "%lu", counts[0]);
	WRITES(header_fd, STRING(";\r\nstatic const GLsizei "));
	write(header_fd, writepath, strlen(writepath));
	if (needed_len == 1)
		WRITES(header_fd, STRING("_GL_TYPE = GL_UNSIGNED_BYTE;\r\n"));
	else if (needed_len == 2)
		WRITES(header_fd, STRING("_GL_TYPE = GL_UNSIGNED_SHORT;\r\n"));
	if (needed_len == 4)
		WRITES(header_fd, STRING("_GL_TYPE = GL_UNSIGNED_INT;\r\n"));
	if (needed_len == 8)
		WRITES(header_fd, STRING("_GL_TYPE = GL_UNSIGNED_INT64;\r\n"));

	WRITES(header_fd, STRING("\r\n#endif\r\n"));

	return 0;
}
