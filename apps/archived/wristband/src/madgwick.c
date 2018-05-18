//=============================================================================================
// MadgwickAHRS.c
//=============================================================================================
//
// Implementation of Madgwick's IMU and AHRS algorithms.
// See: http://www.x-io.co.uk/open-source-imu-and-ahrs-algorithms/
//
// From the x-io website "Open-source resources available on this website are
// provided under the GNU General Public Licence unless an alternative licence
// is provided in source."
//
// Date			Author          Notes
// 29/09/2011	SOH Madgwick    Initial release
// 02/10/2011	SOH Madgwick	Optimised for reduced CPU load
// 19/02/2012	SOH Madgwick	Magnetometer measurement is normalised
//
//=============================================================================================

//-------------------------------------------------------------------------------------------
// Header files

#include "madgwick.h"
#include <math.h>

#include "app_config.h"
#include "custom_log.h"

//-------------------------------------------------------------------------------------------
// Definitions

#define MADGWICK_SAMPLE_FREQUENCY_HZ APP_IMU_FREQUENCY // sample frequency in Hz
#define MADGWICK_BETA_REF 0.05f                          // 2 * proportional gain

//============================================================================================
// Functions

//-------------------------------------------------------------------------------------------
// AHRS algorithm update

void madgwick_init(madgwick_t *mad) {
  mad->beta = MADGWICK_BETA_REF;

  mad->q0 = 1.0f;
  mad->q1 = 0.0f;
  mad->q2 = 0.0f;
  mad->q3 = 0.0f;

  mad->inv_sample_freq = 1.0f / MADGWICK_SAMPLE_FREQUENCY_HZ;
  mad->angles_computed = false;
}

void madgwick_update(madgwick_t *mad, float gx, float gy, float gz, float ax,
                     float ay, float az, float mx, float my, float mz) {
  float recipNorm;
  float s0, s1, s2, s3;
  float qDot1, qDot2, qDot3, qDot4;
  float hx, hy;
  float _2q0mx, _2q0my, _2q0mz, _2q1mx, _2bx, _2bz, _4bx, _4bz, _2q0, _2q1,
      _2q2, _2q3, _2q0q2, _2q2q3, q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3,
      q2q2, q2q3, q3q3;

  // Use IMU algorithm if magnetometer measurement invalid (avoids NaN in
  // magnetometer normalisation)
  if ((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
    madgwick_update_imu(mad, gx, gy, gz, ax, ay, az);
    return;
  }

  // Rate of change of quaternion from gyroscope
  qDot1 = 0.5f * (-mad->q1 * gx - mad->q2 * gy - mad->q3 * gz);
  qDot2 = 0.5f * (mad->q0 * gx + mad->q2 * gz - mad->q3 * gy);
  qDot3 = 0.5f * (mad->q0 * gy - mad->q1 * gz + mad->q3 * gx);
  qDot4 = 0.5f * (mad->q0 * gz + mad->q1 * gy - mad->q2 * gx);

  // Compute feedback only if accelerometer measurement valid (avoids NaN in
  // accelerometer normalisation)
  if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

    // Normalise accelerometer measurement
    recipNorm = inv_sqrt(ax * ax + ay * ay + az * az);
    ax *= recipNorm;
    ay *= recipNorm;
    az *= recipNorm;

    // Normalise magnetometer measurement
    recipNorm = inv_sqrt(mx * mx + my * my + mz * mz);
    mx *= recipNorm;
    my *= recipNorm;
    mz *= recipNorm;

    // Auxiliary variables to avoid repeated arithmetic
    _2q0mx = 2.0f * mad->q0 * mx;
    _2q0my = 2.0f * mad->q0 * my;
    _2q0mz = 2.0f * mad->q0 * mz;
    _2q1mx = 2.0f * mad->q1 * mx;
    _2q0 = 2.0f * mad->q0;
    _2q1 = 2.0f * mad->q1;
    _2q2 = 2.0f * mad->q2;
    _2q3 = 2.0f * mad->q3;
    _2q0q2 = 2.0f * mad->q0 * mad->q2;
    _2q2q3 = 2.0f * mad->q2 * mad->q3;
    q0q0 = mad->q0 * mad->q0;
    q0q1 = mad->q0 * mad->q1;
    q0q2 = mad->q0 * mad->q2;
    q0q3 = mad->q0 * mad->q3;
    q1q1 = mad->q1 * mad->q1;
    q1q2 = mad->q1 * mad->q2;
    q1q3 = mad->q1 * mad->q3;
    q2q2 = mad->q2 * mad->q2;
    q2q3 = mad->q2 * mad->q3;
    q3q3 = mad->q3 * mad->q3;

    // Reference direction of Earth's magnetic field
    hx = mx * q0q0 - _2q0my * mad->q3 + _2q0mz * mad->q2 + mx * q1q1 +
         _2q1 * my * mad->q2 + _2q1 * mz * mad->q3 - mx * q2q2 - mx * q3q3;
    hy = _2q0mx * mad->q3 + my * q0q0 - _2q0mz * mad->q1 + _2q1mx * mad->q2 -
         my * q1q1 + my * q2q2 + _2q2 * mz * mad->q3 - my * q3q3;
    _2bx = sqrtf(hx * hx + hy * hy);
    _2bz = -_2q0mx * mad->q2 + _2q0my * mad->q1 + mz * q0q0 + _2q1mx * mad->q3 -
           mz * q1q1 + _2q2 * my * mad->q3 - mz * q2q2 + mz * q3q3;
    _4bx = 2.0f * _2bx;
    _4bz = 2.0f * _2bz;

    // Gradient decent algorithm corrective step
    s0 = -_2q2 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q1 * (2.0f * q0q1 + _2q2q3 - ay) -
         _2bz * mad->q2 *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (-_2bx * mad->q3 + _2bz * mad->q1) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         _2bx * mad->q2 *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s1 = _2q3 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q0 * (2.0f * q0q1 + _2q2q3 - ay) -
         4.0f * mad->q1 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) +
         _2bz * mad->q3 *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (_2bx * mad->q2 + _2bz * mad->q0) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         (_2bx * mad->q3 - _4bz * mad->q1) *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s2 = -_2q0 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q3 * (2.0f * q0q1 + _2q2q3 - ay) -
         4.0f * mad->q2 * (1 - 2.0f * q1q1 - 2.0f * q2q2 - az) +
         (-_4bx * mad->q2 - _2bz * mad->q0) *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (_2bx * mad->q1 + _2bz * mad->q3) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         (_2bx * mad->q0 - _4bz * mad->q2) *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    s3 = _2q1 * (2.0f * q1q3 - _2q0q2 - ax) +
         _2q2 * (2.0f * q0q1 + _2q2q3 - ay) +
         (-_4bx * mad->q3 + _2bz * mad->q1) *
             (_2bx * (0.5f - q2q2 - q3q3) + _2bz * (q1q3 - q0q2) - mx) +
         (-_2bx * mad->q0 + _2bz * mad->q2) *
             (_2bx * (q1q2 - q0q3) + _2bz * (q0q1 + q2q3) - my) +
         _2bx * mad->q1 *
             (_2bx * (q0q2 + q1q3) + _2bz * (0.5f - q1q1 - q2q2) - mz);
    recipNorm = inv_sqrt(s0 * s0 + s1 * s1 + s2 * s2 +
                         s3 * s3); // normalise step magnitude
    s0 *= recipNorm;
    s1 *= recipNorm;
    s2 *= recipNorm;
    s3 *= recipNorm;

    // Apply feedback step
    qDot1 -= mad->beta * s0;
    qDot2 -= mad->beta * s1;
    qDot3 -= mad->beta * s2;
    qDot4 -= mad->beta * s3;
  }

  // Integrate rate of change of quaternion to yield quaternion
  mad->q0 += qDot1 * mad->inv_sample_freq;
  mad->q1 += qDot2 * mad->inv_sample_freq;
  mad->q2 += qDot3 * mad->inv_sample_freq;
  mad->q3 += qDot4 * mad->inv_sample_freq;

  // Normalise quaternion
  recipNorm = inv_sqrt(mad->q0 * mad->q0 + mad->q1 * mad->q1 +
                       mad->q2 * mad->q2 + mad->q3 * mad->q3);
  mad->q0 *= recipNorm;
  mad->q1 *= recipNorm;
  mad->q2 *= recipNorm;
  mad->q3 *= recipNorm;

  mad->angles_computed = false;
}

//-------------------------------------------------------------------------------------------
// IMU algorithm update

void madgwick_update_imu(madgwick_t *mad, float gx, float gy, float gz,
                         float ax, float ay, float az) {
  float recipNorm;
  float s0, s1, s2, s3;
  float qDot1, qDot2, qDot3, qDot4;
  float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2, q0q0, q1q1, q2q2,
      q3q3;

  // Rate of change of quaternion from gyroscope
  qDot1 = 0.5f * (-mad->q1 * gx - mad->q2 * gy - mad->q3 * gz);
  qDot2 = 0.5f * (mad->q0 * gx + mad->q2 * gz - mad->q3 * gy);
  qDot3 = 0.5f * (mad->q0 * gy - mad->q1 * gz + mad->q3 * gx);
  qDot4 = 0.5f * (mad->q0 * gz + mad->q1 * gy - mad->q2 * gx);

  // Compute feedback only if accelerometer measurement valid (avoids NaN in
  // accelerometer normalisation)
  if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

    // Normalise accelerometer measurement
    recipNorm = inv_sqrt(ax * ax + ay * ay + az * az);
    ax *= recipNorm;
    ay *= recipNorm;
    az *= recipNorm;

    // Auxiliary variables to avoid repeated arithmetic
    _2q0 = 2.0f * mad->q0;
    _2q1 = 2.0f * mad->q1;
    _2q2 = 2.0f * mad->q2;
    _2q3 = 2.0f * mad->q3;
    _4q0 = 4.0f * mad->q0;
    _4q1 = 4.0f * mad->q1;
    _4q2 = 4.0f * mad->q2;
    _8q1 = 8.0f * mad->q1;
    _8q2 = 8.0f * mad->q2;
    q0q0 = mad->q0 * mad->q0;
    q1q1 = mad->q1 * mad->q1;
    q2q2 = mad->q2 * mad->q2;
    q3q3 = mad->q3 * mad->q3;

    // Gradient decent algorithm corrective step
    s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
    s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * mad->q1 - _2q0 * ay - _4q1 +
         _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
    s2 = 4.0f * q0q0 * mad->q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 +
         _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
    s3 = 4.0f * q1q1 * mad->q3 - _2q1 * ax + 4.0f * q2q2 * mad->q3 - _2q2 * ay;
    recipNorm = inv_sqrt(s0 * s0 + s1 * s1 + s2 * s2 +
                         s3 * s3); // normalise step magnitude
    s0 *= recipNorm;
    s1 *= recipNorm;
    s2 *= recipNorm;
    s3 *= recipNorm;

    // Apply feedback step
    qDot1 -= mad->beta * s0;
    qDot2 -= mad->beta * s1;
    qDot3 -= mad->beta * s2;
    qDot4 -= mad->beta * s3;
  }

  // Integrate rate of change of quaternion to yield quaternion
  mad->q0 += qDot1 * mad->inv_sample_freq;
  mad->q1 += qDot2 * mad->inv_sample_freq;
  mad->q2 += qDot3 * mad->inv_sample_freq;
  mad->q3 += qDot4 * mad->inv_sample_freq;

  // Normalise quaternion
  recipNorm = inv_sqrt(mad->q0 * mad->q0 + mad->q1 * mad->q1 +
                       mad->q2 * mad->q2 + mad->q3 * mad->q3);
  mad->q0 *= recipNorm;
  mad->q1 *= recipNorm;
  mad->q2 *= recipNorm;
  mad->q3 *= recipNorm;

  mad->angles_computed = false;
}

//-------------------------------------------------------------------------------------------
// Fast inverse square-root
// See: http://en.wikipedia.org/wiki/Fast_inverse_square_root

float inv_sqrt(float x) {
  //   float halfx = 0.5f * x;
  //   float y = x;
  //   long i = *(long *)&y;
  //   i = 0x5f3759df - (i >> 1);
  //   y = *(float *)&i;
  //   y = y * (1.5f - (halfx * y * y));
  //   y = y * (1.5f - (halfx * y * y));
  //   return y;
  return 1.0 / sqrtf(x);
}

//-------------------------------------------------------------------------------------------

void madgwick_compute_angles(madgwick_t *mad) {
  mad->roll = atan2f(mad->q0 * mad->q1 + mad->q2 * mad->q3,
                     0.5f - mad->q1 * mad->q1 - mad->q2 * mad->q2);
  mad->pitch = asinf(-2.0f * (mad->q1 * mad->q3 - mad->q0 * mad->q2));
  mad->yaw = atan2f(mad->q1 * mad->q2 + mad->q0 * mad->q3,
                    0.5f - mad->q2 * mad->q2 - mad->q3 * mad->q3);
  mad->angles_computed = true;
}

float madgwick_get_roll(madgwick_t *mad) {
  if (!mad->angles_computed) {
    madgwick_compute_angles(mad);
  }
  return mad->roll;
}

float madgwick_get_pitch(madgwick_t *mad) {
  if (!mad->angles_computed) {
    madgwick_compute_angles(mad);
  }
  return mad->pitch;
}

float madgwick_get_yaw(madgwick_t *mad) {
  if (!mad->angles_computed) {
    madgwick_compute_angles(mad);
  }
  return mad->yaw + M_PI;
}
