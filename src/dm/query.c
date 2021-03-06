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

#include <config.h>

#include <anjay_modules/time_defs.h>

#include "query.h"

#include "../dm_core.h"
#include "../anjay_core.h"

VISIBILITY_SOURCE_BEGIN

typedef struct {
    anjay_ssid_t ssid;
    anjay_iid_t out_iid;
} find_iid_args_t;

static int find_server_iid_handler(anjay_t *anjay,
                                   const anjay_dm_object_def_t *const *obj,
                                   anjay_iid_t iid,
                                   void *args_) {
    (void) obj;
    find_iid_args_t *args = (find_iid_args_t*)args_;
    int64_t ssid;
    const anjay_uri_path_t ssid_path =
            MAKE_RESOURCE_PATH(ANJAY_DM_OID_SERVER, iid,
                               ANJAY_DM_RID_SERVER_SSID);

    if (_anjay_dm_res_read_i64(anjay, &ssid_path, &ssid)) {
        return -1;
    }
    if (ssid == (int32_t) args->ssid) {
        args->out_iid = iid;
        return ANJAY_DM_FOREACH_BREAK;
    }
    return 0;
}

int _anjay_find_server_iid(anjay_t *anjay,
                           anjay_ssid_t ssid,
                           anjay_iid_t *out_iid) {
    find_iid_args_t args = {
        .ssid = ssid,
        .out_iid = ANJAY_IID_INVALID
    };

    const anjay_dm_object_def_t *const *obj =
            _anjay_dm_find_object_by_oid(anjay, ANJAY_DM_OID_SERVER);
    if (ssid == ANJAY_SSID_ANY
            || ssid == ANJAY_SSID_BOOTSTRAP
            || _anjay_dm_foreach_instance(anjay, obj,
                                          find_server_iid_handler, &args)
            || args.out_iid == ANJAY_IID_INVALID) {
        return -1;
    }
    *out_iid = args.out_iid;
    return 0;
}

static int security_iid_find_helper(anjay_t *anjay,
                                    const anjay_dm_object_def_t *const *obj,
                                    anjay_iid_t iid,
                                    void *data) {
    (void) obj;
    find_iid_args_t *args = (find_iid_args_t *) data;
    bool looking_for_bootstrap = (args->ssid == ANJAY_SSID_BOOTSTRAP);

    bool is_bootstrap = _anjay_is_bootstrap_security_instance(anjay, iid);
    if (looking_for_bootstrap != is_bootstrap) {
        return 0;
    }

    if (!is_bootstrap) {
        int64_t ssid;
        const anjay_uri_path_t ssid_path =
                MAKE_RESOURCE_PATH(ANJAY_DM_OID_SECURITY, iid,
                                   ANJAY_DM_RID_SECURITY_SSID);

        if (_anjay_dm_res_read_i64(anjay, &ssid_path, &ssid)) {
            return -1;
        }
        if (ssid != (int32_t) args->ssid) {
            return 0;
        }
    }

    args->out_iid = iid;
    return ANJAY_DM_FOREACH_BREAK;
}

int _anjay_find_security_iid(anjay_t *anjay,
                             anjay_ssid_t ssid,
                             anjay_iid_t *out_iid) {
    find_iid_args_t args = {
        .ssid = ssid,
        .out_iid = ANJAY_IID_INVALID
    };
    const anjay_dm_object_def_t *const *obj =
            _anjay_dm_find_object_by_oid(anjay, ANJAY_DM_OID_SECURITY);
    if (_anjay_dm_foreach_instance(anjay, obj, security_iid_find_helper, &args)
            || args.out_iid == ANJAY_IID_INVALID) {
        return -1;
    }
    *out_iid = args.out_iid;
    return 0;
}

bool _anjay_dm_ssid_exists(anjay_t *anjay, anjay_ssid_t ssid) {
    anjay_iid_t dummy_iid;
    return !_anjay_find_security_iid(anjay, ssid, &dummy_iid);
}

int _anjay_ssid_from_server_iid(anjay_t *anjay,
                                anjay_iid_t server_iid,
                                anjay_ssid_t *out_ssid) {
    int64_t ssid;
    const anjay_uri_path_t ssid_path =
            MAKE_RESOURCE_PATH(ANJAY_DM_OID_SERVER, server_iid,
                               ANJAY_DM_RID_SERVER_SSID);
    if (_anjay_dm_res_read_i64(anjay, &ssid_path, &ssid)) {
        return -1;
    }
    *out_ssid = (anjay_ssid_t) ssid;
    return 0;
}

int _anjay_ssid_from_security_iid(anjay_t *anjay,
                                  anjay_iid_t security_iid,
                                  uint16_t *out_ssid) {
    if (_anjay_is_bootstrap_security_instance(anjay, security_iid)) {
        *out_ssid = ANJAY_SSID_BOOTSTRAP;
        return 0;
    }

    int64_t _ssid;
    const anjay_uri_path_t path =
            MAKE_RESOURCE_PATH(ANJAY_DM_OID_SECURITY, security_iid,
                               ANJAY_DM_RID_SECURITY_SSID);

    if (_anjay_dm_res_read_i64(anjay, &path, &_ssid)
            || _ssid <= 0 || _ssid > UINT16_MAX) {
        anjay_log(ERROR, "could not get Short Server ID");
        return -1;
    }

    *out_ssid = (uint16_t)_ssid;
    return 0;
}

#ifdef WITH_BOOTSTRAP
bool _anjay_is_bootstrap_security_instance(anjay_t *anjay,
                                           anjay_iid_t security_iid) {
    bool is_bootstrap;
    const anjay_uri_path_t path =
            MAKE_RESOURCE_PATH(ANJAY_DM_OID_SECURITY, security_iid,
                               ANJAY_DM_RID_SECURITY_BOOTSTRAP);

    if (_anjay_dm_res_read_bool(anjay, &path, &is_bootstrap) || !is_bootstrap) {
        return false;
    }

    return true;
}
#endif

avs_time_duration_t
_anjay_disable_timeout_from_server_iid(anjay_t *anjay,
                                       anjay_iid_t server_iid) {
    static const int32_t DEFAULT_DISABLE_TIMEOUT_S = NUM_SECONDS_IN_A_DAY;

    int64_t timeout_s;
    const anjay_uri_path_t path =
            MAKE_RESOURCE_PATH(ANJAY_DM_OID_SERVER, server_iid,
                               ANJAY_DM_RID_SERVER_DISABLE_TIMEOUT);

    if (_anjay_dm_res_read_i64(anjay, &path, &timeout_s) || timeout_s < 0) {
        timeout_s = DEFAULT_DISABLE_TIMEOUT_S;
    } else if (timeout_s > INT32_MAX) {
        timeout_s = INT32_MAX;
    }

    return avs_time_duration_from_scalar(timeout_s, AVS_TIME_S);
}
