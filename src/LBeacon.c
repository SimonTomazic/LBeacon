/*
 Copyright (c) 2016 Academia Sinica, Institute of Information Science

 License:

      GPL 3.0 : The content of this file is subject to the terms and
      conditions defined in file 'COPYING.txt', which is part of this source
      code package.

 Project Name:

      BeDIS

 File Description:

      This file contains the programs executed by location beacons to
      support indoor poositioning and object tracking functions.

 File Name:

      LBeacon.c

 Version:

       2.0,  20190201

 Abstract:

      BeDIS uses LBeacons to deliver 3D coordinates and textual
      descriptions of their locations to users' devices. Basically, a
      LBeacon is an inexpensive, Bluetooth Smart Ready device. The 3D
      coordinates and location description of every LBeacon are retrieved
      from BeDIS (Building/environment Data and Information System) server
      and stored locally during deployment and maintenance times. Once
      initialized, each LBeacon broadcasts its coordinates and location
      description to Bluetooth enabled user devices within its coverage
      area.

 Authors:

      Holly Wang, hollywang@iis.sinica.edu.tw
      Jake Lee, jakelee@iis.sinica.edu.tw
      Joey Zhou, joeyzhou@iis.sinica.edu.tw
      Kenneth Tang, kennethtang@iis.sinica.edu.tw
      Chun Yu Lai, chunyu1202@gmail.com

*/

#include "LBeacon.h"
#include "zlog.h"
#include "Communication.h"
#include "thpool.h"

#define Debugging

ErrorCode single_running_instance(char *file_name){
    int retry_times = 0;
    int lock_file = 0;
    struct flock fl;

    retry_times = FILE_OPEN_RETRY;
    while(retry_times--){
        lock_file = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);

        if(-1 != lock_file){
            break;
        }
    }

    if(-1 == lock_file){
        zlog_error(category_health_report,
            "Unable to open lock file");
#ifdef Debugging
        zlog_error(category_debug,
            "Unable to open lock file");
#endif
        return E_OPEN_FILE;
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if(fcntl(lock_file, F_SETLK, &fl) == -1){
        zlog_error(category_health_report, "Unable to lock file");
#ifdef Debugging
        zlog_error(category_debug, "Unable to lock file");
#endif
        close(lock_file);
        return E_OPEN_FILE;
    }

    char pids[10];
    snprintf(pids, sizeof(pids), "%d\n", getpid());
    if((size_t)write(lock_file, pids, strlen(pids)) != strlen(pids)){

        zlog_error(category_health_report,
                   "Unable to write pid into lock file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to write pid into lock file");
#endif
        close(lock_file);

        return E_OPEN_FILE;
    }

    return WORK_SUCCESSFULLY;
}

ErrorCode generate_uuid(Config *config){
    double coordinate_X_double;
    double coordinate_Y_double;
    int coordinate_Z_int;

    char coordinate[CONFIG_BUFFER_SIZE];
    char *temp_coordinate = NULL;

    /* construct UUID as 000000ZZ0000xxxxxxxx0000yyyyyyyy format */
    memset(config->uuid, 0, sizeof(config->uuid));

    coordinate_X_double = (double)atof(config->coordinate_X);
    coordinate_Y_double = (double)atof(config->coordinate_Y);
    coordinate_Z_int = config->lowest_basement_level +
                       (int)atoi(config->coordinate_Z);

    if( coordinate_X_double < 0 ||
        coordinate_Y_double < 0 ||
        coordinate_Z_int < 0){

        zlog_error(category_health_report,
                   "Invalid 3D coordinates. X-, Y- or Z- are not "
                   "positive. X=[%s], Y=[%s], Z=[%s]",
                   config->coordinate_X, config->coordinate_Y,
                   config->coordinate_Z);
#ifdef Debugging
        zlog_error(category_debug,
                   "Invalid 3D coordinates. X-, Y-  or Z- are not "
                   "positive. X=[%s], Y=[%s], Z=[%s]",
                   config->coordinate_X, config->coordinate_Y,
                   config->coordinate_Z);
#endif
        return E_INPUT_PARAMETER;
    }

    sprintf(config->uuid, "000000%X%X0000",
            coordinate_Z_int/16,
            coordinate_Z_int%16);

    memset(coordinate, 0, sizeof(coordinate));
    sprintf(coordinate, "%.8f", atof(config->coordinate_X));
    temp_coordinate = strstr(coordinate, FRACTION_DOT);
    temp_coordinate = temp_coordinate + strlen(FRACTION_DOT);
    strcat(config->uuid, temp_coordinate);

    strcat(config->uuid, "0000");

    memset(coordinate, 0, sizeof(coordinate));
    sprintf(coordinate, "%.8f", atof(config->coordinate_Y));
    temp_coordinate = strstr(coordinate, FRACTION_DOT);
    temp_coordinate = temp_coordinate + strlen(FRACTION_DOT);
    strcat(config->uuid, temp_coordinate);

#ifdef Debugging
    zlog_info(category_debug, "Generated UUID: [%s]", config->uuid);
#endif

    return WORK_SUCCESSFULLY;
}

ErrorCode get_config(Config *config, char *file_name) {
    /* Return value is a struct containing all config information */
    int retry_times = 0;
    FILE *file = NULL;

    /* Create spaces for storing the string of the current line being read */
    char config_setting[CONFIG_BUFFER_SIZE];
    char *config_message = NULL;

    retry_times = FILE_OPEN_RETRY;
    while(retry_times--){
        file = fopen(file_name, "r");

        if(NULL != file){
            break;
        }
    }

    if (NULL == file) {
        zlog_error(category_health_report,
                   "Error openning file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning file");
#endif
        return E_OPEN_FILE;
    }

    /* Keep reading each line and store into the config struct */

    /* item 1 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    memset(config->coordinate_X, 0, sizeof(config->coordinate_X));
    memcpy(config->coordinate_X, config_message, strlen(config_message));

    /* item 2 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    memset(config->coordinate_Y, 0, sizeof(config->coordinate_Y));
    memcpy(config->coordinate_Y, config_message, strlen(config_message));

    /* item 3 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    memset(config->coordinate_Z, 0, sizeof(config->coordinate_Z));
    memcpy(config->coordinate_Z, config_message, strlen(config_message));

    /* item 4 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->lowest_basement_level = atoi(config_message);

    /* item 5 */
    if(WORK_SUCCESSFULLY != generate_uuid(config)){

        zlog_error(category_health_report,
                   "Unable to generate uuid");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to generate uuid");
#endif
        return E_INPUT_PARAMETER;
    }

#ifdef Debugging
    zlog_info(category_debug, "Generated UUID: [%s]", config->uuid);
#endif

    /* item 6 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->advertise_dongle_id = atoi(config_message);

    /* item 7 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->advertise_rssi_value = atoi(config_message);

    /* item 8 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->scan_rssi_coverage = atoi(config_message);

    /* item 9 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    memset(config->gateway_addr, 0, sizeof(config->gateway_addr));
    memcpy(config->gateway_addr, config_message, strlen(config_message));

    /* item 10 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->gateway_port = atoi(config_message);

    memset(g_config.local_addr, 0, sizeof(g_config.local_addr));

    /* item 11 */
    fgets(config_setting, sizeof(config_setting), file);
    config_message = strstr((char *)config_setting, DELIMITER);
    config_message = config_message + strlen(DELIMITER);
    trim_string_tail(config_message);
    config->local_client_port = atoi(config_message);

    zlog_info(category_health_report,
              "Gateway conn: addr=[%s], port=[%d], client_port=[%d]",
              config->gateway_addr, config->gateway_port,
              config->local_client_port);
#ifdef Debugging
    zlog_info(category_debug,
              "Gateway conn: addr=[%s], port=[%d], client_port=[%d]",
              config->gateway_addr, config->gateway_port,
              config->local_client_port);
#endif

    fclose(file);

    return WORK_SUCCESSFULLY;
}

void send_to_push_dongle(bdaddr_t *bluetooth_device_address,
                         DeviceType device_type,
                         char *name,
                         int rssi) {

    /* Stores the MAC address as a string */
    char address[LENGTH_OF_MAC_ADDRESS];
    struct ScannedDevice *temp_node;

    /* Converts the bluetooth device address to a string */
    ba2str(bluetooth_device_address, address);
    strcat(address, "\0");

    /* Check whether the MAC address has been seen recently by the LBeacon.*/
    switch(device_type){
        case BLE:
            temp_node = check_is_in_list(address, &BLE_object_list_head, rssi);
            break;
        case BR_EDR:
            /* BR_EDR devices including BR_EDR phone (feature phone):
            scanned_list should have distinct nodes. So we use scanned_list_head
            for checking the existance of MAC address here.
            */
            temp_node = check_is_in_list(address, &scanned_list_head, rssi);
            break;
        default:
#ifdef Debugging
            zlog_error(category_debug, "Unknown device_type=[%d]",
                       device_type);
#endif
            return;
    }

    if(NULL != temp_node){
        /* Update the final scan time */
        temp_node->final_scanned_time = get_system_time();
        /* use the strongest singal strength */
        if(rssi > temp_node->rssi){
            temp_node->rssi = rssi;
        }
        return;
    }

    /* The address is new. */

    /* Allocate memory from memory pool for a new node, initialize the
    node, and insert the new node to the scanned_list_head and
    BR_object_list_head if the address is that of a BR/EDR device;
    else if it is a BLE device, insert the new node into the
    BLE_object_list_head. */

#ifdef Debugging
    zlog_debug(category_debug,
               "New device: device_type[%d] - %17s - %20s - RSSI %4d",
               device_type, address, name, rssi);
#endif

    temp_node = (struct ScannedDevice*) mp_alloc(&mempool);
    if(NULL == temp_node){
        zlog_error(category_health_report,
                   "Unable to get memory from mp_alloc(). "
                   "Skip this new device.");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to get memory from mp_alloc(). "
                   "Skip this new device.");
#endif
        return;
    }

    /* Initialize the list entries */
    init_entry(&temp_node->sc_list_entry);
    init_entry(&temp_node->tr_list_entry);

    /* Get the initial scan time for the new node. */
    temp_node->initial_scanned_time = get_system_time();
    temp_node->final_scanned_time = temp_node->initial_scanned_time;
    temp_node->rssi = rssi;

    /* Copy the MAC address to the node */
    strncpy(temp_node->scanned_mac_address, address,
            LENGTH_OF_MAC_ADDRESS);

    /* Insert the new node into the right lists. */
    pthread_mutex_lock(&list_lock);

    if(BLE == device_type){

        /* Insert the new node to the BLE_object_list_head */
        insert_list_tail(&temp_node->tr_list_entry,
                         &BLE_object_list_head.list_entry);

    }else if(BR_EDR == device_type){

        /* Insert the new node to the scanned list */
        insert_list_first(&temp_node->sc_list_entry,
                          &scanned_list_head.list_entry);

        /* Insert the new node to the BR_object_list_head  */
        insert_list_tail(&temp_node->tr_list_entry,
                         &BR_object_list_head.list_entry);
    }
    pthread_mutex_unlock(&list_lock);

    return;
}

int compare_mac_address(char address[],
                        ScannedDevice *node,
                        int number_digits_compared){
    int ret_val;

    /* Compare the first NUMBER_DIGITS_TO_COMPARE characters
    and only compare the whole MAC address if it matches
    the first part.
    */

    ret_val = strncmp(address, node->scanned_mac_address,
                      number_digits_compared);

    if(0 != ret_val)
        return ret_val;

    ret_val = strncmp(address, node->scanned_mac_address,
                      strlen(address));

    return ret_val;
}

struct ScannedDevice *check_is_in_list(char address[],
                                       ObjectListHead *list,
                                       int rssi) {
    struct List_Entry *list_pointer, *save_list_pointers;
    ScannedDevice *temp = NULL;
    bool temp_is_null = true;
    bool is_empty = false;
    bool is_to_remove_from_scanned_list = false;

    /* If there is no node in the list, reutrn NULL directly. */
    pthread_mutex_lock(&list_lock);

    is_empty = is_entry_list_empty(&list->list_entry);

    pthread_mutex_unlock(&list_lock);

    if(is_empty){
        return NULL;
    }

    /* Go through the list to check whether the input address is in
    the list.
    */
    switch(list->device_type){
        case BR_EDR:
            pthread_mutex_lock(&list_lock);

            list_for_each_safe(list_pointer, save_list_pointers,
                               &list->list_entry) {

                /* BR_EDR device, e.g a BR_EDR phone (feature phone):
                Use scanned_list_head for checking the existance of MAC
                address here.
                */
                temp = ListEntry(list_pointer, ScannedDevice,
                                 sc_list_entry);

                if(true == is_to_remove_from_scanned_list){

                    /* The node has been in the scanned list for more than
                    INTERVAL_FOR_CLEANUP_SCANNED_LIST_IN_SEC seconds. Remove
                    it from the scanned list directly.
                    */
                    remove_list_node(&temp->sc_list_entry);

                    /* If the node no longer is in the BR_object_list_head,
                    free the space back to the memory pool.
                    */
                    if(is_isolated_node(&temp->tr_list_entry)){
#ifdef Debugging
                        zlog_debug(category_debug,
                                   "Remove scanned list [%s] from "
                                   "scanned_list_head",
                                   temp->scanned_mac_address);
#endif
                        mp_free(&mempool, temp);
                    }

                }else if (get_system_time() - temp->initial_scanned_time >
                          INTERVAL_FOR_CLEANUP_SCANNED_LIST_IN_SEC){
                    /* If the device has been in the scanned list for at
                    least INTERVAL_FOR_CLEANUP_SCANNED_LIST_IN_SEC seconds,
                    remove its struct node from the scanned list here.
                    All remaining entries in the scanned list have all
                    been there for more than
                    INTERVAL_FOR_CLEANUP_SCANNED_LIST_IN_SEC seconds.
                    Set the flag is_to_remove_from_scanned_list to true
                    */
                    is_to_remove_from_scanned_list = true;
                    remove_list_node(&temp->sc_list_entry);

                    /* If the node no longer is in the BR object list, free
                    the space back to the memory pool.
                    */
                    if(is_isolated_node(&temp->tr_list_entry)){
#ifdef Debugging
                        zlog_debug(category_debug,
                                   "Remove scanned list [%17s] "
                                   "from scanned_list_head",
                                   temp->scanned_mac_address);
#endif
                        mp_free(&mempool, temp);
                    }

                }else if (0 == compare_mac_address(address, temp,
                                                   NUMBER_DIGITS_TO_COMPARE) &&
                          0 == compare_mac_address(address, temp,
                                                   LENGTH_OF_MAC_ADDRESS)){
                    temp_is_null = false;
                    break;
                }
            } // list for each safe

            pthread_mutex_unlock(&list_lock);

            break;

        case BLE:

            pthread_mutex_lock(&list_lock);

            list_for_each_safe(list_pointer, save_list_pointers,
                               &list->list_entry) {

                temp = ListEntry(list_pointer, ScannedDevice,
                                 tr_list_entry);

                if (0 == compare_mac_address(address, temp,
                                             NUMBER_DIGITS_TO_COMPARE) &&
                    0 == compare_mac_address(address, temp,
                                             LENGTH_OF_MAC_ADDRESS)){
                    temp_is_null = false;
                    break;
                }
            }

            pthread_mutex_unlock(&list_lock);

            break;

        default:

            zlog_error(category_health_report,
                       "Unknown device type=[%d]",
                       list->device_type);
#ifdef Debugging
            zlog_error(category_debug,
                       "Unknown device type=[%d]",
                       list->device_type);
#endif
            break;
    }  // end of swtich

    if(true == temp_is_null){
        return NULL;
    }

    return temp;
}

ErrorCode enable_advertising(int dongle_device_id,
                             int advertising_interval,
                             char *advertising_uuid,
                             int major_number,
                             int minor_number,
                             int rssi_value) {
#ifdef Debugging
    zlog_debug(category_debug, ">> enable_advertising ");
#endif
    int device_handle = 0;
    int retry_times = 0;
    uint8_t status;
    struct hci_request request;
    int return_value = 0;
    uint8_t segment_length = 1;
    unsigned int *uuid = NULL;
    int uuid_iterator;

#ifdef Debugging
    zlog_info(category_debug, "Using dongle id [%d]\n", dongle_device_id);
#endif

    if (dongle_device_id < 0){
        zlog_error(category_health_report,
                   "Error openning the device");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning the device");
#endif
        return E_OPEN_DEVICE;
    }

    retry_times = SOCKET_OPEN_RETRY;
    while(retry_times--){
        device_handle = hci_open_dev(dongle_device_id);

        if(device_handle >= 0){
            break;
        }
    }

    if (device_handle < 0) {
        zlog_error(category_health_report,
                   "Error openning socket");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning socket");
#endif
        return E_OPEN_DEVICE;
    }

    le_set_advertising_parameters_cp advertising_parameters_copy;
    memset(&advertising_parameters_copy, 0,
           sizeof(advertising_parameters_copy));
    advertising_parameters_copy.min_interval = htobs(advertising_interval);
    advertising_parameters_copy.max_interval = htobs(advertising_interval);
    /* advertising non-connectable */
    advertising_parameters_copy.advtype = 3;
    /*set bitmap to 111 (i.e., circulate on channels 37,38,39) */
    advertising_parameters_copy.chan_map = 7; /* all three advertising
                                              channels*/

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISING_PARAMETERS;
    request.cparam = &advertising_parameters_copy;
    request.clen = LE_SET_ADVERTISING_PARAMETERS_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    if (return_value < 0) {
        /* Error handling */
        hci_close_dev(device_handle);
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    le_set_advertise_enable_cp advertisement_copy;
    memset(&advertisement_copy, 0, sizeof(advertisement_copy));
    advertisement_copy.enable = 0x01;

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    request.cparam = &advertisement_copy;
    request.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    if (return_value < 0) {
        /* Error handling */
        hci_close_dev(device_handle);
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    le_set_advertising_data_cp advertisement_data_copy;
    memset(&advertisement_data_copy, 0, sizeof(advertisement_data_copy));

    /* The Advertising data consists of one or more Advertising Data (AD)
    elements. Each element is formatted as follows:

    1st byte: length of the element (excluding the length byte itself)
    2nd byte: AD type – specifies what data is included in the element
    AD data - one or more bytes - the meaning is defined by AD type
    */

    /* 1. Fill the EIR_FLAGS type (0x01 in Bluetooth AD type)
    related information
    */
    segment_length = 1;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(EIR_FLAGS);
    segment_length++;

    /* FLAG information is carried in bits within the flag are as listed below,
    and we choose to use
    0x1A (i.e., 00011010) setting.
    bit 0: LE Limited Discoverable Mode
    bit 1: LE General Discoverable Mode
    bit 2: BR/EDR Not Supported
    bit 3: Simultaneous LE and BR/EDR to Same Device Capable (Controller)
    bit 4: Simultaneous LE and BR/EDR to Same Device Capable (Host)
    bit 5-7: Reserved
    */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x1A);
    segment_length++;

    /* Fill the length for EIR_FLAGS type (0x01 in Bluetooth AD type) */
    advertisement_data_copy
        .data[advertisement_data_copy.length] =
        htobs(segment_length - 1);

    advertisement_data_copy.length += segment_length;

    /* 2. Fill the EIR_MANUFACTURE_SPECIFIC_DATA (0xFF in Bluetooth AD type)
    related information
    */
    segment_length = 1;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(EIR_MANUFACTURE_SPECIFIC_DATA);
    segment_length++;

    /* The first two bytes of EIR_MANUFACTURE_SPECIFIC_DATA type is the company
    identifier
    https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers

    For Raspberry Pi, we should use 0x000F to specify the manufacturer as
    Broadcom Corporation.
    */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x0F);
    segment_length++;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x00);
    segment_length++;

    /* The next byte is Subtype. For beacon-like, we should use 0x02 for iBeacon
    type.
    */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x02);
    segment_length++;

    /* The next byte is the Subtype length of following beacon-like information.
    They are pre-defined and fixed as 0x15 = 21 bytes with following format:

    16 bytes: Proximity UUID
    2 bytes: Major version
    2 bytes: Minor version
    1 byte: Signal power
    */

    /* Subtype length is pre-defined and fixed as 0x15 for beacon-like
    information*/
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(0x15);
    segment_length++;

    /* 16 bytes: Proximity UUID */
    uuid = uuid_str_to_data(advertising_uuid);

    for (uuid_iterator = 0;
         uuid_iterator < strlen(advertising_uuid) / 2;
         uuid_iterator++) {

        advertisement_data_copy
            .data[advertisement_data_copy.length + segment_length] =
            htobs(uuid[uuid_iterator]);

        segment_length++;
    }

    /* 2 bytes: Major number */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(major_number >> 8 & 0x00FF);
    segment_length++;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(major_number & 0x00FF);
    segment_length++;

    /* 2 bytes: Minor number */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(minor_number >> 8 & 0x00FF);
    segment_length++;
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(minor_number & 0x00FF);
    segment_length++;

    /* 1 byte: Signal power (also known as RSSI calibration) */
    advertisement_data_copy
        .data[advertisement_data_copy.length + segment_length] =
        htobs(twoc(rssi_value, 8));
    segment_length++;

    /* Fill the length for EIR_MANUFACTURE_SPECIFIC_DATA type
    (0xFF in Bluetooth AD type) */
    advertisement_data_copy.data[advertisement_data_copy.length] =
        htobs(segment_length - 1);

    advertisement_data_copy.length += segment_length;

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISING_DATA;
    request.cparam = &advertisement_data_copy;
    request.clen = LE_SET_ADVERTISING_DATA_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    hci_close_dev(device_handle);

    if (return_value < 0) {
        /* Error handling */
        zlog_error(category_health_report,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't send request %s (%d)", strerror(errno),
                   errno);
#endif
        return E_SEND_REQUEST_TIMEOUT;
    }

    if (status) {
        /* Error handling */
        zlog_error(category_health_report,
                   "LE set advertise returned status %d", status);
#ifdef Debugging
        zlog_error(category_debug,
                   "LE set advertise returned status %d", status);
#endif
        return E_ADVERTISE_STATUS;
    }
#ifdef Debugging
    zlog_debug(category_debug, "<< enable_advertising ");
#endif
    return WORK_SUCCESSFULLY;
}


ErrorCode disable_advertising(int dongle_device_id) {
    int device_handle = 0;
    int retry_times = 0;
    uint8_t status;
    struct hci_request request;
    int return_value = 0;
    le_set_advertise_enable_cp advertisement_copy;

#ifdef Debugging
    zlog_debug(category_debug, ">> disable_advertising ");
#endif

    /* Open Bluetooth device */
    retry_times = DONGLE_GET_RETRY;

    if (dongle_device_id < 0) {
        zlog_error(category_health_report,
                   "Error openning the device");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning the device");
#endif
        return E_OPEN_DEVICE;
    }

    retry_times = SOCKET_OPEN_RETRY;
    while(retry_times--){
        device_handle = hci_open_dev(dongle_device_id);

        if(device_handle >= 0){
            break;
        }
    }

    if (device_handle < 0) {
        zlog_error(category_health_report,
                   "Error openning socket");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning socket");
#endif
        return E_OPEN_DEVICE;
    }

    memset(&advertisement_copy, 0, sizeof(advertisement_copy));

    memset(&request, 0, sizeof(request));
    request.ogf = OGF_LE_CTL;
    request.ocf = OCF_LE_SET_ADVERTISE_ENABLE;
    request.cparam = &advertisement_copy;
    request.clen = LE_SET_ADVERTISE_ENABLE_CP_SIZE;
    request.rparam = &status;
    request.rlen = 1; /* length of request.rparam */

    return_value = hci_send_req(device_handle, &request,
                                HCI_SEND_REQUEST_TIMEOUT_IN_MS);

    hci_close_dev(device_handle);

    if (return_value < 0) {
        /* Error handling */
        zlog_error(category_health_report,
                   "Can't set advertise mode: %s (%d)",
                   strerror(errno), errno);
#ifdef Debugging
        zlog_error(category_debug,
                   "Can't set advertise mode: %s (%d)",
                   strerror(errno), errno);
#endif
        return E_ADVERTISE_MODE;
    }

    if (status) {
        /* Error handling */
        zlog_error(category_health_report,
                   "LE set advertise enable on returned status %d",
                   status);
#ifdef Debugging
        zlog_error(category_debug,
                   "LE set advertise enable on returned status %d",
                   status);
#endif
        return E_ADVERTISE_STATUS;
    }

#ifdef Debugging
    zlog_debug(category_debug, "<< disable_advertising ");
#endif
    return WORK_SUCCESSFULLY;
}


ErrorCode beacon_basic_info(char *message, size_t message_size, int poll_type){
    char timestamp[LENGTH_OF_EPOCH_TIME];

    // packet type
    message[0] = 0x0F & poll_type;
    message[1] = '\0';

    // LBeacon UUID
    strcat(message, g_config.uuid);
    strcat(message, ";");

    // LBeacon datetime
    memset(timestamp, 0, sizeof(timestamp));
    snprintf(timestamp, sizeof(timestamp), "%d", get_system_time());
    strcat(message, timestamp);
    strcat(message, ";");

    // Local IP address
    strcat(message, g_config.local_addr);
    strcat(message, ";");

    /* Make sure the resulted message (basic information) does not
	exceed our expected length
    */
    if(strlen(message) > MAX_LENGTH_RESP_BASIC_INFO){
        zlog_error(category_health_report,
                   "Error in beacon_basic_info(), the length of basic "
                   "information is [%d], and limitation is [%d].",
                   strlen(message), MAX_LENGTH_RESP_BASIC_INFO);
#ifdef Debugging
        zlog_error(category_debug,
                   "Error in beacon_basic_info(), the length of basic "
                   "information is [%d], and limitation is [%d].",
                   strlen(message), MAX_LENGTH_RESP_BASIC_INFO);
#endif
        return E_BUFFER_SIZE;
    }

    return WORK_SUCCESSFULLY;
}

ErrorCode send_join_request(){
    char message[WIFI_MESSAGE_LENGTH];
    int ret_val = 0;

    memset(message, 0, sizeof(message));

    if(0 != beacon_basic_info(message, sizeof(message), request_to_join)){

        zlog_error(category_health_report,
                   "Unable to prepare basic information for response. "
                   "Abort sending this response to gateway.");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to prepare basic information for response. "
                   "Abort sending this response to gateway.");
#endif
        return E_PREPARE_RESPONSE_BASIC_INFO;
    }

    ret_val = addpkt(&udp_config.send_pkt_queue,
                     UDP, udp_config.send_ipv4_addr, message,
                     sizeof(message));

    if(pkt_Queue_SUCCESS != ret_val){
        zlog_error(category_health_report,
                   "Unable to add packet to queue, error=[%d]",
                   ret_val);
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to add packet to queue, error=[%d]",
                   ret_val);
#endif
        return E_ADD_PACKET_TO_QUEUE;
    }
    return WORK_SUCCESSFULLY;
}


ErrorCode handle_join_response(char *resp_payload){
    char buf[WIFI_MESSAGE_LENGTH];
    char *lbeacon_uuid = NULL;
    char *lbeacon_timestamp = NULL;
    char *lbeacon_ip = NULL;
    char *tail = NULL;

#ifdef Debugging
    zlog_info(category_debug,
              "Received join_request_ack payload=[%s]", resp_payload);
#endif
    memset(buf, 0, sizeof(buf));
    strcpy(buf, resp_payload);

    lbeacon_uuid = buf;
    tail = strstr(lbeacon_uuid, ";");
    if(NULL == tail)
        return E_PARSE_JOIN_RESPONSE;
    *tail = '\0';

    lbeacon_timestamp = tail + 1;
    tail = strstr(lbeacon_timestamp, ";");
    if(NULL == tail)
        return E_PARSE_JOIN_RESPONSE;
    *tail = '\0';

    lbeacon_ip = tail + 1;
    tail = strstr(lbeacon_ip, ";");
    if(NULL == tail)
        return E_PARSE_JOIN_RESPONSE;
    *tail = '\0';

#ifdef Debugging
    zlog_debug(category_debug, "Parsed results: uuid=[%s], "\
               "timestamp=[%s], ip=[%s]\n", lbeacon_uuid,
               lbeacon_timestamp, lbeacon_ip);
#endif

    memset(g_config.local_addr, 0, sizeof(g_config.local_addr));
    strcpy(g_config.local_addr, lbeacon_ip);

#ifdef Debugging
    zlog_debug(category_debug, "LBeacon IP address: [%s]\n",
               g_config.local_addr);
#endif

    return WORK_SUCCESSFULLY;
}

ErrorCode handle_tracked_object_data(){
    char message[WIFI_MESSAGE_LENGTH];
    FILE *br_object_file = NULL;
    FILE *ble_object_file = NULL;
    bool is_br_object_list_empty = false;
    bool is_ble_object_list_empty = false;
    char msg_temp_one[WIFI_MESSAGE_LENGTH];
    char msg_temp_two[WIFI_MESSAGE_LENGTH];
    int max_objects = 0;
    int used_objects = 0;
    int ret_val = 0;

    /* return directly, if both BR and BLE tracked lists are emtpy
    */
    pthread_mutex_lock(&list_lock);

    is_br_object_list_empty =
        is_entry_list_empty(&BR_object_list_head.list_entry);
    is_ble_object_list_empty =
        is_entry_list_empty(&BLE_object_list_head.list_entry);

    pthread_mutex_unlock(&list_lock);

    if(is_br_object_list_empty && is_ble_object_list_empty){
#ifdef Debugging
        zlog_debug(category_debug, "Both BR and BLE lists are empty.");
#endif
        return WORK_SUCCESSFULLY;
    }

    /* Copy track_object data to a file to be transmitted.
    */
    memset(message, 0, sizeof(message));
    memset(msg_temp_one, 0, sizeof(msg_temp_one));
    memset(msg_temp_two, 0, sizeof(msg_temp_two));

    max_objects = (sizeof(message) - MAX_LENGTH_RESP_BASIC_INFO)/
                  MAX_LENGTH_RESP_DEVICE_INFO;

    if(WORK_SUCCESSFULLY !=
        consolidate_tracked_data(&BR_object_list_head,
                                 msg_temp_one, sizeof(msg_temp_one),
                                 max_objects, &used_objects)){

        zlog_error(category_health_report,
            "Unable to consolidate BR_EDR devices, "
            "abort BR_EDR devices this time.");
#ifdef Debugging
        zlog_error(category_debug,
            "Unable to consolidate BR_EDR devices, "
            "abort BR_EDR devices this time.");
#endif
    }

    max_objects = (sizeof(message) -
                  MAX_LENGTH_RESP_BASIC_INFO -
                  used_objects * MAX_LENGTH_RESP_DEVICE_INFO)/
                  MAX_LENGTH_RESP_DEVICE_INFO;

    if(WORK_SUCCESSFULLY !=
        consolidate_tracked_data(&BLE_object_list_head,
                                 msg_temp_two, sizeof(msg_temp_two),
                                 max_objects, &used_objects)){

        zlog_error(category_health_report,
                    "Unable to consolidate BLE devices, "
                    "abort BLE devices this time.");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to consolidate BLE devices, "
                   "abort BLE devices this time.");
#endif
    }

    if(0 != beacon_basic_info(message, sizeof(message),
                              tracked_object_data)){

        zlog_error(category_health_report,
                   "Unable to prepare basic information for response. "
                   "Abort sending this response to gateway.");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to prepare basic information for response. "
                   "Abort sending this response to gateway.");
#endif
        return E_PREPARE_RESPONSE_BASIC_INFO;
    }

    if(sizeof(message) <= strlen(message) + strlen(msg_temp_one) +
        strlen(msg_temp_two)){

        zlog_error(category_health_report,
                   "Abort BR/BLE tracked data, because there is "
                   "potential buffer overflow. strlen(message)=%d, "
                   "strlen(msg_temp_one)=%d,strlen(msg_temp_two)=%d",
                   strlen(message), strlen(msg_temp_one),
                   strlen(msg_temp_two));
 #ifdef Debugging
        zlog_error(category_debug,
                   "Abort BR/BLE tracked data, because there is "
                   "potential buffer overflow. strlen(message)=%d, "
                   "strlen(msg_temp_one)=%d,strlen(msg_temp_two)=%d",
                   strlen(message), strlen(msg_temp_one),
                   strlen(msg_temp_two));
 #endif
        return E_BUFFER_SIZE;
    }

    strcat(message, msg_temp_one);
    strcat(message, msg_temp_two);
    ret_val = addpkt(&udp_config.send_pkt_queue, UDP,
                     udp_config.send_ipv4_addr, message, sizeof(message));

    if(pkt_Queue_SUCCESS != ret_val){
        zlog_error(category_health_report,
                   "Unable to add packet to queue, error=[%d]",
                   ret_val);
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to add packet to queue, error=[%d]",
                   ret_val);
#endif
        return E_ADD_PACKET_TO_QUEUE;
    }
    return WORK_SUCCESSFULLY;
}

ErrorCode handle_health_report(){
    char message[WIFI_MESSAGE_LENGTH];
    char msg_temp_one[WIFI_MESSAGE_LENGTH];
    char temp_buf[WIFI_MESSAGE_LENGTH];
    FILE *health_file = NULL;
    int retry_times = 0;
    int ret_val = 0;

    retry_times = FILE_OPEN_RETRY;
    while(retry_times--){
        health_file =
        fopen(HEALTH_REPORT_LOG_FILE_NAME, "r");

        if(NULL != health_file){
            break;
        }
    }

    if(NULL == health_file){
        zlog_error(category_health_report,
                   "Error openning file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning file");
#endif
        return E_OPEN_FILE;
    }

    /* contructs the content for UDP packet*/
    memset(message, 0, sizeof(message));

    if(0!=beacon_basic_info(message, sizeof(message), health_report)){

        zlog_error(category_health_report,
                   "Unable to prepare basic information for response. "
                   "Abort sending this response to gateway.");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to prepare basic information for response. "
                   "Abort sending this response to gateway.");
#endif
        return E_PREPARE_RESPONSE_BASIC_INFO;
    }

    /* read the last line from Health_Report.log file */
    memset(temp_buf, 0, sizeof(temp_buf));

    while(!feof(health_file)){
        fgets(temp_buf, sizeof(temp_buf), health_file);
    }

    fclose(health_file);

    memset(msg_temp_one, 0, sizeof(msg_temp_one));

    if(NULL == strstr(temp_buf, HEALTH_REPORT_ERROR_SIGN)){
        sprintf(msg_temp_one, "%d;", S_NORMAL);
    }else{
        sprintf(msg_temp_one, "%d;", E_ERROR);
    }

    if(sizeof(message) <= strlen(message) + strlen(msg_temp_one)){
        zlog_error(category_health_report,
                   "Abort health report data, because there is "
                   "potential buffer overflow. strlen(message)=%d, "
                   "strlen(msg_temp_one)=%d",
                   strlen(message), strlen(msg_temp_one));

#ifdef Debugging
        zlog_error(category_debug,
                   "Abort health report data, because there is "
                   "potential buffer overflow. strlen(message)=%d, "
                   "strlen(msg_temp_one)=%d",
                   strlen(message), strlen(msg_temp_one));
#endif
        return E_BUFFER_SIZE;
    }

    strcat(message, msg_temp_one);

    ret_val = addpkt(&udp_config.send_pkt_queue, UDP,
                     udp_config.send_ipv4_addr, message, sizeof(message));

    if(pkt_Queue_SUCCESS != ret_val){
        zlog_error(category_health_report,
                   "Unable to add packet to queue, error=[%d]",
                   ret_val);
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to add packet to queue, error=[%d]",
                   ret_val);
#endif
        return E_ADD_PACKET_TO_QUEUE;
    }
    return WORK_SUCCESSFULLY;
}

ErrorCode manage_communication(){
    int gateway_latest_time = 0;
    int current_time = 0;
    int latest_join_request_time = 0;
    int poll_type;

#ifdef Debugging
    zlog_debug(category_debug, ">> manage_communication ");
#endif

    while(true == ready_to_work){
        if(true == is_null(&udp_config.recv_pkt_queue)){
            /* When LBeacon has not gotten packets from the gateway for
            INTERVAL_RECEIVE_MESSAGE_FROM_GATEWAY_IN_SEC seconds or longer,
            LBeacon sends request_to_join to gateway again. The purpose
            is to handle the gateway version upgrade scenario. In this case if
            gateway is restarted, gateway might not keep the registered
            LBeacon ID map. So LBeacon needs to send request_to_join again to
            establish the relationship with the gateway.
            */
            current_time = get_system_time();
            if((current_time - gateway_latest_time >
                INTERVAL_RECEIVE_MESSAGE_FROM_GATEWAY_IN_SEC) &&
                (current_time - latest_join_request_time >
                INTERVAL_FOR_RECONNECT_GATEWAY_IN_SEC)){
;
#ifdef Debugging
                zlog_info(category_debug,
                          "Send requets_to_join to gateway again");
#endif
                if(WORK_SUCCESSFULLY == send_join_request()){
                    latest_join_request_time = current_time;
                }
            }else{
                /* sleep a short time to prevent occupying CPU in this
                busy while loop.
                */
                usleep(INTERVAL_FOR_BUSY_WAITING_CHECK_IN_MICRO_SECONDS);
            }
        }else{
            /* Update gateway_latest_time to make LBeacon aware that
            its connection to gateway is still okay.
            */
            gateway_latest_time = get_system_time();

            poll_type = undefined;
            /* Get one packet from receive packet queue
            */
            sPkt tmp_pkt = get_pkt(&udp_config.recv_pkt_queue);
            poll_type = 0x0F & tmp_pkt.content[0];

            /* According to the polled data type, prepare a work item
            */
            switch(poll_type){

                case join_request_ack:
#ifdef Debugging
                    zlog_info(category_debug,
                              "Receive join_request_ack from gateway");
#endif
                    handle_join_response(&tmp_pkt.content[1]);
                    break; // join_request_ack case

                case tracked_object_data:
#ifdef Debugging
                    zlog_info(category_debug,
                              "Receive tracked_object_data from gateway");
#endif
                    handle_tracked_object_data();
                    break; // tracked_object_data case

                case health_report:
#ifdef Debugging
                    zlog_info(category_debug,
                              "Receive health_report from gateway");
#endif
                    handle_health_report();
                    break; // health_report case

                default:
#ifdef Debugging
                    zlog_warn(category_debug,
                              "Receive unknown packet type=[%d] from "
                              "gateway",
                              poll_type);
#endif
                    break; // default case
             } // switch
        }
    } // end of the while

#ifdef Debugging
    zlog_debug(category_debug, "<< manage_communication ");
#endif

    return WORK_SUCCESSFULLY;
}

ErrorCode copy_object_data_to_file(char *file_name,
                                   ObjectListHead *list,
                                   const int max_num_objects,
                                   int *used_objects ) {
    FILE *track_file = NULL;;
    struct List_Entry *list_pointer, *save_list_pointers;
    struct List_Entry *head_pointer, *tail_pointer;
    ScannedDevice *temp;
    int number_in_list;
    int number_to_send;
    char basic_info[MAX_LENGTH_RESP_BASIC_INFO];
    char response_buf[MAX_LENGTH_RESP_DEVICE_INFO];
    int retry_times = 0;
    int node_count;
    unsigned timestamp_init;
    unsigned timestamp_end;
    /* Head of a local list for tracked object */
    List_Entry local_list_head;
    DeviceType device_type = list->device_type;

    retry_times = FILE_OPEN_RETRY;
    while(retry_times--){
        track_file = fopen(file_name, "w");

        if(NULL != track_file){
            break;
        }
    }

    if(NULL == track_file){
        retry_times = FILE_OPEN_RETRY;
        while(retry_times--){
            track_file = fopen(file_name, "wt");

            if(NULL != track_file){
                break;
            }
        }
    }

    if(NULL == track_file){
        zlog_error(category_health_report,
                   "Error openning file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning file");
#endif
        return E_OPEN_FILE;
    }

    /* Get the number of objects with data to be transmitted */
    number_in_list = get_list_length(&list->list_entry);
    number_to_send = min(max_num_objects, number_in_list);
    *used_objects = number_to_send;

    /*Check if number_to_send is zero. If yes, no need to do more; close
    file and return */
    if(0 == number_to_send){
        sprintf(basic_info, "%d;%d;", device_type, number_to_send);
        fputs(basic_info, track_file);
        fclose(track_file);
        return WORK_SUCCESSFULLY;
    }

    /* Insert device_type and number_to_send at the start of the track
    file
    */
    sprintf(basic_info, "%d;%d;", device_type, number_to_send);
    fputs(basic_info, track_file);

#ifdef Debugging
    zlog_debug(category_debug,
               "Device type: %d; Number to send: %d",
               device_type, number_to_send);
#endif

    pthread_mutex_lock(&list_lock);

    /* This code block is for debugging the linked list operations. In release
    version, we should not waste resource in iterating the linked list only
    ensure the correctness.

#ifdef Debugging

    list_for_each(list_pointer, &list->list_entry){
        zlog_debug(category_debug,
                   "Input list: list->list_entry %d list_pointer %d "
                   "prev %d next %d",
                   &list->list_entry,
                   list_pointer,
                   list_pointer->prev,
                   list_pointer->next);
    }

#endif
*/

    /* Set temporary pointer to point to the head of the input list */
    head_pointer = list->list_entry.next;
    list_pointer = list->list_entry.next;

    /* Go through the input tracked_object list to move
    number_to_send nodes in the list to a local list
    */
    for (node_count = 1; node_count <= number_to_send;
         list_pointer = list_pointer->next, node_count++){

        /* If the node is the last in the list */
        if(node_count == number_to_send){
            /* Set a marker for the last pointer of the last node */
            tail_pointer = list_pointer;
        }
    }

    /* Set the head of the input list to point to the last node */
    list->list_entry.next = tail_pointer->next;
    tail_pointer->next->prev = &list->list_entry;

    pthread_mutex_unlock(&list_lock);

    /* Initialize the local list */
    init_entry(&local_list_head);
    local_list_head.next = head_pointer;
    head_pointer->prev = &local_list_head;
    local_list_head.prev = tail_pointer;
    tail_pointer->next = &local_list_head;

    /* This code block is for debugging the linked list operations. In release
    version, we should not waste resource in iterating the linked list only
    ensure the correctnedd.

#ifdef Debugging

    list_for_each(list_pointer, &local_list_entry){
        zlog_debug(category_debug,
                   "local list:  list_pointer %d prev %d next %d",
                   list_pointer,
                   list_pointer->prev,
                   list_pointer->next);
    }

#endif
*/

    /* Go throngh the local object list to get the content and write the
    content to file
    */
    list_for_each(list_pointer, &local_list_head){

        temp = ListEntry(list_pointer, ScannedDevice, tr_list_entry);

        /* sprintf() is the function to set a format and convert the
        datatype to char
        */
        memset(response_buf, 0, sizeof(response_buf));
        sprintf(response_buf, "%s;%d;%d;%d;",
                temp->scanned_mac_address,
                temp->initial_scanned_time,
                temp->final_scanned_time,
                temp->rssi);

        /* Write the content to the file */
        fputs(response_buf, track_file);
    }

    /* Remove nodes from the local list. If the node is no longer in the scan
    list, release the allocated memory as well.
    */
    if(BLE == device_type){

        list_for_each_safe(list_pointer,
                           save_list_pointers,
                           &local_list_head){

            temp = ListEntry(list_pointer, ScannedDevice, tr_list_entry);

            remove_list_node(&temp->tr_list_entry);

            mp_free(&mempool, temp);
        }

    }else if(BR_EDR == device_type){
        /* If the device is of BR_EDR type, each node is linked into both
        the scanned list and the BR object list using sc_list_entry and
        tr_list_entry. We should lock list_lock here to prevent scanned list
        is operated in other places at the same time.
        */

        list_for_each_safe(list_pointer,
                           save_list_pointers,
                           &local_list_head){

            temp = ListEntry(list_pointer, ScannedDevice, tr_list_entry);

            remove_list_node(&temp->tr_list_entry);

            pthread_mutex_lock(&list_lock);

            if(is_isolated_node(&temp->sc_list_entry)){
                mp_free(&mempool, temp);
            }

            pthread_mutex_unlock(&list_lock);
        }
    }

    /* Close the file for storing data in the input list */
    fclose(track_file);

    return WORK_SUCCESSFULLY;
}


ErrorCode consolidate_tracked_data(ObjectListHead *list,
                                   char *msg_buf,
                                   size_t msg_size,
                                   const int max_num_objects,
                                   int *used_objects){
    ErrorCode ret_val;
    FILE *file_fd = NULL;
    char *file_name = NULL;
    int retry_times = 0;

    /* Check input parameters to determine whether they are valid */
    if(list != &BR_object_list_head && list != &BLE_object_list_head){
        zlog_error(category_health_report,
                   "Error of invalid input parameter, list is neither BR "
                   "nor BLE list");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error of invalid input parameter, list is neither BR "
                   "nor BLE list");
#endif
        return E_INPUT_PARAMETER;
    }

    if(BR_EDR == list->device_type){
        file_name = TRACKED_BR_TXT_FILE_NAME;
    }else if(BLE == list->device_type){
        file_name = TRACKED_BLE_TXT_FILE_NAME;
    }else{
        zlog_error(category_health_report,
                   "Error of invalid input parameter, "
                   "list device_type=[%d]",
                   list->device_type);
#ifdef Debugging
        zlog_error(category_debug,
                   "Error of invalid input parameter, "
                   "list device_type=[%d]",
                   list->device_type);
#endif
        return E_INPUT_PARAMETER;
    }

    ret_val = copy_object_data_to_file(file_name, list,
                                       max_num_objects, used_objects);

    if(WORK_SUCCESSFULLY == ret_val){
        /* Open the file that is going to be sent to the gateway */
        retry_times = FILE_OPEN_RETRY;
        while(retry_times--){
            file_fd = fopen(file_name, "r");

            if(NULL != file_fd){
                break;
            }
        }

        if (NULL == file_fd){
            zlog_error(category_health_report,
                       "Error openning file");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error openning file");
#endif
            return E_OPEN_FILE;
        }

        fgets(msg_buf, msg_size, file_fd);
        fclose(file_fd);
    }

    return ret_val;
}

/* A static struct function that returns specific bluetooth BLE request. */
const struct hci_request ble_hci_request(uint16_t ocf,
                                         int clen,
                                         void * status,
                                         void * cparam){
    struct hci_request rq;
    memset(&rq, 0, sizeof(rq));
    rq.ogf = OGF_LE_CTL;
    rq.ocf = ocf;
    rq.cparam = cparam;
    rq.clen = clen;
    rq.rparam = status;
    rq.rlen = 1; /* length of request.rparam */

    return rq;
}

/* A static function to prase the uuid from the BLE device. */
static ErrorCode eir_parse_uuid(uint8_t *eir,
                           size_t eir_len,
                           char *buf,
                           size_t buf_len){
    size_t offset;
    uint8_t field_len;
    size_t uuid_len;
    int index;
    int i;
	bool has_uuid = false;

    offset = 0;

    while (offset < eir_len) {
        field_len = eir[0];

        /* Check for the end of EIR */
        if (field_len == 0)
            break;

        if (offset + field_len > eir_len)
            goto failed;

        switch (eir[1]) {
            case EIR_MANUFACTURE_SPECIFIC_DATA:
                uuid_len = field_len - 1;

                if (uuid_len > buf_len)
                    goto failed;
            
			    // according to advertising payload, payload content of 
				// index 6-21 is uuid
			    index = 0;
                if(eir[2] == 15 && eir[3] == 0 && eir[4] == 2 && eir[5] == 21){
                    if(eir[6] == 0 && eir[7] == 0 && eir[8] == 0){
                        for(i = 6 ; i < 22 ; i++){
                            buf[index] = eir[i] / 16 + '0';
                            buf[index+1] = eir[i] % 16 + '0';
                            index=index+2;
                        }
                        buf[index] = '\0';
                        has_uuid = true;
					}
                }
				
				if(true == has_uuid){
					return WORK_SUCCESSFULLY;
				}
			return E_PARSE_UUID;
        }

        offset += field_len + 1;
        eir += field_len + 1;
    }

failed:
    snprintf(buf, buf_len, NULL);
    return E_PARSE_UUID;
}


ErrorCode *start_ble_scanning(void *param){
    /* A buffer for the callback event */
    uint8_t ble_buffer[HCI_MAX_EVENT_SIZE];
    int socket = 0; /* socket number */
    int dongle_device_id = 0; /* dongle id */
    int ret, opt, status, len;
    struct hci_filter new_filter; /* Filter for controlling the events*/
    evt_le_meta_event *meta;
    le_advertising_info *info;
    le_set_event_mask_cp event_mask_cp;
    int retry_times = 0;
    struct hci_request scan_params_rq;
    struct hci_request set_mask_rq;
    /* Time interval is 0.625ms */
    uint16_t interval = htobs(0x0010); /* 16*0.625ms = 10ms */
    uint16_t window = htobs(0x0010); /* 16*0.625ms = 10ms */
    int i=0;
    uint8_t reports_count;
    char name[LENGTH_OF_DEVICE_NAME];
    int rssi;
    char address[LENGTH_OF_MAC_ADDRESS];
    char uuid[LENGTH_OF_UUID];

#ifdef Debugging
    zlog_debug(category_debug, ">> start_ble_scanning... ");
#endif

    while(true == ready_to_work){
        /* Get the dongle id */
        retry_times = DONGLE_GET_RETRY;
        while(retry_times--){
            dongle_device_id = hci_get_route(NULL);

            if(dongle_device_id >= 0){
                break;
            }
        }

        if (dongle_device_id < 0) {
            zlog_error(category_health_report,
                       "Error openning the device");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error openning the device");
#endif
            return E_OPEN_DEVICE;
        }

        /* Open Bluetooth device */
        retry_times = SOCKET_OPEN_RETRY;
        while(retry_times--){
            socket = hci_open_dev(dongle_device_id);

            if(socket >= 0){
                break;
            }
        }
        if (socket < 0) {
            zlog_error(category_health_report,
                       "Error openning socket");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error openning socket");
#endif
             return E_OPEN_SOCKET;
        }

        /* Set BLE scan parameters */
        if( 0> hci_le_set_scan_parameters(socket, 0x01, interval,
                                          window, 0x00, 0x00,
                                          HCI_SEND_REQUEST_TIMEOUT_IN_MS)){

            zlog_info(category_health_report,
                      "Error setting parameters of BLE scanning");
#ifdef Debugging
            zlog_debug(category_debug,
                      "Error setting parameters of BLE scanning");
#endif
        }

        if( 0> hci_le_set_scan_enable(socket, 0x01, 1,
                                      HCI_SEND_REQUEST_TIMEOUT_IN_MS)){

            zlog_info(category_health_report,
                      "Error enabling BLE scanning");
#ifdef Debugging
            zlog_debug(category_debug,
                       "Error enabling BLE scanning");
#endif
        }

        memset(&event_mask_cp, 0, sizeof(le_set_event_mask_cp));

        for (i = 0 ; i < 8 ; i++ ){
            event_mask_cp.mask[i] = 0xFF;
        }

        set_mask_rq = ble_hci_request(OCF_LE_SET_EVENT_MASK,
                                      LE_SET_EVENT_MASK_CP_SIZE,
                                      &status, &event_mask_cp);

        ret = hci_send_req(socket, &set_mask_rq,
                           HCI_SEND_REQUEST_TIMEOUT_IN_MS);

        if ( ret < 0 ) {
            hci_close_dev(socket);
            return E_SCAN_SET_EVENT_MASK;
        }

        hci_filter_clear(&new_filter);
        hci_filter_set_ptype(HCI_EVENT_PKT, &new_filter);
        hci_filter_set_event(EVT_LE_META_EVENT, &new_filter);

        if (0 > setsockopt(socket, SOL_HCI, HCI_FILTER, &new_filter,
                           sizeof(new_filter)) ) {
            /* Error handling */
            hci_close_dev(socket);

            zlog_error(category_health_report,
                       "Error setting HCI filter");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error setting HCI filter");
#endif
        }

        while(true == ready_to_work &&
              HCI_EVENT_HDR_SIZE <=
              read(socket, ble_buffer, sizeof(ble_buffer)) ){

            meta = (evt_le_meta_event*)
                (ble_buffer + HCI_EVENT_HDR_SIZE + 1);

            if(EVT_LE_ADVERTISING_REPORT == meta->subevent){
                info = (le_advertising_info *)(meta->data + 1);

                /* the rssi is in the next byte after the packet*/
                rssi = (signed char)info->data[info->length];

                /* If the rssi vaule is within the threshold */
                if(rssi > g_config.scan_rssi_coverage){
                    ba2str(&info->bdaddr, address);
                    strcat(address, "\0");
                    if(0 == strncmp(address, MAC_ADDRESS_PREFIX,
                                    strlen(MAC_ADDRESS_PREFIX))){
                        memset(uuid, 0, sizeof(uuid));
                        if(WORK_SUCCESSFULLY ==
                           eir_parse_uuid(info->data, info->length,
                                          uuid, sizeof(uuid))){
                            if(0 == strncmp(uuid, g_config.uuid,
                                            LENGTH_OF_UUID)){

#ifdef Debugging
                                zlog_debug(category_debug,
                                           "Detected s-tag[LE]: %s - RSSI %4d",
                                           address, rssi);
#endif
                                send_to_push_dongle(&info->bdaddr, BLE, name,
                                                    rssi);
                            }
                        }else{
#ifdef Debugging
                            zlog_debug(category_debug,
                                       "Detected tag[LE]: %s - RSSI %4d",
                                       address, rssi);
#endif
                            send_to_push_dongle(&info->bdaddr, BLE, name, rssi);
                        }
                    }
                }
            }
        } // end while (HCI_EVENT_HDR_SIZE)

        hci_close_dev(socket);

    } // end while (ready_to_work)

#ifdef Debugging
    zlog_debug(category_debug, "<< start_ble_scanning... ");
#endif
    return WORK_SUCCESSFULLY;
}

ErrorCode *start_br_scanning(void* param) {
    struct hci_filter filter; /*filter for controlling the events*/
    struct pollfd output; /*a callback event from the socket */
    unsigned char event_buffer[HCI_MAX_EVENT_SIZE]; /*a buffer for the
                                                      callback event */
    unsigned char *event_buffer_pointer; /*a pointer for the event buffer */
    hci_event_hdr *event_handler; /*a handler of the event */
    inquiry_cp inquiry_copy; /*a copy of the the message from the socket */
    inquiry_info_with_rssi *info_rssi; /*a record of
                                         EVT_INQUIRY_RESULT_WITH_RSSI
                                         message */
    inquiry_info *info; /*a record of EVT_INQUIRY_RESULT message */
    int event_buffer_length; /*length of the event buffer */
    int dongle_device_id = 1; /*dongle id */
    int socket = 0; /*socket number*/
    int results; /*the result returned via the socket */
    int results_id; /*ID of the result */
    int retry_times = 0;
    bool keep_scanning;
    char name[LENGTH_OF_DEVICE_NAME];
    int rssi;

#ifdef Debugging
    zlog_debug(category_debug, ">> start_br_scanning... ");
#endif

    while(true == ready_to_work){
        /* Open Bluetooth device */
        retry_times = DONGLE_GET_RETRY;
        while(retry_times--){
            dongle_device_id = hci_get_route(NULL);

            if(dongle_device_id >= 0){
                break;
            }
        }

        if(dongle_device_id < 0){

            zlog_error(category_health_report,
                       "Error openning the device");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error openning the device");
#endif
            return E_OPEN_DEVICE;
        }

        retry_times = SOCKET_OPEN_RETRY;
        while(retry_times--){
            socket = hci_open_dev(dongle_device_id);

            if(socket >= 0){
                break;
            }
        }

        if (socket < 0 ){
            zlog_error(category_health_report,
                       "Error openning socket");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error openning socket");
#endif
            return E_OPEN_SOCKET;
        }

        /* Setup filter */
        hci_filter_clear(&filter);
        hci_filter_set_ptype(HCI_EVENT_PKT, &filter);
        hci_filter_set_event(EVT_INQUIRY_RESULT, &filter);
        hci_filter_set_event(EVT_INQUIRY_RESULT_WITH_RSSI, &filter);
        hci_filter_set_event(EVT_INQUIRY_COMPLETE, &filter);

        if (0 > setsockopt(socket, SOL_HCI, HCI_FILTER, &filter,
                           sizeof(filter))) {

            zlog_error(category_health_report,
                       "Error setting HCI filter");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error setting HCI filter");
#endif
            hci_close_dev(socket);
            return E_SCAN_SET_HCI_FILTER;
        }

        hci_write_inquiry_mode(socket, 0x01, 10);

        if (0 > hci_send_cmd(socket, OGF_HOST_CTL, OCF_WRITE_INQUIRY_MODE,
                             WRITE_INQUIRY_MODE_RP_SIZE, &inquiry_copy)) {
            /* Error handling */
            zlog_error(category_health_report,
                       "Error setting inquiry mode");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error setting inquiry mode");
#endif
            hci_close_dev(socket);
            return E_SCAN_SET_INQUIRY_MODE;
        }

        memset(&inquiry_copy, 0, sizeof(inquiry_copy));

        /* Use the global inquiry access code (GIAC), which has
        0x338b9e as its lower address part (LAP)
        */
        inquiry_copy.lap[2] = 0x9e;
        inquiry_copy.lap[1] = 0x8b;
        inquiry_copy.lap[0] = 0x33;

        /* No limit on number of responses per scan */
        inquiry_copy.num_rsp = 0;
        /* Time unit is 1.28 seconds */
        inquiry_copy.length = 0x06; /* 6*1.28 = 7.68 seconds */

#ifdef Debugging
        zlog_debug(category_debug, "Starting inquiry with RSSI...");
#endif

        if (0 > hci_send_cmd(socket, OGF_LINK_CTL, OCF_INQUIRY,
                             INQUIRY_CP_SIZE, &inquiry_copy)) {
            /* Error handling */
            zlog_error(category_health_report,
                       "Error starting inquiry");
#ifdef Debugging
            zlog_error(category_debug,
                       "Error starting inquiry");
#endif
            hci_close_dev(socket);
            return E_SCAN_START_INQUIRY;
        }

        output.fd = socket;
        output.events = POLLIN | POLLERR | POLLHUP;

        /* An indicator for continuing to scan the devices. */
        /* After the inquiring events completing, it should jump
        out of the while loop for getting a new socket
        */

        keep_scanning = true;

        while (true == keep_scanning) {
            output.revents = 0;
            /* Poll the bluetooth device for an event */
            if (0 < poll(&output, 1, -1)) {

                event_buffer_length =
                    read(socket, event_buffer, sizeof(event_buffer));

                if (0 > event_buffer_length) {
                    continue;
                }else if (0 == event_buffer_length) {
                    break;
                }

                event_handler = (void *)(event_buffer + 1);
                event_buffer_pointer =
                    event_buffer + (1 + HCI_EVENT_HDR_SIZE);
                results = event_buffer_pointer[0];

                switch (event_handler->evt) {
                /* Scanned device with no RSSI value */
                case EVT_INQUIRY_RESULT: {
                    for (results_id = 0; results_id < results; results_id++){
                        info = (void *)event_buffer_pointer +
                               (sizeof(*info) * results_id) + 1;
                    }
                }
                break;
                /* Scanned device with RSSI value; when within rangle,
                send message to bluetooth device.
                */
                case EVT_INQUIRY_RESULT_WITH_RSSI: {

                    for (results_id = 0; results_id < results; results_id++){
                        info_rssi = (void *)event_buffer_pointer +
                                    (sizeof(*info_rssi) * results_id) + 1;

                        if (info_rssi->rssi > g_config.scan_rssi_coverage) {
#ifdef Debugging
/* For testing BR scanning parameters
                            char address[LENGTH_OF_MAC_ADDRESS];
                            ba2str(&info_rssi->bdaddr, address);
                            strcat(address, "\0");
                            zlog_debug(category_debug,
                                       "Detected device[BR]: %s - RSSI %4d",
                                       address, info_rssi->rssi);
*/
#endif
                            memset(name, 0, sizeof(name));
                            send_to_push_dongle(&info_rssi->bdaddr,
                                                BR_EDR, name,
                                                info_rssi->rssi);
                        }
                    }
                }
                break;
                /* Stop the scanning process */
                case EVT_INQUIRY_COMPLETE: {

                    /* In order to jump out of the while loop. Set
                    keep_scanning flag to false, new socket will not
                    be received.
                    */
                    keep_scanning = false;
                }
                break;
                default:
                break;
                }
            }
        } //end while
        close(socket);

#ifdef Debugging
        zlog_debug(category_debug, "Scanning done of BR devices");
#endif
    }//end while

#ifdef Debugging
    zlog_debug(category_debug, "<< start_br_scanning... ");
#endif

    return WORK_SUCCESSFULLY;
}

ErrorCode cleanup_lists(ObjectListHead *list_head, bool is_scanned_list_head){
    struct List_Entry *list_pointer, *save_list_pointers;
    ScannedDevice *temp;

    pthread_mutex_lock(&list_lock);

    if(false == is_entry_list_empty(&list_head->list_entry)){

        list_for_each_safe(list_pointer, save_list_pointers,
                           &list_head->list_entry){
            /* If the input list is the scanned list, we should remove the node
            using sc_list_entry first. Otherwise, we remove the node using
            tr_list_entry.
            */
            if(is_scanned_list_head){
                temp = ListEntry(list_pointer, ScannedDevice, sc_list_entry);
                remove_list_node(&temp->sc_list_entry);
            }else{
                temp = ListEntry(list_pointer, ScannedDevice, tr_list_entry);
                remove_list_node(&temp->tr_list_entry);
            }

            /* If the device is of BR_EDR type, each node is linked into both
            the scanned list and the BR object list using sc_list_entry and
            tr_list_entry. Make sure the node is removed from both lists.
            */
            if(BR_EDR == list_head->device_type){
                /* BR_EDR case for scanned list head and BR trakced object header
                */
                if(is_scanned_list_head){
                    if(false == is_isolated_node(&temp->tr_list_entry)){
                        remove_list_node(&temp->tr_list_entry);
                    }
                }else{
                    if(false == is_isolated_node(&temp->sc_list_entry)){
                        remove_list_node(&temp->sc_list_entry);
                    }
                }
            }else if(BLE == list_head->device_type){
                /* BLE case  */
            }
            mp_free(&mempool, temp);
        }
    }
    pthread_mutex_unlock(&list_lock);

    return WORK_SUCCESSFULLY;
}

ErrorCode *timeout_cleanup(void* param){
#ifdef Debugging
    zlog_debug(category_debug, ">> timeout_cleanup... ");
#endif

    while(true == ready_to_work){

        /* sleep a short time to prevent occupying CPU in this
        busy while loop.
        */
        usleep(INTERVAL_FOR_BUSY_WAITING_CHECK_IN_MICRO_SECONDS);

        if(mp_slots_usage_percentage(&mempool) >=
           MEMPOOL_USAGE_THRESHOLD){

#ifdef Debugging
            zlog_info(category_debug,
                      "cleanup all lists in timeout_cleanup function");
#endif
            cleanup_lists(&scanned_list_head, true);
            cleanup_lists(&BR_object_list_head, false);
            cleanup_lists(&BLE_object_list_head, false);
        }

    }

#ifdef Debugging
    zlog_debug(category_debug, "<< timeout_cleanup... ");
#endif
    return WORK_SUCCESSFULLY;
}


ErrorCode cleanup_exit(){
    struct List_Entry *list_pointer, *save_list_pointers;
    ScannedDevice *temp;

#ifdef Debugging
    zlog_debug(category_debug, ">> cleanup_exit... ");
#endif

    /* Set flag to false */
    ready_to_work = false;

    if(&mempool != NULL){
        /* Go throgth all three lists to release all memory allocated
           to the nodes */
#ifdef Debugging
        zlog_info(category_debug,
                  "cleanup all lists in cleanup_exit function");
#endif

        cleanup_lists(&scanned_list_head, true);
        cleanup_lists(&BR_object_list_head, false);
        cleanup_lists(&BLE_object_list_head, false);

        mp_destroy(&mempool);
    }

    pthread_mutex_destroy(&list_lock);

    Wifi_free(&udp_config);

#ifdef Bluetooth_classic
    /* Release the handler for Bluetooth */
    free(g_push_file_path);
#endif

#ifdef Debugging
    zlog_debug(category_debug, "<< cleanup_exit... ");
#endif

    return WORK_SUCCESSFULLY;
}


int main(int argc, char **argv) {
    ErrorCode return_value = WORK_SUCCESSFULLY;
    struct sigaction sigint_handler;
    pthread_t br_scanning_thread;
    pthread_t ble_scanning_thread;
    pthread_t timer_thread;
    Threadpool thpool;
    int id = 0;

    /*Initialize the global flag */
    ready_to_work = true;

    /* Initialize the application log */
    if (zlog_init("../config/zlog.conf") == 0) {

        category_health_report =
            zlog_get_category(LOG_CATEGORY_HEALTH_REPORT);

        if (!category_health_report) {
            zlog_fini();
        }

#ifdef Debugging
    	 category_debug =
           zlog_get_category(LOG_CATEGORY_DEBUG);

       if (!category_debug) {
           zlog_fini();
       }
#endif
    }

    /* Ensure there is only single running instance */
    return_value = single_running_instance(LBEACON_LOCK_FILE);
    if(WORK_SUCCESSFULLY != return_value){
        zlog_error(category_health_report,
                   "Error openning lock file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning lock file");
#endif
        return E_OPEN_FILE;
    }

    zlog_info(category_health_report,
              "LBeacon process is launched...");
#ifdef Debugging
    zlog_info(category_debug,
              "LBeacon process is launched...");
#endif

    /* Load config struct */
    return_value = get_config(&g_config, CONFIG_FILE_NAME);
    if(WORK_SUCCESSFULLY != return_value){
        zlog_error(category_health_report,
                   "Error openning config file");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error openning config file");
#endif
        return E_OPEN_FILE;
    }

    /* Initialize the lock for accessing the lists */
    pthread_mutex_init(&list_lock,NULL);

    /* Initialize the memory pool */
    if(MEMORY_POOL_SUCCESS !=
        mp_init(&mempool, sizeof(struct ScannedDevice), SLOTS_IN_MEM_POOL)){

        zlog_error(category_health_report,
                   "Error allocating memory pool");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error allocating memory pool");
#endif
    }

    /*Initialize the global lists */
    init_entry(&scanned_list_head.list_entry);
    scanned_list_head.device_type = BR_EDR;
    init_entry(&BR_object_list_head.list_entry);
    BR_object_list_head.device_type = BR_EDR;
    init_entry(&BLE_object_list_head.list_entry);
    BLE_object_list_head.device_type = BLE;

    /* Register handler function for SIGINT signal */
    sigint_handler.sa_handler = ctrlc_handler;
    sigemptyset(&sigint_handler.sa_mask);
    sigint_handler.sa_flags = 0;

    if (-1 == sigaction(SIGINT, &sigint_handler, NULL)) {
        zlog_error(category_health_report,
                   "Error registering signal handler for SIGINT");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error registering signal handler for SIGINT");
#endif
    }

    /* Create the thread for track BR_EDR device */
/*
    return_value = startThread(&br_scanning_thread,
                               start_br_scanning, NULL);

    if(return_value != WORK_SUCCESSFULLY){
        zlog_error(category_health_report,
                   "Error creating thread for start_br_scanning");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error creating thread for start_br_scanning");
#endif
        cleanup_exit();
        exit(return_value);
    }
*/

    /* Create the thread for track BLE device */

    return_value = startThread(&ble_scanning_thread,
                               start_ble_scanning, NULL);

    if(return_value != WORK_SUCCESSFULLY){
        zlog_error(category_health_report,
                   "Error creating thread for start_ble_scanning");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error creating thread for start_ble_scanning");
#endif
        cleanup_exit();
        exit(return_value);
    }

    /* Start bluetooth advertising */
    return_value = enable_advertising(g_config.advertise_dongle_id,
                                      INTERVAL_ADVERTISING_IN_MS,
                                      g_config.uuid,
                                      LBEACON_MAJOR_VER,
                                      LBEACON_MINOR_VER,
                                      g_config.advertise_rssi_value);

    if (WORK_SUCCESSFULLY != return_value){
        zlog_error(category_health_report,
                   "Unable to enable advertising donegle id [%d]. Please make "
                   "sure all the hardware devices are ready and try again.",
                    g_config.advertise_dongle_id);
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to enable advertising donegle id [%d]. Please make "
                   "sure all the hardware devices are ready and try again.",
                   g_config.advertise_dongle_id);
#endif
        cleanup_exit();
        exit(return_value);
    }

    /* Create the thread for cleaning up data in tracked objects */
    return_value = startThread(&timer_thread,
                               timeout_cleanup, NULL);

    if(return_value != WORK_SUCCESSFULLY){
        zlog_error(category_health_report,
                   "Error creating thread for timeout_cleanup");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error creating thread for timeout_cleanup");
#endif
    }

    /* Initialize the wifi connection to gateway */
    strcpy(udp_config.send_ipv4_addr, g_config.gateway_addr);
    udp_config.send_portno = g_config.gateway_port;
    udp_config.recv_portno = g_config.local_client_port;

    return_value = Wifi_init(&udp_config);
    if(WORK_SUCCESSFULLY != return_value){
        zlog_error(category_health_report,
                   "Error initializing network connection to gateway");
#ifdef Debugging
        zlog_error(category_debug,
                   "Error initializing network connection to gateway");
#endif
    }

    /* Initialize the thread pool and worker threads */
    thpool = thpool_init(NUMBER_WORK_THREADS);

    if(NULL != thpool){
        for(id = 0; id < NUMBER_RECEIVE_THREAD; id++){
            thpool_add_work(thpool,(void*)receive_data,
                            (sudp_config_beacon *) &udp_config, 0);
        }

        for(id = NUMBER_RECEIVE_THREAD; id < NUMBER_WORK_THREADS; id++){
            thpool_add_work(thpool,(void*)send_data ,
                            (sudp_config_beacon *) &udp_config, 0);
        }
    }else{
        zlog_error(category_health_report,
                   "Unable to initialize thread pool");
#ifdef Debugging
        zlog_error(category_debug,
                   "Unable to initialize thread pool");
#endif
    } // if-else

    while(true == ready_to_work){
        /* Although manage_communication contains a while-loop with the
        same condition inside, we should have this while-loop here to
        make sure the erros returned from manage_communication are
        treated as soft-errors and do not cause this program to terminate.
        */
        manage_communication();
    }

    if(NULL != thpool){
        /* Free the thread pool */
        thpool_destroy(thpool);
    }

    /* When signal is received, disable message advertising */
    disable_advertising(g_config.advertise_dongle_id);

    cleanup_exit();
    return WORK_SUCCESSFULLY;
}

/* Follow are functions for communication via BR/EDR path to Bluetooth
   classic devices */
#ifdef Bluetooth_classic


void start_classic_pushing(void){

    /* An iterator through the list of ScannedDevice structs */
    int device_id;

    int number_of_push_dongles = atoi(g_config.number_of_push_dongles);
    int maximum_number_of_devices_per_dongle =
        maximum_number_of_devices / number_of_push_dongles;

    /* An iterator through each push dongle */
    int push_dongle_id;

    /* An iterator through a block of devices per dongle */
    int block_id;

    int dongle_device_id = 0; /*Device ID of dongle */
    int maximum_number_of_devices;;



    g_push_file_path =
        malloc(g_config.file_path_length + g_config.file_name_length);


    if (g_push_file_path == NULL) {

         /* Error handling */
    //    zlog_info(category_health_report, errordesc[E_MALLOC].message);
        cleanup_exit();
        return E_MALLOC;
    }

    memcpy(g_push_file_path, g_config.file_path,
           g_config.file_path_length - 1);
    memcpy(g_push_file_path + g_config.file_path_length - 1,
           g_config.file_name, g_config.file_name_length - 1);

    /* the  maximum number of devices of an array */
    maximum_number_of_devices = atoi(g_config.maximum_number_of_devices);


    /* Initialize each ThreadStatus struct in the g_idle_handler array */
    for (device_id = 0; device_id < maximum_number_of_devices; device_id++) {

         strncpy(g_idle_handler[device_id].scanned_mac_address, "0",
         LENGTH_OF_MAC_ADDRESS);
        g_idle_handler[device_id].idle = true;
        g_idle_handler[device_id].is_waiting_to_send = false;

    }



    /* Create an arrayof threads for sending message to the scanned MAC
     * address */
    pthread_t send_file_thread[maximum_number_of_devices];

    /* After all the other threads are ready, set this flag to false. */
    send_message_cancelled = false;


   for (device_id = 0; device_id < maximum_number_of_devices; device_id++) {

         if (g_idle_handler[device_id].is_waiting_to_send == true) {

            /* Depending on the number of push dongles, split the threads
             * evenly and assign each thread to a push dongle device ID */
            for (push_dongle_id = 0;
                push_dongle_id < number_of_push_dongles;
                push_dongle_id++) {

                for (block_id = 0;
                     block_id < maximum_number_of_devices_per_dongle;
                     block_id++) {

                    if (device_id ==
                        push_dongle_id *
                        maximum_number_of_devices_per_dongle +
                        block_id) {

                            dongle_device_id = push_dongle_id + 1;

                        }

                    }

                }

            }

        return_value =  startThread(&send_file_thread[device_id], send_file,
                    (void *)dongle_device_id);

        if(return_value != WORK_SUCCESSFULLY){

         //   zlog_info(category_health_report,
         //             errordesc[E_START_THREAD].message);
            cleanup_exit();
            return 1;
        }


    }

    /*Set send_message_cancelled flag to false now. All the thread are
      ready.*/
    send_message_cancelled = false;


     /* ready_to_work = false , shut down.
        wait for send_file_thread to exit. */

    for (device_id = 0; device_id < maximum_number_of_devices; device_id++) {

        return_value = pthread_join(send_file_thread[device_id], NULL);

        if (return_value != 0) {

            zlog_info(category_health_report, strerror(errno));
            cleanup_exit();
            return;

        }
    }


}


char *choose_file(char *message_to_send) {

    DIR *groupdir;           /* A dirent that stores list of directories */
    struct dirent *groupent; /* A dirent struct that stores directory info */
    int message_id = 0;      /* A iterator for number of messages and
                                groups */
    int group_id = 0;        /* A iterator for number of groups */
    char *return_value;      /* Return value which turns file path to a
                                string */

    /* Convert number of groups and messages from a string to an integer */
    int number_of_groups = atoi(g_config.number_of_groups);
    int number_of_messages = atoi(g_config.number_of_messages);

    /* An array of buffer for group file names */
    char groups[number_of_groups][FILE_NAME_BUFFER];

    /* An array of buffer for message file names */
    char messages[number_of_messages][FILE_NAME_BUFFER];

    char file_path[FILE_NAME_BUFFER];

    DIR *messagedir = NULL;
    struct dirent *messageent = NULL;

    /* Stores all the name of files and directories in groups */
    groupdir = opendir("/home/pi/LBeacon/messages/");
    if (groupdir) {
        while ((groupent = readdir(groupdir)) != NULL) {
            if (strcmp(groupent->d_name, ".") != 0 &&
                strcmp(groupent->d_name, "..") != 0) {
                strcpy(groups[message_id], groupent->d_name);
                message_id++;
            }
        }
        closedir(groupdir);
    }
    else {
        /* Error handling */
        return NULL;
    }

    /* Stores file path of message_to_send */
    memset(file_path, 0, FILE_NAME_BUFFER);
    message_id = 0;

    /* Go through each message in directory and store each file name */
    for (group_id = 0; group_id < number_of_groups; group_id++) {
        /* Concatenate strings to make file path */
        sprintf(file_path, "/home/pi/LBeacon/messages/");
        strcat(file_path, groups[group_id]);

        messagedir = opendir(file_path);
        if (messagedir) {
            while ((messageent = readdir(messagedir)) != NULL) {
                if (strcmp(messageent->d_name, ".") != 0 &&
                    strcmp(messageent->d_name, "..") != 0) {
                    strcpy(messages[message_id], messageent->d_name);
                    /* If message name found, return file path */
                    if (0 == strcmp(messages[message_id], message_to_send)) {
                        strcat(file_path, "/");
                        strcat(file_path, messages[message_id]);
                        return_value = &file_path[0];
                        return return_value;
                    }
                    message_id++;
                }
            }
            closedir(messagedir);
        }
        else {
            /* Error handling */
            return NULL;
        }

        return;

    }

    /* Error handling */
    return NULL;
}



void *send_file(void *id) {

    obexftp_client_t *client = NULL; /* ObexFTP client */
    int dongle_device_id = 0;        /* Device ID of each dongle */
    int socket;                      /* ObexFTP client's socket */
    int channel = -1;                /* ObexFTP channel */
    int thread_id = (int)id;         /* Thread ID */
    char *address = NULL;            /* Scanned MAC address */
    char *file_name;                  /* File name of message to be sent */
    int return_value;                /* Return value for error handling */

    /* Get the maximum number of devices from config file. */
    int maximum_number_of_devices = atoi(g_config.maximum_number_of_devices);

    /* An iterator through the array of ScannedDevice struct */
    int device_id;

    int start;
    int end;


    while (true == ready_to_work &&  false == send_message_cancelled) {

        for (device_id = 0; device_id < maximum_number_of_devices;
            device_id++) {

            if (device_id == thread_id &&
                g_idle_handler[device_id].is_waiting_to_send == true) {


                /* Open socket and use current time as start time to keep
                 * of how long has taken to send the message to the device */
                socket = hci_open_dev(dongle_device_id);


                if (dongle_device_id < 0 || socket < 0) {

                    /* Error handling */
                    strncpy(
                            g_idle_handler[device_id].scanned_mac_address,
                            "0",
                            LENGTH_OF_MAC_ADDRESS);

                    g_idle_handler[device_id].idle = true;
                    g_idle_handler[device_id].is_waiting_to_send = false;
                    break;

                }

                start = get_system_time();
                address =
                    (char *)g_idle_handler[device_id].scanned_mac_address;
                channel = obexftp_browse_bt_push(address);

                /* Extract basename from file path */
                file_name = strrchr(g_push_file_path, '/');
                file_name[g_config.file_name_length] = '\0';

                if (!file_name) {

                    file_name = g_push_file_path;

                }
                else {

                    file_name++;

                }
                printf("Sending file %s to %s\n", file_name, address);

                /* Open connection */
                client = obexftp_open(OBEX_TRANS_BLUETOOTH, NULL, NULL,
                                      NULL);
                end = get_system_time();
                printf("Time to open connection: %d seconds\n", end - start);

                if (client == NULL) {

                    /* Error handling */
                    strncpy(
                            g_idle_handler[device_id].scanned_mac_address,
                            "0",
                            LENGTH_OF_MAC_ADDRESS);

                    g_idle_handler[device_id].idle = true;
                    g_idle_handler[device_id].is_waiting_to_send = false;
                    close(socket);
                    break;

                }

                /* Connect to the scanned device */
                return_value = obexftp_connect_push(client, address,
                                                    channel);

                /* If obexftp_connect_push returns a negative integer, then
                 * it goes into error handling */
                if (0 > return_value) {

                    /* Error handling */
                    obexftp_close(client);
                    client = NULL;
                    strncpy(
                            g_idle_handler[device_id].scanned_mac_address,
                            "0",
                            LENGTH_OF_MAC_ADDRESS);

                    g_idle_handler[device_id].idle = true;
                    g_idle_handler[device_id].is_waiting_to_send = false;
                    close(socket);
                    break;

                }

                /* Push file to the scanned device */
                return_value = obexftp_put_file(client, g_push_file_path,
                                                file_name);
                if (0 > return_value) {

                    /* TODO: Error handling */
                }

                /* Disconnect connection */
                return_value = obexftp_disconnect(client);
                if (0 > return_value) {

                    /* TODO: Error handling  */
                    pthread_exit(NULL);
                    return;

                }

               /* Leave the socket open */
                obexftp_close(client);
                client = NULL;
                strncpy(g_idle_handler[device_id].scanned_mac_address,
                        "0",
                        LENGTH_OF_MAC_ADDRESS);

                g_idle_handler[device_id].idle = true;
                g_idle_handler[device_id].is_waiting_to_send = false;
                close(socket);

            }//end if

        }//end for loop

    } //end while loop

    /* Exit forcibly by main thread */
    if(ready_to_work == false){
        return;
    }

}

#endif //Bluetooth_classic
