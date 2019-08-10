/*
 * CRC-32 forcer (C)
 *
 * Copyright (c) 2017 Project Nayuki
 * https://www.nayuki.io/page/forcing-a-files-crc-to-any-value
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
 * along with this program (see COPYING.txt).
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "crc_spoof.h"


/* Forward declarations */

const char *crc_spoof_modify_file_crc32(FakeFileHandle *f, uint64_t offset, uint32_t newcrc, bool printstatus);

uint32_t get_crc32_and_length(FakeFileHandle *f, uint64_t *length);
static void fseek64(FakeFileHandle *f, uint64_t offset);
uint32_t crc_spoof_reverse_bits(uint32_t x);

static uint64_t multiply_mod(uint64_t x, uint64_t y);
static uint64_t pow_mod(uint64_t x, uint64_t y);
static void divide_and_remainder(uint64_t x, uint64_t y, uint64_t *q, uint64_t *r);
static uint64_t reciprocal_mod(uint64_t x);
static int get_degree(uint64_t x);


/*---- Main application ----*/
/*int main(int argc, char *argv[]) {
    // Handle arguments
    if (argc != 4) {
        fprintf(stderr, "Usage: %s FileName ByteOffset NewCrc32Value\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse and check file offset argument
    uint64_t offset;
    if (sscanf(argv[2], "%" SCNu64, &offset) != 1) {
        fprintf(stderr, "Error: Invalid byte offset\n");
        return EXIT_FAILURE;
    }
    char temp[21] = {0};
    sprintf(temp, "%" PRIu64, offset);
    if (strcmp(temp, argv[2]) != 0) {
        fprintf(stderr, "Error: Invalid byte offset\n");
        return EXIT_FAILURE;
    }

    // Parse and check new CRC argument
    uint32_t newcrc;
    if (strlen(argv[3]) != 8 || argv[3][0] == '+' || argv[3][0] == '-'
            || sscanf(argv[3], "%" SCNx32, &newcrc) != 1) {
        fprintf(stderr, "Error: Invalid new CRC-32 value\n");
        return EXIT_FAILURE;
    }
    newcrc = crc_spoof_reverse_bits(newcrc);

    // Process the file
    const char *errmsg = crc_spoof_modify_file_crc32(argv[1], offset, newcrc, true);
    if (errmsg == NULL)
        return EXIT_SUCCESS;
    else if (0 < strcmp(errmsg, "I/O error: ") && strcmp(errmsg, "I/O error:!") < 0)  // Prefix test
        perror(errmsg);
    else
        fprintf(stderr, "%s\n", errmsg);
    return EXIT_FAILURE;
}*/


/*---- Main function ----*/

// Public library function. Returns NULL if successful, a string starting with "I/O error: "
// if an I/O error occurred (please see perror()), or a string if some other error occurred.
const char *crc_spoof_modify_file_crc32(FakeFileHandle *f, uint64_t offset, uint32_t newcrc, bool printstatus) {
    // Read entire file and calculate original CRC-32 value.
    // Note: We can't use crc_spoof_fake_fseek(f, 0, SEEK_END) + ftell(f) to determine the length of the file, due to undefined behavior.
    // To be portable, we also avoid using POSIX crc_spoof_fake_fseeko()+ftello() or Windows GetFileSizeEx()/_filelength().
    uint64_t length;
    uint32_t crc = get_crc32_and_length(f, &length);
    if (offset > UINT64_MAX - 4 || offset + 4 > length) {
        crc_spoof_fake_fclose(f);
        return "Error: Byte offset plus 4 exceeds file length";
    }
    if (printstatus)
        fprintf(stdout, "Original CRC-32: %08" PRIX32 "\n", crc_spoof_reverse_bits(crc));

    // Compute the change to make
    uint32_t delta = crc ^ newcrc;
    delta = (uint32_t)multiply_mod(reciprocal_mod(pow_mod(2, (length - offset) * 8)), delta);

    // Patch 4 bytes in the file
    fseek64(f, offset);
    for (int i = 0; i < 4; i++) {
        int b = crc_spoof_fake_fgetc(f);
        if (b == EOF) {
            crc_spoof_fake_fclose(f);
            return "I/O error: fgetc";
        }
        b ^= (int)((crc_spoof_reverse_bits(delta) >> (i * 8)) & 0xFF);
        if (crc_spoof_fake_fseek(f, -1, SEEK_CUR) != 0) {
            crc_spoof_fake_fclose(f);
            return "I/O error: crc_spoof_fake_fseek";
        }
        if (crc_spoof_fake_fputc(b, f) == EOF) {
            crc_spoof_fake_fclose(f);
            return "I/O error: fputc";
        }
        if (crc_spoof_fake_fflush(f) == EOF) {
            crc_spoof_fake_fclose(f);
            return "I/O error: fflush";
        }
    }
    if (printstatus)
        fprintf(stdout, "Computed and wrote patch\n");

    // Recheck entire file
    bool match = get_crc32_and_length(f, &length) == newcrc;
    crc_spoof_fake_fclose(f);
    if (match) {
        if (printstatus)
            fprintf(stdout, "New CRC-32 successfully verified\n");
        return NULL;  // Success
    } else
        return "Assertion error: Failed to update CRC-32 to desired value";
}


/*---- Utilities ----*/

// Generator polynomial. Do not modify, because there are many dependencies
static const uint64_t POLYNOMIAL = UINT64_C(0x104C11DB7);


uint32_t get_crc32_and_length(FakeFileHandle *f, uint64_t *length) {
    crc_spoof_fake_rewind(f);
    uint32_t crc = UINT32_C(0xFFFFFFFF);
    *length = 0;
    while (true) {
        char buffer[32 * 1024];
        size_t n = crc_spoof_fake_fread(buffer, sizeof(buffer[0]), sizeof(buffer) / sizeof(buffer[0]), f);
        for (size_t i = 0; i < n; i++) {
            for (int j = 0; j < 8; j++) {
                uint32_t bit = ((uint8_t)buffer[i] >> j) & 1;
                crc ^= bit << 31;
                bool xor = (crc >> 31) != 0;
                crc = (crc & UINT32_C(0x7FFFFFFF)) << 1;
                if (xor)
                    crc ^= (uint32_t)POLYNOMIAL;
            }
        }
        *length += n;
        if (crc_spoof_fake_feof(f) != 0)
            return ~crc;
    }
}


static void fseek64(FakeFileHandle *f, uint64_t offset) {
    crc_spoof_fake_rewind(f);
    while (offset > 0) {
        unsigned long n = LONG_MAX;
        if (offset < n)
            n = (unsigned long)offset;
        crc_spoof_fake_fseek(f, (long)n, SEEK_CUR);
        offset -= n;
    }
}


uint32_t crc_spoof_reverse_bits(uint32_t x) {
    uint32_t result = 0;
    for (int i = 0; i < 32; i++)
        result = (result << 1) | ((x >> i) & 1U);
    return result;
}


/*---- Polynomial arithmetic ----*/

// Returns polynomial x multiplied by polynomial y modulo the generator polynomial.
static uint64_t multiply_mod(uint64_t x, uint64_t y) {
    // Russian peasant multiplication algorithm
    uint64_t z = 0;
    while (y != 0) {
        z ^= x * (y & 1);
        y >>= 1;
        x <<= 1;
        if (((x >> 32) & 1) != 0)
            x ^= POLYNOMIAL;
    }
    return z;
}


// Returns polynomial x to the power of natural number y modulo the generator polynomial.
static uint64_t pow_mod(uint64_t x, uint64_t y) {
    // Exponentiation by squaring
    uint64_t z = 1;
    while (y != 0) {
        if ((y & 1) != 0)
            z = multiply_mod(z, x);
        x = multiply_mod(x, x);
        y >>= 1;
    }
    return z;
}


// Computes polynomial x divided by polynomial y, returning the quotient and remainder.
static void divide_and_remainder(uint64_t x, uint64_t y, uint64_t *q, uint64_t *r) {
    if (y == 0) {
        fprintf(stderr, "Division by zero\n");
        exit(EXIT_FAILURE);
    }
    if (x == 0) {
        *q = 0;
        *r = 0;
        return;
    }

    int ydeg = get_degree(y);
    uint64_t z = 0;
    for (int i = get_degree(x) - ydeg; i >= 0; i--) {
        if (((x >> (i + ydeg)) & 1) != 0) {
            x ^= y << i;
            z |= (uint64_t)1 << i;
        }
    }
    *q = z;
    *r = x;
}


// Returns the reciprocal of polynomial x with respect to the generator polynomial.
static uint64_t reciprocal_mod(uint64_t x) {
    // Based on a simplification of the extended Euclidean algorithm
    uint64_t y = x;
    x = POLYNOMIAL;
    uint64_t a = 0;
    uint64_t b = 1;
    while (y != 0) {
        uint64_t q, r;
        divide_and_remainder(x, y, &q, &r);
        uint64_t c = a ^ multiply_mod(q, b);
        x = y;
        y = r;
        a = b;
        b = c;
    }
    if (x == 1)
        return a;
    else {
        fprintf(stderr, "Reciprocal does not exist\n");
        exit(EXIT_FAILURE);
    }
}


static int get_degree(uint64_t x) {
    int result = -1;
    for (; x != 0; x >>= 1)
        result++;
    return result;
}

void crc_spoof_fake_fclose(FakeFileHandle *f) {
}

int crc_spoof_fake_fseek(FakeFileHandle *f, long offset, int type) {
    switch(type) {
        case SEEK_SET:
            f->offset = offset;
            break;
        case SEEK_CUR:
            f->offset += offset;
            break;
        case SEEK_END:
            f->offset = f->size - offset;
            break;
    }
    return 0;
}

void crc_spoof_fake_rewind(FakeFileHandle *f) {
    f->offset = 0;
}

int crc_spoof_fake_fflush(FakeFileHandle *f) {
    return 0;
}

int crc_spoof_fake_fgetc(FakeFileHandle *f) {
    if(crc_spoof_fake_feof(f)) {
        return EOF;
    }
    else {
        return f->data[f->offset++];
    }
}

int crc_spoof_fake_fputc(int c, FakeFileHandle *f) {
    if(crc_spoof_fake_feof(f)) {
        return EOF;
    }
    else {
        f->data[f->offset++] = (char)c;
        return c;
    }
}

int crc_spoof_fake_feof(FakeFileHandle *f) {
    return f->offset == f->size;
}

size_t crc_spoof_fake_fread(void *ptr, size_t size, size_t count, FakeFileHandle *f) {
    char *cptr = (char *)ptr;
    size_t remaining_size = f->size - f->offset;
    if(remaining_size < size * count) {
        count = remaining_size / size;
    }
    size_t total_size = size * count;

    memcpy(cptr, f->data + f->offset, total_size);
    f->offset += total_size;
    return count;
}
