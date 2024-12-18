#ifndef VC_MSGS_H
#define VC_MSGS_H

#include "cchannel.hpp"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
    int           data;
    CChannel::SHK Channel[1];
} HkTlm_t;

#ifdef __cplusplus
}
#endif

#endif /* VC_MSG_H */