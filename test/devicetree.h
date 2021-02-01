/*
 * Copyright (c) The Libre Solar Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Macros copied from devicetree.h and util.h
 */

#define DT_NODE_EXISTS(node_id) DT_CAT(node_id, _EXISTS)

#define DT_PROP(node_id, prop) DT_CAT(node_id, _P_##prop)
#define DT_PROP_LEN(node_id, prop) DT_PROP(node_id, prop##_LEN)

#define DT_CHILD(node_id, child) UTIL_CAT(node_id, DT_S_PREFIX(child))

#define DT_CAT(node_id, prop_suffix) node_id##prop_suffix

#define DT_S_PREFIX(name) _S_##name

#define UTIL_CAT(a, ...) UTIL_PRIMITIVE_CAT(a, __VA_ARGS__)
#define UTIL_PRIMITIVE_CAT(a, ...) a##__VA_ARGS__

#define DT_FOREACH_CHILD(node_id, fn) \
	DT_CAT(node_id, _FOREACH_CHILD)(fn)

#define DT_PHA_BY_IDX(node_id, pha, idx, cell) \
	DT_PROP(node_id, pha##_IDX_##idx##_VAL_##cell)

#define DT_PHA(node_id, pha, cell) DT_PHA_BY_IDX(node_id, pha, 0, cell)

#define DT_PHANDLE_BY_IDX(node_id, prop, idx) \
	DT_PROP(node_id, prop##_IDX_##idx##_PH)

#define DT_PROP_BY_PHANDLE_IDX(node_id, phs, idx, prop) \
	DT_PROP(DT_PHANDLE_BY_IDX(node_id, phs, idx), prop)

#define DT_PROP_BY_PHANDLE(node_id, ph, prop) \
	DT_PROP_BY_PHANDLE_IDX(node_id, ph, 0, prop)

#define DT_HAS_COMPAT_STATUS_OKAY(compat) \
	IS_ENABLED(DT_CAT(DT_COMPAT_HAS_OKAY_, compat))

/*
 * Simplified mocks for certain macros from devicetree.h
 */

#define DT_INST(inst, compat) UTIL_CAT(DT_N_INST_, UTIL_CAT(inst, UTIL_CAT(_, compat)))

#define DT_PATH(node) UTIL_CAT(DT_N_S_, node)
