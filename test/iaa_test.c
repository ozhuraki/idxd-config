// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2019 Intel Corporation. All rights reserved. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "accel_test.h"
#include "iaa.h"

#define IAA_TEST_SIZE 20000

static void usage(void)
{
	printf("<app_name> [options]\n"
	"-w <wq_type> ; 0=dedicated, 1=shared\n"
	"-l <length>  ; total test buffer size\n"
	"-f <test_flags> ; 0x1: block-on-fault\n"
	"                ; 0x4: reserved\n"
	"                ; 0x8: prefault buffers\n"
	"-e <extra_flags> ; specified by each opcpde\n"
	"-o <opcode>     ; opcode, same value as in IAA spec\n"
	"-d              ; wq device such as iax1/wq1.0\n"
	"-n <number of descriptors> ;descriptor count to submit\n"
	"-t <ms timeout> ; ms to wait for descs to complete\n"
	"-v              ; verbose\n"
	"-h              ; print this message\n");
}

static int test_noop(struct acctest_context *ctx, int tflags, int num_desc)
{
	struct task_node *tsk_node;
	int rc = ACCTEST_STATUS_OK;
	int itr = num_desc, i = 0, range = 0;

	info("testnoop: tflags %#x num_desc %ld\n", tflags, num_desc);

	ctx->is_batch = 0;

	if (ctx->dedicated == ACCFG_WQ_SHARED)
		range = ctx->threshold;
	else
		range = ctx->wq_size - 1;

	while (itr > 0 && rc == ACCTEST_STATUS_OK) {
		i = (itr < range) ? itr : range;
		/* Allocate memory to all the task nodes, desc, completion record*/
		rc = acctest_alloc_multiple_tasks(ctx, i);
		if (rc != ACCTEST_STATUS_OK)
			return rc;

		/* allocate memory to src and dest buffers and fill in the desc for all the nodes*/
		tsk_node = ctx->multi_task_node;
		while (tsk_node) {
			tsk_node->tsk->opcode = IAX_OPCODE_NOOP;
			tsk_node->tsk->test_flags = tflags;
			tsk_node = tsk_node->next;
		}

		rc = iaa_noop_multi_task_nodes(ctx);
		if (rc != ACCTEST_STATUS_OK)
			return rc;

		/* Verification of all the nodes*/
		tsk_node = ctx->multi_task_node;
		while (tsk_node) {
			rc = iaa_task_result_verify(tsk_node->tsk, 0);
			tsk_node = tsk_node->next;
		}

		acctest_free_task(ctx);
		itr = itr - range;
	}

	return rc;
}

static int test_crc64(struct acctest_context *ctx, size_t buf_size, int tflags,
		      int extra_flags, uint32_t opcode, int num_desc)
{
	struct task_node *tsk_node;
	int rc = ACCTEST_STATUS_OK;
	int itr = num_desc, i = 0, range = 0;

	info("testcrc64: opcode %d len %#lx tflags %#x num_desc %ld extra_flags %#lx\n",
	     opcode, buf_size, tflags, num_desc, extra_flags);

	ctx->is_batch = 0;

	if (ctx->dedicated == ACCFG_WQ_SHARED)
		range = ctx->threshold;
	else
		range = ctx->wq_size;

	while (itr > 0 && rc == ACCTEST_STATUS_OK) {
		i = (itr < range) ? itr : range;
		/* Allocate memory to all the task nodes, desc, completion record*/
		rc = acctest_alloc_multiple_tasks(ctx, i);
		if (rc != ACCTEST_STATUS_OK)
			return rc;

		/* allocate memory to src and dest buffers and fill in the desc for all the nodes*/
		tsk_node = ctx->multi_task_node;
		while (tsk_node) {
			tsk_node->tsk->iaa_crc64_flags = extra_flags;

			rc = init_task(tsk_node->tsk, tflags, opcode, buf_size);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			tsk_node = tsk_node->next;
		}

		switch (opcode) {
		case IAX_OPCODE_CRC64:
			rc = iaa_crc64_multi_task_nodes(ctx);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			/* Verification of all the nodes*/
			rc = iaa_task_result_verify_task_nodes(ctx, 0);
			if (rc != ACCTEST_STATUS_OK)
				return rc;
			break;
		default:
			err("Unsupported op %#x\n", opcode);
			return -EINVAL;
		}

		acctest_free_task(ctx);
		itr = itr - range;
	}

	return rc;
}

static int test_zcompress(struct acctest_context *ctx, size_t buf_size,
			  int tflags, uint32_t opcode, int num_desc)
{
	struct task_node *tsk_node;
	int rc = ACCTEST_STATUS_OK;
	int itr = num_desc, i = 0, range = 0;

	info("testcrc64: opcode %d len %#lx tflags %#x num_desc %ld\n",
	     opcode, buf_size, tflags, num_desc);

	ctx->is_batch = 0;

	if (ctx->dedicated == ACCFG_WQ_SHARED)
		range = ctx->threshold;
	else
		range = ctx->wq_size;

	while (itr > 0 && rc == ACCTEST_STATUS_OK) {
		i = (itr < range) ? itr : range;
		/* Allocate memory to all the task nodes, desc, completion record*/
		rc = acctest_alloc_multiple_tasks(ctx, i);
		if (rc != ACCTEST_STATUS_OK)
			return rc;

		/* allocate memory to src and dest buffers and fill in the desc for all the nodes*/
		tsk_node = ctx->multi_task_node;
		while (tsk_node) {
			rc = init_task(tsk_node->tsk, tflags, opcode, buf_size);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			tsk_node = tsk_node->next;
		}

		switch (opcode) {
		case IAX_OPCODE_ZCOMPRESS16:
			rc = iaa_zcompress16_multi_task_nodes(ctx);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			/* Verification of all the nodes*/
			rc = iaa_task_result_verify_task_nodes(ctx, 0);
			if (rc != ACCTEST_STATUS_OK)
				return rc;
			break;
		case IAX_OPCODE_ZDECOMPRESS16:
			rc = iaa_zdecompress16_multi_task_nodes(ctx);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			/* Verification of all the nodes*/
			rc = iaa_task_result_verify_task_nodes(ctx, 0);
			if (rc != ACCTEST_STATUS_OK)
				return rc;
			break;
		case IAX_OPCODE_ZCOMPRESS32:
			rc = iaa_zcompress32_multi_task_nodes(ctx);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			/* Verification of all the nodes*/
			rc = iaa_task_result_verify_task_nodes(ctx, 0);
			if (rc != ACCTEST_STATUS_OK)
				return rc;
			break;
		case IAX_OPCODE_ZDECOMPRESS32:
			rc = iaa_zdecompress32_multi_task_nodes(ctx);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			/* Verification of all the nodes*/
			rc = iaa_task_result_verify_task_nodes(ctx, 0);
			if (rc != ACCTEST_STATUS_OK)
				return rc;
			break;
		default:
			err("Unsupported op %#x\n", opcode);
			return -EINVAL;
		}

		acctest_free_task(ctx);
		itr = itr - range;
	}

	return rc;
}

static int test_compress(struct acctest_context *ctx, size_t buf_size, int tflags,
			 int extra_flags, uint32_t opcode, int num_desc)
{
	struct task_node *tsk_node;
	int rc = ACCTEST_STATUS_OK;
	int itr = num_desc, i = 0, range = 0;

	info("testcompress: opcode %d len %#lx tflags %#x num_desc %ld extra_flags %#lx\n",
	     opcode, buf_size, tflags, num_desc, extra_flags);

	ctx->is_batch = 0;

	if (ctx->dedicated == ACCFG_WQ_SHARED)
		range = ctx->threshold;
	else
		range = ctx->wq_size;

	while (itr > 0 && rc == ACCTEST_STATUS_OK) {
		i = (itr < range) ? itr : range;
		/* Allocate memory to all the task nodes, desc, completion record*/
		rc = acctest_alloc_multiple_tasks(ctx, i);
		if (rc != ACCTEST_STATUS_OK)
			return rc;

		/* allocate memory to src and dest buffers and fill in the desc for all the nodes*/
		tsk_node = ctx->multi_task_node;
		while (tsk_node) {
			tsk_node->tsk->iaa_compr_flags = extra_flags;

			rc = init_task(tsk_node->tsk, tflags, opcode, buf_size);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			tsk_node = tsk_node->next;
		}

		switch (opcode) {
		case IAX_OPCODE_COMPRESS:
			rc = iaa_compress_multi_task_nodes(ctx);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			/* Verification of all the nodes*/
			rc = iaa_task_result_verify_task_nodes(ctx, 0);
			if (rc != ACCTEST_STATUS_OK)
				return rc;
			break;
		case IAX_OPCODE_DECOMPRESS:
			rc = iaa_decompress_multi_task_nodes(ctx);
			if (rc != ACCTEST_STATUS_OK)
				return rc;

			/* Verification of all the nodes*/
			rc = iaa_task_result_verify_task_nodes(ctx, 0);
			if (rc != ACCTEST_STATUS_OK)
				return rc;
			break;
		default:
			err("Unsupported op %#x\n", opcode);
			return -EINVAL;
		}

		acctest_free_task(ctx);
		itr = itr - range;
	}

	return rc;
}

int main(int argc, char *argv[])
{
	struct acctest_context *iaa;
	int rc = 0;
	int wq_type = SHARED;
	unsigned long buf_size = IAA_TEST_SIZE;
	int tflags = TEST_FLAGS_BOF;
	int extra_flags = 0;
	int opcode = IAX_OPCODE_NOOP;
	int opt;
	char dev_type[MAX_DEV_LEN];
	int wq_id = ACCTEST_DEVICE_ID_NO_INPUT;
	int dev_id = ACCTEST_DEVICE_ID_NO_INPUT;
	int dev_wq_id = ACCTEST_DEVICE_ID_NO_INPUT;
	unsigned int num_desc = 1;

	while ((opt = getopt(argc, argv, "w:l:f:e:o:b:c:d:n:t:p:vh")) != -1) {
		switch (opt) {
		case 'w':
			wq_type = atoi(optarg);
			break;
		case 'l':
			buf_size = strtoul(optarg, NULL, 0);
			break;
		case 'f':
			tflags = strtoul(optarg, NULL, 0);
			break;
		case 'e':
			extra_flags = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			opcode = strtoul(optarg, NULL, 0);
			break;
		case 'd':
			if (sscanf(optarg, "%[a-z]%u/%*[a-z]%u.%u", dev_type,
				   &dev_id, &dev_wq_id, &wq_id) != 4) {
				err("invalid input device:dev_wq_id:%d ,wq_id:%d\n",
				    dev_wq_id, wq_id);
				return -EINVAL;
			}
			break;
		case 'n':
			num_desc = strtoul(optarg, NULL, 0);
			break;
		case 't':
			ms_timeout = strtoul(optarg, NULL, 0);
			break;
		case 'v':
			debug_logging = 1;
			break;
		case 'h':
			usage();
			exit(0);
		default:
			break;
		}
	}

	iaa = acctest_init(tflags);

	if (!iaa)
		return -ENOMEM;

	rc = acctest_alloc(iaa, wq_type, dev_id, wq_id);
	if (rc < 0)
		return -ENOMEM;

	if (buf_size > iaa->max_xfer_size) {
		err("invalid transfer size: %lu\n", buf_size);
		return -EINVAL;
	}

	switch (opcode) {
	case IAX_OPCODE_NOOP:
		rc = test_noop(iaa, tflags, num_desc);
		if (rc != ACCTEST_STATUS_OK)
			goto error;
		break;

	case IAX_OPCODE_CRC64:
		rc = test_crc64(iaa, buf_size, tflags, extra_flags, opcode, num_desc);
		if (rc != ACCTEST_STATUS_OK)
			goto error;
		break;

	case IAX_OPCODE_ZCOMPRESS16:
	case IAX_OPCODE_ZDECOMPRESS16:
	case IAX_OPCODE_ZCOMPRESS32:
	case IAX_OPCODE_ZDECOMPRESS32:
		rc = test_zcompress(iaa, buf_size, tflags, opcode, num_desc);
		if (rc != ACCTEST_STATUS_OK)
			goto error;
		break;

	case IAX_OPCODE_COMPRESS:
	case IAX_OPCODE_DECOMPRESS:
		rc = test_compress(iaa, buf_size, tflags, extra_flags, opcode, num_desc);
		if (rc != ACCTEST_STATUS_OK)
			goto error;
		break;

	default:
		rc = -EINVAL;
		break;
	}

 error:
	acctest_free(iaa);
	return rc;
}