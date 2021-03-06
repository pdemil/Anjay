/*
 * Copyright 2017-2018 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANJAY_INCLUDE_ANJAY_MODULES_IO_UTILS_H
#define ANJAY_INCLUDE_ANJAY_MODULES_IO_UTILS_H

#include <stdint.h>

#include <avsystem/commons/stream.h>

#include <anjay/io.h>

#include <anjay_modules/raw_buffer.h>

VISIBILITY_PRIVATE_HEADER_BEGIN

uint32_t _anjay_htonf(float f);
uint64_t _anjay_htond(double d);
float _anjay_ntohf(uint32_t v);
double _anjay_ntohd(uint64_t v);

typedef int anjay_input_ctx_constructor_t(anjay_input_ctx_t **out,
                                          avs_stream_abstract_t **stream_ptr,
                                          bool autoclose);

anjay_input_ctx_constructor_t _anjay_input_tlv_create;

int _anjay_input_ctx_destroy(anjay_input_ctx_t **ctx_ptr);

/**
 * Fetches bytes from @p ctx. On success it frees underlying @p buffer storage
 * via @p _anjay_sec_raw_buffer_clear and reinitializes @p buffer properly with
 * obtained data.
 */
int _anjay_io_fetch_bytes(anjay_input_ctx_t *ctx, anjay_raw_buffer_t *buffer);

/**
 * Fetches string from @p ctx. It calls free() on @p *out and, on success,
 * reinitializes @p *out properly with a pointer to (heap allocated) obtained
 * data.
 */
int _anjay_io_fetch_string(anjay_input_ctx_t *ctx, char **out);

VISIBILITY_PRIVATE_HEADER_END

#endif /* ANJAY_INCLUDE_ANJAY_MODULES_IO_UTILS_H */
