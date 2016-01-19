/*
 * spi.h
 *
 * Copyright 2016 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#pragma once
#ifndef __SPI_H__
#define __SPI_H__

void spi_init();
void spi_send_next();
void spi_send_buffer_blocking(U8 *buff, U16 len, U8 *inb);
void spi_send_buffer(U8 *buff, U16 len, U8 *inbuf);
U8 spi_send_byte(U8 data);
U8 spi_buf_sent();

#endif // __SPI_H__
