#include "Display_EPD_W21_spi.h"
#include "Display_EPD_W21.h"

void delay_xms(unsigned int xms)
{
    delay(xms);
}

void Epaper_READBUSY(void)
{
    while (isEPD_W21_BUSY) {
        delay(10);
    }
    delay(200);
}
// Full screen update initialization
void EPD_HW_Init(void)
{
    EPD_W21_RST_0;
    delay_xms(100);
    EPD_W21_RST_1;
    delay_xms(100);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x12);
    delay_xms(100);
    EPD_W21_WriteCMD(0x12);
    delay_xms(100);
    EPD_W21_WriteCMD(0x12);
    delay_xms(100);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x18);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x0C);
    EPD_W21_WriteDATA(0xAE);
    EPD_W21_WriteDATA(0xC7);
    EPD_W21_WriteDATA(0xC3);
    EPD_W21_WriteDATA(0xC0);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x01);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) % 256);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) / 256);
    EPD_W21_WriteDATA(0x02);
    EPD_W21_WriteCMD(0x3C);
    EPD_W21_WriteDATA(0x01);
    EPD_W21_WriteCMD(0x11);
    EPD_W21_WriteDATA(0x03);
    EPD_W21_WriteCMD(0x44);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA((EPD_HEIGHT - 1) % 256);
    EPD_W21_WriteDATA((EPD_HEIGHT - 1) / 256);
    EPD_W21_WriteCMD(0x45);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) % 256);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) / 256);
    EPD_W21_WriteCMD(0x4E);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteCMD(0x4F);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    Epaper_READBUSY();
}
// Fast update initialization
void EPD_HW_Init_Fast(void)
{
    EPD_W21_RST_0;
    delay_xms(100);
    EPD_W21_RST_1;
    delay_xms(100);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x12);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x0C);
    EPD_W21_WriteDATA(0xAE);
    EPD_W21_WriteDATA(0xC7);
    EPD_W21_WriteDATA(0xC3);
    EPD_W21_WriteDATA(0xC0);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x01);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) % 256);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) / 256);
    EPD_W21_WriteDATA(0x02);
    EPD_W21_WriteCMD(0x11);
    EPD_W21_WriteDATA(0x03);
    EPD_W21_WriteCMD(0x44);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA((EPD_HEIGHT - 1) % 256);
    EPD_W21_WriteDATA((EPD_HEIGHT - 1) / 256);
    EPD_W21_WriteCMD(0x45);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) % 256);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) / 256);
    EPD_W21_WriteCMD(0x4E);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteCMD(0x4F);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x3C);
    EPD_W21_WriteDATA(0x01);
    EPD_W21_WriteCMD(0x18);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x1A);
    EPD_W21_WriteDATA(0x6A);
}
// 4 Gray update initialization
void EPD_HW_Init_4G(void)
{
    EPD_W21_RST_0;
    delay_xms(100);
    EPD_W21_RST_1;
    delay_xms(100);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x12);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x0C);
    EPD_W21_WriteDATA(0xAE);
    EPD_W21_WriteDATA(0xC7);
    EPD_W21_WriteDATA(0xC3);
    EPD_W21_WriteDATA(0xC0);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x01);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) % 256);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) / 256);
    EPD_W21_WriteDATA(0x02);
    EPD_W21_WriteCMD(0x11);
    EPD_W21_WriteDATA(0x02);
    EPD_W21_WriteCMD(0x44);
    EPD_W21_WriteDATA((EPD_HEIGHT - 1) % 256);
    EPD_W21_WriteDATA((EPD_HEIGHT - 1) / 256);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteCMD(0x45);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) % 256);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) / 256);
    EPD_W21_WriteCMD(0x4E);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteCMD(0x4F);
    EPD_W21_WriteDATA(0x00);
    EPD_W21_WriteDATA(0x00);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x3C);
    EPD_W21_WriteDATA(0x01);
    EPD_W21_WriteCMD(0x18);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x1A);
    EPD_W21_WriteDATA(0x5A);
}

void EPD_Update(void)
{
    EPD_W21_WriteCMD(0x22);
    EPD_W21_WriteDATA(0xF7);
    EPD_W21_WriteCMD(0x20);
    Epaper_READBUSY();
}
void EPD_Update_Fast(void)
{
    EPD_W21_WriteCMD(0x22);
    EPD_W21_WriteDATA(0xD7);
    EPD_W21_WriteCMD(0x20);
    Epaper_READBUSY();
}
void EPD_Update_4G(void)
{
    EPD_W21_WriteCMD(0x22);
    EPD_W21_WriteDATA(0xD7);
    EPD_W21_WriteCMD(0x20);
    Epaper_READBUSY();
}
void EPD_Part_Update(void)
{
    EPD_W21_WriteCMD(0x22);
    EPD_W21_WriteDATA(0xFF);
    EPD_W21_WriteCMD(0x20);
    Epaper_READBUSY();
}

void EPD_WhiteScreen_ALL(const unsigned char *datas)
{
    unsigned int i;
    EPD_W21_WriteCMD(0x24);
    for (i = 0; i < EPD_ARRAY; i++) {
        EPD_W21_WriteDATA(datas[i]);
    }
    EPD_W21_WriteCMD(0x26);
    for (i = 0; i < EPD_ARRAY; i++) {
        EPD_W21_WriteDATA(0xff);
    }
    EPD_Update();
}
void EPD_WhiteScreen_ALL_Fast(const unsigned char *datas)
{
    unsigned int i;
    EPD_W21_WriteCMD(0x24);
    for (i = 0; i < EPD_ARRAY; i++) {
        EPD_W21_WriteDATA(datas[i]);
    }
    EPD_W21_WriteCMD(0x26);
    for (i = 0; i < EPD_ARRAY; i++) {
        EPD_W21_WriteDATA(0xff);
    }
    EPD_Update_Fast();
}
void EPD_WhiteScreen_White(void)
{
    unsigned int i;
    EPD_W21_WriteCMD(0x24);
    for (i = 0; i < EPD_ARRAY; i++) {
        EPD_W21_WriteDATA(0xff);
    }
    EPD_W21_WriteCMD(0x26);
    for (i = 0; i < EPD_ARRAY; i++) {
        EPD_W21_WriteDATA(0xff);
    }
    EPD_Update();
}
void EPD_WhiteScreen_Black(void)
{
    unsigned int i;
    EPD_W21_WriteCMD(0x24);
    for (i = 0; i < EPD_ARRAY; i++) {
        EPD_W21_WriteDATA(0x00);
    }
    EPD_Update();
}
void EPD_DeepSleep(void)
{
    EPD_W21_WriteCMD(0x10);
    EPD_W21_WriteDATA(0x01);
    delay_xms(100);
}

void EPD_Dis_Part(unsigned int x_start, unsigned int y_start, const unsigned char *datas, unsigned int PART_COLUMN,
                  unsigned int PART_LINE)
{
    unsigned int i, x_end, y_end;
    x_start = x_start - x_start % 8;
    x_end   = x_start + PART_LINE - 1;
    y_end   = y_start + PART_COLUMN - 1;
    EPD_W21_RST_0;
    delay_xms(10);
    EPD_W21_RST_1;
    delay_xms(10);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x12);
    Epaper_READBUSY();
    EPD_W21_WriteCMD(0x0C);
    EPD_W21_WriteDATA(0xAE);
    EPD_W21_WriteDATA(0xC7);
    EPD_W21_WriteDATA(0xC3);
    EPD_W21_WriteDATA(0xC0);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x01);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) % 256);
    EPD_W21_WriteDATA((EPD_WIDTH - 1) / 256);
    EPD_W21_WriteDATA(0x02);
    /* Partial writes require X/Y increment mode used by 1-bit RAM writes. */
    EPD_W21_WriteCMD(0x11);
    EPD_W21_WriteDATA(0x03);
    EPD_W21_WriteCMD(0x18);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x3C);
    EPD_W21_WriteDATA(0x80);
    EPD_W21_WriteCMD(0x44);
    EPD_W21_WriteDATA(x_start % 256);
    EPD_W21_WriteDATA(x_start / 256);
    EPD_W21_WriteDATA(x_end % 256);
    EPD_W21_WriteDATA(x_end / 256);
    EPD_W21_WriteCMD(0x45);
    EPD_W21_WriteDATA(y_start % 256);
    EPD_W21_WriteDATA(y_start / 256);
    EPD_W21_WriteDATA(y_end % 256);
    EPD_W21_WriteDATA(y_end / 256);
    EPD_W21_WriteCMD(0x4E);
    EPD_W21_WriteDATA(x_start % 256);
    EPD_W21_WriteDATA(x_start / 256);
    EPD_W21_WriteCMD(0x4F);
    EPD_W21_WriteDATA(y_start % 256);
    EPD_W21_WriteDATA(y_start / 256);
    EPD_W21_WriteCMD(0x24);
    for (i = 0; i < PART_COLUMN * PART_LINE / 8; i++) {
        EPD_W21_WriteDATA(datas[i]);
    }
    EPD_Part_Update();
}

unsigned char In2bytes_Out1byte_RAM1(unsigned char data1, unsigned char data2)
{
    unsigned int i;
    unsigned char TempData1, TempData2;
    unsigned char outdata = 0x00;
    TempData1             = data1;
    TempData2             = data2;
    for (i = 0; i < 4; i++) {
        outdata = outdata << 1;
        if (((TempData1 & 0xC0) == 0xC0) || ((TempData1 & 0xC0) == 0x40))
            outdata = outdata | 0x01;
        else
            outdata = outdata | 0x00;
        TempData1 = TempData1 << 2;
    }
    for (i = 0; i < 4; i++) {
        outdata = outdata << 1;
        if ((TempData2 & 0xC0) == 0xC0 || (TempData2 & 0xC0) == 0x40)
            outdata = outdata | 0x01;
        else
            outdata = outdata | 0x00;
        TempData2 = TempData2 << 2;
    }
    return outdata;
}
unsigned char In2bytes_Out1byte_RAM2(unsigned char data1, unsigned char data2)
{
    unsigned int i;
    unsigned char TempData1, TempData2;
    unsigned char outdata = 0x00;
    TempData1             = data1;
    TempData2             = data2;
    for (i = 0; i < 4; i++) {
        outdata = outdata << 1;
        if (((TempData1 & 0xC0) == 0xC0) || ((TempData1 & 0xC0) == 0x80))
            outdata = outdata | 0x01;
        else
            outdata = outdata | 0x00;
        TempData1 = TempData1 << 2;
    }
    for (i = 0; i < 4; i++) {
        outdata = outdata << 1;
        if ((TempData2 & 0xC0) == 0xC0 || (TempData2 & 0xC0) == 0x80)
            outdata = outdata | 0x01;
        else
            outdata = outdata | 0x00;
        TempData2 = TempData2 << 2;
    }
    return outdata;
}
void EPD_WhiteScreen_ALL_4G(const unsigned char *datas)
{
    unsigned int i;
    unsigned char tempOriginal;
    EPD_W21_WriteCMD(0x24);
    for (i = 0; i < EPD_ARRAY * 2; i += 2) {
        tempOriginal = In2bytes_Out1byte_RAM1(*(datas + i), *(datas + i + 1));
        EPD_W21_WriteDATA(~tempOriginal);
    }
    EPD_W21_WriteCMD(0x26);
    for (i = 0; i < EPD_ARRAY * 2; i += 2) {
        tempOriginal = In2bytes_Out1byte_RAM2(*(datas + i), *(datas + i + 1));
        EPD_W21_WriteDATA(~tempOriginal);
    }
    EPD_Update_4G();
}
