/*
 * test_file1.h
 *
 *  Created on: Aug 20, 2020
 *      Author: lgomez
 *
 *This file is used by the testing suite. It is meant to be used as an
 *object file that will be executed by Juicer and checked for correctness
 *on our unit tests.
 */

#define CFE_MISSION_ES_PERF_MAX_IDS 128

#include "stdint.h"

union Oject
{
    int32_t id;
    uint8_t data[32];
};

/*************************************************************************/

/**
**  \cfeestlm Executive Services Housekeeping Packet
**
**  struct taken from https://github.com/nasa/cFE/blob/fa5031f3f5db91b482e947f4dc08d9103019d48e/modules/es/config/default_cfe_es_msgstruct.h#L440
**/

extern "C" {
typedef struct CFE_ES_HousekeepingTlm_Payload
{
    uint8_t  CommandCounter;      /**< \cfetlmmnemonic \ES_CMDPC
                                 \brief The ES Application Command Counter */
    uint8_t  CommandErrorCounter; /**< \cfetlmmnemonic \ES_CMDEC
                            \brief The ES Application Command Error Counter */

    uint16_t CFECoreChecksum;     /**< \cfetlmmnemonic \ES_CKSUM
                                     \brief Checksum of cFE Core Code */
    uint8_t  CFEMajorVersion;     /**< \cfetlmmnemonic \ES_CFEMAJORVER
                                     \brief Major Version Number of cFE */
    uint8_t  CFEMinorVersion;     /**< \cfetlmmnemonic \ES_CFEMINORVER
                                     \brief Minor Version Number of cFE */
    uint8_t  CFERevision;         /**< \cfetlmmnemonic \ES_CFEREVISION
                                     \brief Sub-Minor Version Number of cFE */
    uint8_t  CFEMissionRevision;  /**< \cfetlmmnemonic \ES_CFEMISSIONREV
                                     \brief Mission Version Number of cFE */
    uint8_t  OSALMajorVersion;    /**< \cfetlmmnemonic \ES_OSMAJORVER
                                     \brief OS Abstraction Layer Major Version Number */
    uint8_t  OSALMinorVersion;    /**< \cfetlmmnemonic \ES_OSMINORVER
                                     \brief OS Abstraction Layer Minor Version Number */
    uint8_t  OSALRevision;        /**< \cfetlmmnemonic \ES_OSREVISION
                                     \brief OS Abstraction Layer Revision Number */
    uint8_t  OSALMissionRevision; /**< \cfetlmmnemonic \ES_OSMISSIONREV
                                     \brief OS Abstraction Layer MissionRevision Number */

    uint8_t  PSPMajorVersion;    /**< \cfetlmmnemonic \ES_PSPMAJORVER
                                    \brief Platform Support Package Major Version Number */
    uint8_t  PSPMinorVersion;    /**< \cfetlmmnemonic \ES_PSPMINORVER
                                    \brief Platform Support Package Minor Version Number */
    uint8_t  PSPRevision;        /**< \cfetlmmnemonic \ES_PSPREVISION
                                    \brief Platform Support Package Revision Number */
    uint8_t  PSPMissionRevision; /**< \cfetlmmnemonic \ES_PSPMISSIONREV
                                    \brief Platform Support Package MissionRevision Number */

    uint32_t SysLogBytesUsed; /**< \cfetlmmnemonic \ES_SYSLOGBYTEUSED
                                             \brief Total number of bytes used in system log */
    uint32_t SysLogSize;      /**< \cfetlmmnemonic \ES_SYSLOGSIZE
                                             \brief Total size of the system log */
    uint32_t SysLogEntries;   /**< \cfetlmmnemonic \ES_SYSLOGENTRIES
                                 \brief Number of entries in the system log */
    uint32_t SysLogMode;      /**< \cfetlmmnemonic \ES_SYSLOGMODE
                                 \brief Write/Overwrite Mode */

    uint32_t ERLogIndex;   /**< \cfetlmmnemonic \ES_ERLOGINDEX
                              \brief Current index of the ER Log (wraps around) */
    uint32_t ERLogEntries; /**< \cfetlmmnemonic \ES_ERLOGENTRIES
                              \brief Number of entries made in the ER Log since the power on */

    uint32_t RegisteredCoreApps;     /**< \cfetlmmnemonic \ES_REGCOREAPPS
                                        \brief Number of Applications registered with ES */
    uint32_t RegisteredExternalApps; /**< \cfetlmmnemonic \ES_REGEXTAPPS
                                        \brief Number of Applications registered with ES */
    uint32_t RegisteredTasks;        /**< \cfetlmmnemonic \ES_REGTASKS
                                        \brief Number of Tasks ( main AND child tasks ) registered with ES */
    uint32_t RegisteredLibs;         /**< \cfetlmmnemonic \ES_REGLIBS
                                        \brief Number of Libraries registered with ES */

    uint32_t ResetType;          /**< \cfetlmmnemonic \ES_RESETTYPE
                                    \brief Reset type ( PROCESSOR or POWERON ) */
    uint32_t ResetSubtype;       /**< \cfetlmmnemonic \ES_RESETSUBTYPE
                                    \brief Reset Sub Type */
    uint32_t ProcessorResets;    /**< \cfetlmmnemonic \ES_PROCRESETCNT
                                    \brief Number of processor resets since last power on */
    uint32_t MaxProcessorResets; /**< \cfetlmmnemonic \ES_MAXPROCRESETS
                                    \brief Max processor resets before a power on is done */
    uint32_t BootSource;         /**< \cfetlmmnemonic \ES_BOOTSOURCE
                                    \brief Boot source ( as provided from BSP ) */

    uint32_t PerfState;                                         /**< \cfetlmmnemonic \ES_PERFSTATE
                                                                   \brief Current state of Performance Analyzer */
    uint32_t PerfMode;                                          /**< \cfetlmmnemonic \ES_PERFMODE
                                                                   \brief Current mode of Performance Analyzer */
    uint32_t PerfTriggerCount;                                  /**< \cfetlmmnemonic \ES_PERFTRIGCNT
                                                                   \brief Number of Times Performance Analyzer has Triggered */
    uint32_t PerfFilterMask[CFE_MISSION_ES_PERF_MAX_IDS / 32];  /**< \cfetlmmnemonic \ES_PERFFLTRMASK
                                                           \brief Current Setting of Performance Analyzer Filter Masks */
    uint32_t PerfTriggerMask[CFE_MISSION_ES_PERF_MAX_IDS / 32]; /**< \cfetlmmnemonic \ES_PERFTRIGMASK
                                                             \brief Current Setting of Performance Analyzer Trigger Masks */
    uint32_t PerfDataStart;                                     /**< \cfetlmmnemonic \ES_PERFDATASTART
                                                                   \brief Identifies First Stored Entry in Performance Analyzer Log */
    uint32_t PerfDataEnd;                                       /**< \cfetlmmnemonic \ES_PERFDATAEND
                                                                   \brief Identifies Last Stored Entry in Performance Analyzer Log */
    uint32_t PerfDataCount;                                     /**< \cfetlmmnemonic \ES_PERFDATACNT
                                                                   \brief Number of Entries Put Into the Performance Analyzer Log */
    uint32_t PerfDataToWrite;                                   /**< \cfetlmmnemonic \ES_PERFDATA2WRITE
                                                                     \brief Number of Performance Analyzer Log Entries Left to be Written to Log Dump File */
    uint32_t HeapBytesFree;                                     /**< \cfetlmmnemonic \ES_HEAPBYTESFREE
                                                                             \brief Number of free bytes remaining in the OS heap */
    uint32_t HeapBlocksFree;                                    /**< \cfetlmmnemonic \ES_HEAPBLKSFREE
                                                                             \brief Number of free blocks remaining in the OS heap */
    uint32_t HeapMaxBlockSize;                                  /**< \cfetlmmnemonic \ES_HEAPMAXBLK
                                                                             \brief Number of bytes in the largest free block */
} CFE_ES_HousekeepingTlm_Payload_t;
}
/**
 * \brief cFS telemetry header
 *
 * This provides the definition of CFE_MSG_TelemetryHeader_t
 */
struct CFE_MSG_TelemetryHeader
{
    uint8_t Msg;      /**< \brief Base message */
    uint8_t Sec;      /**< \brief Secondary header */
    uint8_t Spare[4]; /**< \brief Pad to avoid compiler padding if payload
                                requires 64 bit alignment */
};

typedef struct CFE_MSG_TelemetryHeader CFE_MSG_TelemetryHeader_t;

typedef struct CFE_MSG_TelemetryHeader CFE_MSG_TelemetryHeader_t3;

typedef CFE_MSG_TelemetryHeader_t      CFE_MSG_TelemetryHeader_t2;

typedef struct CFE_ES_HousekeepingTlm
{
    CFE_MSG_TelemetryHeader_t2       TelemetryHeader;  /**< \brief Telemetry header */
    CFE_MSG_TelemetryHeader_t3       TelemetryHeader2; /**< \brief Telemetry header */
    CFE_ES_HousekeepingTlm_Payload_t Payload;          /**< \brief Telemetry payload */
} CFE_ES_HousekeepingTlm_t;

/**
 *The fields padding1 and padding2(as the name implies) are to prevent
 *gcc from inserting padding at compile-time and altering the expected results in our tests.
 * Tested on Ubuntu 20.04 and Ubuntu 18.04.
 */
typedef struct
{
    int32_t  width = 101;
    uint16_t stuff;
    uint16_t padding1;
    int32_t  length;
    uint16_t more_stuff;
    uint16_t padding2;
    float    floating_stuff;
    float    matrix3D[2][4][4];
    float    matrix1D[2];
    uint8_t  extra;
} Square;

enum ModeSlot_t
{
    MODE_SLOT_NONE = -1,
    MODE_SLOT_1    = 0,
    MODE_SLOT_2    = 1,
    MODE_SLOT_3    = 2,
    MODE_SLOT_4    = 3,
    MODE_SLOT_5    = 4,
    MODE_SLOT_6    = 5,
    MODE_SLOT_MAX  = 6
};

struct Circle
{
    float      diameter = 7;
    float      radius;
    int        points[128];
    ModeSlot_t mode;
    Oject      union_object;
};

enum Color
{
    RED,
    BLUE,
    YELLOW
};

struct S
{
    uint8_t before;
    int     j : 5;
    int     k : 6;
    int     m : 5;
    uint8_t p;
    int     n : 8;
    uint8_t after;
};

#define CFE_TBL_FILDEF_MAX_NAME_LEN 64
#define CFE_TBL_FILDEF_FS_HDR_LEN   64
#define CFE_TBL_FILDEF_OS_FILE_LEN  64
typedef struct
{
    char    ObjectName[64];                          /**< \brief Name of instantiated variable that contains desired table image */
    char    TableName[CFE_TBL_FILDEF_MAX_NAME_LEN];  /**< \brief Name of Table as defined onboard */
    char    Description[CFE_TBL_FILDEF_FS_HDR_LEN];  /**< \brief Description of table image that is included in cFE File Header */
    char    TgtFilename[CFE_TBL_FILDEF_OS_FILE_LEN]; /**< \brief Default filename to be used for output of elf2cfetbl utility  */
    int32_t ObjectSize;                              /**< \brief Size, in bytes, of instantiated object */
} CFE_TBL_FileDef_t;
