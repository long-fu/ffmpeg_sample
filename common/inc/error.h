#pragma once
#include <unistd.h>

const int ACLLITE_OK = 0;
// malloc or new memory failed
const int ACLLITE_ERROR_MALLOC = 101;
// aclrtMalloc failed
const int ACLLITE_ERROR_MALLOC_DEVICE = 102;

const int ACLLITE_ERROR_MALLOC_DVPP = 103;
// access file failed
const int ACLLITE_ERROR_ACCESS_FILE = 201;
// the file is invalid
const int ACLLITE_ERROR_INVALID_FILE = 202;
// open file failed
const int ACLLITE_ERROR_OPEN_FILE = 203;

// load model repeated
const int ACLLITE_ERROR_LOAD_MODEL_REPEATED = 301;

const int ACLLITE_ERROR_NO_MODEL_DESC = 302;
// load mode by acl failed
const int ACLLITE_ERROR_LOAD_MODEL = 303;

const int ACLLITE_ERROR_CREATE_MODEL_DESC = 304;

const int ACLLITE_ERROR_GET_MODEL_DESC = 305;

const int ACLLITE_ERROR_CREATE_DATASET = 306;

const int ACLLITE_ERROR_CREATE_DATA_BUFFER = 307;

const int ACLLITE_ERROR_ADD_DATASET_BUFFER = 308;

const int ACLLITE_ERROR_EXECUTE_MODEL = 309;

const int ACLLITE_ERROR_GET_DATASET_BUFFER = 310;

const int ACLLITE_ERROR_GET_DATA_BUFFER_ADDR = 311;

const int ACLLITE_ERROR_GET_DATA_BUFFER_SIZE = 312;

const int ACLLITE_ERROR_COPY_DATA = 313;

const int ACLLITE_ERROR_SET_CAMERA = 400;

const int ACLLITE_ERROR_CAMERA_NO_ACCESSABLE = 401;

const int ACLLITE_ERROR_OPEN_CAMERA = 402;

const int ACLLITE_ERROR_READ_CAMERA_FRAME = 403;

const int ACLLITE_ERROR_UNSURPPORT_PROPERTY = 404;

const int ACLLITE_ERROR_INVALID_PROPERTY_VALUE = 405;

const int ACLLITE_ERROR_UNSURPPORT_VIDEO_CAPTURE = 406;

const int ACLLITE_ERROR_CREATE_DVPP_CHANNEL_DESC = 501;

const int ACLLITE_ERRROR_CREATE_DVPP_CHANNEL = 502;

const int ACLLITE_ERROR_CREATE_PIC_DESC = 503;