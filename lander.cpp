// Mars lander simulator
// Version 1.11
// Mechanical simulation functions
// Gabor Csanyi and Andrew Gee, August 2019

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation, to make use of it
// for non-commercial purposes, provided that (a) its original authorship
// is acknowledged and (b) no modified versions of the source code are
// published. Restriction (b) is designed to protect the integrity of the
// exercise for future generations of students. The authors would be happy
// to receive any suggested modifications by private correspondence to
// ahg@eng.cam.ac.uk and gc121@eng.cam.ac.uk.

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include "lander.h"

vector3d acceleration(vector3d position, vector3d velocity)
{
    vector3d a, a_gravity, thrust, self_drag, para_drag, drag; // (m/s2, m/s2, N, N)
    double mass;
    // acceleration due to gravity
    a_gravity = -GRAVITY * MARS_MASS * position.norm() / position.abs2();
    // thrust and drag forces
    thrust = thrust_wrt_world();
    self_drag = -0.5 * atmospheric_density(position) * DRAG_COEF_LANDER * (3.1415 * pow(LANDER_SIZE, 2))
            * velocity.abs2() * velocity.norm();
    para_drag = -0.5 * atmospheric_density(position) * DRAG_COEF_CHUTE * pow(LANDER_SIZE*2, 2)
        * velocity.abs2() * velocity.norm() * 5;
    if (parachute_status == DEPLOYED) drag = self_drag + para_drag;
    if (parachute_status == NOT_DEPLOYED) drag = self_drag;
    // current mass of the lander
    mass = fuel * FUEL_CAPACITY * FUEL_DENSITY;
    // total acceleration
    a = a_gravity + (thrust + drag) / mass;
    return a;
}

void autopilot (void)
  // Autopilot to adjust the engine throttle, parachute and attitude control
{
    vector3d acceleration;
    acceleration = - GRAVITY * MARS_MASS * position.norm() / position.abs2();
    double force;
    force = acceleration.y* (fuel * FUEL_CAPACITY * FUEL_DENSITY + UNLOADED_LANDER_MASS);
    static double scaling;
    double alt;
    alt = abs(position.y) - MARS_RADIUS;
    scaling = (4000 - alt) / 4000;
    if (scaling < 0) { scaling = 0; };
    throttle = force/MAX_THRUST*scaling;
    if (velocity.y < 0)
    {
        throttle = 0;
    }
    if (alt < 300)
    {
        if (velocity.y > 0.6 and velocity.y < 3)
        {
            throttle = force / MAX_THRUST * 0.5;
        }
        if (velocity.y > 3)
        {
            throttle = force / MAX_THRUST ;
        }
        if (velocity.y < 0.6)
        {
            throttle = 0;
        }
    }
    /*static double Kh = 0.001, gain = 1, Kp = 1;
    double m,altitude,error, P_out;
    m = FUEL_CAPACITY * FUEL_DENSITY + UNLOADED_LANDER_MASS;
    altitude = position.y - MARS_RADIUS;
    error = -(0.5 + Kh * altitude + velocity.y);
    P_out = Kp * error;
    static double delta = 0.1;
    if (P_out >= 1 - delta)
    {
        throttle = 1;
    }
    if (P_out <= - delta)
    {
        throttle = 0;
    }
    if (P_out < 1 - delta and P_out > -delta)
    {
        throttle = delta + P_out;
    }*/
}

void numerical_dynamics(void)
{
    vector3d acc, next; 
    static vector3d previous; 

    if (simulation_time == 0.0)
    {
        acc = acceleration(position, velocity);
        previous = position;
        position = previous + velocity * delta_t + 0.5 * delta_t * delta_t * acc;
        velocity = velocity + delta_t * acc;
    }
    else
    {
        // update position and velocity
        acc = acceleration(position, velocity);
        next = 2 * position - previous + acc * delta_t * delta_t;

        velocity = (next - previous) / (2 * delta_t);

        // update previous status
        previous = position;
        position = next;
    }

    // Here we can apply an autopilot to adjust the thrust, parachute and attitude
    if (autopilot_enabled) autopilot();

    // Here we can apply 3-axis stabilization to ensure the base is always pointing downwards
    if (stabilized_attitude) attitude_stabilization();
}

void initialize_simulation (void)
  // Lander pose initialization - selects one of 10 possible scenarios
{
  // The parameters to set are:
  // position - in Cartesian planetary coordinate system (m)
  // velocity - in Cartesian planetary coordinate system (m/s)
  // orientation - in lander coordinate system (xyz Euler angles, degrees)
  // delta_t - the simulation time step
  // boolean state variables - parachute_status, stabilized_attitude, autopilot_enabled
  // scenario_description - a descriptive string for the help screen

  scenario_description[0] = "circular orbit";
  scenario_description[1] = "descent from 10km";
  scenario_description[2] = "elliptical orbit, thrust changes orbital plane";
  scenario_description[3] = "polar launch at escape velocity (but drag prevents escape)";
  scenario_description[4] = "elliptical orbit that clips the atmosphere and decays";
  scenario_description[5] = "descent from 200km";
  scenario_description[6] = "";
  scenario_description[7] = "";
  scenario_description[8] = "";
  scenario_description[9] = "";

  switch (scenario) {

  case 0:
    // a circular equatorial orbit
    position = vector3d(1.2*MARS_RADIUS, 0.0, 0.0);
    velocity = vector3d(0.0, -3247.087385863725, 0.0);
    orientation = vector3d(0.0, 90.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 1:
    // a descent from rest at 10km altitude
    position = vector3d(0.0, -(MARS_RADIUS + 10000.0), 0.0);
    velocity = vector3d(0.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = true;
    autopilot_enabled = false;
    break;

  case 2:
    // an elliptical polar orbit
    position = vector3d(0.0, 0.0, 1.2*MARS_RADIUS);
    velocity = vector3d(3500.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 3:
    // polar surface launch at escape velocity (but drag prevents escape)
    position = vector3d(0.0, 0.0, MARS_RADIUS + LANDER_SIZE/2.0);
    velocity = vector3d(0.0, 0.0, 5027.0);
    orientation = vector3d(0.0, 0.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 4:
    // an elliptical orbit that clips the atmosphere each time round, losing energy
    position = vector3d(0.0, 0.0, MARS_RADIUS + 100000.0);
    velocity = vector3d(4000.0, 0.0, 0.0);
    orientation = vector3d(0.0, 90.0, 0.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = false;
    autopilot_enabled = false;
    break;

  case 5:
    // a descent from rest at the edge of the exosphere
    position = vector3d(0.0, -(MARS_RADIUS + EXOSPHERE), 0.0);
    velocity = vector3d(0.0, 0.0, 0.0);
    orientation = vector3d(0.0, 0.0, 90.0);
    delta_t = 0.1;
    parachute_status = NOT_DEPLOYED;
    stabilized_attitude = true;
    autopilot_enabled = false;
    break;

  case 6:
    break;

  case 7:
    break;

  case 8:
    break;

  case 9:
    break;

  }
}
