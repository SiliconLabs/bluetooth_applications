/**
 * @file     json_interface.h
 * @brief    specification for JSON interface in between Juniper and GUI
 *
 *
 * @author Decawave and Applications team
 *
 * @attention Copyright 2019 (c) Decawave Ltd, Dublin, Ireland.
 *            All rights reserved.
 *
 */

/***************************************************************************//*
 * General structure
 * The general structure is JSON, incapsulated in the TLV format
 * The type is 'JS', then 4bytes with the length of the JSON message
 *
 * The 4 bytes are in Hex?
 * If not, if we need to read more than 9999 than we need 5 bytes(characters)
 *
 * 'JSxxxx{json_object}'
 *
 * json_object is standard
 *
 * The input object from GUI to ARM can be either command or command with value.
 * the list of all JSON input commands object names can be found in the
 * const command_t known_commands [] of cmd_fn.c file.
 *
 * 1. Commands to set system config, run-time and calibration variables
 *    Format: JSxxxx{"PARM": %d}, where %d is the integer value for a given PARM
 *    If need to specify "true" condition for PARM, then it is 1 and for "false"
 *   it is 0
 *
 * 2. Anytime commands
 *    Format: JSxxxx{"PARM"}
 *
 * 3. Application start commands
 *    Format: JSxxxx{"START": "%s"} where %s is the NAME of Application, see
 *   known_commands [] section 3.
 *
 * 4. node application commands
 *
 * 5. service commands
 *
 * 6. TBD
 */

/***************************************************************************//*
 *          Interface from GUI  to Juniper
 *
 *          EXAMPLES
 *
 *****************************************************************************/

/* Stop any running application
 *
 * 'JSxxxx{"STOP"}'
 */

/* Get status of the connected application
 *
 * 'JSxxxx{"STAT"}'
 */

/* Configure the UWB parameters
 *
 * 'JSxxxx{"UWB PARAM": {"CHAN": %d, "PRF": %d, "PLEN": %d, "PAC": %d, "TXCODE":
 *   %d, "DATARATE": %d, TBD } }' //TODO: add the rest for DW3000
 */

/* Get the UWB parameters (the output is the same as object "UWB PARAM")
 *
 * 'JSxxxx{"UWBCFG"}'
 *
 */

/* SAVE the UWB parameters to the FLASH
 *
 * 'JSxxxx{"SAVE"}'
 */

/* Start the PDoA TWR application in the Node mode
 *
 * 'JSxxxx{"Start": "PDoA_Node" }'
 */

/* Start the PDoA application in the Tag mode
 *
 * 'JSxxxx{"Start": "PDoA_Tag" }'
 */

/***************************************************************************//*
 *          Interface from Juniper to GUI
 *
 *          EXAMPLES
 *
 *****************************************************************************/

/* A new Tag was discovered
 *
 * 'JSxxxx{"NewTag":
 *             <string>//address64, string
 *        }'
 */

/* Report from PDoA Node Application
 *
 *  'JSxxxx{"TWR":
 *    {     "a16":%04X, //addr16
 *          "R":%d,//range num
 *          "T":%d,//sys timestamp of Final WRTO Node's SuperFrame start, us
 *          "D":%d,//distance
 *          "P":%d,//raw pdoa
 *          "Xcm":%d,//X, cm
 *          "Ycm":%d,//Y, cm
 *          "O":%d,//clock offset in hundreds part of ppm
 *          "V":%d //service message data from the tag: (stationary, etc)
 *          "X":%d //service message data from the tag: (stationary, etc)
 *          "Y":%d //service message data from the tag: (stationary, etc)
 *          "Z":%d //service message data from the tag: (stationary, etc)
 *    }
 *   }'
 */

// TODO: some of the output has a non-JSON format, say Diag output or ACCUM,
//   will be TBD
