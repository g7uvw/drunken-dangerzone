// data structure
struct data_t{
    uint8_t     calibrating;    // non zero means we're calibrating the strain guage
    uint8_t     recording;      // non zero means record
    uint8_t     highrange;      // non zero means Torque is set to high range
    uint8_t     padding;       // just to pad the stuct
    uint16_t    torque;         // from strain guage
    uint16_t    speedo;          // MPH from controller board
    uint16_t    revs;
    uint16_t    humidity;
    uint16_t    pressure;
} __attribute__((__packed__)) data_packet;

unsigned long uBufSize = sizeof(data_t);
