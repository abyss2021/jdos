/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-26 17:00:09
 * @LastEditors: 江小鉴 abyss_er@163.com
 * @LastEditTime: 2024-09-27 10:32:03
 * @FilePath: \jdos\jd_printf.c
 * @Description: jd_printf打印函数实现
 */
#include "jdos.h"
#include <stdarg.h> // 包含标准变长参数库

// 外部定义的UART句柄
extern UART_HandleTypeDef huart1;

/**
 * @description: 通过UART发送数据
 * @param {jd_uint8_t*} pData 数据指针
 * @param {jd_uint16_t} Size 数据大小
 * @return {*}
 */
void jd_transmit(jd_uint8_t *pData, jd_uint16_t Size)
{
    //此处对接硬件发送的接口
    HAL_UART_Transmit(&huart1, pData, Size, HAL_MAX_DELAY);
}

/**
 * @description:  发送单个字符
 * @param {char} ch 要发送的字符
 * @return {*}
 */
void jd_putchar(char ch)
{
    jd_transmit((jd_uint8_t *)&ch, 1);
}

/**
 * @description:  发送字符串
 * @param {char*} str 要发送的字符串
 * @return {*}
 */
void jd_printstring(const char *str)
{
    while (*str)
    {
        jd_putchar(*str++);
    }
}

/**
 * @description:  打印整数
 * @param {int} num 要打印的整数
 * @param {int} base 进制（如10进制、16进制等）
 * @param {int} width 最小宽度
 * @param {int} zero_pad 是否用0填充
 * @return {*}
 */
void jd_printint(int num, int base, int width, int zero_pad)
{
    char buffer[32]; // 足够存储整数和符号
    int i = 0;
    int is_negative = 0;

    if (num < 0)
    {
        is_negative = 1;
        num = -num;
    }

    do
    {
        int digit = num % base;
        buffer[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'A');
        num /= base;
    } while (num > 0);

    if (is_negative)
    {
        buffer[i++] = '-';
    }

    while (i < width)
    {
        buffer[i++] = zero_pad ? '0' : ' ';
    }

    for (int j = i - 1; j >= 0; j--)
    {
        jd_putchar(buffer[j]);
    }
}

/**
 * @description: 打印浮点数
 * @param {float} num  要打印的浮点数
 * @param {int} precision 小数点后的精度
 * @param {int} width 最小宽度
 * @return {*}
 */
void jd_printfloat(float num, int precision, int width)
{
    if (num < 0)
    {
        jd_putchar('-');
        num = -num;
    }

    int int_part = (int)num;
    float frac_part = num - int_part;

    jd_printint(int_part, 10, width - precision - 1, 0);
    jd_putchar('.');

    for (int i = 0; i < precision; i++)
    {
        frac_part *= 10;
        int digit = (int)frac_part;
        jd_putchar(digit + '0');
        frac_part -= digit;
    }
}

/**
 * @description:  格式化打印函数
 * @param {char*} format  格式化字符串
 * @param ... 可变参数列表
 * @return {*}
 */
void jd_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    while (*format)
    {
        if (*format == '%')
        {
            format++;
            int width = 0;
            int precision = 6; // 默认精度为6位小数
            int zero_pad = 0;

            // 解析宽度
            while (*format >= '0' && *format <= '9')
            {
                width = width * 10 + (*format - '0');
                format++;
            }

            // 解析精度
            if (*format == '.')
            {
                format++;
                precision = 0;
                while (*format >= '0' && *format <= '9')
                {
                    precision = precision * 10 + (*format - '0');
                    format++;
                }
            }

            // 解析对齐和填充
            if (*format == '0')
            {
                zero_pad = 1;
                format++;
            }

            switch (*format)
            {
            case 'c':
            {
                char ch = (char)va_arg(args, int);
                jd_putchar(ch);
                break;
            }
            case 'd':
            {
                int num = va_arg(args, int);
                jd_printint(num, 10, width, zero_pad);
                break;
            }
            case 'f':
            {
                float num = (float)va_arg(args, double);
                jd_printfloat(num, precision, width);
                break;
            }
            case 's':
            {
                const char *str = va_arg(args, const char *);
                jd_printstring(str);
                break;
            }
            case 'x':
            {
                int num = va_arg(args, int);
                jd_printint(num, 16, width, zero_pad);
                break;
            }
            case 'X':
            {
                int num = va_arg(args, int);
                jd_printint(num, 16, width, zero_pad);
                break;
            }
            case 'p':
            {
                void *ptr = va_arg(args, void *);
                jd_printint((jd_uint32_t)ptr, 16, width, zero_pad);
                break;
            }
            case 'u':
            {
                unsigned int num = va_arg(args, unsigned int);
                jd_printint(num, 10, width, zero_pad);
                break;
            }
            case 'o':
            {
                unsigned int num = va_arg(args, unsigned int);
                jd_printint(num, 8, width, zero_pad);
                break;
            }
            case 'b':
            {
                unsigned int num = va_arg(args, unsigned int);
                jd_printint(num, 2, width, zero_pad);
                break;
            }
            case '%':
            {
                jd_putchar('%');
                break;
            }
            default:
                jd_putchar(*format);
                break;
            }
        }
        else
        {
            jd_putchar(*format);
        }
        format++;
    }

    va_end(args);
}
