#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "nbt.h"

#pragma GCC diagnostic ignored "-Wpointer-sign"

#define RED	0
#define GREEN	1
#define	BLUE	2
#define	ALPHA	3

typedef union {
	char a[4];
	uint32_t i;
	float f;
} color_int;

int main(int argc, char * argv[]) {

	if (argc != 2) {
		printf("usage: %s <filename.txt>\n", argv[0]);
		return 0;
	}

	int fd;
	fd = open(argv[1], O_RDONLY);

	uint8_t c;
	uint8_t reading_string = 0, reading_name = 1, reading_color = 0, reading_value = 0;
	uint8_t finished = 0;
	char s[256];
	int s_len = 0;
	nbt_t nbt;
	uint8_t * data;
	uint32_t data_len = 0;
	data = (uint8_t *)malloc(sizeof(*data)*32768);
	uint8_t which_color;
	color_int col;
	uint8_t channels = 0;

	data_len += addGroup("data", data);

	while (read(fd, &c, 1)) {
		if (c == '{')
			break;
	}

	while (read(fd, &c, 1)) {
		//putchar(c);
		if (reading_string && c != '"') {
			s[s_len] = c;
			s_len ++;
			s[s_len] = 0;
			continue;
		}

		switch (c) {
			case 'r':
				which_color = RED;
				break;
			case 'g':
				which_color = GREEN;
				break;
			case 'b':
				which_color = BLUE;
				break;
			case 'a':
				which_color = ALPHA;
		}

		switch (c) {
			case '"':
				if (!reading_string) {
					s[0] = 0;
					s_len = 0;
				}
				reading_string = !reading_string && !reading_color;
				if (s_len) {
					if (reading_name) {
						strcpy(nbt.name, s);
						nbt.name_length = strlen(s);
						s_len = 0;
						s[0] = 0;
					}
					else {
						finished = NBT_STRING;
					}
					//puts(s);
				}
				break;
			case ':':
				reading_name = 0;
				break;
			case '{':
				reading_color = 1;
				break;
			case '}':
				finished = NBT_COLOR;
				reading_color = 0;
				break;
			case ',':
				if (!reading_color)
					reading_name = 1;
				else if (reading_value) {
					if (!reading_color) 
						finished = NBT_FLOAT;
					reading_value = 0;
				}
				break;
			case 't':
			case 'f':
				finished = NBT_BOOLEAN;
				break;
			default:
				if (isdigit(c) || c == '.') {
					reading_value = 1;
					s[s_len] = c;
					s_len++;
					s[s_len] = 0;
					if (reading_color) {
						col.a[which_color] = atoi(s);
						channels ++;
					}
					else
						col.f = atof(s);
				}
				break;
		}

		color temp_color;
		switch (finished) {
			case NBT_BOOLEAN:
				data_len += addBool(nbt.name, c == 't', 0, data + data_len);
				//printf("%s %c\n", nbt.name, c);
				break;
			case NBT_INT:
			case NBT_FLOAT:
				data_len += addValue(nbt.name, col.f, 0, data + data_len);
				break;
			case NBT_COLOR:
				if (channels >= 3) {
					temp_color.r = col.a[0];
					temp_color.g = col.a[1];
					temp_color.b = col.a[2];
					temp_color.a = col.a[3];
					data_len += addColor(nbt.name, &temp_color, 0, data + data_len);
					//printf("%d %d %d\n", col.a[0], col.a[1], col.a[2]);
				}
				channels = 0;
				break;
			case NBT_STRING:
				data_len += addString(nbt.name, s, 0, data + data_len);
				s[0] = 0;
				s_len = 0;
				break;
		}
/*		if (finished)
			printf("0x%02x %s\n", finished, nbt.name); */
		finished = 0;

	}	
	close(fd);
	char filename[256];
	strcpy(filename, argv[1]);
	*strrchr(filename, '.') = 0;
	strcat(filename, ".dat");
	fd = open(filename, O_CREAT | O_WRONLY, 0664);
	write(fd, data, data_len);
	close(fd);
	return 0;
}

