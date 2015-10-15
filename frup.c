/*
 * Copyright (C) 2003-2014 FreeIPMI Core Team
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
/*****************************************************************************\
 *  Copyright (C) 2007-2014 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
 *  UCRL-CODE-232183
 *
 *  This file is part of Ipmi-fru, a tool used for retrieving
 *  motherboard field replaceable unit (FRU) information. For details,
 *  see http://www.llnl.gov/linux/.
 *
 *  Ipmi-fru is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  Ipmi-fru is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Ipmi-fru.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define uint8_t unsigned char
#define uint32_t unsigned int

#define ASSERT(x) if(!(x)) return -1;


#define IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX            512
#define IPMI_FRU_SENTINEL_VALUE                        0xC1
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_MASK            0xC0
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_SHIFT           0x06
#define IPMI_FRU_TYPE_LENGTH_NUMBER_OF_DATA_BYTES_MASK 0x3F
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_LANGUAGE_CODE   0x03


struct ipmi_fru_field
{
  uint8_t type_length_field[IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX];
  /* store length of data stored in buffer */
  unsigned int type_length_field_length;
};

typedef struct ipmi_fru_field ipmi_fru_field_t;

static int
_parse_type_length (const void *areabuf,
                    unsigned int areabuflen,
                    unsigned int current_area_offset,
                    uint8_t *number_of_data_bytes,
                    ipmi_fru_field_t *field)
{
  const uint8_t *areabufptr = (const uint8_t*) areabuf;
  uint8_t type_length;
  uint8_t type_code;

  ASSERT (areabuf);
  ASSERT (areabuflen);
  ASSERT (number_of_data_bytes);
  
  type_length = areabufptr[current_area_offset];

  /* IPMI Workaround 
   *
   * Dell P weredge R610
   *
   * My reading of the FRU Spec is that all non-custom fields are
   * required to be listed by the vendor.  However, on this
   * motherboard, some areas list this, indicating that there is
   * no more data to be parsed.  So now, for "required" fields, I
   * check to see if the type-length field is a sentinel before
   * calling this function.
   */

  ASSERT (type_length != IPMI_FRU_SENTINEL_VALUE);

  type_code = (type_length & IPMI_FRU_TYPE_LENGTH_TYPE_CODE_MASK) >> IPMI_FRU_TYPE_LENGTH_TYPE_CODE_SHIFT;
  (*number_of_data_bytes) = type_length & IPMI_FRU_TYPE_LENGTH_NUMBER_OF_DATA_BYTES_MASK;

  /* Special Case: This shouldn't be a length of 0x01 (see type/length
   * byte format in FRU Information Storage Definition).
   */
  if (type_code == IPMI_FRU_TYPE_LENGTH_TYPE_CODE_LANGUAGE_CODE
      && (*number_of_data_bytes) == 0x01)
    {
      return (-1);
    }

  if ((current_area_offset + 1 + (*number_of_data_bytes)) > areabuflen)
    {
      return (-1);
    }

  if (field)
    {
      memset (field->type_length_field,
              '\0',
              IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX);
      memcpy (field->type_length_field,
              &areabufptr[current_area_offset],
              1 + (*number_of_data_bytes));
      field->type_length_field_length = 1 + (*number_of_data_bytes);
    }
      
  return (0);
}
                    
int
ipmi_fru_chassis_info_area (const void *areabuf,
			    unsigned int areabuflen,
			    uint8_t *chassis_type,
			    ipmi_fru_field_t *chassis_part_number,
			    ipmi_fru_field_t *chassis_serial_number,
			    ipmi_fru_field_t *chassis_custom_fields,
			    unsigned int chassis_custom_fields_len)
{
  const uint8_t *areabufptr = (const uint8_t*) areabuf;
  unsigned int area_offset = 0;
  unsigned int custom_fields_index = 0;
  uint8_t number_of_data_bytes;
  int rv = -1;

  if (!areabuf || !areabuflen)
    {
      return (-1);
    }

  if (chassis_part_number)
    memset (chassis_part_number,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (chassis_serial_number)
    memset (chassis_serial_number,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (chassis_custom_fields && chassis_custom_fields_len)
    memset (chassis_custom_fields,
            '\0',
            sizeof (ipmi_fru_field_t) * chassis_custom_fields_len);

  if (chassis_type)
    (*chassis_type) = areabufptr[area_offset];
  area_offset++;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          chassis_part_number) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          chassis_serial_number) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  while (area_offset < areabuflen
         && areabufptr[area_offset] != IPMI_FRU_SENTINEL_VALUE)
    {
      ipmi_fru_field_t *field_ptr = NULL;

      if (chassis_custom_fields && chassis_custom_fields_len)
        {
          if (custom_fields_index < chassis_custom_fields_len)
            field_ptr = &chassis_custom_fields[custom_fields_index];
          else
            {
              goto cleanup;
            }
        }

      if (_parse_type_length (areabufptr,
                              areabuflen,
                              area_offset,
                              &number_of_data_bytes,
                              field_ptr) < 0)
        goto cleanup;

      area_offset += 1;          /* type/length byte */
      area_offset += number_of_data_bytes;
      custom_fields_index++;
    }


 out:
  rv = 0;
 cleanup:
  return (rv);
}

int
ipmi_fru_board_info_area (const void *areabuf,
			  unsigned int areabuflen,
			  uint8_t *language_code,
			  uint32_t *mfg_date_time,
			  ipmi_fru_field_t *board_manufacturer,
			  ipmi_fru_field_t *board_product_name,
			  ipmi_fru_field_t *board_serial_number,
			  ipmi_fru_field_t *board_part_number,
			  ipmi_fru_field_t *board_fru_file_id,
			  ipmi_fru_field_t *board_custom_fields,
			  unsigned int board_custom_fields_len)
{
  const uint8_t *areabufptr = (const uint8_t*) areabuf;
  uint32_t mfg_date_time_tmp = 0;
  unsigned int area_offset = 0;
  unsigned int custom_fields_index = 0;
  uint8_t number_of_data_bytes;
  int rv = -1;

  if (!areabuf || !areabuflen)
    {
      return (-1);
    }

  if (board_manufacturer)
    memset (board_manufacturer,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (board_product_name)
    memset (board_product_name,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (board_serial_number)
    memset (board_serial_number,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (board_part_number)
    memset (board_part_number,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (board_fru_file_id)
    memset (board_fru_file_id,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (board_custom_fields && board_custom_fields_len)
    memset (board_custom_fields,
            '\0',
            sizeof (ipmi_fru_field_t) * board_custom_fields_len);

  if (language_code)
    (*language_code) = areabufptr[area_offset];
  area_offset++;

  if (mfg_date_time)
    {
      struct tm tm;
      time_t t;

      /* mfg_date_time is little endian - see spec */
      mfg_date_time_tmp |= areabufptr[area_offset];
      area_offset++;
      mfg_date_time_tmp |= (areabufptr[area_offset] << 8);
      area_offset++;
      mfg_date_time_tmp |= (areabufptr[area_offset] << 16);
      area_offset++;
      
      /* mfg_date_time is in minutes, so multiple by 60 to get seconds */
      mfg_date_time_tmp *= 60;

      /* Posix says individual calls need not clear/set all portions of
       * 'struct tm', thus passing 'struct tm' between functions could
       * have issues.  So we need to memset.
       */
      memset (&tm, '\0', sizeof(struct tm));

      /* In FRU, epoch is 0:00 hrs 1/1/96
       *
       * So convert into ansi epoch
       */

      tm.tm_year = 96;          /* years since 1900 */
      tm.tm_mon = 0;            /* months since January */
      tm.tm_mday = 1;           /* 1-31 */
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
      tm.tm_isdst = -1;

      if ((t = mktime (&tm)) == (time_t)-1)
        {
          goto cleanup;
        }

      mfg_date_time_tmp += (uint32_t)t;
      (*mfg_date_time) = mfg_date_time_tmp;
    }
  else
    area_offset += 3;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          board_manufacturer) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          board_product_name) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          board_serial_number) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (
                          areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          board_part_number) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          board_fru_file_id) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  while (area_offset < areabuflen
         && areabufptr[area_offset] != IPMI_FRU_SENTINEL_VALUE)
    {
      ipmi_fru_field_t *field_ptr = NULL;

      if (board_custom_fields && board_custom_fields_len)
        {
          if (custom_fields_index < board_custom_fields_len)
            field_ptr = &board_custom_fields[custom_fields_index];
          else
            {
              goto cleanup;
            }
        }

      if (_parse_type_length (areabufptr,
                              areabuflen,
                              area_offset,
                              &number_of_data_bytes,
                              field_ptr) < 0)
        goto cleanup;

      area_offset += 1;          /* type/length byte */
      area_offset += number_of_data_bytes;
      custom_fields_index++;
    }

 out:
  rv = 0;
 cleanup:
  return (rv);
}

int
ipmi_fru_product_info_area (const void *areabuf,
			    unsigned int areabuflen,
			    uint8_t *language_code,
			    ipmi_fru_field_t *product_manufacturer_name,
			    ipmi_fru_field_t *product_name,
			    ipmi_fru_field_t *product_part_model_number,
			    ipmi_fru_field_t *product_version,
			    ipmi_fru_field_t *product_serial_number,
			    ipmi_fru_field_t *product_asset_tag,
			    ipmi_fru_field_t *product_fru_file_id,
			    ipmi_fru_field_t *product_custom_fields,
			    unsigned int product_custom_fields_len)
{
  const uint8_t *areabufptr = (const uint8_t*) areabuf;
  unsigned int area_offset = 0;
  unsigned int custom_fields_index = 0;
  uint8_t number_of_data_bytes;
  int rv = -1;

  if (!areabuf || !areabuflen)
    {
      return (-1);
    }

  if (product_manufacturer_name)
    memset (product_manufacturer_name,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (product_name)
    memset (product_name,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (product_part_model_number)
    memset (product_part_model_number,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (product_version)
    memset (product_version,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (product_serial_number)
    memset (product_serial_number,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (product_asset_tag)
    memset (product_asset_tag,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (product_fru_file_id)
    memset (product_fru_file_id,
            '\0',
            sizeof (ipmi_fru_field_t));
  if (product_custom_fields && product_custom_fields_len)
    memset (product_custom_fields,
            '\0',
            sizeof (ipmi_fru_field_t) * product_custom_fields_len);

  if (language_code)
    (*language_code) = areabufptr[area_offset];
  area_offset++;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          product_manufacturer_name) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          product_name) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          product_part_model_number) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          product_version) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          product_serial_number) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          product_asset_tag) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  if (areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE)
    goto out;

  if (_parse_type_length (areabufptr,
                          areabuflen,
                          area_offset,
                          &number_of_data_bytes,
                          product_fru_file_id) < 0)
    goto cleanup;
  area_offset += 1;          /* type/length byte */
  area_offset += number_of_data_bytes;

  while (area_offset < areabuflen
         && areabufptr[area_offset] != IPMI_FRU_SENTINEL_VALUE)
    {
      ipmi_fru_field_t *field_ptr = NULL;

      if (product_custom_fields && product_custom_fields_len)
        {
          if (custom_fields_index < product_custom_fields_len)
            field_ptr = &product_custom_fields[custom_fields_index];
          else
            {
              goto cleanup;
            }
        }

      if (_parse_type_length (areabufptr,
                              areabuflen,
                              area_offset,
                              &number_of_data_bytes,
                              field_ptr) < 0)
        goto cleanup;

      area_offset += 1;          /* type/length byte */
      area_offset += number_of_data_bytes;
      custom_fields_index++;
    }


 out:
  rv = 0;
 cleanup:
  return (rv);
}
