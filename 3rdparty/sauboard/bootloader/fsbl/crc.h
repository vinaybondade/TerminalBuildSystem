/*
 * crc.h
 *
 *  Created on: 12 Jul 2020
 *      Author: amit
 */

#ifndef SRC_CRC_H_
#define SRC_CRC_H_

#include "stdint.h"
#include "stddef.h"

uint8_t crc8(const void *buf, size_t size);

#endif /* SRC_CRC_H_ */
