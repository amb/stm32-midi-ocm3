
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

void itoa7(int16_t num, char *str) {
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
    if(sign) {
        str[i++] = '-';
    }

    // Reverse the string
    reverse(str, i);

    // Null terminate the string
    str[i] = '\0';
}
