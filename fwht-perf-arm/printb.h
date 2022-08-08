#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define for_endian(size) for (int i = 0; i < size; ++i)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define for_endian(size) for (int i = size - 1; i >= 0; --i)
#else
#error "Endianness not detected"
#endif

#define printb(value)                                   \
({                                                      \
        typeof(value) _v = value;                       \
        __printb((typeof(_v) *) &_v, sizeof(_v));       \
})

#define MSB_MASK 1 << (CHAR_BIT - 1)

void __printb(void *value, size_t size)
{
        unsigned char uc;
        unsigned char bits[CHAR_BIT + 1];

        bits[CHAR_BIT] = '\0';
        for_endian(size) {
                uc = ((unsigned char *) value)[i];
                memset(bits, '0', CHAR_BIT);
                for (int j = 0; uc && j < CHAR_BIT; ++j) {
                        if (uc & MSB_MASK)
                                bits[j] = '1';
                        uc <<= 1;
                }
                Serial.printf("%s ", bits);
        }
}

/*int main(void)
{
        uint8_t c1 = 0xff, c2 = 0x44;
        uint8_t c3 = c1 + c2;

        printb(c1);
        printb((char) 0xff);
        printb((short) 0xff);
        printb(0xff);
        printb(c2);
        printb(0x44);
        printb(0x4411ff01);
        printb((uint16_t) c3);
        printb('A');
        printf("\n");

        return 0;
}*/