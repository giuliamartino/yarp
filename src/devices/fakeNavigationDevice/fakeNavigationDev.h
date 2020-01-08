/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/Time.h>
#include <yarp/os/Port.h>
#include <yarp/dev/INavigation2D.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/os/PeriodicThread.h>
#include <math.h>


class fakeNavigationThread :
        public yarp::os::PeriodicThread
{
public:
    fakeNavigationThread(double _period, yarp::os::Searchable& _cfg);
    virtual bool threadInit() override;
    virtual void threadRelease() override;
    virtual void run() override;
};

class fakeNavigation :
        public yarp::dev::DeviceDriver,
        public yarp::dev::INavigation2DTargetActions,
        public yarp::dev::INavigation2DControlActions
{
public:
    fakeNavigationThread  *navThread;

public:
    virtual bool open(yarp::os::Searchable& config) override;

    fakeNavigation();

    //module cleanup
    virtual bool close() override;

public:
    /**
     * Sets a new navigation target, expressed in the absolute (map) coordinate frame.
     * @param loc the location to be reached
     * @return true/false if the command is accepted
     */
    bool gotoTargetByAbsoluteLocation(yarp::dev::Map2DLocation loc) override;

    /**
     * //Sets a new relative target, expressed in local (robot) coordinate frame.
     * @param v a three-element vector (x,y,theta) representing the location to be reached
     * @return true/false if the command is accepted
     */
    bool gotoTargetByRelativeLocation(double x, double y, double theta) override;

    /**
     * //Sets a new relative target, expressed in local (robot) coordinate frame.
     * @param v a three-element vector (x,y,theta) representing the location to be reached
     * @return true/false if the command is accepted
     */
    bool gotoTargetByRelativeLocation(double x, double y) override;

    /**
     * //Gets the last target set through a setNewAbsTarget() command.
     * @return a Map2DLocation containing data of the current target.
     * @return true if a target is currently available, false otherwise (in this case returned target is invalid)
     */
    bool getAbsoluteLocationOfCurrentTarget(yarp::dev::Map2DLocation& target) override;

    /**
     * //Gets the last target set through a setNewRelTarget command, expressed in absolute coordinates.
     * @param a Map2DLocation containing data of the current target.
     * @return true if a target is currently available, false otherwise (in this case returned target is invalid)
     */
    bool getRelativeLocationOfCurrentTarget(double& x, double& y, double& theta) override;

    /**
     * //Gets the status of the current navigation task. Typically stored into navigation_status variable.
     * @return the current navigation status expressed as NavigationStatusEnum.
     */
    bool getNavigationStatus(yarp::dev::NavigationStatusEnum& status) override;

    /**
     * //Stops the current navigation task.
     * @return true/false if the command is executed successfully.
     */
    bool stopNavigation() override;

    /**
     * //Pauses the current navigation task.
     * @return true/false if the command is executed successfully.
     */
    bool suspendNavigation(double time) override;

    /**
     * //Resumes a previously paused navigation task.
     * @return true/false if the command is executed successfully.
     */
    bool resumeNavigation() override;

    /**
     * Returns the list of waypoints generated by the navigation algorithm
     * @param waypoints the list of waypoints generated by the navigation algorithm
     * @return true/false
     */
    bool getAllNavigationWaypoints(std::vector<yarp::dev::Map2DLocation>& waypoints) override;

    /**
     * Returns the current waypoint pursued by the navigation algorithm
     * @param curr_waypoint the current waypoint pursued by the navigation algorithm
     * @return true/false
     */
    bool getCurrentNavigationWaypoint(yarp::dev::Map2DLocation& curr_waypoint) override;

    /**
     * Returns the current navigation map processed by the navigation algorithm
     * @param map_type the map to be requested (e.g. global, local, etc.)
     * @param map the map, currently used by the navigation algorithm
     * @return true/false
     */
    bool getCurrentNavigationMap(yarp::dev::NavigationMapTypeEnum map_type, yarp::dev::MapGrid2D& map) override;

    /**
     * Forces the navigation system to recompute the path from the current robot position to the current goal.
     * If no goal has been set, the command has no effect.
     * @return true/false
     */
    bool recomputeCurrentNavigationPath() override;

    /**
     * Apply a velocity command. velocities are expressed in the robot reference frame
     * @param x [m/s]
     * @param y [m/s]
     * @param theta [deg/s]
     * @param timeout The velocity command expires after the specified amount of time (by default 0.1 seconds)
     * @return true/false
     */
    virtual bool applyVelocityCommand(double x_vel, double y_vel, double theta_vel, double timeout = 0.1) override;
};
