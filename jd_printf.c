/*
 * @Author: 江小鉴 abyss_er@163.com
 * @Date: 2024-09-26 17:00:09
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-09-27 18:19:40
 * @FilePath: \jdos\jd_printf.c
 * @Description: jd_printf打印函数实现
 */

#include "jdos.h"
#include <stdarg.h> // 包含标准变长参数库
#ifdef JD_PRINTF_ENABLE

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
 * @param {jd_int8_t} ch 要发送的字符
 * @return {*}
 */
void jd_putjd_int8_t(jd_int8_t ch)
{
    jd_transmit((jd_uint8_t *)&ch, 1);
}

/**
 * @description:  发送字符串
 * @param {jd_int8_t*} str 要发送的字符串
 * @return {*}
 */
void jd_printstring(const jd_int8_t *str)
{
    while (*str)
    {
        jd_putjd_int8_t(*str++);
    }
}

/**
 * @description:  打印整数
 * @param {jd_int32_t} num 要打印的整数
 * @param {jd_int32_t} base 进制（如10进制、16进制等）
 * @param {jd_int32_t} width 最小宽度
 * @param {jd_int32_t} zero_pad 是否用0填充
 * @return {*}
 */
void jd_printint(jd_int32_t num, jd_int32_t base, jd_int32_t width, jd_int32_t zero_pad)
{
    jd_int8_t buffer[32]; // 足够存储整数和符号
    jd_int32_t i = 0;
    jd_int32_t is_negative = 0;

    if (num < 0)
    {
        is_negative = 1;
        num = -num;
    }

    do
    {
        jd_int32_t digit = num % base;
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

    for (jd_int32_t j = i - 1; j >= 0; j--)
    {
        jd_putjd_int8_t(buffer[j]);
    }
}

/**
 * @description: 打印浮点数
 * @param {float} num  要打印的浮点数
 * @param {jd_int32_t} precision 小数点后的精度
 * @param {jd_int32_t} width 最小宽度
 * @return {*}
 */
void jd_printfloat(float num, jd_int32_t precision, jd_int32_t width)
{
    if (num < 0)
    {
        jd_putjd_int8_t('-');
        num = -num;
    }

    jd_int32_t int_part = (jd_int32_t)num;
    float frac_part = num - int_part;

    jd_printint(int_part, 10, width - precision - 1, 0);
    jd_putjd_int8_t('.');

    for (jd_int32_t i = 0; i < precision; i++)
    {
        frac_part *= 10;
        jd_int32_t digit = (jd_int32_t)frac_part;
        jd_putjd_int8_t(digit + '0');
        frac_part -= digit;
    }
}

/**
 * @description:  格式化打印函数
 * @param {jd_int8_t*} format  格式化字符串
 * @param ... 可变参数列表
 * @return {*}
 */
void jd_printf(const jd_int8_t *format, ...)
{
    va_list args;
    va_start(args, format);

    while (*format)
    {
        if (*format == '%')
        {
            format++;
            jd_int32_t width = 0;
            jd_int32_t precision = 6; // 默认精度为6位小数
            jd_int32_t zero_pad = 0;

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
                jd_int8_t ch = (jd_int8_t)va_arg(args, jd_int32_t);
                jd_putjd_int8_t(ch);
                break;
            }
            case 'd':
            {
                jd_int32_t num = va_arg(args, jd_int32_t);
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
                const jd_int8_t *str = va_arg(args, const jd_int8_t *);
                jd_printstring(str);
                break;
            }
            case 'x':
            {
                jd_int32_t num = va_arg(args, jd_int32_t);
                jd_printint(num, 16, width, zero_pad);
                break;
            }
            case 'X':
            {
                jd_int32_t num = va_arg(args, jd_int32_t);
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
                jd_uint32_t num = va_arg(args,jd_uint32_t);
                jd_printint(num, 10, width, zero_pad);
                break;
            }
            case 'o':
            {
                jd_uint32_t num = va_arg(args, jd_uint32_t);
                jd_printint(num, 8, width, zero_pad);
                break;
            }
            case 'b':
            {
                jd_uint32_t num = va_arg(args,jd_uint32_t);
                jd_printint(num, 2, width, zero_pad);
                break;
            }
            case '%':
            {
                jd_putjd_int8_t('%');
                break;
            }
            default:
                jd_putjd_int8_t(*format);
                break;
            }
        }
        else
        {
            jd_putjd_int8_t(*format);
        }
        format++;
    }

    va_end(args);
}
#endif

