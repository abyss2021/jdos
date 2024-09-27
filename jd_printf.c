/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-26 17:00:09
 * @LastEditors: 江小鉴 abyss_er@163.com
 * @LastEditTime: 2024-09-27 09:21:45
 * @FilePath: \jdos\jd_printf.c
 * @Description: jd_printf打印
 */
#include "jdos.h"

#include <stdarg.h>

extern UART_HandleTypeDef huart1;

void jd_transmit(jd_uint8_t* pData, jd_uint16_t Size) {
    HAL_UART_Transmit(&huart1, pData, Size, HAL_MAX_DELAY);
}

void jd_putchar(char ch) {
    jd_transmit((jd_uint8_t*)&ch, 1);
}

void usart_printstring(const char* str) {
    while (*str) {
        jd_putchar(*str++);
    }
}

void jd_printint(int num, int base, int width, int zero_pad) {
    char buffer[32];  // 足够存储整数和符号
    int i = 0;
    int is_negative = 0;

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    do {
        int digit = num % base;
        buffer[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        num /= base;
    } while (num > 0);

    if (is_negative) {
        buffer[i++] = '-';
    }

    while (i < width) {
        buffer[i++] = zero_pad ? '0' : ' ';
    }

    for (int j = i - 1; j >= 0; j--) {
        jd_putchar(buffer[j]);
    }
}

void jd_printfloat(float num, int precision, int width) {
    if (num < 0) {
        jd_putchar('-');
        num = -num;
    }

    int int_part = (int)num;
    float frac_part = num - int_part;

    jd_printint(int_part, 10, width - precision - 1, 0);
    jd_putchar('.');

    for (int i = 0; i < precision; i++) {
        frac_part *= 10;
        int digit = (int)frac_part;
        jd_putchar(digit + '0');
        frac_part -= digit;
    }
}

void jd_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;
            int width = 0;
            int precision = 6;  // 默认精度为6位小数
            int zero_pad = 0;

            // 解析宽度
            while (*format >= '0' && *format <= '9') {
                width = width * 10 + (*format - '0');
                format++;
            }

            // 解析精度
            if (*format == '.') {
                format++;
                precision = 0;
                while (*format >= '0' && *format <= '9') {
                    precision = precision * 10 + (*format - '0');
                    format++;
                }
            }

            // 解析对齐和填充
            if (*format == '0') {
                zero_pad = 1;
                format++;
            }

            switch (*format) {
                case 'c': {
                    char ch = (char)va_arg(args, int);
                    jd_putchar(ch);
                    break;
                }
                case 'd': {
                    int num = va_arg(args, int);
                    jd_printint(num, 10, width, zero_pad);
                    break;
                }
                case 'f': {
                    float num = (float)va_arg(args, double);
                    jd_printfloat(num, precision, width);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    usart_printstring(str);
                    break;
                }
                case 'x': {
                    int num = va_arg(args, int);
                    jd_printint(num, 16, width, zero_pad);
                    break;
                }
                case 'X': {
                    int num = va_arg(args, int);
                    jd_printint(num, 16, width, zero_pad);
                    break;
                }
                case 'p': {
                    void* ptr = va_arg(args, void*);
                    jd_printint((jd_uint32_t)ptr, 16, width, zero_pad);
                    break;
                }
                case 'u': {
                    unsigned int num = va_arg(args, unsigned int);
                    jd_printint(num, 10, width, zero_pad);
                    break;
                }
                case 'o': {
                    unsigned int num = va_arg(args, unsigned int);
                    jd_printint(num, 8, width, zero_pad);
                    break;
                }
                case 'b': {
                    unsigned int num = va_arg(args, unsigned int);
                    jd_printint(num, 2, width, zero_pad);
                    break;
                }
                case '%': {
                    jd_putchar('%');
                    break;
                }
                default:
                    jd_putchar(*format);
                    break;
            }
        } else {
            jd_putchar(*format);
        }
        format++;
    }

    va_end(args);
}
