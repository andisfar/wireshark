/* packet-nvme.h
 * data structures for NVMe Dissection
 * Copyright 2016
 * Code by Parav Pandit
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef _PACKET_NVME_H_
#define _PACKET_NVME_H_

#define NVME_CMD_SIZE 64
#define NVME_CQE_SIZE 16

#define NVME_FABRIC_OPC 0x7F

struct nvme_q_ctx {
    wmem_tree_t *pending_cmds;
    wmem_tree_t *done_cmds;
    wmem_tree_t *data_requests;
    wmem_tree_t *data_responses;
    wmem_tree_t *data_offsets;
    guint16     qid;
};

#define NVME_CMD_MAX_TRS (16)

struct nvme_cmd_ctx {
    guint32 cmd_pkt_num;  /* pkt number of the cmd */
    guint32 cqe_pkt_num;  /* pkt number of the cqe */

    guint32 data_req_pkt_num;
    guint32 data_tr_pkt_num[NVME_CMD_MAX_TRS];
    guint32 first_tr_psn;

    nstime_t cmd_start_time;
    nstime_t cmd_end_time;
    guint32 tr_bytes;   /* bytes transferred so far */
    gboolean fabric;     /* indicate whether cmd fabric type or not */

    union {
        struct {
            guint16 cns;
        } cmd_identify;
        struct {
            guint records;
            guint tr_rcrd_id;
            guint tr_off;
            guint tr_sub_entries;
            guint16 lsi;
            guint8 lid;
            guint8 lsp;
            guint64 off;
            guint8 uid_idx;
        } get_logpage;
        struct {
            guint8 fid;
        } set_features;
        struct {
            guint8 fctype; /* fabric cmd type */
            struct {
                guint8 offset;
            } prop_get;
        } fabric_cmd;
    } cmd_ctx;
    guint8  opcode;
};

extern int hf_nvmeof_cmd_pkt;
extern int hf_nvmeof_data_req;

const gchar *get_nvmeof_cmd_string(guint8 fctype);

void
nvme_publish_qid(proto_tree *tree, int field_index, guint16 qid);

void
nvme_publish_cmd_latency(proto_tree *tree, struct nvme_cmd_ctx *cmd_ctx,
                         int field_index);
void
nvme_publish_to_cmd_link(proto_tree *tree, tvbuff_t *tvb,
                             int hf_index, struct nvme_cmd_ctx *cmd_ctx);
void
nvme_publish_to_cqe_link(proto_tree *tree, tvbuff_t *tvb,
                             int hf_index, struct nvme_cmd_ctx *cmd_ctx);
void
nvme_publish_to_data_req_link(proto_tree *tree, tvbuff_t *tvb,
                             int hf_index, struct nvme_cmd_ctx *cmd_ctx);
void
nvme_publish_to_data_resp_link(proto_tree *tree, tvbuff_t *tvb,
                             int hf_index, struct nvme_cmd_ctx *cmd_ctx);
void
nvme_publish_link(proto_tree *tree, tvbuff_t *tvb, int hf_index,
                             guint32 pkt_no, gboolean zero_ok);

void nvme_update_cmd_end_info(packet_info *pinfo, struct nvme_cmd_ctx *cmd_ctx);

void
nvme_add_cmd_to_pending_list(packet_info *pinfo, struct nvme_q_ctx *q_ctx,
                             struct nvme_cmd_ctx *cmd_ctx,
                             void *ctx, guint16 cmd_id);
void* nvme_lookup_cmd_in_pending_list(struct nvme_q_ctx *q_ctx, guint16 cmd_id);

struct keyed_data_req
{
    guint64 addr;
    guint32 key;
    guint32 size;
};

void
dissect_nvmeof_fabric_cmd(tvbuff_t *nvme_tvb, packet_info *pinfo, proto_tree *nvme_tree,
                                struct nvme_q_ctx *q_ctx, struct nvme_cmd_ctx *cmd, guint off, gboolean link_data_req);
void
dissect_nvmeof_cmd_data(tvbuff_t *data_tvb, packet_info *pinfo, proto_tree *data_tree,
                                 guint pkt_off, struct nvme_q_ctx *q_ctx, struct nvme_cmd_ctx *cmd, guint len);
void
dissect_nvmeof_fabric_cqe(tvbuff_t *nvme_tvb, packet_info *pinfo,
                        proto_tree *nvme_tree,
                        struct nvme_cmd_ctx *cmd_ctx, guint off);

void
nvme_add_data_request(struct nvme_q_ctx *q_ctx, struct nvme_cmd_ctx *cmd_ctx,
                                struct keyed_data_req *req);

struct nvme_cmd_ctx*
nvme_lookup_data_request(struct nvme_q_ctx *q_ctx, struct keyed_data_req *req);

void
nvme_add_data_tr_pkt(struct nvme_q_ctx *q_ctx,
                       struct nvme_cmd_ctx *cmd_ctx, guint32 rkey, guint32 frame_num);
struct nvme_cmd_ctx*
nvme_lookup_data_tr_pkt(struct nvme_q_ctx *q_ctx,
                          guint32 rkey, guint32 frame_num);

void
nvme_add_data_tr_off(struct nvme_q_ctx *q_ctx, guint32 off, guint32 frame_num);

guint32
nvme_lookup_data_tr_off(struct nvme_q_ctx *q_ctx, guint32 frame_num);

void
nvme_add_cmd_cqe_to_done_list(struct nvme_q_ctx *q_ctx,
                              struct nvme_cmd_ctx *cmd_ctx, guint16 cmd_id);
void*
nvme_lookup_cmd_in_done_list(packet_info *pinfo, struct nvme_q_ctx *q_ctx,
                             guint16 cmd_id);

void dissect_nvme_cmd_sgl(tvbuff_t *cmd_tvb, proto_tree *cmd_tree, int field_index,
                 struct nvme_q_ctx *q_ctx, struct nvme_cmd_ctx *cmd_ctx, guint cmd_off, gboolean visited);

void
dissect_nvme_cmd(tvbuff_t *nvme_tvb, packet_info *pinfo, proto_tree *root_tree,
                 struct nvme_q_ctx *q_ctx, struct nvme_cmd_ctx *cmd_ctx);

void nvme_update_transfer_request(packet_info *pinfo, struct nvme_cmd_ctx *cmd_ctx, struct nvme_q_ctx *q_ctx);

void
dissect_nvme_data_response(tvbuff_t *nvme_tvb, packet_info *pinfo, proto_tree *root_tree,
                 struct nvme_q_ctx *q_ctx, struct nvme_cmd_ctx *cmd_ctx, guint len, gboolean is_inline);

void
dissect_nvme_cqe(tvbuff_t *nvme_tvb, packet_info *pinfo, proto_tree *root_tree,
                 struct nvme_q_ctx *q_ctx, struct nvme_cmd_ctx *cmd_ctx);

/**
 * Returns string representation of opcode according
 * to opcode and queue id
 */
const gchar *
nvme_get_opcode_string(guint8  opcode, guint16 qid);

/*
 * Tells if opcode can be an opcode of io queue.
 * Used to "Guess" queue type for nvme-tcp in case that "connect"
 * command was not recorded
 */
int
nvme_is_io_queue_opcode(guint8  opcode);

#endif

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
