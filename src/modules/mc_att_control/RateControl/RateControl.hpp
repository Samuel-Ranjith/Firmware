/****************************************************************************
 *
 *   Copyright (c) 2019 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file RateControl.hpp
 *
 * PID 3 axis angular rate / angular velocity control.
 */

#pragma once

#include <matrix/matrix/math.hpp>
#include <mathlib/math/filter/LowPassFilter2pVector3f.hpp>

#include <lib/mixer/mixer.h>
#include <uORB/topics/rate_ctrl_status.h>

class RateControl
{
public:
	RateControl() = default;
	~RateControl() = default;

	/**
	 * Set the rate control gains
	 * @param P 3D vector of proportional gains for body x,y,z axis
	 * @param I 3D vector of integral gains
	 * @param D 3D vector of derivative gains
	 */
	void setGains(const matrix::Vector3f &P, const matrix::Vector3f &I, const matrix::Vector3f &D);

	/**
	 * Set the mximum absolute value of the integrator for all axes
	 * @param integrator_limit limit value for all axes x, y, z
	 */
	void setIntegratorLimit(const matrix::Vector3f &integrator_limit) { _lim_int = integrator_limit; };

	/**
	 * Set update frequency and low-pass filter cutoff that is applied to the derivative term
	 * @param loop_rate [Hz] rate with which update function is called
	 * @param cutoff [Hz] cutoff frequency for the low-pass filter on the dervative term
	 * @param force flag to force an expensive update even if the cutoff didn't change
	 */
	void setDTermCutoff(const float loop_rate, const float cutoff, const bool force);

	/**
	 * Set direct rate to torque feed forward gain
	 * @see _gain_ff
	 * @param FF 3D vector of feed forward gains for body x,y,z axis
	 */
	void setFeedForwardGain(const matrix::Vector3f &FF) { _gain_ff = FF; };

	/**
	 * Set the rate control gains
	 * @param breakpoint parameter 3D vector for P, I and D
	 * @param rate parameter 3D vector for P, I and D
	 */
	void setTPA(const matrix::Vector3f &breakpoint, const matrix::Vector3f &rate);

	/**
	 * Set saturation status
	 * @param status message from mixer reporting about saturation
	 */
	void setSaturationStatus(const MultirotorMixer::saturation_status &status);

	/**
	 * Run one control loop cycle calculation
	 * @param rate estimation of the current vehicle angular rate
	 * @param rate_sp desired vehicle angular rate setpoint
	 * @param dt desired vehicle angular rate setpoint
	 * @param thrust_sp total thrust setpoint to be used for TPA
	 * @return [-1,1] normalized torque vector to apply to the vehicle
	 */
	matrix::Vector3f update(const matrix::Vector3f rate, const matrix::Vector3f rate_sp, const float dt, const bool landed,
				const float thrust_sp);

	/**
	 * Set the integral term to 0 to prevent windup
	 * @see _rate_int
	 */
	void resetIntegral() { _rate_int.zero(); }

	/**
	 * Get status message of controller for logging/debugging
	 * @param rate_ctrl_status status message to fill with internal states
	 */
	void getRateControlStatus(rate_ctrl_status_s &rate_ctrl_status);

private:
	void updateIntegral(matrix::Vector3f &rate_error, const float dt, const matrix::Vector3f &gain_i_tpa);

	// Gains
	matrix::Vector3f _gain_p; ///< rate control proportional gain for all axes x, y, z
	matrix::Vector3f _gain_i; ///< rate control integral gain
	matrix::Vector3f _gain_d; ///< rate control derivative gain
	matrix::Vector3f _lim_int; ///< integrator term maximum absolute value
	matrix::Vector3f _gain_ff; ///< direct rate to torque feed forward gain only useful for helicopters

	// States
	matrix::Vector3f _rate_prev; ///< angular rates of previous update
	matrix::Vector3f _rate_prev_filtered; ///< low-pass filtered angular rates of previous update
	matrix::Vector3f _rate_int; ///< integral term of the rate controller
	math::LowPassFilter2pVector3f _lp_filters_d{0.f, 0.f}; ///< low-pass filters for D-term (roll, pitch & yaw)
	bool _mixer_saturation_positive[3] {};
	bool _mixer_saturation_negative[3] {};

	/*
	* Throttle PID attenuation
	* Lowers the overall gain of the PID controller linearly depending on total thrust.
	* Function visualization available here https://www.desmos.com/calculator/gn4mfoddje
	* @param tpa_breakpoint
	* @param tpa_rate
	* @param thrust_sp
	* @return attenuation [0,1] per axis in a vector
	*/
	matrix::Vector3f tpa_attenuations(float tpa_breakpoint, float tpa_rate, float thrust_sp);
	matrix::Vector3f _tpa_breakpoint;
	matrix::Vector3f _tpa_rate;
};
