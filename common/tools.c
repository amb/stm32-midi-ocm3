
#include "tools.h"

void reverse(char *str, int length) {
    int start = 0;
    int end = length - 1;
    while(start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void itoa7(int32_t num, char *str) {
    int16_t i = 0;
    bool sign = false;

    // Handle negative numbers
    if(num < 0) {
        num = -num;
        sign = true;
    }

    // Handle zero explicitly
    if(num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    // Extract digits
    while(num != 0) {
        str[i++] = (num % 10) + '0';
        num /= 10;
    }

    // Add sign if the number was negative
    if(sign) { str[i++] = '-'; }

    // Reverse the string
    reverse(str, i);

    // Null terminate the string
    str[i] = '\0';
}


/* compute exp2(a) in s15.16 fixed-point arithmetic, -16 < a < 15 */
int32_t fixed_exp2 (int32_t a)
{
    int32_t i, f, r, s;
    /* split a = i + f, such that f in [-0.5, 0.5] */
    i = (a + 0x8000) & ~0xffff; // 0.5
    f = a - i;   
    s = ((15 << 16) - i) >> 16;
    /* minimax approximation for exp2(f) on [-0.5, 0.5] */
    r = 0x00000e20;                 // 5.5171669058037949e-2
    r = (r * f + 0x3e1cc333) >> 17; // 2.4261112219321804e-1
    r = (r * f + 0x58bd46a6) >> 16; // 6.9326098546062365e-1
    r = r * f + 0x7ffde4a3;         // 9.9992807353939517e-1
    return (uint32_t)r >> s;
}

