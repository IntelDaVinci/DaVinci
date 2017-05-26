/**
 * @file remote_screen.h
 *
 * @brief Define interface of remote screen capture module
 */

#ifndef _REMOTE_SCREEN_H_
#define _REMOTE_SCREEN_H_

#if defined(_WIN32)
#pragma message("It is on windows platform")
#ifdef _WINDLL
#pragma message("It is a windows dll")
#define DllDecl __declspec(dllexport)
#else
#pragma message("It is not a windows dll")
#define DllDecl __declspec(dllimport)
#endif
#else
#pragma message("It is not on windows platform")
#define DllDecl
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef
 */
typedef unsigned int RS_STATUS;

/**
 * @defgroup RS_STATUS The status indicate whether the functio is called successfully or not.
 * @{
 */
#define RS_SUCCESS                 0x00000000  ///< Succeeded to call remote screen function
#define RS_UNINITIALIZED           0x00000001  ///< The module has not been initialized
#define RS_INVALID_PARAMETERS      0x00000002  ///< The parameters of functions are invalid
#define RS_UNCONNECTED             0x00000003  ///< Does not connect to the remote target
#define RS_VEDIO_ON_THE_WAY        0x00000004  ///< Has not received the remote target screen frame
#define RS_UNCONNECTED_SERVER      0x00000005  ///< Has not connected to the server
#define RS_CODEC_ABORTED           0x00000006  ///< The codec aborted with error

#define RS_DEVICE_CODEC_ERR        0x00000007  ///< The device codec is not available
#define RS_DEVICE_NO_AVAIL_CODEC   0x00000008  ///< Cannot find available codec
#define RS_DEVICE_NO_AVAIL_FORMAT  0x00000009  ///< Cannot find available color format
#define RS_DEVICE_CONFIG_CODEC_ERR 0x0000000A  ///< Cannot configure codec

#define RS_UNIMPLEMENTED           0xFFFF0000  ///< Function has not been implemented
#define RS_GENERAL_FAILURE         0xFFFFFFFF  ///< Un-known error
/** @} */

/**
 * Log level
 */
typedef enum {
    LOG_ERRO,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
} LOG_LEVEL;

/**
 * @typedef
 */
typedef void (*_GeneralExceptionCallBack) (
  RS_STATUS error_code
  );

/**
 * @typedef
 */
typedef void (*_CaptureFrameCallBack) (
  unsigned int   frame_width,
  unsigned int   frame_height
  );

/**
 * @brief Initialize remote screen capture module
 *
 * @retval RS_SUCCESS          Succeeded to initialized remote screen capture
 * @retval RS_GENERAL_FAILURE  Falied to initialized remote screen capture
 *
 * @see UninitalizeRSModule()
 */
DllDecl RS_STATUS InitializeRSModule();

/**
 * @brief Config log level
 *
 * @param[in] log_level        Log level to be recorded.
 *                             Error  : 0
 *                             Warning: 1
 *                             Info   : 2
 *                             Debug  : 3
 */
DllDecl void ConfigLogLevel(LOG_LEVEL log_level);

/**
 * @brief Uninitialize remote screen capture module.
 *
 * @note If you have initialized remote screen capture module,
 *       you must uninitialize the module, otherwise it will cause memory leak.
 *
 * @see InitializeRSModule()
 */
DllDecl void UninitializeRSModule();

/**
 * @brief Try to log in singaling server
 *
 * @param[in] p2p_server_address     The p2p server IP address
 * @param[in] p2p_server_port        The p2p server listen port
 * @param[in] p2p_peer_id            peer id
 * @param[in] on_capture_frame       Callback function in reponse to receive a new remote target frame
 * @param[in] on_exception           Callback function in reponse to raise exception
 *
 * @retval RS_SUCCESS             Succeeded to connect to remote target
 * @retval RS_UNINITIALIZED       Unintialized the module
 * @retval RS_INVALID_PARAMETERS  Some parameters are invalid
 * @retval RS_GENERAL_FAILURE     Unknown error
 */
DllDecl RS_STATUS Start(
  const char                *p2p_server_address,
  int                        p2p_server_port,
  int                        p2p_peer_id,
  bool                       stat_mode,
  _CaptureFrameCallBack      on_capture_frame,
  _GeneralExceptionCallBack  on_exception
  );

/**
 * @brief stop server
 *
 * @retval RS_SUCCESS             Succeeded to connect to remote target
 */
DllDecl RS_STATUS Stop();

/**
 * @brief Set frame buffer containter to receive remote frame
 *
 * @param[in] frame_buffer_container_len   Capacity of frame buffer container
 * @param[in] frame_buffer_container       Frame buffer container
 *
 */
DllDecl RS_STATUS GetFrameBuffer(
  unsigned char *frame_buffer_data,
  unsigned int   frame_buffer_data_len
  );

#ifdef __cplusplus
}
#endif

#endif
