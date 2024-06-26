/**
 * @file      translate.h
 *
 * @brief     Header fiel for Decawave convertion
 *
 * @author    Decawave
 *
 * @attention Copyright 2017-2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

#ifndef SRC_INC_TRANSLATE_H_
#define SRC_INC_TRANSLATE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Channel */
int chan_to_deca(int i);
int deca_to_chan(int i);

/* Bitrate */
int bitrate_to_deca(int i);
int deca_to_bitrate(int i);

/* PRF */
int prf_to_deca(int i);
int deca_to_prf(int i);

/* PAC */
int pac_to_deca(int i);
int deca_to_pac(int i);

/* PLEN */
int plen_to_deca(int i);
int deca_to_plen(int i);

/*STS Length*/
int sts_length_to_deca(int i);
int deca_to_sts_length(int i);

#ifdef __cplusplus
}
#endif

#endif /* SRC_INC_TRANSLATE_H_ */
