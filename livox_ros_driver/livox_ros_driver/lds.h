//
// The MIT License (MIT)
//
// Copyright (c) 2019 Livox. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

// livox lidar data source

#ifndef LIVOX_ROS_DRIVER_LDS_H_
#define LIVOX_ROS_DRIVER_LDS_H_

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "ldq.h"

#include "livox_def.h"

namespace livox_ros {

/** Max lidar data source num */
const uint32_t kMaxSourceLidar = 32;

/** Eth packet relative info parama */
const uint32_t kMaxPointPerEthPacket = 100;
const uint32_t kMinEthPacketQueueSize = 32;   /**< must be 2^n */
const uint32_t kMaxEthPacketQueueSize = 8192; /**< must be 2^n */
const uint32_t kImuEthPacketQueueSize = 256;

const uint32_t KEthPacketHeaderLength = 18; /**< (sizeof(LivoxEthPacket) - 1) */
// const uint32_t KEthPacketMaxLength          = 1500;
const uint32_t KCartesianPointSize = 13;
const uint32_t KSphericalPointSzie = 9;

const int64_t kPacketTimeGap = 1000000; /**< 1ms = 1000000ns */
const int64_t kMaxPacketTimeGap =
    1700000; /**< the threshold of packet continuous */
const int64_t kDeviceDisconnectThreshold =
    1000000000; /**< the threshold of device disconect */
const int64_t kNsPerSecond = 1000000000; /**< 1s  = 1000000000ns */

const int kPathStrMinSize = 4;   /**< Must more than 4 char */
const int kPathStrMaxSize = 256; /**< Must less than 256 char */
const int kBdCodeSize = 15;

const uint32_t kPointXYZRSize = 16;
const uint32_t kPointXYZRTRSize = 18;

const double PI = 3.14159265358979323846;

/** Lidar connect state */
typedef enum {
  kConnectStateOff = 0,
  kConnectStateOn = 1,
  kConnectStateConfig = 2,
  kConnectStateSampling = 3,
} LidarConnectState;

/** Device data source type */
typedef enum {
  kSourceRawLidar = 0, /**< Data from raw lidar. */
  kSourceRawHub = 1,   /**< Data from lidar hub. */
  kSourceLvxFile,      /**< Data from parse lvx file. */
  kSourceUndef,
} LidarDataSourceType;

/** Lidar Data output type */
typedef enum {
  kOutputToRos = 0,
  kOutputToRosBagFile = 1,
} LidarDataOutputType;

typedef enum { kCoordinateCartesian = 0, kCoordinateSpherical } CoordinateType;

typedef enum {
  kConfigFan = 1 << 0,
  kConfigReturnMode = 1 << 1,
  kConfigCoordinate = 1 << 2,
  kConfigImuRate    = 1 << 3,
  kConfigGetExtrinsicParameter = 1 << 4,
  kConfigSetHighSensitivity    = 1 << 5,  
  kConfigUndef
} LidarConfigCodeBit;

typedef enum {
  kNoneExtrinsicParameter,
  kExtrinsicParameterFromLidar,
  kExtrinsicParameterFromXml
} ExtrinsicParameterType;

typedef struct {
  uint32_t receive_packet_count;
  uint32_t loss_packet_count;
  int64_t last_timestamp;
  int64_t timebase; /**< unit:ns */
  int64_t last_imu_timestamp;
  int64_t imu_timebase; /**< unit:ns */
  uint32_t timebase_state;
} LidarPacketStatistic;

/** 8bytes stamp to uint64_t stamp */
typedef union {
  struct {
    uint32_t low;
    uint32_t high;
  } stamp_word;

  uint8_t stamp_bytes[8];
  int64_t stamp;
} LdsStamp;

/** Configuration in json config file for livox lidar */
typedef struct {
  char broadcast_code[16];
  bool enable_connect;
  bool enable_fan;
  uint32_t return_mode;
  uint32_t coordinate;
  uint32_t imu_rate;
  uint32_t extrinsic_parameter_source;
  bool enable_high_sensitivity;
} UserRawConfig;

typedef struct {
  bool enable_fan;
  uint32_t return_mode;
  uint32_t coordinate;
  uint32_t imu_rate;
  uint32_t extrinsic_parameter_source;
  bool enable_high_sensitivity;
  volatile uint32_t set_bits;
  volatile uint32_t get_bits;
} UserConfig;

typedef float EulerAngle[3];        /**< Roll, Pitch, Yaw, unit:radian. */
typedef float TranslationVector[3]; /**< x, y, z translation, unit: m. */
typedef float RotationMatrix[3][3];

typedef struct {
  EulerAngle euler;
  TranslationVector trans;
  RotationMatrix rotation;
  bool enable;
} ExtrinsicParameter;

/** Lidar data source info abstract */
typedef struct {
  uint8_t handle;          /**< Lidar access handle. */
  uint8_t data_src;        /**< From raw lidar or livox file. */
  uint8_t raw_data_type;   /**< The data type in eth packaet */
  bool data_is_pubulished; /**< Indicate the data of lidar whether is
                                pubulished. */
  volatile uint32_t packet_interval;/**< The time interval between packets 
                                       of current lidar, unit:ns */
  volatile uint32_t packet_interval_max; /**< If more than it,
                                            have packet loss */                            
  volatile LidarConnectState connect_state;
  DeviceInfo info;
  LidarPacketStatistic statistic_info;
  LidarDataQueue data;
  LidarDataQueue imu_data;
  uint32_t firmware_ver; /**< Firmware version of lidar  */
  UserConfig config;
  ExtrinsicParameter extrinsic_parameter;
} LidarDevice;

typedef struct {
  uint32_t points_per_packet;
  uint32_t points_per_second;
  uint32_t point_interval;  /**< unit:ns */
  uint32_t packet_interval; /**< unit:ns */
  uint32_t packet_length;
} PacketInfoPair;

#pragma pack(1)

typedef struct {
  float x; /**< X axis, Unit:m */
  float y; /**< Y axis, Unit:m */
  float z; /**< Z axis, Unit:m */
} PointXyz;

typedef struct {
  float x;            /**< X axis, Unit:m */
  float y;            /**< Y axis, Unit:m */
  float z;            /**< Z axis, Unit:m */
  float reflectivity; /**< Reflectivity   */
} LivoxPointXyzr;

typedef struct {
  float x;            /**< X axis, Unit:m */
  float y;            /**< Y axis, Unit:m */
  float z;            /**< Z axis, Unit:m */
  float reflectivity; /**< Reflectivity   */
  uint8_t tag;        /**< Livox point tag   */
  uint8_t line;       /**< Laser line id     */
} LivoxPointXyzrtl;

#pragma pack()

typedef uint8_t *(*PointConvertHandler)(uint8_t *point_buf,
                                        LivoxEthPacket *eth_packet,
                                        ExtrinsicParameter &extrinsic);

const PacketInfoPair packet_info_pair_table[kMaxPointDataType] = {
    {100, 100000, 10000, 1000000, 1318}, {100, 100000, 10000, 1000000, 918},
    {96, 240000, 4167, 400000, 1362},    {96, 240000, 4167, 400000, 978},
    {96, 480000, 4167, 400000, 1362},    {48, 480000, 4167, 400000, 978},
    {1, 200, 10000000, 10000000, 42}};

/**
 * Global function for general use.
 */
bool IsFilePathValid(const char *path_str);
uint64_t GetStoragePacketTimestamp(StoragePacket *packet, uint8_t data_src_);
uint32_t CalculatePacketQueueSize(uint32_t interval_ms, uint32_t data_type);
void ParseCommandlineInputBdCode(const char *cammandline_str,
                                 std::vector<std::string> &bd_code_list);
PointConvertHandler GetConvertHandler(uint8_t data_type);
uint8_t *LivoxPointToPxyzrtl(uint8_t *point_buf, LivoxEthPacket *eth_packet,
                             ExtrinsicParameter &extrinsic);
uint8_t *FillZeroPointXyzrtl(uint8_t *point_buf, uint32_t num);
uint8_t *LivoxImuDataProcess(uint8_t *point_buf, LivoxEthPacket *eth_packet);
void EulerAnglesToRotationMatrix(EulerAngle euler, RotationMatrix matrix);
void PointExtrisincCompensation(PointXyz *dst_point,
                                ExtrinsicParameter &extrinsic);

inline uint32_t GetPointInterval(uint32_t data_type) {
  return packet_info_pair_table[data_type].point_interval;
}

inline uint32_t GetPacketNumPerSec(uint32_t data_type) {
  return packet_info_pair_table[data_type].points_per_second /
         packet_info_pair_table[data_type].points_per_packet;
}

inline uint32_t GetPointsPerPacket(uint32_t data_type) {
  return packet_info_pair_table[data_type].points_per_packet;
}

inline uint32_t GetPacketInterval(uint32_t data_type) {
  return packet_info_pair_table[data_type].packet_interval;
}

inline uint32_t GetEthPacketLen(uint32_t data_type) {
  return packet_info_pair_table[data_type].packet_length;
}

inline void RawPointConvert(LivoxPointXyzr *dst_point, LivoxPoint *raw_point) {
  dst_point->x = raw_point->x;
  dst_point->y = raw_point->y;
  dst_point->z = raw_point->z;
  dst_point->reflectivity = (float)raw_point->reflectivity;
}

inline void RawPointConvert(LivoxPointXyzr *dst_point,
                            LivoxRawPoint *raw_point) {
  dst_point->x = raw_point->x / 1000.0f;
  dst_point->y = raw_point->y / 1000.0f;
  dst_point->z = raw_point->z / 1000.0f;
  dst_point->reflectivity = (float)raw_point->reflectivity;
}

inline void RawPointConvert(LivoxPointXyzr *dst_point,
                            LivoxSpherPoint *raw_point) {
  double radius = raw_point->depth / 1000.0;
  double theta = raw_point->theta / 100.0 / 180 * PI;
  double phi = raw_point->phi / 100.0 / 180 * PI;
  dst_point->x = radius * sin(theta) * cos(phi);
  dst_point->y = radius * sin(theta) * sin(phi);
  dst_point->z = radius * cos(theta);
  dst_point->reflectivity = (float)raw_point->reflectivity;
}

inline void RawPointConvert(LivoxPointXyzr *dst_point1,
                            LivoxPointXyzr *dst_point2,
                            LivoxDualExtendSpherPoint *raw_point) {
  double radius1 = raw_point->depth1 / 1000.0;
  double radius2 = raw_point->depth2 / 1000.0;
  double theta = raw_point->theta / 100.0 / 180 * PI;
  double phi = raw_point->phi / 100.0 / 180 * PI;
  dst_point1->x = radius1 * sin(theta) * cos(phi);
  dst_point1->y = radius1 * sin(theta) * sin(phi);
  dst_point1->z = radius1 * cos(theta);
  dst_point1->reflectivity = (float)raw_point->reflectivity1;

  (dst_point2 + 1)->x = radius2 * sin(theta) * cos(phi);
  (dst_point2 + 1)->y = radius2 * sin(theta) * sin(phi);
  (dst_point2 + 1)->z = radius2 * cos(theta);
  (dst_point2 + 1)->reflectivity = (float)raw_point->reflectivity2;
}

/**
 * Lidar data source abstract.
 */
class Lds {
public:
  Lds(uint32_t buffer_time_ms, uint8_t data_src);
  virtual ~Lds();

  uint8_t GetDeviceType(uint8_t handle);
  static void ResetLidar(LidarDevice *lidar, uint8_t data_src);
  static void SetLidarDataSrc(LidarDevice *lidar, uint8_t data_src);
  void ResetLds(uint8_t data_src);
  void RequestExit() { request_exit_ = true; }
  void CleanRequestExit() { request_exit_ = false; }
  bool IsRequestExit() { return request_exit_; }
  virtual void PrepareExit(void);

  uint8_t lidar_count_;                 /**< Lidar access handle. */
  LidarDevice lidars_[kMaxSourceLidar]; /**< The index is the handle */

protected:
  uint32_t buffer_time_ms_; /**< Buffer time before data in queue is read */
  uint8_t data_src_;

private:
  volatile bool request_exit_;
};

} // namespace livox_ros
#endif
